// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "stdafx.h"
#include "resource.h"
#include "..\\TortoiseShell\\resource.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "Utils.h"
#include "StringUtils.h"
#include "DirFileEnum.h"
#include "SVNConfig.h"
#include "SVNProperties.h"
#include "SVN.h"
#include "LogDlg.h"
#include "SVNProgressDlg.h"
#include "SysImageList.h"
#include ".\svnstatuslistctrl.h"
#include "TSVNPath.h"
#include "Registry.h"

const UINT CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED
			= ::RegisterWindowMessage(_T("SVNSLNM_ITEMCOUNTCHANGED"));

#define IDSVNLC_REVERT			1
#define IDSVNLC_COMPARE			2
#define IDSVNLC_OPEN			3
#define IDSVNLC_DELETE			4
#define IDSVNLC_IGNORE			5
#define IDSVNLC_GNUDIFF1		6
#define IDSVNLC_UPDATE			7
#define IDSVNLC_LOG				8
#define IDSVNLC_EDITCONFLICT	9
#define IDSVNLC_IGNOREMASK	   10
#define IDSVNLC_ADD			   11

BEGIN_MESSAGE_MAP(CSVNStatusListCtrl, CListCtrl)
	ON_NOTIFY(HDN_ITEMCLICKA, 0, OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, OnHdnItemclick)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnLvnItemchanged)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


bool	CSVNStatusListCtrl::m_bAscending = false;
int		CSVNStatusListCtrl::m_nSortedColumn = -1;

CSVNStatusListCtrl::CSVNStatusListCtrl() : CListCtrl()
	, m_HeadRev(SVNRev::REV_HEAD)
{
	m_pStatLabel = NULL;
	m_pSelectButton = NULL;
}

CSVNStatusListCtrl::~CSVNStatusListCtrl()
{
	m_tempFileList.DeleteAllFiles();
	ClearStatusArray();
}

void CSVNStatusListCtrl::ClearStatusArray()
{
	for (size_t i=0; i < m_arStatusArray.size(); i++)
	{
		delete m_arStatusArray[i];
	}
	m_arStatusArray.clear();
}


CSVNStatusListCtrl::FileEntry * CSVNStatusListCtrl::GetListEntry(int index)
{
	if (index < 0)
		return NULL;
	if (index >= (int)m_arListArray.size())
		return NULL;
	if ((INT_PTR)m_arListArray[index] >= m_arStatusArray.size())
		return NULL;
	return m_arStatusArray[m_arListArray[index]];
}

