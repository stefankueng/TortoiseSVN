// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include "Svnstatuslistctrl.h"
#include "TSVNPath.h"
#include "Registry.h"
#include "SVNStatus.h"
#include "InputDlg.h"
#include "ShellUpdater.h"
#include ".\svnstatuslistctrl.h"

const UINT CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED
					= ::RegisterWindowMessage(_T("SVNSLNM_ITEMCOUNTCHANGED"));
const UINT CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH
					= ::RegisterWindowMessage(_T("SVNSLNM_NEEDSREFRESH"));

#define IDSVNLC_REVERT			 1
#define IDSVNLC_COMPARE			 2
#define IDSVNLC_OPEN			 3
#define IDSVNLC_DELETE			 4
#define IDSVNLC_IGNORE			 5
#define IDSVNLC_GNUDIFF1		 6
#define IDSVNLC_UPDATE           7
#define IDSVNLC_LOG              8
#define IDSVNLC_EDITCONFLICT     9
#define IDSVNLC_IGNOREMASK	    10
#define IDSVNLC_ADD			    11
#define IDSVNLC_RESOLVECONFLICT 12
#define IDSVNLC_LOCK			13
#define IDSVNLC_LOCKFORCE		14
#define IDSVNLC_UNLOCK			15
#define IDSVNLC_UNLOCKFORCE		16
#define IDSVNLC_OPENWITH		17
#define IDSVNLC_EXPLORE			18


BEGIN_MESSAGE_MAP(CSVNStatusListCtrl, CListCtrl)
	ON_NOTIFY(HDN_ITEMCLICKA, 0, OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, OnHdnItemclick)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnLvnItemchanged)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_WM_SETCURSOR()
	ON_WM_GETDLGCODE()
	ON_NOTIFY_REFLECT(NM_RETURN, OnNMReturn)
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()


bool	CSVNStatusListCtrl::m_bAscending = false;
int		CSVNStatusListCtrl::m_nSortedColumn = -1;
int		CSVNStatusListCtrl::m_nSortedInternalColumn = -1;

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

void CSVNStatusListCtrl::Init(DWORD dwColumns, DWORD dwContextMenus /* = SVNSLC_POPALL */, bool bHasCheckboxes /* = true */)
{
	m_dwColumns = dwColumns;
	m_dwContextMenus = dwContextMenus;
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
	if (dwColumns & SVNSLC_COLEXT)
	{
		temp.LoadString(IDS_STATUSLIST_COLEXT);
		InsertColumn(nCol++, temp);
	}
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
	if (dwColumns & SVNSLC_COLLOCK)
	{
		temp.LoadString(IDS_STATUSLIST_COLLOCK);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLLOCKCOMMENT)
	{
		temp.LoadString(IDS_STATUSLIST_COLLOCKCOMMENT);
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

BOOL CSVNStatusListCtrl::GetStatus(const CTSVNPathList& pathList, bool bUpdate /* = FALSE */)
{
	BOOL bRet = TRUE;
	m_nTargetCount = 0;
	m_bHasExternalsFromDifferentRepos = FALSE;
	m_bHasExternals = FALSE;
	m_bHasUnversionedItems = FALSE;
	m_nSortedColumn = 0;
	m_bBlock = TRUE;

	// force the cursor to change
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);

	// first clear possible status data left from
	// previous GetStatus() calls
	ClearStatusArray();

	m_StatusFileList = pathList;

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

	m_nTargetCount = pathList.GetCount();

	SVNStatus status;
	if(m_nTargetCount > 1 && pathList.AreAllPathsFilesInOneDirectory())
	{
		// This is a special case, where we've been given a list of files
		// all from one folder
		// We handle them by setting a status filter, then requesting the SVN status of 
		// all the files in the directory, filtering for the ones we're interested in
		status.SetFilter(pathList);

		if(!FetchStatusForSingleTarget(config, status, pathList.GetCommonDirectory(), bUpdate, sUUID, arExtPaths, true))
		{
			bRet = FALSE;
		}
	}
	else
	{
		for(int nTarget = 0; nTarget < m_nTargetCount; nTarget++)
		{
			if(!FetchStatusForSingleTarget(config, status, pathList[nTarget], bUpdate, sUUID, arExtPaths, false))
			{
				bRet = FALSE;
			}
		}
	}

	BuildStatistics();
	m_bBlock = FALSE;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
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
							CTSVNPathList& arExtPaths,
							bool bAllDirect
							)
{
	apr_array_header_t* pIgnorePatterns = NULL;
	//BUGBUG - Some error handling needed here
	config.GetDefaultIgnores(&pIgnorePatterns);

	CTSVNPath workingTarget(target);

	svn_wc_status2_t * s;
	CTSVNPath svnPath;
	s = status.GetFirstFileStatus(workingTarget, svnPath, bFetchStatusFromRepository);

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
					s = status.GetNextFileStatus(svnPath);
					if(workingTarget.IsEquivalentTo(svnPath))
					{
						break;
					}
				}
				// Now, set working target to be the base folder of this item
				workingTarget = workingTarget.GetDirectory();
			}
		}

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
			if (SVNStatus::GetAllStatus(workingTarget) != svn_wc_status_unversioned)
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
			ReadRemainingItemsStatus(status, workingTarget, strCurrentRepositoryUUID, arExtPaths, pIgnorePatterns, bAllDirect);
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
			const svn_wc_status2_t* pSVNStatus,  // The return from the SVN GetStatus functions
			const CTSVNPath& path,				// The path of the item we're adding
			const CTSVNPath& basePath,			// The base directory for this status build
			bool bDirectItem,					// Was this item the first found by GetFirstFileStatus or by a subsequent GetNextFileStatus call
			bool bInExternal					// Are we in an 'external' folder
			)
{
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
	entry->lRevision = 0;
	entry->isNested = false;
	if (pSVNStatus->entry)
	{
		entry->isfolder = (pSVNStatus->entry->kind == svn_node_dir);
		entry->lRevision = pSVNStatus->entry->revision;

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
		if (pSVNStatus->entry->lock_owner)
			entry->lock_owner = CUnicodeUtils::GetUnicode(pSVNStatus->entry->lock_owner);
		if (pSVNStatus->entry->lock_token)
			entry->lock_token = CUnicodeUtils::GetUnicode(pSVNStatus->entry->lock_token);
		if (pSVNStatus->entry->lock_comment)
			entry->lock_comment = CUnicodeUtils::GetUnicode(pSVNStatus->entry->lock_comment);
	}
	else
	{
		entry->isfolder = path.IsDirectory();
	}
	if (pSVNStatus->repos_lock)
	{
		if (pSVNStatus->repos_lock->owner)
			entry->lock_remoteowner = CUnicodeUtils::GetUnicode(pSVNStatus->repos_lock->owner);
		if (pSVNStatus->repos_lock->token)
			entry->lock_remotetoken = CUnicodeUtils::GetUnicode(pSVNStatus->repos_lock->token);
		if (pSVNStatus->repos_lock->comment)
			entry->lock_comment = CUnicodeUtils::GetUnicode(pSVNStatus->repos_lock->comment);
	}

	// Pass ownership of the entry to the array
	m_arStatusArray.push_back(entry);

	return entry;
}