void CSVNStatusListCtrl::Init(DWORD dwColumns, bool bHasCheckboxes /* = TRUE */)
{
	m_dwColumns = dwColumns;
	// set the extended style of the listcontrol
	DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	exStyle |= (bHasCheckboxes ? LVS_EX_CHECKBOXES : 0);
	SetExtendedStyle(exStyle);

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
	// clear all previously set header columns
	DeleteAllItems();
	int c = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		DeleteColumn(c--);

	// now set up the requested columns
	CString temp;
	int nCol = 0;
	// the relative path is always visible
	temp.LoadString(IDS_STATUSLIST_COLFILE);
	InsertColumn(nCol++, temp);
	if (dwColumns & SVNSLC_COLSTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLREMOTESTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLREMOTESTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLTEXTSTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLTEXTSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLPROPSTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLPROPSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLREMOTETEXT)
	{
		temp.LoadString(IDS_STATUSLIST_COLREMOTETEXTSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLREMOTEPROP)
	{
		temp.LoadString(IDS_STATUSLIST_COLREMOTEPROPSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLURL)
	{
		temp.LoadString(IDS_STATUSLIST_COLURL);
		InsertColumn(nCol++, temp);
	}

	SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	SetRedraw(true);
}

BOOL CSVNStatusListCtrl::GetStatus(CString sFilePath, bool bUpdate /* = FALSE */)
{
	BOOL bRet = TRUE;
	m_nTargetCount = 0;
	m_bHasExternalsFromDifferentRepos = FALSE;
	m_bHasExternals = FALSE;
	m_bHasUnversionedItems = FALSE;

	m_bBlock = TRUE;

	// first clear possible status data left from
	// previous GetStatus() calls
	ClearStatusArray();

	// Since svn_client_status() returns all files, even those in
	// folders included with svn:externals we need to check if all
	// files we get here belongs to the same repository.
	// It is possible to commit changes in an external folder, as long
	// as the folder belongs to the same repository (but another path),
	// but it is not possible to commit all files if the externals are
	// from a different repository.
	//
	// To check if all files belong to the same repository, we compare the
	// UUID's - if they're identical then the files belong to the same
	// repository and can be committed. But if they're different, then
	// tell the user to commit all changes in the external folders
	// first and exit.
	CStringA sUUID;					// holds the repo UUID
	CTSVNPathList arExtPaths;		// list of svn:external paths

	SVNConfig config;

	m_sURL.Empty();

	CTSVNPathList targets;
	if(targets.LoadFromTemporaryFile(sFilePath))
	{

		m_nTargetCount = targets.GetCount();

		// for every selected file/folder
		//		while (file.ReadString(strLine))
		//		{
		//			strLine.Replace('\\', '/');
		//
		//			// remove trailing / characters since they mess up the filename list
		//			// However "/" and "c:/" will be left alone.
		//			if (strLine.GetLength() > 1 && strLine.Right(1) == _T("/")) 
		//			{
		//				strLine.Delete(strLine.GetLength()-1,1);
		//			}

		SVNStatus status;
		if(m_nTargetCount > 1 && targets.AreAllPathsFilesInOneDirectory())
		{
			// This is a special case, where we've been given a list of files
			// all from one folder
			// We handle them by setting a status filter, then requesting the SVN status of 
			// all the files in the directory, filtering for the ones we're interested in
			status.SetFilter(targets);

			if(!FetchStatusForSingleTarget(config, status, targets.GetCommonDirectory(), bUpdate, sUUID, arExtPaths))
			{
				bRet = FALSE;
			}
		}
		else
		{
			for(int nTarget = 0; nTarget < m_nTargetCount; nTarget++)
			{
				if(!FetchStatusForSingleTarget(config, status, targets[nTarget], bUpdate, sUUID, arExtPaths))
				{
					bRet = FALSE;
				}
			}
		}
	}

	BuildStatistics();
	m_bBlock = FALSE;
	return bRet;
}

//
// Work on a single item from the list of paths which is provided to us
//
bool CSVNStatusListCtrl::FetchStatusForSingleTarget(
							SVNConfig& config,
							SVNStatus& status,
							const CTSVNPath& target, 
							bool bFetchStatusFromRepository,
							CStringA& strCurrentRepositoryUUID,
							CTSVNPathList& arExtPaths
							)
{
	apr_array_header_t* pIgnorePatterns = NULL;
	//BUGBUG - Some error handling needed here
	config.GetDefaultIgnores(&pIgnorePatterns);

	CTSVNPath workingTarget(target);
//	const BOOL bTargetIsFolder = workingTarget.IsDirectory();

	svn_wc_status_t * s;
	// TODO: GetFirstFileStatus should take workingTarget directly
	CTSVNPath svnPath;
	{
		const TCHAR * pSVNItemPath = NULL;
		s = status.GetFirstFileStatus(workingTarget.GetWinPath(), &pSVNItemPath, bFetchStatusFromRepository);
		// TODO: This should be passed back directly from GetFirstFileStatus
		svnPath.SetFromSVN(pSVNItemPath);
	}
	m_HeadRev = SVNRev(status.headrev);
	if (s!=0)
	{
		svn_wc_status_kind wcFileStatus = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);

		// This one fixes a problem with externals: 
		// If a strLine is a file, svn:externals and its parent directory
		// will also be returned by GetXXXFileStatus. Hence, we skip all
		// status info until we find the one matching workingTarget.
//TODO: This could probably be better done with the new filtering mechanism on 
// the SVNstatus calls
		if (!workingTarget.IsDirectory())
		{
			if (!workingTarget.IsEquivalentTo(svnPath))
			{
				while (s != 0)
				{
					const TCHAR * pSVNItemPath = NULL;
					s = status.GetNextFileStatus(&pSVNItemPath);
					svnPath.SetFromSVN(pSVNItemPath);
					if(workingTarget.IsEquivalentTo(svnPath))
					{
						break;
					}
				}
				// Now, set working target to be the base folder of this item
				workingTarget = workingTarget.GetDirectory();
			}
		}

		// By the time we get here, the workingTarget line should always be a directory
//WGD - Actually, I think I was wrong on this
//Doing, for example, an 'Add' on a single target, gets us here with a file in workingTarget
//		ASSERT(workingTarget.IsDirectory());

		// Is this a versioned item with an associated repos UUID?
		if ((s->entry)&&(s->entry->uuid))
		{
			// Have we seen a repos UUID yet?
			if (strCurrentRepositoryUUID.IsEmpty())
			{
				// This is the first repos UUID we've seen - record it
				strCurrentRepositoryUUID = s->entry->uuid;
				m_sUUID = strCurrentRepositoryUUID;
			}
			else
			{
				if (strCurrentRepositoryUUID.Compare(s->entry->uuid)!=0)
				{
					// This item comes from a different repository than our main one
					m_bHasExternalsFromDifferentRepos = TRUE;
					if (s->entry->kind == svn_node_dir)
						arExtPaths.AddPath(workingTarget);
				} 
			}
		}
		if ((wcFileStatus == svn_wc_status_unversioned)&& svnPath.IsDirectory())
		{
			// check if the unversioned folder is maybe versioned. This
			// could happen with nested layouts
			if (SVNStatus::GetAllStatus(workingTarget.GetWinPath()) != svn_wc_status_unversioned)
			{
				return true;	//ignore nested layouts
			}
		}
		if (s->text_status == svn_wc_status_external)
		{
			m_bHasExternals = TRUE;
		}

		AddNewFileEntry(s, svnPath, workingTarget, true, m_bHasExternals);

		if ((wcFileStatus == svn_wc_status_unversioned) && svnPath.IsDirectory())
		{
			//we have an unversioned folder -> get all files in it recursively!
			AddUnversionedFolder(svnPath, workingTarget.GetContainingDirectory(), pIgnorePatterns);
		}

		// for folders, get all statuses inside it too
		if(workingTarget.IsDirectory())
		{
			ReadRemainingItemsStatus(status, workingTarget, strCurrentRepositoryUUID, arExtPaths, pIgnorePatterns);
		} 

	} // if (s!=0) 
	else
	{
		m_sLastError = status.GetLastErrorMsg();
		return false;
	}

	return true;
}

const CSVNStatusListCtrl::FileEntry* 
CSVNStatusListCtrl::AddNewFileEntry(
			const svn_wc_status_t* pSVNStatus,  // The return from the SVN GetStatus functions
			const CTSVNPath& path,				// The path of the item we're adding
			const CTSVNPath& basePath,			// The base directory for this status build
			bool bDirectItem,					// Was this item the first found by GetFirstFileStatus or by a subsequent GetNextFileStatus call
			bool bInExternal					// Are we in an 'external' folder
			)
{
//WGD - I think I was wrong with this assumption
//	ASSERT(basePath.IsDirectory());

	FileEntry * entry = new FileEntry();
	entry->path = path;
	entry->basepath = basePath;
	entry->status = SVNStatus::GetMoreImportant(pSVNStatus->text_status, pSVNStatus->prop_status);
	entry->textstatus = pSVNStatus->text_status;
	entry->propstatus = pSVNStatus->prop_status;
	entry->remotestatus = SVNStatus::GetMoreImportant(pSVNStatus->repos_text_status, pSVNStatus->repos_prop_status);
	entry->remotetextstatus = pSVNStatus->repos_text_status;
	entry->remotepropstatus = pSVNStatus->repos_prop_status;
	entry->inunversionedfolder = false;
	entry->checked = false;
	entry->inexternal = bInExternal;
	entry->direct = bDirectItem;
	if (pSVNStatus->entry)
	{
		entry->isfolder = (pSVNStatus->entry->kind == svn_node_dir);

		if (pSVNStatus->entry->url)
		{
			CUtils::Unescape((char *)pSVNStatus->entry->url);
			entry->url = CUnicodeUtils::GetUnicode(pSVNStatus->entry->url);
		}

		if(bDirectItem)
		{
			if (m_sURL.IsEmpty())
				m_sURL = entry->url;
			else
				m_sURL.LoadString(IDS_STATUSLIST_MULTIPLETARGETS);
		}
	}
	else
	{
		entry->isfolder = path.IsDirectory();
	}

	// Pass ownership of the entry to the array
	m_arStatusArray.push_back(entry);

	return entry;
}

void CSVNStatusListCtrl::AddUnversionedFolder(const CTSVNPath& folderName, 
												const CTSVNPath& basePath, 
												apr_array_header_t *pIgnorePatterns)
{
	CDirFileEnum filefinder(folderName.GetWinPathString());
	CString filename;
	bool bIsDirectory;
	m_bHasUnversionedItems = TRUE;
	while (filefinder.NextFile(filename,&bIsDirectory))
	{
		if (!SVNConfig::MatchIgnorePattern(filename,pIgnorePatterns))
		{
			FileEntry * entry = new FileEntry();
			entry->path.SetFromWin(filename,bIsDirectory);
			entry->basepath = basePath; 
			entry->status = svn_wc_status_unversioned;
			entry->textstatus = svn_wc_status_unversioned;
			entry->propstatus = svn_wc_status_unversioned;
			entry->remotestatus = svn_wc_status_unversioned;
			entry->remotetextstatus = svn_wc_status_unversioned;
			entry->remotepropstatus = svn_wc_status_unversioned;
			entry->inunversionedfolder = TRUE;
			entry->checked = false;
			entry->inexternal = false;
			entry->direct = false;
			entry->isfolder = bIsDirectory; 

			m_arStatusArray.push_back(entry);
		}
	} // while (filefinder.NextFile(filename,&bIsDirectory))
}


void CSVNStatusListCtrl::ReadRemainingItemsStatus(SVNStatus& status, const CTSVNPath& basePath, 
										  CStringA& strCurrentRepositoryUUID, 
										  CTSVNPathList& arExtPaths, apr_array_header_t *pIgnorePatterns)
{
	svn_wc_status_t * s;
	// This is the item path returned from GetNextFileStatus - it's a forward-slash path
	const TCHAR * pSVNItemPath = NULL;

	CTSVNPath lastexternalpath;
	while ((s = status.GetNextFileStatus(&pSVNItemPath)) != NULL)
	{
		CTSVNPath svnPath;
		svnPath.SetFromSVN(pSVNItemPath);
		svn_wc_status_kind wcFileStatus = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
		if ((wcFileStatus == svn_wc_status_unversioned) && (svnPath.IsDirectory()))
		{
			// check if the unversioned folder is maybe versioned. This
			// could happen with nested layouts
			if (SVNStatus::GetAllStatus(pSVNItemPath) != svn_wc_status_unversioned)
				continue;	//ignore nested layouts
		}
		bool bDirectoryIsExternal = false;

		if (s->entry)
		{
			if (s->entry->uuid)
			{
				if (strCurrentRepositoryUUID.IsEmpty())
					strCurrentRepositoryUUID = s->entry->uuid;
				else
				{
					if (strCurrentRepositoryUUID.Compare(s->entry->uuid)!=0)
					{
						if (SVNStatus::IsImportant(wcFileStatus))
							m_bHasExternalsFromDifferentRepos = TRUE;
						if (s->entry->kind == svn_node_dir)
						{
							if ((lastexternalpath.IsEmpty())||(!lastexternalpath.IsAncestorOf(svnPath)))
							{
								arExtPaths.AddPath(svnPath);
								lastexternalpath = svnPath;
							}
						}
					}
				}
			} 
		} // if (s->entry)

		// Do we have any external paths?
		if(arExtPaths.GetCount() > 0)
		{
			// If do have external paths, we need to check if the current item belongs
			// to one of them
			for (int ix=0; ix<arExtPaths.GetCount(); ix++)
			{
				if (arExtPaths[ix].IsAncestorOf(svnPath))
				{
					bDirectoryIsExternal = true;
					break;
				}
			}
		}

		if (s->text_status == svn_wc_status_external)
			m_bHasExternals = TRUE;
		if (wcFileStatus == svn_wc_status_unversioned)
			m_bHasUnversionedItems = TRUE;

		const FileEntry* entry = AddNewFileEntry(s, svnPath, basePath, false, bDirectoryIsExternal);

		if ((wcFileStatus == svn_wc_status_unversioned)&&(!SVNConfig::MatchIgnorePattern(entry->path.GetSVNPathString(),pIgnorePatterns)))
		{
			if (entry->isfolder)
			{
				//we have an unversioned folder -> get all files in it recursively!
				AddUnversionedFolder(svnPath, basePath, pIgnorePatterns);
			}
		}
	} // while ((s = status.GetNextFileStatus(&pSVNItemPath)) != NULL)
}

// Get the show-flags bitmap value which corresponds to a particular SVN status 
DWORD CSVNStatusListCtrl::GetShowFlagsFromSVNStatus(svn_wc_status_kind status)
{
	switch (status)
	{
	case svn_wc_status_none:
	case svn_wc_status_unversioned:
		return SVNSLC_SHOWUNVERSIONED;
	case svn_wc_status_ignored:
		return SVNSLC_SHOWDIRECTS;
	case svn_wc_status_incomplete:
		return SVNSLC_SHOWINCOMPLETE;
	case svn_wc_status_normal:
		return SVNSLC_SHOWNORMAL;
	case svn_wc_status_external:
		return SVNSLC_SHOWEXTERNAL;
	case svn_wc_status_added:
		return SVNSLC_SHOWADDED;
	case svn_wc_status_missing:
		return SVNSLC_SHOWMISSING;
	case svn_wc_status_deleted:
		return SVNSLC_SHOWREMOVED;
	case svn_wc_status_replaced:
		return SVNSLC_SHOWREPLACED;
	case svn_wc_status_modified:
		return SVNSLC_SHOWMODIFIED;
	case svn_wc_status_merged:
		return SVNSLC_SHOWMERGED;
	case svn_wc_status_conflicted:
		return SVNSLC_SHOWCONFLICTED;
	case svn_wc_status_obstructed:
		return SVNSLC_SHOWOBSTRUCTED;
	default:
		// we should NEVER get here!
		ASSERT(FALSE);
		break;	
	}
	return 0;
}

void CSVNStatusListCtrl::Show(DWORD dwShow, DWORD dwCheck /*=0*/)
{
	WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	CWinApp * pApp = AfxGetApp();
	if (pApp)
		pApp->DoWaitCursor(1);
	m_dwShow = dwShow;
	m_nSelected = 0;
	SetRedraw(FALSE);
	DeleteAllItems();

	m_arListArray.clear();

	// TODO: This might reserve rather a lot of memory - not necessarily strictly necessary
	// It might be better to have a guess at how much we'll need
	m_arListArray.reserve(m_arStatusArray.size());
	SetItemCount(m_arStatusArray.size());

	int listIndex = 0;
	for (int i=0; i< (int)m_arStatusArray.size(); ++i)
	{
		FileEntry * entry = m_arStatusArray[i];
		if (entry->inexternal && (!(dwShow & SVNSLC_SHOWINEXTERNALS)))
			continue;
		svn_wc_status_kind status = SVNStatus::GetMoreImportant(entry->status, entry->remotestatus);
		DWORD showFlags = GetShowFlagsFromSVNStatus(status);

		// status_ignored is a special case - we must have the 'direct' flag set to add a status_ignored item
		if(status != svn_wc_status_ignored || (entry->direct))
		{
			if(dwShow & showFlags)
			{
				m_arListArray.push_back(i);
				if(dwCheck & showFlags)
				{
					entry->checked = true;
				}
				AddEntry(entry, langID, listIndex++);
			}
		}
	} // for (int i=0; i<m_arStatusArray.GetCount(); ++i)

	SetItemCount(listIndex);

	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	SetRedraw(TRUE);
	GetStatisticsString();

	if (pApp)
		pApp->DoWaitCursor(-1);
}

void CSVNStatusListCtrl::AddEntry(const FileEntry * entry, WORD langID, int listIndex)
{
	static CString ponly(MAKEINTRESOURCE(IDS_STATUSLIST_PROPONLY));
	static HINSTANCE hResourceHandle(AfxGetResourceHandle());

	m_bBlock = TRUE;
	TCHAR buf[100];
	int index = listIndex;
	int nCol = 1;
	CString entryname = entry->path.GetDisplayString(&entry->basepath);
	if (entryname.IsEmpty())
		entryname = entry->path.GetFileOrDirectoryName();
	int icon_idx = 0;
	if (entry->isfolder)
		icon_idx = m_nIconFolder;
	else
	{
		icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(entry->path);
	}
	InsertItem(index, entryname, icon_idx);
	if (m_dwColumns & SVNSLC_COLSTATUS)
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		if ((entry->status == entry->propstatus)&&
			(entry->status != svn_wc_status_normal)&&
			(entry->status != svn_wc_status_unversioned)&&
			(!SVNStatus::IsImportant(entry->textstatus)))
			_tcscat(buf, ponly);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLREMOTESTATUS)
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->remotestatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		if ((entry->remotestatus == entry->remotepropstatus)&&
			(entry->status != svn_wc_status_normal)&&
			(entry->status != svn_wc_status_unversioned)&&
			(!SVNStatus::IsImportant(entry->remotetextstatus)))
			_tcscat(buf, ponly);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLTEXTSTATUS)
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->textstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLPROPSTATUS)
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->propstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLREMOTETEXT)
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->remotetextstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLREMOTEPROP)
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->remotepropstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLURL)
	{
		SetItemText(index, nCol++, entry->url);
	}
	SetCheck(index, entry->checked);
	if (entry->checked)
		m_nSelected++;
	m_bBlock = FALSE;
}

void CSVNStatusListCtrl::Sort()
{
	std::sort(m_arStatusArray.begin(), m_arStatusArray.end(), SortCompare);
	Show(m_dwShow);
}

bool CSVNStatusListCtrl::SortCompare(const FileEntry* entry1, const FileEntry* entry2)
{
	int result = 0;
	switch (m_nSortedColumn)
	{
	case 7:
		{
			if (result == 0)
			{
				result = entry1->url.CompareNoCase(entry2->url);
			}
		}
	case 6:
		{
			if (result == 0)
			{
				result = entry1->remotepropstatus - entry2->remotepropstatus;
			}
		}
	case 5:
		{
			if (result == 0)
			{
				result = entry1->remotetextstatus - entry2->remotetextstatus;
			}
		}
	case 4:
		{
			if (result == 0)
			{
				result = entry1->propstatus - entry2->propstatus;
			}
		}
	case 3:
		{
			if (result == 0)
			{
				result = entry1->textstatus - entry2->textstatus;
			}
		}
	case 2:
		{
			if (result == 0)
			{
				result = entry1->remotestatus - entry2->remotestatus;
			}
		}
	case 1:
		{
			if (result == 0)
			{
				result = entry1->status - entry2->status;
			}
		}
	case 0:		//path column
		{
			if (result == 0)
			{
				result = CTSVNPath::Compare(entry1->path, entry2->path);
			}
		}
		break;
	default:
		break;
	} // switch (m_nSortedColumn)
	if (!m_bAscending)
		result = -result;

	return result < 0;
}