void CSVNStatusListCtrl::AddUnversionedFolder(const CTSVNPath& folderName, 
												const CTSVNPath& basePath, 
												apr_array_header_t *pIgnorePatterns)
{
	CSimpleFileFind filefinder(folderName.GetWinPathString());

	CTSVNPath filename;
	m_bHasUnversionedItems = TRUE;
	while (filefinder.FindNextFileNoDots())
	{
		filename.SetFromWin(filefinder.GetFilePath(), filefinder.IsDirectory());
		if (!SVNConfig::MatchIgnorePattern(filename.GetSVNPathString(),pIgnorePatterns))
		{
			FileEntry * entry = new FileEntry();
			entry->path = filename;
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
			entry->isfolder = filefinder.IsDirectory(); 
			entry->lRevision = 0;
			entry->isNested = false;
			
			m_arStatusArray.push_back(entry);
			if (entry->isfolder)
			{
				if (!PathFileExists(entry->path.GetWinPathString() + _T("\\") + _T(SVN_WC_ADM_DIR_NAME)))
					AddUnversionedFolder(entry->path, basePath, pIgnorePatterns);
			}
		}
	} // while (filefinder.NextFile(filename,&bIsDirectory))
}


void CSVNStatusListCtrl::ReadRemainingItemsStatus(SVNStatus& status, const CTSVNPath& basePath, 
										  CStringA& strCurrentRepositoryUUID, 
										  CTSVNPathList& arExtPaths, apr_array_header_t *pIgnorePatterns, bool bAllDirect)
{
	svn_wc_status2_t * s;

	CTSVNPath lastexternalpath;
	CTSVNPath svnPath;
	while ((s = status.GetNextFileStatus(svnPath)) != NULL)
	{
		svn_wc_status_kind wcFileStatus = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
		if ((wcFileStatus == svn_wc_status_unversioned) && (svnPath.IsDirectory()))
		{
			// check if the unversioned folder is maybe versioned. This
			// could happen with nested layouts
			if (SVNStatus::GetAllStatus(svnPath) != svn_wc_status_unversioned)
			{
				FileEntry * entry = new FileEntry();
				entry->path = svnPath;
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
				entry->isfolder = true; 
				entry->lRevision = 0;
				entry->isNested = true;
				m_arStatusArray.push_back(entry);
				continue;
			}
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

		const FileEntry* entry = AddNewFileEntry(s, svnPath, basePath, bAllDirect, bDirectoryIsExternal);

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

void CSVNStatusListCtrl::Show(DWORD dwShow, DWORD dwCheck /*=0*/, bool bShowFolders /* = true */)
{
	WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	CWinApp * pApp = AfxGetApp();
	if (pApp)
		pApp->DoWaitCursor(1);
	m_dwShow = dwShow;
	m_bShowFolders = bShowFolders;
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
		if (entry->IsFolder() && (!bShowFolders))
			continue;	// don't show folders if they're not wanted.
		svn_wc_status_kind status = SVNStatus::GetMoreImportant(entry->status, entry->remotestatus);
		DWORD showFlags = GetShowFlagsFromSVNStatus(status);
		if (entry->IsLocked())
			showFlags |= SVNSLC_SHOWLOCKS;
			
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
	}

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

	CHeaderCtrl * pHeader = GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	if (m_nSortedColumn)
	{
		pHeader->GetItem(m_nSortedColumn, &HeaderItem);
		HeaderItem.fmt |= (m_bAscending ? HDF_SORTDOWN : HDF_SORTUP);
		pHeader->SetItem(m_nSortedColumn, &HeaderItem);
	}

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
	if (m_dwColumns & SVNSLC_COLEXT)
	{
		SetItemText(index, nCol++, entry->path.GetFileExtension());
	}
	if (m_dwColumns & SVNSLC_COLSTATUS)
	{
		if (entry->isNested)
		{
			CString sTemp(MAKEINTRESOURCE(IDS_STATUSLIST_NESTED));
			SetItemText(index, nCol++, sTemp);			
		}
		else
		{
			SVNStatus::GetStatusString(hResourceHandle, entry->status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
			if ((entry->status == entry->propstatus)&&
				(entry->status != svn_wc_status_normal)&&
				(entry->status != svn_wc_status_unversioned)&&
				(!SVNStatus::IsImportant(entry->textstatus)))
				_tcscat(buf, ponly);
			SetItemText(index, nCol++, buf);
		}
	}
	if (m_dwColumns & SVNSLC_COLREMOTESTATUS)
	{
		if (entry->isNested)
		{
			CString sTemp(MAKEINTRESOURCE(IDS_STATUSLIST_NESTED));
			SetItemText(index, nCol++, sTemp);			
		}
		else
		{
			SVNStatus::GetStatusString(hResourceHandle, entry->remotestatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
			if ((entry->remotestatus == entry->remotepropstatus)&&
				(entry->status != svn_wc_status_normal)&&
				(entry->status != svn_wc_status_unversioned)&&
				(!SVNStatus::IsImportant(entry->remotetextstatus)))
				_tcscat(buf, ponly);
			SetItemText(index, nCol++, buf);
		}
	}
	if (m_dwColumns & SVNSLC_COLTEXTSTATUS)
	{
		if (entry->isNested)
		{
			CString sTemp(MAKEINTRESOURCE(IDS_STATUSLIST_NESTED));
			SetItemText(index, nCol++, sTemp);			
		}
		else
		{
			SVNStatus::GetStatusString(hResourceHandle, entry->textstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
			SetItemText(index, nCol++, buf);
		}
	}
	if (m_dwColumns & SVNSLC_COLPROPSTATUS)
	{
		if (entry->isNested)
		{
			SetItemText(index, nCol++, _T(""));
		}
		else
		{
			SVNStatus::GetStatusString(hResourceHandle, entry->propstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
			SetItemText(index, nCol++, buf);
		}
	}
	if (m_dwColumns & SVNSLC_COLREMOTETEXT)
	{
		if (entry->isNested)
		{
			SetItemText(index, nCol++, _T(""));
		}
		else
		{
			SVNStatus::GetStatusString(hResourceHandle, entry->remotetextstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
			SetItemText(index, nCol++, buf);
		}
	}
	if (m_dwColumns & SVNSLC_COLREMOTEPROP)
	{
		if (entry->isNested)
		{
			SetItemText(index, nCol++, _T(""));
		}
		else
		{
			SVNStatus::GetStatusString(hResourceHandle, entry->remotepropstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
			SetItemText(index, nCol++, buf);
		}
	}
	if (m_dwColumns & SVNSLC_COLURL)
	{
		SetItemText(index, nCol++, entry->url);
	}
	if (m_dwColumns & SVNSLC_COLLOCK)
	{
		if (!m_HeadRev.IsHead())
		{
			// we have contacted the repository
			
			// decision-matrix
			// wc		repository		text
			// ""		""				""
			// ""		UID1			owner
			// UID1		UID1			owner
			// UID1		""				lock has been broken
			// UID1		UID2			lock has been stolen
			if (entry->lock_token.IsEmpty() || (entry->lock_token.Compare(entry->lock_remotetoken)==0))
			{
				if (entry->lock_owner.IsEmpty())
					SetItemText(index, nCol++, entry->lock_remoteowner);
				else
					SetItemText(index, nCol++, entry->lock_owner);
			}
			else if (entry->lock_remotetoken.IsEmpty())
			{
				// broken lock
				CString temp(MAKEINTRESOURCE(IDS_STATUSLIST_LOCKBROKEN));
				SetItemText(index, nCol++, temp);
			}
			else
			{
				// stolen lock
				CString temp;
				temp.Format(IDS_STATUSLIST_LOCKSTOLEN, entry->lock_remoteowner);
				SetItemText(index, nCol++, temp);
			}			
		}
		else
			SetItemText(index, nCol++, entry->lock_owner);
	}
	if (m_dwColumns & SVNSLC_COLLOCKCOMMENT)
	{
		SetItemText(index, nCol++, entry->lock_comment);
	}
	SetCheck(index, entry->checked);
	if (entry->checked)
		m_nSelected++;
	m_bBlock = FALSE;
}

void CSVNStatusListCtrl::Sort()
{
	std::sort(m_arStatusArray.begin(), m_arStatusArray.end(), SortCompare);
	Show(m_dwShow, 0, m_bShowFolders);
}

bool CSVNStatusListCtrl::SortCompare(const FileEntry* entry1, const FileEntry* entry2)
{
	int result = 0;
	switch (m_nSortedInternalColumn)
	{
	case 10:
		{
			if (result == 0)
			{
				result = entry1->lock_comment.CompareNoCase(entry2->lock_comment);
			}
		}
	case 9:
		{
			if (result == 0)
			{
				result = entry1->lock_owner.CompareNoCase(entry2->lock_owner);
			}
		}
	case 8:
		{
			if (result == 0)
			{
				result = entry1->url.CompareNoCase(entry2->url);
			}
		}
	case 7:
		{
			if (result == 0)
			{
				result = entry1->remotepropstatus - entry2->remotepropstatus;
			}
		}
	case 6:
		{
			if (result == 0)
			{
				result = entry1->remotetextstatus - entry2->remotetextstatus;
			}
		}
	case 5:
		{
			if (result == 0)
			{
				result = entry1->propstatus - entry2->propstatus;
			}
		}
	case 4:
		{
			if (result == 0)
			{
				result = entry1->textstatus - entry2->textstatus;
			}
		}
	case 3:
		{
			if (result == 0)
			{
				result = entry1->remotestatus - entry2->remotestatus;
			}
		}
	case 2:
		{
			if (result == 0)
			{
				result = entry1->status - entry2->status;
			}
		}
	case 1:
		{
			if (result == 0)
			{
				result = entry1->path.GetFileExtension().CompareNoCase(entry2->path.GetFileExtension());
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
	m_nSortedInternalColumn = 0;
	// get the internal column from the visible columns
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLEXT)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLSTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLREMOTESTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLTEXTSTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLPROPSTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLREMOTETEXT)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLREMOTEPROP)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLURL)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLLOCK)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		m_nSortedInternalColumn++;
		if (m_dwColumns & SVNSLC_COLLOCKCOMMENT)
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
					if (testEntry->path.IsAncestorOf(folderpath) && (!testEntry->path.IsEquivalentTo(folderpath)))
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
					if (testEntry->path.IsAncestorOf(entry->path) && (!testEntry->path.IsEquivalentTo(entry->path)))
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
	return pEntry1->path < pEntry2->path;
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

void CSVNStatusListCtrl::GetMinMaxRevisions(LONG& lMin, LONG& lMax)
{
	lMin = LONG_MAX;
	lMax = 0;
	for (int i=0; i < (int)m_arStatusArray.size(); ++i)
	{
		const FileEntry * entry = m_arStatusArray[i];
		
		if ((entry)&&(entry->lRevision))
		{
			lMin = min(lMin, entry->lRevision);
			lMax = max(lMax, entry->lRevision);
		}	
	}
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
			point = rect.CenterPoint();
		}
		if (selIndex >= 0)
		{
			FileEntry * entry = GetListEntry(selIndex);
			ASSERT(entry != NULL);
			if (entry == NULL)
				return;
			const CTSVNPath& filepath = entry->path;
			svn_wc_status_kind wcStatus = entry->status;
			//entry is selected, now show the popup menu
			CMenu popup;
			if (popup.CreatePopupMenu())
			{
				CString temp;
				if ((wcStatus >= svn_wc_status_normal)
					&&(wcStatus != svn_wc_status_missing)&&(wcStatus != svn_wc_status_deleted))
				{
					if (GetSelectedCount() == 1)
					{
						if (entry->remotestatus <= svn_wc_status_normal)
						{
							if ((wcStatus > svn_wc_status_normal)
								&&(wcStatus != svn_wc_status_added)
								&&(wcStatus != svn_wc_status_missing)
								&&(wcStatus != svn_wc_status_deleted))
							{
								if (m_dwContextMenus & SVNSLC_POPCOMPAREWITHBASE)
								{
									temp.LoadString(IDS_LOG_COMPAREWITHBASE);
									popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMPARE, temp);
									popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
								}
								if (m_dwContextMenus & SVNSLC_POPGNUDIFF)
								{
									temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
									popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_GNUDIFF1, temp);
								}
							}
						}
						else
						{
							if (m_dwContextMenus & SVNSLC_POPCOMPARE)
							{
								temp.LoadString(IDS_LOG_POPUP_COMPARE);
								popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMPARE, temp);
								popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
							}
							if (m_dwContextMenus & SVNSLC_POPGNUDIFF)
							{
								temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
								popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_GNUDIFF1, temp);
							}
						}
						popup.AppendMenu(MF_SEPARATOR);
					}
				}
				if (wcStatus > svn_wc_status_normal)
				{
					if (m_dwContextMenus & SVNSLC_POPREVERT)
					{
						temp.LoadString(IDS_MENUREVERT);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_REVERT, temp);
					}
				}
				if (entry->remotestatus > svn_wc_status_normal)
				{
					if (m_dwContextMenus & SVNSLC_POPUPDATE)
					{
						temp.LoadString(IDS_MENUUPDATE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_UPDATE, temp);
					}
					if (GetSelectedCount() == 1)
					{
						if (m_dwContextMenus & SVNSLC_POPSHOWLOG)
						{
							temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_LOG, temp);
						}
					}
				}
				if ((wcStatus != svn_wc_status_deleted)&&(wcStatus != svn_wc_status_missing) && (GetSelectedCount() == 1))
				{
					if (m_dwContextMenus & SVNSLC_POPOPEN)
					{
						temp.LoadString(IDS_REPOBROWSE_OPEN);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_OPEN, temp);
						temp.LoadString(IDS_LOG_POPUP_OPENWITH);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_OPENWITH, temp);
					}
					if (m_dwContextMenus & SVNSLC_POPEXPLORE)
					{
						temp.LoadString(IDS_STATUSLIST_CONTEXT_EXPLORE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_EXPLORE, temp);
					}
				}
				if (wcStatus == svn_wc_status_unversioned)
				{
					if (m_dwContextMenus & SVNSLC_POPDELETE)
					{
						temp.LoadString(IDS_REPOBROWSE_DELETE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_DELETE, temp);
					}
					if (m_dwContextMenus & SVNSLC_POPADD)
					{
						temp.LoadString(IDS_STATUSLIST_CONTEXT_ADD);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_ADD, temp);
					}

					if (GetSelectedCount() == 1)
					{
						if (m_dwContextMenus & SVNSLC_POPIGNORE)
						{
							CString filename = filepath.GetFileOrDirectoryName();
							if ((filename.ReverseFind('.')>=0)&&(!filepath.IsDirectory()))
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
				}
				if ((wcStatus == svn_wc_status_conflicted)&&(GetSelectedCount()==1))
				{
					if ((m_dwContextMenus & SVNSLC_POPCONFLICT)||(m_dwContextMenus & SVNSLC_POPRESOLVE))
					popup.AppendMenu(MF_SEPARATOR);
					
					if (m_dwContextMenus & SVNSLC_POPCONFLICT)
					{
						temp.LoadString(IDS_MENUCONFLICT);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_EDITCONFLICT, temp);
					}
					if (m_dwContextMenus & SVNSLC_POPRESOLVE)
					{
						temp.LoadString(IDS_MENURESOLVE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_RESOLVECONFLICT, temp);
					}
				}
				if ((!entry->IsFolder())&&(wcStatus >= svn_wc_status_normal)
					&&(wcStatus!=svn_wc_status_missing)&&(wcStatus!=svn_wc_status_deleted)
					&&(wcStatus!=svn_wc_status_added))
				{
					popup.AppendMenu(MF_SEPARATOR);
					if (!entry->IsFolder())
					{
						if (m_dwContextMenus & SVNSLC_POPLOCK)
						{
							temp.LoadString(IDS_MENU_LOCK);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_LOCK, temp);					
						}
					}
					if ((!entry->lock_token.IsEmpty())&&(!entry->IsFolder()))
					{
						if (m_dwContextMenus & SVNSLC_POPUNLOCK)
						{
							temp.LoadString(IDS_MENU_UNLOCK);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_UNLOCK, temp);
						}
					}
					if ((!entry->IsFolder())&&((!entry->lock_token.IsEmpty())||(!entry->lock_remotetoken.IsEmpty())))
					{
						if (m_dwContextMenus & SVNSLC_POPUNLOCKFORCE)
						{
							temp.LoadString(IDS_MENU_UNLOCKFORCE);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_UNLOCKFORCE, temp);
						}
					}
				}
				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
				m_bBlock = TRUE;
				AfxGetApp()->DoWaitCursor(1);
				int iItemCountBeforeMenuCmd = GetItemCount();
				bool bForce = false;
				switch (cmd)
				{
				case IDSVNLC_REVERT:
					{
						if (CMessageBox::Show(this->m_hWnd, IDS_PROC_WARNREVERT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)==IDYES)
						{
							CTSVNPathList targetList;
							FillListOfSelectedItemPaths(targetList);

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
									FileEntry * fentry = m_arStatusArray[m_arListArray[index]];
									if (fentry->remotestatus <= svn_wc_status_normal)
									{
										if (fentry->textstatus == svn_wc_status_added)
										{
											fentry->textstatus = svn_wc_status_unversioned;
											fentry->status = svn_wc_status_unversioned;
											SetItemState(index, 0, LVIS_SELECTED);
											Show(m_dwShow, 0, m_bShowFolders);
										}
										else
										{
											m_nTotal--;
											if (GetCheck(index))
												m_nSelected--;
											RemoveListEntry(index);
										}
									}
									else
									{
										SetItemState(index, 0, LVIS_SELECTED);
										Show(m_dwShow, 0, m_bShowFolders);
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
						CTSVNPath tempfile = CUtils::GetTempFilePath(CTSVNPath(_T("Test.diff")));
						m_tempFileList.AddPath(tempfile);
						SVN svn;
						if (entry->remotestatus <= svn_wc_status_normal)
						{
							if (!svn.Diff(entry->path, SVNRev::REV_BASE, entry->path, SVNRev::REV_WC, TRUE, FALSE, FALSE, FALSE, _T(""), tempfile))
							{
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								break;		//exit
							}
						}
						else
						{
							if (!svn.PegDiff(entry->path, SVNRev::REV_WC, SVNRev::REV_WC, SVNRev::REV_HEAD, TRUE, FALSE, FALSE, FALSE, _T(""), tempfile))
							{
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								break;		//exit
							}
						}
						CUtils::StartUnifiedDiffViewer(tempfile);
					}
					break;
				case IDSVNLC_UPDATE:
					{
						CTSVNPathList targetList;
						FillListOfSelectedItemPaths(targetList);
						CSVNProgressDlg dlg;
						dlg.SetParams(CSVNProgressDlg::Enum_Update, 0, targetList);
						dlg.DoModal();
					}
					break;
				case IDSVNLC_LOG:
					{
						CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
						int limit = (int)(LONG)reg;
						CLogDlg dlg;
						dlg.SetParams(filepath, SVNRev::REV_HEAD, 1, limit);
						dlg.DoModal();
					}
					break;
				case IDSVNLC_OPEN:
					{
						ShellExecute(this->m_hWnd, _T("open"), filepath.GetWinPath(), NULL, NULL, SW_SHOW);
					}
					break;
				case IDSVNLC_OPENWITH:
					{
						CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
						cmd += filepath.GetWinPathString();
						CUtils::LaunchApplication(cmd, NULL, false);
					}
					break;
				case IDSVNLC_EXPLORE:
					{
						ShellExecute(this->m_hWnd, _T("explore"), filepath.GetDirectory().GetWinPath(), NULL, NULL, SW_SHOW);
					}
					break;
				case IDSVNLC_DELETE:
					{
						CTSVNPathList pathlist;
						FillListOfSelectedItemPaths(pathlist);
						pathlist.RemoveChildren();
						CString filelist;
						for (INT_PTR i=0; i<pathlist.GetCount(); ++i)
						{
							filelist += pathlist[i].GetWinPathString();
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
							CTSVNPathList deletedlist;	//to store list of deleted folders
							while ((pos = GetFirstSelectedItemPosition()) != 0)
							{
								int index = GetNextSelectedItem(pos);
								if (GetCheck(index))
									m_nSelected--;
								m_nTotal--;
								FileEntry * fentry = GetListEntry(index);
								if (fentry->isfolder)
									deletedlist.AddPath(fentry->path);
								RemoveListEntry(index);
							}
							// now go through the list of deleted folders
							// and remove all their children from the list too!
							int nListboxEntries = GetItemCount();
							for (int folderindex = 0; folderindex < deletedlist.GetCount(); ++folderindex)
							{
								CTSVNPath folderpath = deletedlist[folderindex];
								for (int i=0; i<nListboxEntries; ++i)
								{
									FileEntry * entry = GetListEntry(i);
									if (folderpath.IsAncestorOf(entry->path))
									{
										RemoveListEntry(i--);
										nListboxEntries--;
									}
								}
							}
						}
					}
					break;
				case IDSVNLC_IGNOREMASK:
					{
						CString name = _T("*")+filepath.GetFileExtension();
						CTSVNPath parentFolder = filepath.GetDirectory(); 
						SVNProperties props(parentFolder);
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
								// since we ignored files with a mask (e.g. *.exe)
								// we have to find find all files in the same
								// folder (IsAncestorOf() returns TRUE for _all_ children, 
								// not just the immediate ones) which match the
								// mask and remove them from the list too.
								if ((entry->status == svn_wc_status_unversioned)&&(parentFolder.IsAncestorOf(entry->path)))
								{
									CString f = entry->path.GetSVNPathString();
									if (f.Mid(parentFolder.GetSVNPathString().GetLength()).Find('/')<=0)
									{
										if (CStringUtils::WildCardMatch(name, f))
										{
											if (GetCheck(i))
												m_nSelected--;
											m_nTotal--;
											RemoveListEntry(i);
											i--;
											nListboxEntries--;
										}
									}
								}
							}
							SVNStatus status;
							svn_wc_status2_t * s;
							CTSVNPath svnPath;
							s = status.GetFirstFileStatus(parentFolder, svnPath, FALSE);
							CShellUpdater::Instance().AddPathForUpdate(parentFolder);
							if (s!=0)
							{
								// first check if the folder isn't already present in the list
								bool bFound = false;
								for (int i=0; i<nListboxEntries; ++i)
								{
									FileEntry * entry = GetListEntry(i);
									if (entry->path.IsEquivalentTo(svnPath))
									{
										bFound = true;
										break;
									}
								}
								if (!bFound)
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
									entry->isfolder = true;
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
					}
					break;
				case IDSVNLC_IGNORE:
					{
						CString name = filepath.GetFileOrDirectoryName();
						CTSVNPath parentfolder = filepath.GetContainingDirectory();
						SVNProperties props(parentfolder);
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
						
						//now, if we ignored a folder, remove all its children
						if (filepath.IsDirectory())
						{
							for (int i=0; i<(int)m_arListArray.size(); ++i)
							{
								FileEntry * entry = GetListEntry(i);
								if (entry->status == svn_wc_status_unversioned)
								{
									if (!filepath.IsEquivalentTo(entry->GetPath())&&(filepath.IsAncestorOf(entry->GetPath())))
									{
										entry->status = svn_wc_status_ignored;
										entry->textstatus = svn_wc_status_ignored;
										if (GetCheck(i))
											m_nSelected--;
										m_nTotal--;
										RemoveListEntry(i);
										i--;
									}
								}
							}
						}
						
						CTSVNPath basepath = m_arStatusArray[m_arListArray[selIndex]]->basepath;
						RemoveListEntry(selIndex);
						SVNStatus status;
						svn_wc_status2_t * s;
						CTSVNPath svnPath;
						s = status.GetFirstFileStatus(parentfolder, svnPath, FALSE);
						CShellUpdater::Instance().AddPathForUpdate(parentfolder);
						// first check if the folder isn't already present in the list
						bool bFound = false;
						int nListboxEntries = GetItemCount();
						for (int i=0; i<nListboxEntries; ++i)
						{
							FileEntry * entry = GetListEntry(i);
							if (entry->path.IsEquivalentTo(svnPath))
							{
								bFound = true;
								break;
							}
						}
						if (!bFound)
						{
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
								entry->isfolder = true;
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
				case IDSVNLC_EDITCONFLICT:
					SVN::StartConflictEditor(filepath);
					break;
				case IDSVNLC_RESOLVECONFLICT:
					{
						if (CMessageBox::Show(m_hWnd, IDS_PROC_RESOLVE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO)==IDYES)
						{
							SVN svn;
							if (!svn.Resolve(filepath, FALSE))
							{
								CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							}
							else
							{
								entry->status = svn_wc_status_modified;
								entry->textstatus = svn_wc_status_modified;
								Show(m_dwShow, 0, m_bShowFolders);
							}
						}
					}
					break;
				case IDSVNLC_ADD:
					{
						SVN svn;
						CTSVNPathList itemsToAdd;
						FillListOfSelectedItemPaths(itemsToAdd);

						// We must sort items before adding, so that folders are always added
						// *before* any of their children
						itemsToAdd.SortByPathname();

						if (svn.Add(itemsToAdd, FALSE, TRUE))
						{
							// The add went ok, but we now need to run through the selected items again
							// and update their status
							POSITION pos = GetFirstSelectedItemPosition();
							int index;
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
						Show(m_dwShow, 0, m_bShowFolders);
					}
					break;
				case IDSVNLC_LOCK:
				{
					CTSVNPathList itemsToLock;
					FillListOfSelectedItemPaths(itemsToLock);
					CInputDlg inpDlg;
					inpDlg.m_sTitle.LoadString(IDS_MENU_LOCK);
					inpDlg.m_sHintText.LoadString(IDS_LOCK_MESSAGEHINT);
					inpDlg.m_sCheckText.LoadString(IDS_LOCK_STEALCHECK);
					ProjectProperties props;
					props.ReadPropsPathList(itemsToLock);
					props.nMinLogSize = 0;		// the lock message is optional, so no minimum!
					inpDlg.m_pProjectProperties = &props;
					if (inpDlg.DoModal()==IDOK)
					{
						CSVNProgressDlg progDlg;
						progDlg.SetParams(CSVNProgressDlg::Lock, inpDlg.m_iCheck ? ProgOptLockForce : 0, itemsToLock, CString(), inpDlg.m_sInputText);
						progDlg.DoModal();
						// refresh!
						CWnd* pParent = GetParent();
						if (NULL != pParent && NULL != pParent->GetSafeHwnd())
						{
							pParent->SendMessage(SVNSLNM_NEEDSREFRESH);
						}
					}
				}
				break;
				case IDSVNLC_UNLOCKFORCE:
					bForce = true;
				case IDSVNLC_UNLOCK:
				{
					CTSVNPathList itemsToUnlock;
					FillListOfSelectedItemPaths(itemsToUnlock);
					CSVNProgressDlg progDlg;
					progDlg.SetParams(CSVNProgressDlg::Unlock, bForce ? ProgOptLockForce : 0, itemsToUnlock);
					progDlg.DoModal();
					// refresh!
					CWnd* pParent = GetParent();
					if (NULL != pParent && NULL != pParent->GetSafeHwnd())
					{
						pParent->SendMessage(SVNSLNM_NEEDSREFRESH);
					}
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
	if ((entry->status == svn_wc_status_normal)&&(entry->remotestatus <= svn_wc_status_normal))
		return;		//normal files won't show anything interesting in a diff
	if (entry->status == svn_wc_status_deleted)
		return;		//we don't compare a deleted file (nothing) with something
	if (entry->status == svn_wc_status_missing)
		return;		//we don't compare a missing file (nothing) with something
	if (entry->status == svn_wc_status_unversioned)
		return;		//we don't compare new files with nothing
	if (entry->path.IsDirectory())
		return;		//we also don't compare folders
	CTSVNPath wcPath;
	CTSVNPath basePath;
	CTSVNPath remotePath;

	if (entry->status > svn_wc_status_normal)
		basePath = SVN::GetPristinePath(entry->path);

	if (entry->remotestatus > svn_wc_status_normal)
	{
		remotePath = CUtils::GetTempFilePath(entry->path);

		SVN svn;
		if (!svn.Cat(entry->path, SVNRev(SVNRev::REV_HEAD), SVNRev::REV_HEAD, remotePath))
		{
			CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		}
		m_tempFileList.AddPath(remotePath);
	}

	wcPath = entry->path;
	if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\ConvertBase"), TRUE))
	{
		CTSVNPath temporaryFile = CUtils::GetTempFilePath(wcPath);
		SVN svn;
		if (svn.Cat(entry->path, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE), temporaryFile))
		{
			basePath = temporaryFile;
			m_tempFileList.AddPath(basePath);
		}
	}

	CString name = entry->path.GetFilename();
	CString n1, n2, n3;
	n1.Format(IDS_DIFF_WCNAME, name);
	n2.Format(IDS_DIFF_BASENAME, name);
	n3.Format(IDS_DIFF_REMOTENAME, name);

	if (basePath.IsEmpty())
		// Hasn't changed locally - diff remote against WC
		CUtils::StartExtDiff(wcPath, remotePath, n1, n3);
	else if (remotePath.IsEmpty())
		// Diff WC against its base
		CUtils::StartExtDiff(basePath, wcPath, n2, n1);
	else
		// Three-way diff
		CUtils::StartExtMerge(basePath, remotePath, wcPath, CTSVNPath(), n2, n3, n1);
}

CString CSVNStatusListCtrl::GetStatisticsString()
{
	CString sNormal, sAdded, sDeleted, sModified, sConflicted, sUnversioned;
	WORD langID = (WORD)(DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
	TCHAR buf[MAX_STATUS_STRING_LENGTH];
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

CTSVNPath CSVNStatusListCtrl::GetCommonDirectory(bool bStrict)
{
	if (!bStrict)
	{
		// not strict means that the selected folder has priority
		if (!m_StatusFileList.GetCommonDirectory().IsEmpty())
			return m_StatusFileList.GetCommonDirectory();
	}

	CTSVNPath commonBaseDirectory;
	int nListItems = GetItemCount();
	for (int i=0; i<nListItems; ++i)
	{
		const CTSVNPath& baseDirectory = GetListEntry(i)->GetPath().GetDirectory();
		if(commonBaseDirectory.IsEmpty())
		{
			commonBaseDirectory = baseDirectory;
		}
		else 
		{
			if (commonBaseDirectory.GetWinPathString().GetLength() > baseDirectory.GetWinPathString().GetLength())
			{
				if (baseDirectory.IsAncestorOf(commonBaseDirectory))
					commonBaseDirectory = baseDirectory;
			}
		}
	}
	return commonBaseDirectory;
}

void CSVNStatusListCtrl::SelectAll(bool bSelect)
{
	CWaitCursor waitCursor;
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
			if (entry == NULL)
				return;

			// coloring
			// ========
			// black  : unversioned, normal
			// purple : added
			// blue   : modified
			// brown  : missing, deleted, replaced
			// green  : merged (or potential merges)
			// red    : conflicts or sure conflicts
			switch (entry->status)
			{
			case svn_wc_status_added:
				if (entry->remotestatus > svn_wc_status_unversioned)
					// locally added file, but file already exists in repository!
					crText = CUtils::MyColor(CUtils::MyColors::RED);
				else
					crText = CUtils::MyColor(CUtils::MyColors::PURPLE);
				break;
			case svn_wc_status_missing:
			case svn_wc_status_deleted:
			case svn_wc_status_replaced:
				crText = CUtils::MyColor(CUtils::MyColors::BROWN);
				break;
			case svn_wc_status_modified:
				if (entry->remotestatus == svn_wc_status_modified)
					// indicate a merge (both local and remote changes will require a merge)
					crText = CUtils::MyColor(CUtils::MyColors::GREEN);
				else if (entry->remotestatus == svn_wc_status_deleted)
					// locally modified, but already deleted in the repository
					crText = CUtils::MyColor(CUtils::MyColors::RED);					
				else
					crText = CUtils::MyColor(CUtils::MyColors::BLUE);
				break;
			case svn_wc_status_merged:
				crText = CUtils::MyColor(CUtils::MyColors::GREEN);
				break;
			case svn_wc_status_conflicted:
			case svn_wc_status_obstructed:
				crText = CUtils::MyColor(CUtils::MyColors::RED);
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

/*
CTSVNPath CSVNStatusListCtrl::BuildTargetFile()
{
	CTSVNPathList targetList;

	// Get all the currently selected items into a PathList
	FillListOfSelectedItemPaths(targetList);

	CTSVNPath tempFile = CUtils::GetTempFilePath();
	m_tempFileList.AddPath(tempFile);
	VERIFY(targetList.WriteToTemporaryFile(tempFile.GetWinPathString()));

	return tempFile;
}
*/

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
// NEVER, EVER call SetCheck directly, because you'll end-up with the checkboxes and the 'checked' flag getting out of sync
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

void CSVNStatusListCtrl::WriteCheckedNamesToPathList(CTSVNPathList& pathList)
{
	pathList.Clear();
	int nListItems = GetItemCount();
	for (int i=0; i< nListItems; i++)
	{
		const FileEntry* entry = GetListEntry(i);
		ASSERT(entry != NULL);
		if (entry->IsChecked())
		{
			pathList.AddPath(entry->path);
		}
	}
	pathList.SortByPathname();
}


/// Build a path list of all the selected items in the list (NOTE - SELECTED, not CHECKED)
void CSVNStatusListCtrl::FillListOfSelectedItemPaths(CTSVNPathList& pathList)
{
	pathList.Clear();

	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		pathList.AddPath(GetListEntry(index)->path);
	}
}

UINT CSVNStatusListCtrl::OnGetDlgCode()
{
	// we want to process the return key and not have that one
	// routed to the default pushbutton
	return CListCtrl::OnGetDlgCode() | DLGC_WANTALLKEYS;
}

void CSVNStatusListCtrl::OnNMReturn(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;
	if (m_bBlock)
		return;
	int selIndex = GetSelectionMark();
	if (selIndex < 0)
		return;
	StartDiff(selIndex);
}

void CSVNStatusListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Since we catch all keystrokes (to have the enter key processed here instead
	// of routed to the default pushbutton) we have to make sure that other
	// keys like Tab and Esc still do what they're supposed to do
	// Tab = change focus to next/previous control
	// Esc = quit the dialog
	switch (nChar)
	{
	case (VK_TAB):
		{
			::PostMessage(GetParent()->GetSafeHwnd(), WM_NEXTDLGCTL, GetKeyState(VK_SHIFT)&0x8000, 0);
			return;
		}
		break;
	case (VK_ESCAPE):
		{
			::SendMessage(GetParent()->GetSafeHwnd(), WM_CLOSE, 0, 0);
		}
		break;
	}

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