void CSVNStatusListCtrl::OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;
	if (m_bBlock)
		return;
	m_bBlock = TRUE;
	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = 0;
	// get the internal column from the visible columns
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLSTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLREMOTESTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLTEXTSTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLPROPSTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLREMOTETEXT)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLREMOTEPROP)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLURL)
			m_nSortedColumn++;
	}
	Sort();

#ifdef UNICODE
	CHeaderCtrl * pHeader = GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	pHeader->GetItem(m_nSortedColumn, &HeaderItem);
	HeaderItem.fmt |= (m_bAscending ? HDF_SORTDOWN : HDF_SORTUP);
	pHeader->SetItem(m_nSortedColumn, &HeaderItem);
#endif
	m_bBlock = FALSE;
}

void CSVNStatusListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if ((pNMLV->uNewState==0)||(pNMLV->uNewState & LVIS_SELECTED))
		return;

	if (m_bBlock)
		return;

	FileEntry * entry = GetListEntry(pNMLV->iItem);
	ASSERT(entry != NULL);
	if (entry == NULL)
		return;

	int nListItems = GetItemCount();

	m_bBlock = TRUE;
	//was the item checked?
	if (GetCheck(pNMLV->iItem))
	{
		// if an unversioned item was checked, then we need to check if
		// the parent folders are unversioned too. If the parent folders actually
		// are unversioned, then check those too.
		if (entry->status == svn_wc_status_unversioned)
		{
			//we need to check the parent folder too
			const CTSVNPath& folderpath = entry->path;
			for (int i=0; i< nListItems; ++i)
			{
				FileEntry * testEntry = GetListEntry(i);
				ASSERT(testEntry != NULL);
				if (testEntry == NULL)
					continue;
				if (!testEntry->checked)
				{
					if (testEntry->path.IsAncestorOf(folderpath))
					{
						SetEntryCheck(testEntry,i,true);
						m_nSelected++;
					} 
				}
			}
		}
		if (entry->status == svn_wc_status_deleted)
		{
			// if a deleted folder gets checked, we have to check all
			// children of that folder too.
			if (entry->path.IsDirectory())
			{
				SetCheckOnAllDescendentsOf(entry, true);
			}

			// if a deleted file or folder gets checked, we have to
			// check all parents of this item too.
			for (int i=0; i<nListItems; ++i)
			{
				FileEntry * testEntry = GetListEntry(i);
				ASSERT(testEntry != NULL);
				if (testEntry == NULL)
					continue;
				if (!testEntry->checked)
				{
					if (testEntry->path.IsAncestorOf(entry->path))
					{
						if (testEntry->status == svn_wc_status_deleted)
						{
							SetEntryCheck(testEntry,i,true);
							m_nSelected++;
							//now we need to check all children of this parent folder
							SetCheckOnAllDescendentsOf(testEntry, true);
						}
					}
				} // if (!GetCheck(i)) 
			} // for (int i=0; i<GetItemCount(); ++i)
		} // if (entry->status == svn_wc_status_deleted) 
		entry->checked = TRUE;
		m_nSelected++;
	} // if (GetCheck(pNMLV->iItem)) 
	else
	{
		//item was unchecked
		if (entry->path.IsDirectory())
		{
			//disable all files within an unselected folder
			SetCheckOnAllDescendentsOf(entry, false);
		}
		else if (entry->status == svn_wc_status_deleted)
		{
			//a "deleted" file was unchecked, so uncheck all parent folders
			//and all children of those parents
			for (int i=0; i<nListItems; i++)
			{
				FileEntry * testEntry = GetListEntry(i);
				ASSERT(testEntry != NULL);
				if (testEntry == NULL)
					continue;
				if (testEntry->checked)
				{
					if (testEntry->path.IsAncestorOf(entry->path))
					{
						if (testEntry->status == svn_wc_status_deleted)
						{
							SetEntryCheck(testEntry,i,false);
							m_nSelected--;

							SetCheckOnAllDescendentsOf(testEntry, false);
						}
					}
				}
			} // for (int i=0; i<GetItemCount(); i++)
		} // else if (entry->status == svn_wc_status_deleted)
		entry->checked = FALSE;
		m_nSelected--;
	} // else -> from if (GetCheck(pNMLV->iItem)) 
	GetStatisticsString();
	m_bBlock = FALSE;
}

bool CSVNStatusListCtrl::EntryPathCompareNoCase(const FileEntry* pEntry1, const FileEntry* pEntry2)
{
	return CTSVNPath::ComparisonPredicate(pEntry1->path, pEntry2->path);
}

bool CSVNStatusListCtrl::IsEntryVersioned(const FileEntry* pEntry1)
{
	return pEntry1->status != svn_wc_status_unversioned;
}

void CSVNStatusListCtrl::BuildStatistics()
{
	// We partition the list of items so that it's arrange with all the versioned items first
	// then all the unversioned items afterwards.
	// Then we sort the versioned part of this, so that we can do quick look-ups in it
	FileEntryVector::iterator itFirstUnversionedEntry;
	itFirstUnversionedEntry = std::partition(m_arStatusArray.begin(), m_arStatusArray.end(), IsEntryVersioned);
	std::sort(m_arStatusArray.begin(), itFirstUnversionedEntry, EntryPathCompareNoCase);
	// Also sort the unversioned section, to make the list look nice...
	std::sort(itFirstUnversionedEntry, m_arStatusArray.end(), EntryPathCompareNoCase);

	//now gather some statistics
	m_nUnversioned = 0;
	m_nNormal = 0;
	m_nModified = 0;
	m_nAdded = 0;
	m_nDeleted = 0;
	m_nConflicted = 0;
	m_nTotal = 0;
	m_nSelected = 0;
	for (int i=0; i < (int)m_arStatusArray.size(); ++i)
	{
		const FileEntry * entry = m_arStatusArray[i];
		if (entry)
		{
			switch (entry->status)
			{
			case svn_wc_status_normal:
				m_nNormal++;
				break;
			case svn_wc_status_added:
				m_nAdded++;
				break;
			case svn_wc_status_missing:
			case svn_wc_status_deleted:
				m_nDeleted++;
				break;
			case svn_wc_status_replaced:
			case svn_wc_status_modified:
			case svn_wc_status_merged:
				m_nModified++;
				break;
			case svn_wc_status_conflicted:
			case svn_wc_status_obstructed:
				m_nConflicted++;
				break;
			case svn_wc_status_ignored:
				m_nUnversioned++;
				break;
			default:
				{
					if (SVNStatus::IsImportant(entry->remotestatus))
						break;
					m_nUnversioned++;
					if (!entry->inunversionedfolder)
					{
						// check if the unversioned item is just
						// a file differing in case but still versioned
						FileEntryVector::iterator itMatchingItem;
						if(std::binary_search(m_arStatusArray.begin(), itFirstUnversionedEntry, entry, EntryPathCompareNoCase))
						{
							// We've confirmed that there *is* a matching file
							// Find its exact location
							FileEntryVector::iterator itMatchingItem;
							itMatchingItem = std::lower_bound(m_arStatusArray.begin(), itFirstUnversionedEntry, entry, EntryPathCompareNoCase);

							// adjust the case of the filename
							MoveFileEx(entry->path.GetWinPath(), (*itMatchingItem)->path.GetWinPath(), MOVEFILE_REPLACE_EXISTING);
							DeleteItem(i);
							m_arStatusArray.erase(m_arStatusArray.begin()+i);
							delete entry;
							i--;
							m_nUnversioned--;
							break;
						}
					}
				}
				break;
			} // switch (entry->status) 
		} // if (entry) 
	} // for (int i=0; i < (int)m_arStatusArray.size(); ++i)
}

void CSVNStatusListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	if (pWnd == this)
	{
		int selIndex = GetSelectionMark();
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			GetItemRect(selIndex, &rect, LVIR_LABEL);
			ClientToScreen(&rect);
			point.x = rect.left + rect.Width()/2;
			point.y = rect.top + rect.Height()/2;
		}
		if (selIndex >= 0)
		{
			FileEntry * entry = GetListEntry(selIndex);
			ASSERT(entry != NULL);
			if (entry == NULL)
				return;
			CTSVNPath filepath = entry->path;
			svn_wc_status_kind wcStatus = entry->status;
			//entry is selected, now show the popup menu
			CMenu popup;
			if (popup.CreatePopupMenu())
			{
				CString temp;
				if ((wcStatus > svn_wc_status_normal)&&(wcStatus != svn_wc_status_added)
					&&(wcStatus != svn_wc_status_missing)&&(wcStatus != svn_wc_status_deleted))
				{
					if (GetSelectedCount() == 1)
					{
						if (entry->remotestatus <= svn_wc_status_normal)
						{
							temp.LoadString(IDS_LOG_COMPAREWITHBASE);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMPARE, temp);
						}
						popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
						if ((wcStatus != svn_wc_status_deleted)&&(wcStatus != svn_wc_status_missing))
						{
							temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_GNUDIFF1, temp);
						}
					}
				}
				if (wcStatus > svn_wc_status_normal)
				{
					temp.LoadString(IDS_MENUREVERT);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_REVERT, temp);
				}
				if (entry->remotestatus > svn_wc_status_normal)
				{
					temp.LoadString(IDS_MENUUPDATE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_UPDATE, temp);
					if (GetSelectedCount() == 1)
					{
						temp.LoadString(IDS_LOG_POPUP_COMPARE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMPARE, temp);
						temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_LOG, temp);
					}
				} // if (repoStatus > svn_wc_status_normal)
				if ((wcStatus > svn_wc_status_normal)&&(wcStatus != svn_wc_status_deleted)&&(wcStatus != svn_wc_status_missing) && (GetSelectedCount() == 1))
				{
					temp.LoadString(IDS_REPOBROWSE_OPEN);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_OPEN, temp);
				}
				if (wcStatus == svn_wc_status_unversioned)
				{
					temp.LoadString(IDS_REPOBROWSE_DELETE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_DELETE, temp);
					temp.LoadString(IDS_STATUSLIST_CONTEXT_ADD);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_ADD, temp);

					if (GetSelectedCount() == 1)
					{
						CString filename = filepath.GetFileOrDirectoryName();
						if (filename.ReverseFind('.')>=0)
						{
							CMenu submenu;
							if (submenu.CreateMenu())
							{
								submenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNORE, filename);
								filename = _T("*")+filename.Mid(filename.ReverseFind('.'));
								submenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNOREMASK, filename);
								temp.LoadString(IDS_MENUIGNORE);
								popup.InsertMenu((UINT)-1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)submenu.m_hMenu, temp);
							}
						}
						else
						{
							temp.LoadString(IDS_MENUIGNORE);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNORE, temp);
						}
					}
				}
				if (wcStatus == svn_wc_status_conflicted)
				{
					temp.LoadString(IDS_MENUCONFLICT);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_EDITCONFLICT, temp);
				}
				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
				m_bBlock = TRUE;
				AfxGetApp()->DoWaitCursor(1);
				int iItemCountBeforeMenuCmd = GetItemCount();
				switch (cmd)
				{
				case IDSVNLC_REVERT:
					{
						if (CMessageBox::Show(this->m_hWnd, IDS_PROC_WARNREVERT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)==IDYES)
						{
							POSITION pos = GetFirstSelectedItemPosition();
							int index;
							CTSVNPathList targetList;
							
                            while ((index = GetNextSelectedItem(pos)) >= 0)
							{
								targetList.AddPath(GetListEntry(index)->path);
							}
							SVN svn;
							if (!svn.Revert(targetList, FALSE))
							{
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							}
							else
							{
								//since the entries got reverted we need to remove
								//them from the list too, if no remote changes are shown
								POSITION pos;
								while ((pos = GetFirstSelectedItemPosition())!=0)
								{
									int index;
									index = GetNextSelectedItem(pos);
									const FileEntry * fentry = m_arStatusArray[m_arListArray[index]];
									if (fentry->propstatus <= svn_wc_status_normal)
									{
										m_nTotal--;
										if (GetCheck(index))
											m_nSelected--;
										RemoveListEntry(index);
									}
									else
									{
										SetItemState(index, 0, LVIS_SELECTED);
										Show(m_dwShow);
									}
								}
							}
						}  
					} 
					break;
				case IDSVNLC_COMPARE:
					{
						StartDiff(selIndex);
					}
					break;
				case IDSVNLC_GNUDIFF1:
					{
						CTSVNPath tempfile = CUtils::GetTempFilePath();
						tempfile.AppendString(_T(".diff"));
						SVN svn;
						if (!svn.PegDiff(entry->path, SVNRev::REV_WC, SVNRev::REV_WC, SVNRev::REV_HEAD, TRUE, FALSE, TRUE, _T(""), tempfile))
						{
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							DeleteFile(tempfile.GetWinPath());
							break;		//exit
						}
						m_tempFileList.AddPath(tempfile);
						CUtils::StartDiffViewer(tempfile);
					}
					break;
				case IDSVNLC_UPDATE:
					{
						CTSVNPath tempFile = BuildTargetFile();
						CSVNProgressDlg dlg;
						dlg.SetParams(CSVNProgressDlg::Enum_Update, ProgOptPathIsTempFile, tempFile.GetWinPathString());
						dlg.DoModal();
					}
					break;
				case IDSVNLC_LOG:
					{
						CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
						long revend = reg;
						revend = -revend;
						CLogDlg dlg;
						dlg.SetParams(filepath.GetSVNPathString(), SVNRev::REV_HEAD, revend);
						dlg.DoModal();
					}
					break;
				case IDSVNLC_OPEN:
					{
						ShellExecute(this->m_hWnd, _T("open"), filepath.GetWinPath(), NULL, NULL, SW_SHOW);
					}
					break;
				case IDSVNLC_DELETE:
					{
						CString filelist;
						POSITION pos = GetFirstSelectedItemPosition();
						int index;
						while ((index = GetNextSelectedItem(pos)) >= 0)
						{
							filelist += GetListEntry(index)->path.GetWinPathString();
							filelist += _T("|");
						}
						filelist += _T("|");
						int len = filelist.GetLength();
						TCHAR * buf = new TCHAR[len+2];
						_tcscpy(buf, filelist);
						for (int i=0; i<len; ++i)
							if (buf[i] == '|')
								buf[i] = 0;
						SHFILEOPSTRUCT fileop;
						fileop.hwnd = this->m_hWnd;
						fileop.wFunc = FO_DELETE;
						fileop.pFrom = buf;
						fileop.pTo = NULL;
						fileop.fFlags = FOF_ALLOWUNDO | FOF_NO_CONNECTED_ELEMENTS;
						fileop.lpszProgressTitle = _T("deleting file");
						SHFileOperation(&fileop);
						delete [] buf;

						if (! fileop.fAnyOperationsAborted)
						{
							POSITION pos = NULL;
							while ((pos = GetFirstSelectedItemPosition()) != 0)
							{
								int index = GetNextSelectedItem(pos);
								if (GetCheck(index))
									m_nSelected--;
								m_nTotal--;
								RemoveListEntry(index);
//TODO: If the item we deleted was a directory, we should really remove all its children, too
							}
						}
					}
					break;
				case IDSVNLC_IGNOREMASK:
					{
						CString name = _T("*.")+filepath.GetFileExtension();
						CTSVNPath parentFolder = filepath.GetDirectory(); 
						SVNProperties props(parentFolder.GetSVNPathString());
						CStringA value;
						for (int i=0; i<props.GetCount(); i++)
						{
							CString propname(props.GetItemName(i).c_str());
							if (propname.CompareNoCase(_T("svn:ignore"))==0)
							{
								stdstring stemp;
								stdstring tmp = props.GetItemValue(i);
								//treat values as normal text even if they're not
								value = (char *)tmp.c_str();
							}
						}
						if (value.IsEmpty())
							value = name;
						else
						{
							value = value.Trim("\n\r");
							value += "\n";
							value += name;
							value.Remove('\r');
						}
						if (!props.Add(_T("svn:ignore"), value))
						{
							CString temp;
							temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, name);
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						}
						else
						{
							CTSVNPath basepath;
							int nListboxEntries = GetItemCount();
							for (int i=0; i<nListboxEntries; ++i)
							{
								FileEntry * entry = GetListEntry(i);
								ASSERT(entry != NULL);
								if (entry == NULL)
									continue;	
								if (basepath.IsEmpty())
									basepath = entry->basepath;
								if (parentFolder.IsAncestorOf(entry->path))
								{
									CString f = entry->path.GetSVNPathString();
// TODO: WGD I don't think I understand this bit
									if (f.Mid(parentFolder.GetSVNPathString().GetLength()).Find('/')<=0)
									{
										if (CStringUtils::WildCardMatch(name, f))
										{
											if (GetCheck(i))
												m_nSelected--;
											m_nTotal--;
											RemoveListEntry(i);
											i--;
										}
									}
								}
							}
							SVNStatus status;
							svn_wc_status_t * s;
							CTSVNPath svnPath;
							{
								const TCHAR * pSVNItemPath = NULL;
								s = status.GetFirstFileStatus(parentFolder.GetSVNPathString(), &pSVNItemPath, FALSE);
								svnPath.SetFromSVN(pSVNItemPath);
							}

							if (s!=0)
							{
								FileEntry * entry = new FileEntry();
								entry->path = svnPath;
								entry->basepath = basepath;
								entry->status = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
								entry->textstatus = s->text_status;
								entry->propstatus = s->prop_status;
								entry->remotestatus = SVNStatus::GetMoreImportant(s->repos_text_status, s->repos_prop_status);
								entry->remotetextstatus = s->repos_text_status;
								entry->remotepropstatus = s->repos_prop_status;
								entry->inunversionedfolder = false;
								entry->checked = true;
								entry->inexternal = false;
								entry->direct = false;
								if (s->entry)
								{
									if (s->entry->url)
									{
										CUtils::Unescape((char *)s->entry->url);
										entry->url = CUnicodeUtils::GetUnicode(s->entry->url);
									}
								}
								m_arStatusArray.push_back(entry);
								m_arListArray.push_back(m_arStatusArray.size()-1);
								AddEntry(entry, langID, GetItemCount());
							}

						}
					}
					break;
				case IDSVNLC_IGNORE:
					{
						CString name = filepath.GetFilename();
						CTSVNPath parentfolder = filepath.GetDirectory();
						SVNProperties props(parentfolder.GetSVNPathString());
						CStringA value;
						for (int i=0; i<props.GetCount(); i++)
						{
							CString propname(props.GetItemName(i).c_str());
							if (propname.CompareNoCase(_T("svn:ignore"))==0)
							{
								stdstring stemp;
								stdstring tmp = props.GetItemValue(i);
								//treat values as normal text even if they're not
								value = (char *)tmp.c_str();
							}
						}
						if (value.IsEmpty())
							value = name;
						else
						{
							value = value.Trim("\n\r");
							value += "\n";
							value += name;
							value.Remove('\r');
						}
						if (!props.Add(_T("svn:ignore"), value))
						{
							CString temp;
							temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, name);
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
						if (GetCheck(selIndex))
							m_nSelected--;
						m_nTotal--;
						CTSVNPath basepath = m_arStatusArray[m_arListArray[selIndex]]->basepath;
						RemoveListEntry(selIndex);
						SVNStatus status;
						svn_wc_status_t * s;
						CTSVNPath svnPath;
						{
							const TCHAR * pSVNItemPath = NULL;
							s = status.GetFirstFileStatus(parentfolder.GetSVNPathString(), &pSVNItemPath, FALSE);
							svnPath.SetFromSVN(pSVNItemPath);
						}
						if (s!=0)
						{
							FileEntry * entry = new FileEntry();
							entry->path = svnPath;
							entry->basepath = basepath;
							entry->status = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
							entry->textstatus = s->text_status;
							entry->propstatus = s->prop_status;
							entry->remotestatus = SVNStatus::GetMoreImportant(s->repos_text_status, s->repos_prop_status);
							entry->remotetextstatus = s->repos_text_status;
							entry->remotepropstatus = s->repos_prop_status;
							entry->inunversionedfolder = FALSE;
							entry->checked = true;
							entry->inexternal = false;
							entry->direct = false;
							if (s->entry)
							{
								if (s->entry->url)
								{
									CUtils::Unescape((char *)s->entry->url);
									entry->url = CUnicodeUtils::GetUnicode(s->entry->url);
								}
							}
							m_arStatusArray.push_back(entry);
							m_arListArray.push_back(m_arStatusArray.size()-1);
							AddEntry(entry, langID, GetItemCount());
						}
					}
					break;
				case IDSVNLC_EDITCONFLICT:
					{
						CTSVNPath merge = filepath;
						CTSVNPath directory = merge.GetDirectory();
						CTSVNPath theirs(directory);
						CTSVNPath mine(directory);
						CTSVNPath base(directory);

						//we have the conflicted file (%merged)
						//now look for the other required files
						SVNStatus stat;
						stat.GetStatus(merge.GetSVNPathString());
						if (stat.status->entry)
						{
							if (stat.status->entry->conflict_new)
							{
								theirs.AppendString(_T("/")+CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new));
							}
							if (stat.status->entry->conflict_old)
							{
								base.AppendString(_T("/")+CUnicodeUtils::GetUnicode(stat.status->entry->conflict_old));
							}
							if (stat.status->entry->conflict_wrk)
							{
								mine.AppendString(_T("/")+CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk));
							}
						}
						CUtils::StartExtMerge(base.GetWinPathString(),theirs.GetWinPathString(),mine.GetWinPathString(),merge.GetWinPathString());
					}
					break;
				case IDSVNLC_ADD:
					{
						SVN svn;
						CTSVNPathList itemsToAdd;
						POSITION pos = GetFirstSelectedItemPosition();
						int index;
						while ((index = GetNextSelectedItem(pos)) >= 0)
						{
							itemsToAdd.AddPath(GetListEntry(index)->path);
						}

						// We must sort items before adding, so that folders are always added
						// *before* any of their children
						itemsToAdd.SortByPathname();

						if (svn.Add(itemsToAdd, FALSE, TRUE))
						{
							// The add went ok, but we now need to run through the selected items again
							// and update their status
							pos = GetFirstSelectedItemPosition();
							while ((index = GetNextSelectedItem(pos)) >= 0)
							{
								FileEntry * e = GetListEntry(index);
								e->textstatus = svn_wc_status_added;
								e->status = svn_wc_status_added;
								SetEntryCheck(e,index,true);
							}
						}
						else
						{
							CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
						Show(m_dwShow);
					}
					break;
				default:
					m_bBlock = FALSE;
					break;
				} // switch (cmd)
				m_bBlock = FALSE;
				AfxGetApp()->DoWaitCursor(-1);
				GetStatisticsString();
				int iItemCountAfterMenuCmd = GetItemCount();
				if (iItemCountAfterMenuCmd != iItemCountBeforeMenuCmd)
				{
					CWnd* pParent = GetParent();
					if (NULL != pParent && NULL != pParent->GetSafeHwnd())
					{
						pParent->SendMessage(SVNSLNM_ITEMCOUNTCHANGED);
					}
				}
			} // if (popup.CreatePopupMenu())
		} // if (selIndex >= 0)
	} // if (pWnd == this)
}

void CSVNStatusListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (m_bBlock)
		return;

	StartDiff(pNMLV->iItem);
}

void CSVNStatusListCtrl::StartDiff(int fileindex)
{
	if (fileindex < 0)
		return;
	FileEntry * entry = GetListEntry(fileindex);
	ASSERT(entry != NULL);
	if (entry == NULL)
		return;
	if (entry->status == svn_wc_status_added)
		return;		//we don't compare an added file to itself
	if (entry->status == svn_wc_status_deleted)
		return;		//we don't compare a deleted file (nothing) with something
	if (entry->status == svn_wc_status_unversioned)
		return;		//we don't compare new files with nothing
	if (entry->path.IsDirectory())
		return;		//we also don't compare folders
	CTSVNPath path1;
	CString path2;
	CString path3;

	if (entry->status > svn_wc_status_normal)
		path2 = SVN::GetPristinePath(entry->path.GetSVNPathString());

	if (entry->remotestatus > svn_wc_status_normal)
	{
		path3 = CUtils::GetTempFile();

		SVN svn;
		if (!svn.Cat(entry->path, SVNRev::REV_HEAD, CTSVNPath(path3)))
		{
			CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		}
		m_tempFileList.AddPath(CTSVNPath(path3));
	}

	if ((!CRegDWORD(_T("Software\\TortoiseSVN\\DontConvertBase"), TRUE))&&(SVN::GetTranslatedFile(path1, entry->path)))
	{
		m_tempFileList.AddPath(path1);
	}
	else
	{
		path1 = entry->path;
	}

	CString name = entry->path.GetFilename();
	CString ext = entry->path.GetFileExtension();
	CString n1, n2, n3;
	n1.Format(IDS_DIFF_WCNAME, name);
	n2.Format(IDS_DIFF_BASENAME, name);
	n3.Format(IDS_DIFF_REMOTENAME, name);

	if (path2.IsEmpty())
		CUtils::StartDiffViewer(path1, CTSVNPath(path3), FALSE, n1, n3, ext);
	else if (path3.IsEmpty())
		CUtils::StartDiffViewer(CTSVNPath(path2), path1, FALSE, n2, n1, ext);
	else
		CUtils::StartExtMerge(path2, path3, path1.GetWinPathString(), _T(""), n2, n3, n1);
}

CString CSVNStatusListCtrl::GetStatisticsString()
{
	CString sNormal, sAdded, sDeleted, sModified, sConflicted, sUnversioned;
	WORD langID = (WORD)(DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
	TCHAR buf[MAX_PATH];
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_normal, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sNormal = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_added, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sAdded = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_deleted, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sDeleted = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_modified, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sModified = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_conflicted, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sConflicted = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_unversioned, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sUnversioned = buf;
	CString sToolTip;
	sToolTip.Format(_T("%s = %d\n%s = %d\n%s = %d\n%s = %d\n%s = %d\n%s = %d"),
		(LPCTSTR)sNormal, m_nNormal,
		(LPCTSTR)sUnversioned, m_nUnversioned,
		(LPCTSTR)sModified, m_nModified,
		(LPCTSTR)sAdded, m_nAdded,
		(LPCTSTR)sDeleted, m_nDeleted,
		(LPCTSTR)sConflicted, m_nConflicted
		);
	CString sStats;
	sStats.Format(IDS_LOGPROMPT_STATISTICSFORMAT, m_nSelected, GetItemCount());
	if (m_pStatLabel)
	{
		m_pStatLabel->SetWindowText(sStats);
	}

	if (m_pSelectButton)
	{
		if (m_nSelected == 0)
			m_pSelectButton->SetCheck(BST_UNCHECKED);
		else if (m_nSelected != GetItemCount())
			m_pSelectButton->SetCheck(BST_INDETERMINATE);
		else
			m_pSelectButton->SetCheck(BST_CHECKED);
	}
	return sToolTip;
}

void CSVNStatusListCtrl::SelectAll(bool bSelect)
{
	CWinApp * pApp = AfxGetApp();
	if (pApp)
		pApp->DoWaitCursor(1);
	m_bBlock = TRUE;
	SetRedraw(FALSE);	
	int nListItems = GetItemCount();
	for (int i=0; i<nListItems; ++i)
	{
		FileEntry * entry = GetListEntry(i);
		ASSERT(entry != NULL);
		if (entry == NULL)
			continue;
		SetEntryCheck(entry,i,bSelect);
	}
	if (bSelect)
		m_nSelected = nListItems;
	else
		m_nSelected = 0;
	SetRedraw(TRUE);
	GetStatisticsString();
	if (pApp)
		pApp->DoWaitCursor(1);
	m_bBlock = FALSE;
}

void CSVNStatusListCtrl::OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	if (GetListEntry(pGetInfoTip->iItem))
		if (pGetInfoTip->cchTextMax > GetListEntry(pGetInfoTip->iItem)->path.GetSVNPathString().GetLength())
			_tcsncpy(pGetInfoTip->pszText, GetListEntry(pGetInfoTip->iItem)->path.GetSVNPathString(), pGetInfoTip->cchTextMax);
	*pResult = 0;
}

void CSVNStatusListCtrl::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

		if (m_arListArray.size() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
		{
			FileEntry * entry = GetListEntry((int)pLVCD->nmcd.dwItemSpec);
			ASSERT(entry != NULL);
			if (entry == NULL)
				return;

			switch (entry->status)
			{
			case svn_wc_status_added:
				crText = GetSysColor(COLOR_HIGHLIGHT);
				break;
			case svn_wc_status_missing:
			case svn_wc_status_deleted:
			case svn_wc_status_replaced:
				crText = RGB(100,0,0);
				break;
			case svn_wc_status_modified:
				crText = GetSysColor(COLOR_HIGHLIGHT);
				break;
			case svn_wc_status_merged:
				crText = RGB(0, 100, 0);
				break;
			case svn_wc_status_conflicted:
			case svn_wc_status_obstructed:
				crText = RGB(255, 0, 0);
				break;
			case svn_wc_status_none:
			case svn_wc_status_unversioned:
			case svn_wc_status_ignored:
			case svn_wc_status_incomplete:
			case svn_wc_status_normal:
			case svn_wc_status_external:
			default:
				crText = GetSysColor(COLOR_WINDOWTEXT);
				break;
			}
			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
		}
	}
}

CTSVNPath CSVNStatusListCtrl::BuildTargetFile()
{
	CTSVNPath tempFile = CUtils::GetTempFilePath();
	m_tempFileList.AddPath(tempFile);
	HANDLE file = ::CreateFile (tempFile.GetWinPath(),
		GENERIC_WRITE, 
		FILE_SHARE_READ, 
		0, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_TEMPORARY,
		0);
	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		DWORD written = 0;
		::WriteFile (file, (LPCTSTR)GetListEntry(index)->path.GetSVNPathString(), GetListEntry(index)->path.GetSVNPathString().GetLength()*sizeof(TCHAR), &written, 0);
		::WriteFile (file, _T("\n"), 2, &written, 0);
	}
	::CloseHandle(file);
	return tempFile;
}

BOOL CSVNStatusListCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd != this)
		return CListCtrl::OnSetCursor(pWnd, nHitTest, message);
	if (!m_bBlock)
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
		SetCursor(hCur);
		return CListCtrl::OnSetCursor(pWnd, nHitTest, message);
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
	SetCursor(hCur);
	return TRUE;
}

void CSVNStatusListCtrl::RemoveListEntry(int index)
{
	DeleteItem(index);
	delete m_arStatusArray[m_arListArray[index]];
	m_arStatusArray.erase(m_arStatusArray.begin()+m_arListArray[index]);
	m_arListArray.erase(m_arListArray.begin()+index);
	for (int i=index; i< (int)m_arListArray.size(); ++i)
	{
		m_arListArray[i]--;
	}
}

///< Set a checkbox on an entry in the listbox
void CSVNStatusListCtrl::SetEntryCheck(FileEntry* pEntry, int listboxIndex, bool bCheck)
{
	pEntry->checked = bCheck;
	SetCheck(listboxIndex, bCheck);
}


void CSVNStatusListCtrl::SetCheckOnAllDescendentsOf(const FileEntry* parentEntry, bool bCheck)
{
	int nListItems = GetItemCount();
	for (int j=0; j< nListItems ; ++j)
	{
		FileEntry * childEntry = GetListEntry(j);
		ASSERT(childEntry != NULL);
		if (childEntry == NULL || childEntry == parentEntry)
			continue;
		if (childEntry->checked != bCheck)
		{
			if (parentEntry->path.IsAncestorOf(childEntry->path))
			{
				SetEntryCheck(childEntry,j,bCheck);
				if(bCheck)
				{
					m_nSelected++;
				}
				else
				{
					m_nSelected--;
				}
			}
		}
	}
}

bool CSVNStatusListCtrl::WriteCheckedNamesToFile(const CString& sFilename)
{
	CTSVNPathList checkedPaths;
	int nListItems = GetItemCount();
	for (int i=0; i< nListItems; i++)
	{
		const FileEntry* entry = GetListEntry(i);
		ASSERT(entry != NULL);
		if (entry->IsChecked())
		{
			checkedPaths.AddPath(entry->path);
		}
	}
	checkedPaths.SortByPathname();
	return checkedPaths.WriteToTemporaryFile(sFilename);
}
