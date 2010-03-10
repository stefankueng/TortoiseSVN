// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "stdafx.h"
#include "resource.h"
#include "..\\TortoiseShell\\resource.h"
#include "MessageBox.h"
#include "MyMemDC.h"
#include "UnicodeUtils.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "TempFile.h"
#include "StringUtils.h"
#include "DirFileEnum.h"
#include "SVNConfig.h"
#include "SVNProperties.h"
#include "SVN.h"
#include "SVNDiff.h"
#include "LogDialog\LogDlg.h"
#include "SVNProgressDlg.h"
#include "SysImageList.h"
#include "Svnstatuslistctrl.h"
#include "SVNDataObject.h"
#include "TSVNPath.h"
#include "Registry.h"
#include "SVNStatus.h"
#include "SVNHelpers.h"
#include "InputDlg.h"
#include "ShellUpdater.h"
#include "SVNAdminDir.h"
#include "IconMenu.h"
#include "AddDlg.h"
#include "EditPropertiesDlg.h"
#include "CreateChangelistDlg.h"
#include "SysInfo.h"
#include "ProgressDlg.h"
#include "StringUtils.h"
#include "auto_buffer.h"
#include "svntrace.h"
#include "FormatMessageWrapper.h"
#include "AsyncCall.h"

const UINT CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED
					= ::RegisterWindowMessage(_T("SVNSLNM_ITEMCOUNTCHANGED"));
const UINT CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH
					= ::RegisterWindowMessage(_T("SVNSLNM_NEEDSREFRESH"));
const UINT CSVNStatusListCtrl::SVNSLNM_ADDFILE
					= ::RegisterWindowMessage(_T("SVNSLNM_ADDFILE"));
const UINT CSVNStatusListCtrl::SVNSLNM_CHECKCHANGED
					= ::RegisterWindowMessage(_T("SVNSLNM_CHECKCHANGED"));
const UINT CSVNStatusListCtrl::SVNSLNM_CHANGELISTCHANGED
					= ::RegisterWindowMessage(_T("SVNSLNM_CHANGELISTCHANGED"));

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
#define IDSVNLC_RESOLVETHEIRS	19
#define IDSVNLC_RESOLVEMINE		20
#define IDSVNLC_REMOVE			21
#define IDSVNLC_COMMIT			22
#define IDSVNLC_PROPERTIES		23
#define IDSVNLC_COPY			24
#define IDSVNLC_COPYEXT			25
#define IDSVNLC_REPAIRMOVE		26
#define IDSVNLC_REMOVEFROMCS	27
#define IDSVNLC_CREATECS		28
#define IDSVNLC_CREATEIGNORECS	29
#define IDSVNLC_CHECKGROUP		30
#define IDSVNLC_UNCHECKGROUP	31
#define IDSVNLC_ADD_RECURSIVE   32
#define IDSVNLC_COMPAREWC		33
#define IDSVNLC_BLAME			34
#define IDSVNLC_CREATEPATCH		35
#define IDSVNLC_CHECKFORMODS	36
#define IDSVNLC_REPAIRCOPY		37
// the IDSVNLC_MOVETOCS *must* be the last index, because it contains a dynamic submenu where 
// the submenu items get command ID's sequent to this number
#define IDSVNLC_MOVETOCS		38


BEGIN_MESSAGE_MAP(CSVNStatusListCtrl, CListCtrl)
	ON_NOTIFY(HDN_ITEMCLICKA, 0, OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, OnHdnItemclick)
	ON_NOTIFY(HDN_ENDTRACK, 0, OnColumnResized)
	ON_NOTIFY(HDN_ENDDRAG, 0, OnColumnMoved)
	ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGED, OnLvnItemchanged)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_WM_SETCURSOR()
	ON_WM_GETDLGCODE()
	ON_NOTIFY_REFLECT(NM_RETURN, OnNMReturn)
	ON_WM_KEYDOWN()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_WM_PAINT()
	ON_NOTIFY(HDN_BEGINTRACKA, 0, &CSVNStatusListCtrl::OnHdnBegintrack)
	ON_NOTIFY(HDN_BEGINTRACKW, 0, &CSVNStatusListCtrl::OnHdnBegintrack)
	ON_NOTIFY(HDN_ITEMCHANGINGA, 0, &CSVNStatusListCtrl::OnHdnItemchanging)
	ON_NOTIFY(HDN_ITEMCHANGINGW, 0, &CSVNStatusListCtrl::OnHdnItemchanging)
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBeginDrag)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGING, &CSVNStatusListCtrl::OnLvnItemchanging)
END_MESSAGE_MAP()



CSVNStatusListCtrl::CSVNStatusListCtrl() : CListCtrl()
	, m_HeadRev(SVNRev::REV_HEAD)
	, m_pbCanceled(NULL)
	, m_pStatLabel(NULL)
	, m_pSelectButton(NULL)
	, m_pConfirmButton(NULL)
	, m_bBusy(false)
	, m_bEmpty(false)
	, m_bUnversionedRecurse(true)
	, m_bShowIgnores(false)
	, m_bIgnoreRemoveOnly(false)
	, m_bCheckChildrenWithParent(false)
	, m_bUnversionedLast(true)
	, m_bHasChangeLists(false)
	, m_bExternalsGroups(false)
	, m_bHasLocks(false)
	, m_bBlock(false)
	, m_bBlockUI(false)
	, m_bHasCheckboxes(false)
	, m_bCheckIfGroupsExist(true)
	, m_bFileDropsEnabled(false)
	, m_bOwnDrag(false)
    , m_dwDefaultColumns(0)
    , m_ColumnManager(this)
    , m_bAscending(false)
    , m_nSortedColumn(-1)
	, m_sNoPropValueText(MAKEINTRESOURCE(IDS_STATUSLIST_NOPROPVALUE))
	, m_bDepthInfinity(false)
{
	m_critSec.Init();
}

CSVNStatusListCtrl::~CSVNStatusListCtrl()
{
	ClearStatusArray();
}

void CSVNStatusListCtrl::ClearStatusArray()
{
	Locker lock(m_critSec);
	for (size_t i=0; i < m_arStatusArray.size(); i++)
	{
		delete m_arStatusArray[i];
	}
	m_arStatusArray.clear();
}

CSVNStatusListCtrl::FileEntry * CSVNStatusListCtrl::GetListEntry(UINT_PTR index)
{
	if (index >= (UINT_PTR)m_arListArray.size())
		return NULL;
	if (m_arListArray[index] >= m_arStatusArray.size())
		return NULL;
	return m_arStatusArray[m_arListArray[index]];
}

CSVNStatusListCtrl::FileEntry * CSVNStatusListCtrl::GetVisibleListEntry(const CTSVNPath& path)
{
	int itemCount = GetItemCount();
	for (int i=0; i < itemCount; i++)
	{
		FileEntry * entry = GetListEntry(i);
		if (entry->GetPath().IsEquivalentTo(path))
			return entry;
	}
	return NULL;
}

CSVNStatusListCtrl::FileEntry * CSVNStatusListCtrl::GetListEntry(const CTSVNPath& path)
{
	for (size_t i=0; i < m_arStatusArray.size(); i++)
	{
		FileEntry * entry = m_arStatusArray[i];
		if (entry->GetPath().IsEquivalentTo(path))
			return entry;
	}
	return NULL;
}

int CSVNStatusListCtrl::GetIndex(const CTSVNPath& path)
{
	int itemCount = GetItemCount();
	for (int i=0; i < itemCount; i++)
	{
		FileEntry * entry = GetListEntry(i);
		if (entry->GetPath().IsEquivalentTo(path))
			return i;
	}
	return -1;
}

void CSVNStatusListCtrl::Init(DWORD dwColumns, const CString& sColumnInfoContainer, DWORD dwContextMenus /* = SVNSLC_POPALL */, bool bHasCheckboxes /* = true */)
{
	Locker lock(m_critSec);

    m_dwDefaultColumns = dwColumns | 1;
    m_dwContextMenus = dwContextMenus;
	m_bHasCheckboxes = bHasCheckboxes;

    // set the extended style of the listcontrol
	// the style LVS_EX_FULLROWSELECT interferes with the background watermark image but it's more important to be able to select in the whole row.
	CRegDWORD regFullRowSelect(_T("Software\\TortoiseSVN\\FullRowSelect"), TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	exStyle |= (bHasCheckboxes ? LVS_EX_CHECKBOXES : 0);
	SetRedraw(false);
	SetExtendedStyle(exStyle);

	CXPTheme theme;
	theme.SetWindowTheme(m_hWnd, L"Explorer", NULL);

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	m_nExternalOvl = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_EXTERNALOVL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	SYS_IMAGE_LIST().SetOverlayImage(m_nExternalOvl, OVL_EXTERNAL);
	m_nNestedOvl = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_NESTEDOVL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	SYS_IMAGE_LIST().SetOverlayImage(m_nNestedOvl, OVL_NESTED);
	m_nDepthFilesOvl = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_DEPTHFILESOVL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	SYS_IMAGE_LIST().SetOverlayImage(m_nDepthFilesOvl, OVL_DEPTHFILES);
	m_nDepthImmediatesOvl = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_DEPTHIMMEDIATEDOVL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	SYS_IMAGE_LIST().SetOverlayImage(m_nDepthImmediatesOvl, OVL_DEPTHIMMEDIATES);
	m_nDepthEmptyOvl = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_DEPTHEMPTYOVL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	SYS_IMAGE_LIST().SetOverlayImage(m_nDepthEmptyOvl, OVL_DEPTHEMPTY);
	SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);

    m_ColumnManager.ReadSettings (m_dwDefaultColumns, sColumnInfoContainer);

	// enable file drops
	if (m_pDropTarget.get() == NULL)
	{
		m_pDropTarget.reset (new CSVNStatusListCtrlDropTarget(this));
		RegisterDragDrop(m_hWnd,m_pDropTarget.get());
		// create the supported formats:
		FORMATETC ftetc={0};
		ftetc.dwAspect = DVASPECT_CONTENT;
		ftetc.lindex = -1;
		ftetc.tymed = TYMED_HGLOBAL;
		ftetc.cfFormat=CF_HDROP;
		m_pDropTarget->AddSuportedFormat(ftetc);
		ftetc.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DROPDESCRIPTION);
		m_pDropTarget->AddSuportedFormat(ftetc);
	}

	SetRedraw(true);

	m_bUnversionedRecurse = !!((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\UnversionedRecurse"), TRUE));
}

bool CSVNStatusListCtrl::SetBackgroundImage(UINT nID)
{
	return CAppUtils::SetListCtrlBackgroundImage(GetSafeHwnd(), nID);
}

BOOL CSVNStatusListCtrl::GetStatus ( const CTSVNPathList& pathList
                                   , bool bUpdate /* = FALSE */
                                   , bool bShowIgnores /* = false */
                                   , bool bShowUserProps /* = false */)
{
	Locker lock(m_critSec);
	int refetchcounter = 0;
	BOOL bRet = TRUE;
	Invalidate();
	// force the cursor to change
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);

	m_mapFilenameToChecked.clear();
	m_StatusUrlList.Clear();
	m_externalSet.clear();
	bool bHasChangelists = (m_changelists.size()>1 || (m_changelists.size()>0 && !m_bHasIgnoreGroup));
	m_changelists.clear();
	for (size_t i=0; i < m_arStatusArray.size(); i++)
	{
		FileEntry * entry = m_arStatusArray[i];
		if ( bHasChangelists && entry->checked)
		{
			// If change lists are present, remember all checked entries
			CString path = entry->GetPath().GetSVNPathString();
			m_mapFilenameToChecked[path] = true;
		}
		if ( (entry->status==svn_wc_status_unversioned || entry->status==svn_wc_status_missing ) && entry->checked )
		{
			// The user manually selected an unversioned or missing file. We remember
			// this so that the selection can be restored when refreshing.
			CString path = entry->GetPath().GetSVNPathString();
			m_mapFilenameToChecked[path] = true;
		}
		else if ( entry->status > svn_wc_status_normal && !entry->checked )
		{
			// The user manually deselected a versioned file. We remember
			// this so that the deselection can be restored when refreshing.
			CString path = entry->GetPath().GetSVNPathString();
			m_mapFilenameToChecked[path] = false;
		}
	}

	// use a sorted path list to make sure we fetch the status of
	// parent items first, *then* the child items (if any)
	CTSVNPathList sortedPathList = pathList;
	sortedPathList.SortByPathname();
	do
	{
		bRet = TRUE;
		m_nTargetCount = 0;
		m_bHasExternalsFromDifferentRepos = FALSE;
		m_bHasExternals = FALSE;
		m_bHasUnversionedItems = FALSE;
		m_bHasLocks = false;
		m_bHasChangeLists = false;
		m_bShowIgnores = bShowIgnores;
		m_nSortedColumn = 0;
		m_bBlock = TRUE;
		m_bBusy = true;
		m_bEmpty = false;
		Invalidate();

		// first clear possible status data left from
		// previous GetStatus() calls
		ClearStatusArray();

		m_StatusFileList = sortedPathList;

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

		m_nTargetCount = sortedPathList.GetCount();

		SVNStatus status(m_pbCanceled);
		if(m_nTargetCount > 1 && sortedPathList.AreAllPathsFilesInOneDirectory())
		{
			// This is a special case, where we've been given a list of files
			// all from one folder
			// We handle them by setting a status filter, then requesting the SVN status of
			// all the files in the directory, filtering for the ones we're interested in
			status.SetFilter(sortedPathList);

			// if all selected entries are files, we don't do a recursive status
			// fetching. But if only one is a directory, we have to recurse.
			svn_depth_t depth = svn_depth_files;
			// We have set a filter. If all selected items were files or we fetch
			// the status not recursively, then we have to include
			// ignored items too - the user has selected them
			bool bShowIgnoresRight = true;
			for (int fcindex=0; fcindex<sortedPathList.GetCount(); ++fcindex)
			{
				if (sortedPathList[fcindex].IsDirectory())
				{
					depth = m_bDepthInfinity ? svn_depth_infinity : svn_depth_unknown;
					bShowIgnoresRight = false;
					break;
				}
			}
			if(!FetchStatusForSingleTarget(config, status, sortedPathList.GetCommonDirectory(), bUpdate, sUUID, arExtPaths, true, depth, bShowIgnoresRight))
			{
				bRet = FALSE;
			}
		}
		else
		{
			for(int nTarget = 0; nTarget < m_nTargetCount; nTarget++)
			{
				// check whether the path we want the status for is already fetched due to status-fetching
				// of a parent path.
				// this check is only done for file paths, because folder paths could be included already
				// but not recursively
				if (sortedPathList[nTarget].IsDirectory() || GetListEntry(sortedPathList[nTarget]) == NULL)
				{
					if(!FetchStatusForSingleTarget(config, status, sortedPathList[nTarget], bUpdate, sUUID, arExtPaths, false, m_bDepthInfinity ? svn_depth_infinity : svn_depth_unknown, bShowIgnores))
					{
						bRet = FALSE;
					}
				}
			}
		}

		// remove the 'helper' files of conflicted items from the list.
		// otherwise they would appear as unversioned files.
		for (INT_PTR cind = 0; cind < m_ConflictFileList.GetCount(); ++cind)
		{
			for (size_t i=0; i < m_arStatusArray.size(); i++)
			{
				if (m_arStatusArray[i]->GetPath().IsEquivalentTo(m_ConflictFileList[cind]))
				{
					delete m_arStatusArray[i];
					m_arStatusArray.erase(m_arStatusArray.begin()+i);
					break;
				}
			}
		}
		refetchcounter++;
	} while(!BuildStatistics() && (refetchcounter < 2) && (*m_pbCanceled==false));

    if (bShowUserProps)
        FetchUserProperties();

    m_ColumnManager.UpdateUserPropList (m_arStatusArray);

	m_bBlock = FALSE;
	m_bBusy = false;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	return bRet;
}

//
// Fetch all local properties for all elements in the status array
//

void CSVNStatusListCtrl::FetchUserProperties (FileEntry* entry)
{
    SVNTRACE_BLOCK

    // local / temp pool to hold parameters and props for a single item

	SVNPool pool;

    // open working copy for this path

    const char* path = entry->path.GetSVNApiPath (pool);
     
    svn_wc_adm_access_t *adm_access = NULL;          
    svn_error_t * error = svn_wc_adm_probe_open3 ( &adm_access
                                                 , NULL
                                                 , path
                                                 , FALSE	// no write lock
												 , 0		// lock just the directory/file itself
                                                 , NULL
												 , NULL
                                                 , pool);
    if (error == NULL)
    {
        // get the props and add them to the status info

        apr_hash_t* props = NULL;
        error = svn_wc_prop_list ( &props
                                   , path
                                   , adm_access
                                   , pool);
        if (error == NULL)
        {
            for ( apr_hash_index_t *index = apr_hash_first (pool, props)
                ; index != NULL
                ; index = apr_hash_next (index))
            {
                // extract next entry from hash

                const char* key = NULL;
                ptrdiff_t keyLen;
                const char** val = NULL;

                apr_hash_this ( index
                              , reinterpret_cast<const void**>(&key)
                              , &keyLen
                              , reinterpret_cast<void**>(&val));

                // decode / dispatch it

    	        CString name = CUnicodeUtils::GetUnicode (key);
                CString value = CUnicodeUtils::GetUnicode (*val);

                // store in property container (truncate it after ~100 chars)

                entry->present_props[name] 
                    = value.Left (SVNSLC_MAXUSERPROPLENGTH);
            }
        }
        error = svn_wc_adm_close2 (adm_access, pool);
    }

	svn_error_clear (error);
}


void CSVNStatusListCtrl::FetchUserProperties()
{
	async::CJobScheduler queries (0, async::CJobScheduler::GetHWThreadCount());

    for (size_t i = 0, count = m_arStatusArray.size(); i < count; ++i)
		new async::CAsyncCall ( &CSVNStatusListCtrl::FetchUserProperties
							  , m_arStatusArray[i]
							  , &queries);
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
							bool bAllDirect,
							svn_depth_t depth,
							bool bShowIgnores
							)
{
	config.GetDefaultIgnores();

	CTSVNPath workingTarget(target);

	svn_wc_status2_t * s;
	CTSVNPath svnPath;
	s = status.GetFirstFileStatus(workingTarget, svnPath, bFetchStatusFromRepository, depth, bShowIgnores);
	status.GetExternals(m_externalSet);

	m_HeadRev = SVNRev(status.headrev);
	if (s==0)
	{
		m_sLastError = status.GetLastErrorMsg();
		return false;
	}

	svn_wc_status_kind wcFileStatus = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);

	// This one fixes a problem with externals:
	// If a strLine is a file, svn:externals and its parent directory
	// will also be returned by GetXXXFileStatus. Hence, we skip all
	// status info until we find the one matching workingTarget.
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
			if (s == 0)
			{
				m_sLastError = status.GetLastErrorMsg();
				return false;
			}
			// Now, set working target to be the base folder of this item
			workingTarget = workingTarget.GetDirectory();
		}
	}
	bool bEntryFromDifferentRepo = false;
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
				bEntryFromDifferentRepo = true;
				if (s->entry->kind == svn_node_dir)
					arExtPaths.AddPath(workingTarget);
			}
		}
	}
	else if (strCurrentRepositoryUUID.IsEmpty() && (s->text_status == svn_wc_status_added))
	{
		// An added entry doesn't have an UUID assigned to it yet.
		// So we fetch the status of the parent directory instead and
		// check if that one has an UUID assigned to it.
		svn_wc_status2_t * sparent;
		CTSVNPath path = workingTarget;
		do
		{
			CTSVNPath svnParentPath;
			SVNStatus tempstatus;
			sparent = tempstatus.GetFirstFileStatus(path.GetContainingDirectory(), svnParentPath, false, svn_depth_empty, false);
			path = svnParentPath;
		} while ( (sparent) && (sparent->entry) && (!sparent->entry->uuid) && (sparent->text_status==svn_wc_status_added) );
		if (sparent && sparent->entry && sparent->entry->uuid)
		{
			strCurrentRepositoryUUID = sparent->entry->uuid;
			m_sUUID = strCurrentRepositoryUUID;
		}
	}

	if ((wcFileStatus == svn_wc_status_unversioned)&& svnPath.IsDirectory())
	{
		// check if the unversioned folder is maybe versioned. This
		// could happen with nested layouts
		svn_wc_status_kind st = SVNStatus::GetAllStatus(workingTarget);
		if ((st != svn_wc_status_unversioned)&&(st != svn_wc_status_none))
		{
			return true;	// ignore nested layouts
		}
	}
	if (status.IsExternal(svnPath))
	{
		m_bHasExternals = TRUE;
	}

	AddNewFileEntry(s, svnPath, workingTarget, true, m_bHasExternals, bEntryFromDifferentRepo);

	if (((wcFileStatus == svn_wc_status_unversioned)||(wcFileStatus == svn_wc_status_none)||((wcFileStatus == svn_wc_status_ignored)&&(m_bShowIgnores))) && svnPath.IsDirectory())
	{
		// we have an unversioned folder -> get all files in it recursively!
		AddUnversionedFolder(svnPath, workingTarget.GetContainingDirectory(), &config);
	}

	// for folders, get all statuses inside it too
	if(workingTarget.IsDirectory())
	{
		ReadRemainingItemsStatus(status, workingTarget, strCurrentRepositoryUUID, arExtPaths, &config, bAllDirect);
	}

	return true;
}

const CSVNStatusListCtrl::FileEntry*
CSVNStatusListCtrl::AddNewFileEntry(
			const svn_wc_status2_t* pSVNStatus,  // The return from the SVN GetStatus functions
			const CTSVNPath& path,				// The path of the item we're adding
			const CTSVNPath& basePath,			// The base directory for this status build
			bool bDirectItem,					// Was this item the first found by GetFirstFileStatus or by a subsequent GetNextFileStatus call
			bool bInExternal,					// Are we in an 'external' folder
			bool bEntryfromDifferentRepo		// if the entry is from a different repository
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
	entry->inexternal = bInExternal;
	entry->file_external = !!pSVNStatus->file_external;
	entry->differentrepo = bEntryfromDifferentRepo;
	entry->direct = bDirectItem;
	entry->copied = !!pSVNStatus->copied;
	entry->switched = !!pSVNStatus->switched;
	entry->tree_conflicted = (pSVNStatus->tree_conflict != NULL);

	entry->last_commit_date = pSVNStatus->ood_last_cmt_date;
	if ((entry->last_commit_date == NULL)&&(pSVNStatus->entry))
		entry->last_commit_date = pSVNStatus->entry->cmt_date;
	entry->remoterev = pSVNStatus->ood_last_cmt_rev;
	if (pSVNStatus->entry)
		entry->last_commit_rev = pSVNStatus->entry->cmt_rev;
	if (pSVNStatus->ood_last_cmt_author)
		entry->last_commit_author = CUnicodeUtils::GetUnicode(pSVNStatus->ood_last_cmt_author);
	if ((entry->last_commit_author.IsEmpty())&&(pSVNStatus->entry)&&(pSVNStatus->entry->cmt_author))
		entry->last_commit_author = CUnicodeUtils::GetUnicode(pSVNStatus->entry->cmt_author);

	if (pSVNStatus->entry)
		entry->isConflicted = (pSVNStatus->entry->conflict_wrk && PathFileExistsA(pSVNStatus->entry->conflict_wrk)) ? true : false;

	if ((entry->status == svn_wc_status_conflicted)||(entry->isConflicted))
	{
		entry->isConflicted = true;
		if (pSVNStatus->entry)
		{
			CTSVNPath cpath;
			if (pSVNStatus->entry->conflict_wrk)
			{
				cpath = path.GetDirectory();
				cpath.AppendPathString(CUnicodeUtils::GetUnicode(pSVNStatus->entry->conflict_wrk));
				m_ConflictFileList.AddPath(cpath);
			}
			if (pSVNStatus->entry->conflict_old)
			{
				cpath = path.GetDirectory();
				cpath.AppendPathString(CUnicodeUtils::GetUnicode(pSVNStatus->entry->conflict_old));
				m_ConflictFileList.AddPath(cpath);
			}
			if (pSVNStatus->entry->conflict_new)
			{
				cpath = path.GetDirectory();
				cpath.AppendPathString(CUnicodeUtils::GetUnicode(pSVNStatus->entry->conflict_new));
				m_ConflictFileList.AddPath(cpath);
			}
			if (pSVNStatus->entry->prejfile)
			{
				cpath = path.GetDirectory();
				cpath.AppendPathString(CUnicodeUtils::GetUnicode(pSVNStatus->entry->prejfile));
				m_ConflictFileList.AddPath(cpath);
			}
		}
	}

	if (pSVNStatus->url)
	{
		entry->url = CPathUtils::PathUnescape(pSVNStatus->url);
	}

	if (pSVNStatus->entry)
	{
		entry->isfolder = (pSVNStatus->entry->kind == svn_node_dir);
		entry->Revision = pSVNStatus->entry->revision;
		entry->keeplocal = !!pSVNStatus->entry->keep_local;
		entry->working_size = pSVNStatus->entry->working_size;
		entry->depth = pSVNStatus->entry->depth;

		if (pSVNStatus->entry->url)
		{
			entry->url = CPathUtils::PathUnescape(pSVNStatus->entry->url);
		}
		if (pSVNStatus->entry->copyfrom_url)
		{
			entry->copyfrom_url = CPathUtils::PathUnescape (pSVNStatus->entry->copyfrom_url);
			entry->copyfrom_rev = pSVNStatus->entry->copyfrom_rev;
		}
		else
			entry->copyfrom_rev = 0;

		if(bDirectItem)
		{
			if (m_sURL.IsEmpty())
				m_sURL = entry->url;
			else
				m_sURL.LoadString(IDS_STATUSLIST_MULTIPLETARGETS);
			m_StatusUrlList.AddPath(CTSVNPath(entry->url));
		}
		if (pSVNStatus->entry->lock_owner)
			entry->lock_owner = CUnicodeUtils::GetUnicode(pSVNStatus->entry->lock_owner);
		if (pSVNStatus->entry->lock_token)
		{
			entry->lock_token = CUnicodeUtils::GetUnicode(pSVNStatus->entry->lock_token);
			m_bHasLocks = true;
		}
		if (pSVNStatus->entry->lock_comment)
			entry->lock_comment = CUnicodeUtils::GetUnicode(pSVNStatus->entry->lock_comment);
		if (pSVNStatus->entry->lock_creation_date)
			entry->lock_date = pSVNStatus->entry->lock_creation_date;

		if (pSVNStatus->entry->present_props)
		{
			entry->present_props = pSVNStatus->entry->present_props;
		}

		if (pSVNStatus->entry->changelist)
		{
			entry->changelist = CUnicodeUtils::GetUnicode(pSVNStatus->entry->changelist);
			m_changelists[entry->changelist] = -1;
			m_bHasChangeLists = true;
		}
		entry->needslock = (pSVNStatus->entry->present_props && (strstr(pSVNStatus->entry->present_props, SVN_PROP_NEEDS_LOCK)!=NULL) );
	}
	else
	{
		if (pSVNStatus->ood_kind == svn_node_none)
			entry->isfolder = path.IsDirectory();
		else
			entry->isfolder = (pSVNStatus->ood_kind == svn_node_dir);
	}
	if (pSVNStatus->repos_lock)
	{
		if (pSVNStatus->repos_lock->owner)
			entry->lock_remoteowner = CUnicodeUtils::GetUnicode(pSVNStatus->repos_lock->owner);
		if (pSVNStatus->repos_lock->token)
			entry->lock_remotetoken = CUnicodeUtils::GetUnicode(pSVNStatus->repos_lock->token);
		if (pSVNStatus->repos_lock->comment)
			entry->lock_comment = CUnicodeUtils::GetUnicode(pSVNStatus->repos_lock->comment);
		if ((entry->lock_date == 0)&&(pSVNStatus->repos_lock->creation_date))
			entry->lock_date = pSVNStatus->repos_lock->creation_date;
	}

	// Pass ownership of the entry to the array
	m_arStatusArray.push_back(entry);

	// store the repository root
	if ((!bEntryfromDifferentRepo)&&(pSVNStatus->entry)&&(pSVNStatus->entry->repos)&&(m_sRepositoryRoot.IsEmpty()))
	{
		m_sRepositoryRoot = CUnicodeUtils::GetUnicode(pSVNStatus->entry->repos);
	}

	return entry;
}

void CSVNStatusListCtrl::AddUnversionedFolder(const CTSVNPath& folderName,
												const CTSVNPath& basePath,
												SVNConfig * config)
{
	if (!m_bUnversionedRecurse)
		return;
	CSimpleFileFind filefinder(folderName.GetWinPathString());

	CTSVNPath filename;
	m_bHasUnversionedItems = TRUE;
	while (filefinder.FindNextFileNoDots())
	{
		filename.SetFromWin(filefinder.GetFilePath(), filefinder.IsDirectory());

		bool bMatchIgnore = !!config->MatchIgnorePattern(filename.GetFileOrDirectoryName());
		bMatchIgnore = bMatchIgnore || config->MatchIgnorePattern(filename.GetSVNPathString());
		if (((bMatchIgnore)&&(m_bShowIgnores))||(!bMatchIgnore))
		{
			FileEntry * entry = new FileEntry();
			entry->path = filename;
			entry->basepath = basePath;
			entry->inunversionedfolder = true;
			entry->isfolder = filefinder.IsDirectory();

			m_arStatusArray.push_back(entry);
			if (entry->isfolder)
			{
				if (!g_SVNAdminDir.HasAdminDir(entry->path.GetWinPathString(), true))
					AddUnversionedFolder(entry->path, basePath, config);
			}
		}
	}
}


void CSVNStatusListCtrl::ReadRemainingItemsStatus(SVNStatus& status, const CTSVNPath& basePath,
										  CStringA& strCurrentRepositoryUUID,
										  CTSVNPathList& arExtPaths, SVNConfig * config, bool bAllDirect)
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
			svn_wc_status_kind st = SVNStatus::GetAllStatus(svnPath);
			if ((st != svn_wc_status_unversioned)&&(st != svn_wc_status_none))
			{
				FileEntry * entry = new FileEntry();
				entry->path = svnPath;
				entry->basepath = basePath;
				entry->inunversionedfolder = true;
				entry->isfolder = true;
				entry->isNested = true;
				m_externalSet.insert(svnPath);
				m_arStatusArray.push_back(entry);
				continue;
			}
		}
		bool bDirectoryIsExternal = false;
		bool bEntryfromDifferentRepo = false;
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
						bEntryfromDifferentRepo = true;
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
			else
			{
				// we don't have an UUID - maybe an added file/folder
				if (!strCurrentRepositoryUUID.IsEmpty())
				{
					if ((!lastexternalpath.IsEmpty())&&
						(lastexternalpath.IsAncestorOf(svnPath)))
					{
						bEntryfromDifferentRepo = true;
						m_bHasExternalsFromDifferentRepos = TRUE;
					}
				}
			}
		}
		else
		{
			// if unversioned items lie around in external
			// directories from different repos, we have to mark them
			// as such too.
			if (!strCurrentRepositoryUUID.IsEmpty())
			{
				if ((!lastexternalpath.IsEmpty())&&
					(lastexternalpath.IsAncestorOf(svnPath)))
				{
					bEntryfromDifferentRepo = true;
				}
			}
		}
		if (status.IsExternal(svnPath))
		{
			arExtPaths.AddPath(svnPath);
			m_bHasExternals = TRUE;
		}
		if ((!bEntryfromDifferentRepo)&&(status.IsInExternal(svnPath)))
		{
			// if the externals are inside an unversioned folder (this happens if
			// the externals are specified with e.g. "ext\folder url" instead of just
			// "folder url"), then a commit won't succeed.
			// therefore, we treat those as if the externals come from a different
			// repository
			CTSVNPath extpath = svnPath;
			while (basePath.IsAncestorOf(extpath))
			{
				if (!extpath.HasAdminDir())
				{
					bEntryfromDifferentRepo = true;
					break;
				}
				extpath = extpath.GetContainingDirectory();
			}
		}
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

		if ((wcFileStatus == svn_wc_status_unversioned)&&(!bDirectoryIsExternal))
			m_bHasUnversionedItems = TRUE;

		const FileEntry* entry = AddNewFileEntry(s, svnPath, basePath, bAllDirect, bDirectoryIsExternal, bEntryfromDifferentRepo);

		bool bMatchIgnore = !!config->MatchIgnorePattern(entry->path.GetFileOrDirectoryName());
		bMatchIgnore = bMatchIgnore || config->MatchIgnorePattern(entry->path.GetSVNPathString());
		if ((((wcFileStatus == svn_wc_status_unversioned)||(wcFileStatus == svn_wc_status_none))&&(!bMatchIgnore))||
			((wcFileStatus == svn_wc_status_ignored)&&(m_bShowIgnores))||
			(((wcFileStatus == svn_wc_status_unversioned)||(wcFileStatus == svn_wc_status_none))&&(bMatchIgnore)&&(m_bShowIgnores)))
		{
			if (entry->isfolder)
			{
				// we have an unversioned folder -> get all files in it recursively!
				AddUnversionedFolder(svnPath, basePath, config);
			}
		}
	} // while ((s = status.GetNextFileStatus(svnPath)) != NULL) 
}

// Get the show-flags bitmap value which corresponds to a particular SVN status
DWORD CSVNStatusListCtrl::GetShowFlagsFromFileEntry(const FileEntry* entry)
{
	if (entry == NULL)
		return 0;

	DWORD showFlags = 0;
	svn_wc_status_kind status = SVNStatus::GetMoreImportant(entry->status, entry->remotestatus);
	
	switch (status)
	{
	case svn_wc_status_none:
	case svn_wc_status_unversioned:
		showFlags = SVNSLC_SHOWUNVERSIONED;
		break;
	case svn_wc_status_ignored:
		if (!m_bShowIgnores)
			showFlags = SVNSLC_SHOWDIRECTS;
		else
			showFlags = SVNSLC_SHOWDIRECTS|SVNSLC_SHOWIGNORED;
		break;
	case svn_wc_status_incomplete:
		showFlags = SVNSLC_SHOWINCOMPLETE;
		break;
	case svn_wc_status_normal:
		showFlags = SVNSLC_SHOWNORMAL;
		break;
	case svn_wc_status_external:
		showFlags = SVNSLC_SHOWEXTERNAL;
		break;
	case svn_wc_status_added:
		showFlags = SVNSLC_SHOWADDED;
		break;
	case svn_wc_status_missing:
		showFlags = SVNSLC_SHOWMISSING;
		break;
	case svn_wc_status_deleted:
		showFlags = SVNSLC_SHOWREMOVED;
		break;
	case svn_wc_status_replaced:
		showFlags = SVNSLC_SHOWREPLACED;
		break;
	case svn_wc_status_modified:
		showFlags = SVNSLC_SHOWMODIFIED;
		break;
	case svn_wc_status_merged:
		showFlags = SVNSLC_SHOWMERGED;
		break;
	case svn_wc_status_conflicted:
		showFlags = SVNSLC_SHOWCONFLICTED;
		break;
	case svn_wc_status_obstructed:
		showFlags = SVNSLC_SHOWOBSTRUCTED;
		break;
	default:
		// we should NEVER get here!
		ASSERT(FALSE);
		break;
	}

	if (entry->IsLocked())
		showFlags |= SVNSLC_SHOWLOCKS;
	if (entry->switched)
		showFlags |= SVNSLC_SHOWSWITCHED;
	if (!entry->changelist.IsEmpty())
		showFlags |= SVNSLC_SHOWINCHANGELIST;
	if (entry->tree_conflicted)
		showFlags |= SVNSLC_SHOWCONFLICTED;
	if (entry->isNested) 
		showFlags |= SVNSLC_SHOWNESTED;
	if (entry->IsFolder())
		showFlags |= SVNSLC_SHOWFOLDERS;
	else
		showFlags |= SVNSLC_SHOWFILES;
	if (!entry->copyfrom_url.IsEmpty())
	{
		showFlags |= SVNSLC_SHOWADDEDHISTORY;
		showFlags &= ~SVNSLC_SHOWADDED;
	}

	return showFlags;
}

void CSVNStatusListCtrl::Show(DWORD dwShow, const CTSVNPathList& checkedList, DWORD dwCheck, bool bShowFolders, bool bShowFiles)
{
	Locker lock(m_critSec);
	WORD langID = (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	CWinApp * pApp = AfxGetApp();
	if (pApp)
		pApp->DoWaitCursor(1);
	m_dwShow = dwShow;
	m_bShowFolders = bShowFolders;
	m_bShowFiles = bShowFiles;
	m_nSelected = 0;
	int nTopIndex = GetTopIndex();
	int selMark = GetSelectionMark();
	POSITION posSelectedEntry = GetFirstSelectedItemPosition();
	int nSelectedEntry = 0;
	if (posSelectedEntry)
		nSelectedEntry = GetNextSelectedItem(posSelectedEntry);
	SetRedraw(FALSE);
	DeleteAllItems();

	m_nShownUnversioned = 0;
	m_nShownNormal = 0;
	m_nShownModified = 0;
	m_nShownAdded = 0;
	m_nShownDeleted = 0;
	m_nShownConflicted = 0;
	m_nShownFiles = 0;
	m_nShownFolders = 0;

	PrepareGroups();

	m_arListArray.clear();

	m_arListArray.reserve(m_arStatusArray.size());
	SetItemCount (static_cast<int>(m_arStatusArray.size()));

	int listIndex = 0;
	for (size_t i=0; i < m_arStatusArray.size(); ++i)
	{
		FileEntry * entry = m_arStatusArray[i];
		if ((entry->inexternal) && (!(dwShow & SVNSLC_SHOWINEXTERNALS)))
			continue;
		if ((entry->differentrepo) && (! (dwShow & SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO)))
			continue;
		if ((entry->isNested) && (! (dwShow & SVNSLC_SHOWNESTED)))
			continue;
		if (entry->IsFolder() && (!bShowFolders))
			continue;	// don't show folders if they're not wanted.
		if (!entry->IsFolder() && (!bShowFiles))
			continue;
		svn_wc_status_kind status = SVNStatus::GetMoreImportant(entry->status, entry->remotestatus);
		DWORD showFlags = GetShowFlagsFromFileEntry(entry);
		bool bAllowCheck = ((entry->changelist.Compare(SVNSLC_IGNORECHANGELIST) != 0) && (m_bCheckIfGroupsExist || (m_changelists.size()==0 || (m_changelists.size()==1 && m_bHasIgnoreGroup))));

		// status_ignored is a special case - we must have the 'direct' flag set to add a status_ignored item
		if (status != svn_wc_status_ignored || (entry->direct) || (dwShow & SVNSLC_SHOWIGNORED))
		{
			for (int npath = 0; npath < checkedList.GetCount(); ++npath)
			{
				if (entry->GetPath().IsEquivalentTo(checkedList[npath]))
				{
					if (!entry->IsFromDifferentRepository())
						entry->checked = true;
					break;
				}
			}
			if ((!entry->IsFolder()) && (status == svn_wc_status_deleted) && (dwShow & SVNSLC_SHOWREMOVEDANDPRESENT))
			{
				if (PathFileExists(entry->GetPath().GetWinPath()))
				{
					m_arListArray.push_back(i);
					if ((dwCheck & SVNSLC_SHOWREMOVEDANDPRESENT)||((dwCheck & SVNSLC_SHOWDIRECTS)&&(entry->direct)))
					{
						if ((bAllowCheck)&&(!entry->IsFromDifferentRepository()))
							entry->checked = true;
					}
					AddEntry(entry, langID, listIndex++);
				}
			}
			else if ((dwShow & showFlags)||((dwShow & SVNSLC_SHOWDIRECTFILES)&&(entry->direct)&&(!entry->IsFolder())))
			{
				m_arListArray.push_back(i);
				if ((dwCheck & showFlags)||((dwCheck & SVNSLC_SHOWDIRECTS)&&(entry->direct)))
				{
					if ((bAllowCheck)&&(!entry->IsFromDifferentRepository()))
						entry->checked = true;
				}
				AddEntry(entry, langID, listIndex++);
			}
			else if ((dwShow & showFlags)||((dwShow & SVNSLC_SHOWDIRECTFOLDER)&&(entry->direct)&&entry->IsFolder()))
			{
				m_arListArray.push_back(i);
				if ((dwCheck & showFlags)||((dwCheck & SVNSLC_SHOWDIRECTS)&&(entry->direct)))
				{
					if ((bAllowCheck)&&(!entry->IsFromDifferentRepository()))
						entry->checked = true;
				}
				AddEntry(entry, langID, listIndex++);
			}
		}
	}

	SetItemCount(listIndex);

    m_ColumnManager.UpdateRelevance (m_arStatusArray, m_arListArray);

	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
        SetColumnWidth (col, m_ColumnManager.GetWidth (col, true));

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
		HeaderItem.fmt |= (m_bAscending ? HDF_SORTUP : HDF_SORTDOWN);
		pHeader->SetItem(m_nSortedColumn, &HeaderItem);
	}

	if (nSelectedEntry)
	{
		SetItemState(nSelectedEntry, LVIS_SELECTED, LVIS_SELECTED);
		EnsureVisible(nSelectedEntry, false);
	}
	else
	{
		// Restore the item at the top of the list.
		for (int i=0;GetTopIndex() != nTopIndex;i++)
		{
			if ( !EnsureVisible(nTopIndex+i,false) )
			{
				break;
			}
		}
	}
	if (selMark >= 0)
	{
		SetSelectionMark(selMark);
		SetItemState(selMark, LVIS_FOCUSED , LVIS_FOCUSED);
	}

	if (pApp)
		pApp->DoWaitCursor(-1);

	m_bEmpty = (GetItemCount() == 0);
	Invalidate();
}

void CSVNStatusListCtrl::AddEntry(FileEntry * entry, WORD langID, int listIndex)
{
	static const CString ponly(MAKEINTRESOURCE(IDS_STATUSLIST_PROPONLY));
	static const CString treeconflict(MAKEINTRESOURCE(IDS_STATUSLIST_TREECONFLICT));
	static const CString sNested(MAKEINTRESOURCE(IDS_STATUSLIST_NESTED));
	static const CString sLockBroken(MAKEINTRESOURCE(IDS_STATUSLIST_LOCKBROKEN));
	static HINSTANCE hResourceHandle(AfxGetResourceHandle());

	const CString& path = entry->GetPath().GetSVNPathString();
	if ( m_mapFilenameToChecked.size()!=0 && m_mapFilenameToChecked.find(path) != m_mapFilenameToChecked.end() )
	{
		// The user manually de-/selected an item. We now restore this status
		// when refreshing.
		entry->checked = m_mapFilenameToChecked[path];
	}

	m_bBlock = TRUE;
	TCHAR buf[100];
	int index = listIndex;
	int nCol = 1;
	int icon_idx = 0;
	if (entry->isfolder)
	{
		icon_idx = m_nIconFolder;
		m_nShownFolders++;
	}
	else
	{
		icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(entry->path);
		m_nShownFiles++;
	}

	if (entry->tree_conflicted)
		m_nShownConflicted++;
	else
	{
		switch (entry->status)
		{
		case svn_wc_status_normal:
			m_nShownNormal++;
			break;
		case svn_wc_status_added:
			m_nShownAdded++;
			break;
		case svn_wc_status_missing:
		case svn_wc_status_deleted:
			m_nShownDeleted++;
			break;
		case svn_wc_status_replaced:
		case svn_wc_status_modified:
		case svn_wc_status_merged:
			m_nShownModified++;
			break;
		case svn_wc_status_conflicted:
		case svn_wc_status_obstructed:
			m_nShownConflicted++;
			break;
		case svn_wc_status_ignored:
			m_nShownUnversioned++;
			break;
		default:
			m_nShownUnversioned++;
			break;
		}
	}

	// relative path
	InsertItem(index, entry->GetDisplayName(), icon_idx);
	if (entry->IsNested())
		SetItemState(index, INDEXTOOVERLAYMASK(OVL_NESTED), LVIS_OVERLAYMASK);
	else if (entry->IsInExternal()||entry->file_external)
		SetItemState(index, INDEXTOOVERLAYMASK(OVL_EXTERNAL), LVIS_OVERLAYMASK);
	else if (entry->depth == svn_depth_files)
		SetItemState(index, INDEXTOOVERLAYMASK(OVL_DEPTHFILES), LVIS_OVERLAYMASK);
	else if (entry->depth == svn_depth_immediates)
		SetItemState(index, INDEXTOOVERLAYMASK(OVL_DEPTHIMMEDIATES), LVIS_OVERLAYMASK);
	else if (entry->depth == svn_depth_empty)
		SetItemState(index, INDEXTOOVERLAYMASK(OVL_DEPTHEMPTY), LVIS_OVERLAYMASK);

	// SVNSLC_COLFILENAME
	SetItemText(index, nCol++, entry->path.GetFileOrDirectoryName());
	// SVNSLC_COLEXT
	SetItemText(index, nCol++, entry->path.GetFileExtension());
	// SVNSLC_COLSTATUS
	if (entry->isNested)
	{
		SetItemText(index, nCol++, sNested);
	}
	else
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		if ((entry->copied)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (+)"));
		if ((entry->switched)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (s)"));
		if ((entry->status == entry->propstatus)&&
			(entry->status != svn_wc_status_normal)&&
			(entry->status != svn_wc_status_unversioned)&&
			(!SVNStatus::IsImportant(entry->textstatus)))
			_tcscat_s(buf, 100, ponly);
		if (entry->tree_conflicted)
		{
			_tcscat_s(buf, 100, _T(", "));
			_tcscat_s(buf, 100, treeconflict);
		}
		SetItemText(index, nCol++, buf);
	}
	// SVNSLC_COLREMOTESTATUS
	if (entry->isNested)
	{
		SetItemText(index, nCol++, sNested);
	}
	else
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->remotestatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		if ((entry->copied)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (+)"));
		if ((entry->switched)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (s)"));
		if ((entry->remotestatus == entry->remotepropstatus)&&
			(entry->remotestatus != svn_wc_status_none)&&
			(entry->remotestatus != svn_wc_status_normal)&&
			(entry->remotestatus != svn_wc_status_unversioned)&&
			(!SVNStatus::IsImportant(entry->remotetextstatus)))
			_tcscat_s(buf, 100, ponly);
		SetItemText(index, nCol++, buf);
	}
	// SVNSLC_COLTEXTSTATUS
	if (entry->isNested)
	{
		SetItemText(index, nCol++, sNested);
	}
	else
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->textstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		if ((entry->copied)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (+)"));
		if ((entry->switched)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (s)"));
		if (entry->tree_conflicted)
		{
			_tcscat_s(buf, 100, _T(", "));
			_tcscat_s(buf, 100, treeconflict);
		}
		SetItemText(index, nCol++, buf);
	}
	// SVNSLC_COLPROPSTATUS
	if (entry->isNested)
	{
		nCol++;
	}
	else
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->propstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		if ((entry->copied)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (+)"));
		if ((entry->switched)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (s)"));
		SetItemText(index, nCol++, buf);
	}
	// SVNSLC_COLREMOTETEXT
	if (entry->isNested)
	{
		nCol++;
	}
	else
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->remotetextstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	// SVNSLC_COLREMOTEPROP
	if (entry->isNested)
	{
		_T("");
	}
	else
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->remotepropstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	// SVNSLC_COLDEPTH
	SetItemText(index, nCol++, SVNStatus::GetDepthString(entry->depth));
	// SVNSLC_COLURL
	SetItemText(index, nCol++, entry->url);
	// SVNSLC_COLLOCK
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
			SetItemText(index, nCol++, sLockBroken);
		}
		else
		{
			// stolen lock
			CString temp;
			temp.Format(IDS_STATUSLIST_LOCKSTOLEN, (LPCTSTR)entry->lock_remoteowner);
			SetItemText(index, nCol++, temp);
		}
	}
	else
		SetItemText(index, nCol++, entry->lock_owner);

	// SVNSLC_COLLOCKCOMMENT
	SetItemText(index, nCol++, entry->lock_comment);
	// SVNSLC_COLLOCKDATE
	TCHAR datebuf[SVN_DATE_BUFFER];
	apr_time_t date = entry->lock_date;
	if (date)
	{
		SVN::formatDate(datebuf, date, true);
		SetItemText(index, nCol++, datebuf);
	}
	else
		nCol++;
	// SVNSLC_COLAUTHOR
	SetItemText(index, nCol++, entry->last_commit_author);
	// SVNSLC_COLREVISION
	if (entry->last_commit_rev > 0)
	{
		_itot_s (entry->last_commit_rev, buf, 10);
		SetItemText(index, nCol++, buf);
	}
	else
		nCol++;
	// SVNSLC_COLREMOTEREVISION
	if (entry->remoterev > 0)
	{
		_itot_s (entry->remoterev, buf, 10);
		SetItemText(index, nCol++, buf);
	}
	else
		nCol++;
	// SVNSLC_COLDATE
	date = entry->last_commit_date;
	if (date)
	{
		SVN::formatDate(datebuf, date, true);
		SetItemText(index, nCol++, datebuf);
	}
	else
		nCol++;

	// SVNSLC_COLSVNNEEDSLOCK
    bool bFoundSVNNeedsLock = entry->present_props.IsNeedsLockSet();
	SetItemText(index, nCol++, bFoundSVNNeedsLock ? _T("*") : _T(""));

	// SVNSLC_COLCOPYFROM
	if (CStringUtils::GetMatchingLength (m_sRepositoryRoot, entry->copyfrom_url) == m_sRepositoryRoot.GetLength())
		SetItemText(index, nCol++, (LPCTSTR)entry->copyfrom_url + m_sRepositoryRoot.GetLength());
	else
		SetItemText(index, nCol++, entry->copyfrom_url);

	// SVNSLC_COLCOPYFROMREV
	if (entry->copyfrom_rev > 0)
	{
		_itot_s (entry->copyfrom_rev, buf, 10);
		SetItemText(index, nCol++, buf);
	}
	else
		nCol++;
	// SVNSLC_COLMODIFICATIONDATE
	__int64 filetime = entry->GetPath().GetLastWriteTime();
	if ( (filetime) && (entry->textstatus!=svn_wc_status_deleted) )
	{
		FILETIME* f = (FILETIME*)(__int64*)&filetime;
		SVN::formatDate(datebuf,*f,true);
		SetItemText(index, nCol++, datebuf);
	}
	else
	{
		nCol++;
	}
	// SVNSLC_COLSIZE
	if (entry->IsFolder())
		nCol++;
	else
	{
		__int64 filesize = entry->working_size != (-1) ? entry->working_size : entry->GetPath().GetFileSize();
		StrFormatByteSize64(filesize, buf, 100);
		SetItemText(index, nCol++, buf);
	}

    // user-defined properties
    for ( int i = SVNSLC_NUMCOLUMNS, count = m_ColumnManager.GetColumnCount()
        ; i < count
        ; ++i)
    {
        assert (i == nCol++);
        assert (m_ColumnManager.IsUserProp (i));

        const CString& name = m_ColumnManager.GetName(i);
        if (entry->present_props.HasProperty (name))
		{
			const CString& propVal = entry->present_props [name];

			if (propVal.IsEmpty())
				SetItemText(index, i, m_sNoPropValueText);
			else
				SetItemText(index, i, propVal);
		}
    }

	SetCheck(index, entry->checked);
	if (entry->checked)
		m_nSelected++;
	if (m_changelists.find(entry->changelist) != m_changelists.end())
		SetItemGroup(index, m_changelists[entry->changelist]);
	else if (m_bExternalsGroups)
	{
		// maybe we have externals groups
		if (entry->IsInExternal() && !m_externalSet.empty())
		{
			int groupIndex = 0;
			int foundIndex = 0;
			// in case an external has its own externals, we have
			// to loop through all externals: the set is sorted, so the last
			// path that matches will be the 'lowest' path in the hierarchy
			// and that's the path we want.
			for (std::set<CTSVNPath>::iterator it = m_externalSet.begin(); it != m_externalSet.end(); ++it)
			{
				groupIndex++;
				if (it->IsAncestorOf(entry->path))
					foundIndex = groupIndex;
			}
			SetItemGroup(index, foundIndex);
			if ((m_dwShow & SVNSLC_SHOWEXTDISABLED)&&(entry->IsFromDifferentRepository() || entry->IsNested()))
			{
				SetCheck(index, FALSE);
			}
		}
		else
			SetItemGroup(index, 0);
	}
	else
		SetItemGroup(index, 0);

	m_bBlock = FALSE;
}

bool CSVNStatusListCtrl::SetItemGroup(int item, int groupindex)
{
	if (((m_dwContextMenus & SVNSLC_POPCHANGELISTS) == NULL)&&(m_externalSet.empty()))
		return false;
	if (groupindex < 0)
		return false;
	LVITEM i = {0};
	i.mask = LVIF_GROUPID;
	i.iItem = item;
	i.iSubItem = 0;
	i.iGroupId = groupindex;

	return !!SetItem(&i);
}

void CSVNStatusListCtrl::Sort()
{
	Locker lock(m_critSec);

    CSorter predicate (&m_ColumnManager, m_nSortedColumn, m_bAscending);

	std::sort(m_arStatusArray.begin(), m_arStatusArray.end(), predicate);
	SaveColumnWidths();
	Show(m_dwShow, CTSVNPathList(), 0, m_bShowFolders, m_bShowFiles);
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
	m_nSortedColumn = phdr->iItem;
	m_mapFilenameToChecked.clear();
	Sort();

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
	HeaderItem.fmt |= (m_bAscending ? HDF_SORTUP : HDF_SORTDOWN);
	pHeader->SetItem(m_nSortedColumn, &HeaderItem);

	// the checked state of the list control items must be restored
	for (int i=0; i<GetItemCount(); ++i)
	{
		FileEntry * entry = GetListEntry(i);
		SetCheck(i, entry->IsChecked());
	}

	m_bBlock = FALSE;
}

void CSVNStatusListCtrl::OnLvnItemchanging(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	FileEntry * entry = GetListEntry(pNMLV->iItem);

#define ISCHECKED(x) ((x) ? ((((x)&LVIS_STATEIMAGEMASK)>>12)-1) : FALSE)
	if (((m_bBlock)&&(m_bBlockUI)) ||
		(entry&&(m_dwShow & SVNSLC_SHOWEXTDISABLED)&&(entry->IsFromDifferentRepository() || entry->IsNested())))
	{
		// if we're blocked or an item from a different repository, prevent changing of the check state
		if ((!ISCHECKED(pNMLV->uOldState) && ISCHECKED(pNMLV->uNewState))||
			(ISCHECKED(pNMLV->uOldState) && !ISCHECKED(pNMLV->uNewState)))
			*pResult = TRUE;
	}
}

BOOL CSVNStatusListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if ((pNMLV->uNewState==0)||(pNMLV->uNewState & LVIS_SELECTED)||(pNMLV->uNewState & LVIS_FOCUSED))
		return FALSE;

	if (m_bBlock)
		return FALSE;

	bool bSelected = !!(ListView_GetItemState(m_hWnd, pNMLV->iItem, LVIS_SELECTED) & LVIS_SELECTED);
	int nListItems = GetItemCount();

	m_bBlock = TRUE;
	// was the item checked?
	if (GetCheck(pNMLV->iItem))
	{
		CheckEntry(pNMLV->iItem, nListItems);
		if (bSelected)
		{
			POSITION pos = GetFirstSelectedItemPosition();
			int index;
			while ((index = GetNextSelectedItem(pos)) >= 0)
			{
				if (index != pNMLV->iItem)
					CheckEntry(index, nListItems);
			}
		}
	}
	else
	{
		UncheckEntry(pNMLV->iItem, nListItems);
		if (bSelected)
		{
			POSITION pos = GetFirstSelectedItemPosition();
			int index;
			while ((index = GetNextSelectedItem(pos)) >= 0)
			{
				if (index != pNMLV->iItem)
					UncheckEntry(index, nListItems);
			}
		}
	}
	GetStatisticsString();
	m_bBlock = FALSE;
	NotifyCheck();

	return FALSE;
}

void CSVNStatusListCtrl::OnColumnResized(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER header = reinterpret_cast<LPNMHEADER>(pNMHDR);
    if (   (header != NULL) 
        && (header->iItem >= 0) 
        && (header->iItem < m_ColumnManager.GetColumnCount()))
    {
        m_ColumnManager.ColumnResized (header->iItem);
    }

    *pResult = FALSE;
}

void CSVNStatusListCtrl::OnColumnMoved(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER header = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = TRUE;
    if (   (header != NULL) 
        && (header->iItem >= 0) 
        && (header->iItem < m_ColumnManager.GetColumnCount())
		// only allow the reordering if the column was not moved left of the first
		// visible item - otherwise the 'invisible' columns are not at the far left
		// anymore and we get all kinds of redrawing problems.
		&& (header->pitem)
		&& (header->pitem->iOrder > m_ColumnManager.GetInvisibleCount()))
    {
        m_ColumnManager.ColumnMoved (header->iItem, header->pitem->iOrder);
		*pResult = FALSE;
    }

    Invalidate(FALSE);
}

void CSVNStatusListCtrl::CheckEntry(int index, int nListItems)
{
	Locker lock(m_critSec);
	FileEntry * entry = GetListEntry(index);
	ASSERT(entry != NULL);
	if (entry == NULL)
		return;
	SetCheck(index, TRUE);
	entry = GetListEntry(index);
	// if an unversioned item was checked, then we need to check if
	// the parent folders are unversioned too. If the parent folders actually
	// are unversioned, then check those too.
	if (entry->status == svn_wc_status_unversioned)
	{
		// we need to check the parent folder too
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
	bool bShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	if ( (entry->textstatus == svn_wc_status_deleted) || (m_bCheckChildrenWithParent) || (bShift) )
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
					if ((testEntry->textstatus == svn_wc_status_deleted)||(m_bCheckChildrenWithParent))
					{
						SetEntryCheck(testEntry,i,true);
						m_nSelected++;
						// now we need to check all children of this parent folder
						SetCheckOnAllDescendentsOf(testEntry, true);
					}
				}
			}
		}
	}

	if ( !entry->checked )
	{
		entry->checked = TRUE;
		m_nSelected++;
	}
}

void CSVNStatusListCtrl::UncheckEntry(int index, int nListItems)
{
	Locker lock(m_critSec);
	FileEntry * entry = GetListEntry(index);
	ASSERT(entry != NULL);
	if (entry == NULL)
		return;
	SetCheck(index, FALSE);
	entry = GetListEntry(index);
	// item was unchecked
	if (entry->path.IsDirectory())
	{
		// disable all files within an unselected folder, except when unchecking a folder with property changes
		bool bShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
		if ( (entry->status != svn_wc_status_modified) || (bShift) )
		{
			SetCheckOnAllDescendentsOf(entry, false);
		}
	}
	else if (entry->textstatus == svn_wc_status_deleted)
	{
		// a "deleted" file was unchecked, so uncheck all parent folders
		// and all children of those parents
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
					if (testEntry->textstatus == svn_wc_status_deleted)
					{
						SetEntryCheck(testEntry,i,false);
						m_nSelected--;

						SetCheckOnAllDescendentsOf(testEntry, false);
					}
				}
			}
		}
	}

	if ( entry->checked )
	{
		entry->checked = FALSE;
		m_nSelected--;
	}
}

bool CSVNStatusListCtrl::EntryPathCompareNoCase(const FileEntry* pEntry1, const FileEntry* pEntry2)
{
	return pEntry1->path < pEntry2->path;
}

bool CSVNStatusListCtrl::IsEntryVersioned(const FileEntry* pEntry1)
{
	return pEntry1->status != svn_wc_status_unversioned;
}

bool CSVNStatusListCtrl::BuildStatistics()
{
	bool bRefetchStatus = false;
	FileEntryVector::iterator itFirstUnversionedEntry;
	itFirstUnversionedEntry = std::partition(m_arStatusArray.begin(), m_arStatusArray.end(), IsEntryVersioned);
	if (m_bUnversionedLast)
	{
		// We partition the list of items so that it's arrange with all the versioned items first
		// then all the unversioned items afterwards.
		// Then we sort the versioned part of this, so that we can do quick look-ups in it
		std::sort(m_arStatusArray.begin(), itFirstUnversionedEntry, EntryPathCompareNoCase);
		// Also sort the unversioned section, to make the list look nice...
		std::sort(itFirstUnversionedEntry, m_arStatusArray.end(), EntryPathCompareNoCase);
	}

	// now gather some statistics
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
		if (!entry)
			continue;
		if (entry->tree_conflicted)
		{
			m_nConflicted++;
			continue;
		}
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
			if (SVNStatus::IsImportant(entry->remotestatus))
				break;
			m_nUnversioned++;
			// If an entry is in an unversioned folder, we don't have to do an expensive array search
			// to find out if it got case-renamed: an unversioned folder can't have versioned files
			// But nested folders are also considered to be in unversioned folders, we have to do the
			// check in that case too, otherwise we would miss case-renamed folders - they show up
			// as nested folders.
			if (((!entry->inunversionedfolder)||(entry->isNested))&&(m_bUnversionedLast))
			{
				// check if the unversioned item is just
				// a file differing in case but still versioned
				FileEntryVector::iterator itMatchingItem;
				if(std::binary_search(m_arStatusArray.begin(), itFirstUnversionedEntry, entry, EntryPathCompareNoCase))
				{
					// We've confirmed that there *is* a matching file
					// Find its exact location
					itMatchingItem = std::lower_bound(m_arStatusArray.begin(), itFirstUnversionedEntry, entry, EntryPathCompareNoCase);
					// adjust the case of the filename
					if (MoveFileEx(entry->path.GetWinPath(), (*itMatchingItem)->path.GetWinPath(), MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED))
					{
						// We successfully adjusted the case in the filename. But there is now a file with status 'missing'
						// in the array, because that's the status of the file before we adjusted the case.
						// We have to refetch the status of that file.
						// Since fetching the status of single files/directories is very expensive and there can be
						// multiple case-renames here, we just set a flag and refetch the status at the end from scratch.
						bRefetchStatus = true;
						DeleteItem(i);
						m_arStatusArray.erase(m_arStatusArray.begin()+i);
						delete entry;
						i--;
						m_nUnversioned--;
						// now that we removed an unversioned item from the array, find the first unversioned item in the 'new'
						// list again.
						itFirstUnversionedEntry = std::partition(m_arStatusArray.begin(), m_arStatusArray.end(), IsEntryVersioned);
					}
					break;
				}
			}
			break;
		} // switch (entry->status)
	} // for (int i=0; i < (int)m_arStatusArray.size(); ++i)
	return !bRefetchStatus;
}

void CSVNStatusListCtrl::GetMinMaxRevisions(svn_revnum_t& rMin, svn_revnum_t& rMax, bool bShownOnly, bool bCheckedOnly)
{
	Locker lock(m_critSec);
	rMin = LONG_MAX;
	rMax = 0;

	if ((bShownOnly)||(bCheckedOnly))
	{
		for (int i=0; i<GetItemCount(); ++i)
		{
			const FileEntry * entry = GetListEntry(i);

			if ((entry)&&(entry->last_commit_rev))
			{
				if ((!bCheckedOnly)||(entry->IsChecked()))
				{
					if (entry->last_commit_rev >= 0)
					{
						rMin = min(rMin, entry->last_commit_rev);
						rMax = max(rMax, entry->last_commit_rev);
					}
				}
			}
		}
	}
	else
	{
		for (int i=0; i < (int)m_arStatusArray.size(); ++i)
		{
			const FileEntry * entry = m_arStatusArray[i];
			if ((entry)&&(entry->last_commit_rev))
			{
				if (entry->last_commit_rev >= 0)
				{
					rMin = min(rMin, entry->last_commit_rev);
					rMax = max(rMax, entry->last_commit_rev);
				}
			}
		}
	}
	if (rMin == LONG_MAX)
		rMin = 0;
}

int CSVNStatusListCtrl::GetGroupFromPoint(POINT * ppt, bool bHeader /* = true */)
{
	// the point must be relative to the upper left corner of the control

	if (ppt == NULL)
		return -1;
	if (!IsGroupViewEnabled())
		return -1;

	POINT pt = *ppt;
	pt.x = 10;
	UINT flags = 0;
	int nItem = -1;
	RECT rc;
	GetWindowRect(&rc);
	while (((flags & LVHT_BELOW) == 0)&&(pt.y < rc.bottom))
	{
		nItem = HitTest(pt, &flags);
		if ((flags & LVHT_ONITEM)||(flags & LVHT_EX_GROUP_HEADER))
		{
			// the first item below the point

			// check if the point is too much right (i.e. if the point
			// is farther to the right than the width of the item)
			RECT r;
			GetItemRect(nItem, &r, LVIR_LABEL);
			if (ppt->x > r.right)
				return -1;

			LVITEM lv = {0};
			lv.mask = LVIF_GROUPID;
			lv.iItem = nItem;
			GetItem(&lv);
			int groupID = lv.iGroupId;
			if ((groupID != I_GROUPIDNONE)&&(!bHeader))
				return groupID;
			// now we search upwards and check if the item above this one
			// belongs to another group. If it belongs to the same group,
			// we're not over a group header
			while (pt.y >= 0)
			{
				pt.y -= 2;
				nItem = HitTest(pt, &flags);
				if ((flags & LVHT_ONITEM)&&(nItem >= 0))
				{
					// the first item below the point
					LVITEM lv2 = {0};
					lv2.mask = LVIF_GROUPID;
					lv2.iItem = nItem;
					GetItem(&lv2);
					if (lv2.iGroupId != groupID)
						return groupID;
					else
						return -1;
				}
			}
			if (pt.y < 0)
				return groupID;
			return -1;
		}
		pt.y += 2;
	};
	return -1;
}

void CSVNStatusListCtrl::OnContextMenuGroup(CWnd * /*pWnd*/, CPoint point)
{
	POINT clientpoint = point;
	ScreenToClient(&clientpoint);
	if ((!IsGroupViewEnabled())||(GetGroupFromPoint(&clientpoint) < 0))
		return;

	if (!m_bHasCheckboxes)
		return;		// no checkboxes, so nothing to select
	CMenu popup;
	if (!popup.CreatePopupMenu())
		return;

	CString temp;
	temp.LoadString(IDS_STATUSLIST_CHECKGROUP);
	popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_CHECKGROUP, temp);
	temp.LoadString(IDS_STATUSLIST_UNCHECKGROUP);
	popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_UNCHECKGROUP, temp);
	int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
	bool bCheck = false;
	switch (cmd)
	{
	case IDSVNLC_CHECKGROUP:
		bCheck = true;
		// fall through here
	case IDSVNLC_UNCHECKGROUP:
		{
			int group = GetGroupFromPoint(&clientpoint);
			// go through all items and check/uncheck those assigned to the group
			// but block the OnLvnItemChanged handler
			m_bBlock = true;
			LVITEM lv;
			for (int i=0; i<GetItemCount(); ++i)
			{
				SecureZeroMemory(&lv, sizeof(LVITEM));
				lv.mask = LVIF_GROUPID;
				lv.iItem = i;
				GetItem(&lv);
				if (lv.iGroupId != group)
					continue;

				FileEntry * entry = GetListEntry(i);
				if (!entry)
					continue;
				bool bOldCheck = !!GetCheck(i);
				SetEntryCheck(entry, i, bCheck);
				if (bCheck != bOldCheck)
				{
					if (bCheck)
						m_nSelected++;
					else
						m_nSelected--;
				}
			}
			GetStatisticsString();
			NotifyCheck();
			m_bBlock = false;
		}
		break;
	}
}

void CSVNStatusListCtrl::OnContextMenuList(CWnd * pWnd, CPoint point)
{
	const static CString svnPropIgnore (SVN_PROP_IGNORE);

	WORD langID = (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	bool bShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);

	bool bInactiveItem = false;
	int selIndex = GetSelectionMark();

	UINT selectedCount = GetSelectedCount();
	// inactive items (e.g., from different repositories) are not selectable,
	// so the selectedCount is always 0 for those. To prevent using the header
	// context menu but still the item context menu, we check here where
	// the right-click exactly happened
	if (selectedCount == 0)
	{
		CPoint pt = point;
		ScreenToClient(&pt);
		UINT hitFlags = LVHT_ONITEM;
		int hitIndex = this->HitTest(pt, &hitFlags);
		if ((hitIndex >= 0) && (hitFlags & LVHT_ONITEM))
		{
			FileEntry * testentry = GetListEntry(hitIndex);
			if (testentry)
			{
				if (testentry->IsFromDifferentRepository())
				{
					selIndex = hitIndex;
					bInactiveItem = true;
					selectedCount = 1;
				}
			}
		}
	}

	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		GetItemRect(selIndex, &rect, LVIR_LABEL);
		ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	if (!bInactiveItem && (selectedCount == 0) && IsGroupViewEnabled())
	{
		// nothing selected could mean the context menu is requested for
		// a group header
		OnContextMenuGroup(pWnd, point);
	}
	else if (selIndex >= 0)
	{
		FileEntry * entry = GetListEntry(selIndex);
		ASSERT(entry != NULL);
		if (entry == NULL)
			return;
		const CTSVNPath& filepath = entry->path;
		svn_wc_status_kind wcStatus = entry->status;
		// entry is selected, now show the popup menu
		Locker lock(m_critSec);
		CIconMenu popup;
		CMenu changelistSubMenu;
		CMenu ignoreSubMenu;
		if (popup.CreatePopupMenu())
		{
			if ((wcStatus >= svn_wc_status_normal)&&(wcStatus != svn_wc_status_ignored)&&(wcStatus != svn_wc_status_external))
			{
				if (m_dwContextMenus & SVNSLC_POPCOMPAREWITHBASE)
				{
					popup.AppendMenuIcon(IDSVNLC_COMPARE, IDS_LOG_COMPAREWITHBASE, IDI_DIFF);
					if ((wcStatus != svn_wc_status_normal)||(entry->remotestatus > svn_wc_status_normal))
						popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
				}

				if (selectedCount == 1)
				{
					bool bEntryAdded = false;
					if (entry->remotestatus <= svn_wc_status_normal)
					{
						if (wcStatus > svn_wc_status_normal)
						{
							if ((m_dwContextMenus & SVNSLC_POPGNUDIFF)&&(wcStatus != svn_wc_status_deleted)&&(wcStatus != svn_wc_status_missing))
							{
								popup.AppendMenuIcon(IDSVNLC_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
								bEntryAdded = true;
							}
						}
						if (wcStatus > svn_wc_status_normal)
						{
							if (m_dwContextMenus & SVNSLC_POPCOMMIT)
							{
								popup.AppendMenuIcon(IDSVNLC_COMMIT, IDS_STATUSLIST_CONTEXT_COMMIT, IDI_COMMIT);
								popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
								bEntryAdded = true;
							}
						}
					}
					else if (wcStatus != svn_wc_status_deleted)
					{
						if (m_dwContextMenus & SVNSLC_POPCOMPARE)
						{
							popup.AppendMenuIcon(IDSVNLC_COMPAREWC, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
							popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
							bEntryAdded = true;
						}
						if (m_dwContextMenus & SVNSLC_POPGNUDIFF)
						{
							popup.AppendMenuIcon(IDSVNLC_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
							bEntryAdded = true;
						}
					}
					if (bEntryAdded)
						popup.AppendMenu(MF_SEPARATOR);
				}
				else if (GetSelectedCount() > 1)
				{
					if (m_dwContextMenus & SVNSLC_POPCOMMIT)
					{
						popup.AppendMenuIcon(IDSVNLC_COMMIT, IDS_STATUSLIST_CONTEXT_COMMIT, IDI_COMMIT);
						popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
					}
				}
			}
			if (selectedCount > 0)
			{
				if ((selectedCount == 2)&&
					((m_dwContextMenus & SVNSLC_POPREPAIRMOVE)||(m_dwContextMenus & SVNSLC_POPREPAIRCOPY)))
				{
					POSITION pos = GetFirstSelectedItemPosition();
					int index = GetNextSelectedItem(pos);
					if (index >= 0)
					{
						FileEntry * entry2 = GetListEntry(index);
						svn_wc_status_kind status1 = svn_wc_status_none;
						svn_wc_status_kind status2 = svn_wc_status_none;
						if (entry2)
							status1 = entry2->status;
						index = GetNextSelectedItem(pos);
						if (index >= 0)
						{
							entry2 = GetListEntry(index);
							if (entry2)
								status2 = entry2->status;
							if (m_dwContextMenus & SVNSLC_POPREPAIRMOVE)
							{
								if ((status1 == svn_wc_status_missing && ((status2 == svn_wc_status_unversioned)||(status2 == svn_wc_status_none))) ||
									(status2 == svn_wc_status_missing && ((status1 == svn_wc_status_unversioned)||(status1 == svn_wc_status_none))))
								{
									popup.AppendMenuIcon(IDSVNLC_REPAIRMOVE, IDS_STATUSLIST_CONTEXT_REPAIRMOVE);
								}
							}
							if (m_dwContextMenus & SVNSLC_POPREPAIRCOPY)
							{
								if ((((status1 == svn_wc_status_normal)||(status1 == svn_wc_status_modified)||(status1 == svn_wc_status_merged)) && ((status2 == svn_wc_status_unversioned)||(status2 == svn_wc_status_none))) ||
									(((status2 == svn_wc_status_normal)||(status2 == svn_wc_status_modified)||(status2 == svn_wc_status_merged)) && ((status1 == svn_wc_status_unversioned)||(status1 == svn_wc_status_none))))
								{
									popup.AppendMenuIcon(IDSVNLC_REPAIRCOPY, IDS_STATUSLIST_CONTEXT_REPAIRCOPY);
								}
							}
						}
					}
				}
				if ((wcStatus > svn_wc_status_normal)&&(wcStatus != svn_wc_status_ignored))
				{
					if (m_dwContextMenus & SVNSLC_POPREVERT)
					{
						// reverting missing folders is not possible
						if (!entry->IsFolder() || (wcStatus != svn_wc_status_missing))
						{
							popup.AppendMenuIcon(IDSVNLC_REVERT, IDS_MENUREVERT, IDI_REVERT);
						}
					}
				}
				if (entry->remotestatus > svn_wc_status_normal)
				{
					if (m_dwContextMenus & SVNSLC_POPUPDATE)
					{
						popup.AppendMenuIcon(IDSVNLC_UPDATE, IDS_MENUUPDATE, IDI_UPDATE);
					}
				}
			}
			if ((selectedCount == 1)&&(wcStatus >= svn_wc_status_normal)
				&&(wcStatus != svn_wc_status_ignored)
				&&((wcStatus != svn_wc_status_added)||(!entry->copyfrom_url.IsEmpty())))
			{
				if (m_dwContextMenus & SVNSLC_POPSHOWLOG)
				{
					popup.AppendMenuIcon(IDSVNLC_LOG, IDS_REPOBROWSE_SHOWLOG, IDI_LOG);
				}
				if (m_dwContextMenus & SVNSLC_POPBLAME)
				{
					popup.AppendMenuIcon(IDSVNLC_BLAME, IDS_MENUBLAME, IDI_BLAME);
				}
			}
			if ((wcStatus != svn_wc_status_deleted)&&(wcStatus != svn_wc_status_missing) && (GetSelectedCount() == 1))
			{
				if (m_dwContextMenus & SVNSLC_POPOPEN)
				{
					popup.AppendMenuIcon(IDSVNLC_OPEN, IDS_REPOBROWSE_OPEN, IDI_OPEN);
					popup.AppendMenuIcon(IDSVNLC_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
				}
				if (m_dwContextMenus & SVNSLC_POPEXPLORE)
				{
					popup.AppendMenuIcon(IDSVNLC_EXPLORE, IDS_STATUSLIST_CONTEXT_EXPLORE, IDI_EXPLORER);
				}
				if ((m_dwContextMenus & SVNSLC_POPCHECKFORMODS)&&(entry->IsFolder()))
				{
					popup.AppendMenuIcon(IDSVNLC_CHECKFORMODS, IDS_MENUSHOWCHANGED, IDI_SHOWCHANGED);
				}
			}
			if (selectedCount > 0)
			{
				if (((wcStatus == svn_wc_status_unversioned)||(wcStatus == svn_wc_status_ignored))&&(m_dwContextMenus & SVNSLC_POPDELETE))
				{
					popup.AppendMenuIcon(IDSVNLC_DELETE, IDS_MENUREMOVE, IDI_DELETE);
				}
				if ((wcStatus != svn_wc_status_unversioned)&&(wcStatus != svn_wc_status_ignored)&&
					(wcStatus != svn_wc_status_deleted)&&(wcStatus != svn_wc_status_added)&&(wcStatus != svn_wc_status_missing)&&(m_dwContextMenus & SVNSLC_POPDELETE))
				{
					if (bShift)
						popup.AppendMenuIcon(IDSVNLC_REMOVE, IDS_MENUREMOVEKEEP, IDI_DELETE);
					else
						popup.AppendMenuIcon(IDSVNLC_REMOVE, IDS_MENUREMOVE, IDI_DELETE);
				}
				if ((wcStatus == svn_wc_status_unversioned)||(wcStatus == svn_wc_status_deleted)||(wcStatus == svn_wc_status_ignored))
				{
					if (m_dwContextMenus & SVNSLC_POPADD)
					{
						if (entry->IsFolder())
						{
							if (wcStatus != svn_wc_status_deleted)
								popup.AppendMenuIcon(IDSVNLC_ADD_RECURSIVE, IDS_STATUSLIST_CONTEXT_ADD_RECURSIVE, IDI_ADD);
						}
						else
						{
							popup.AppendMenuIcon(IDSVNLC_ADD, IDS_STATUSLIST_CONTEXT_ADD, IDI_ADD);
						}
					}
				}
				if ( (wcStatus == svn_wc_status_unversioned) || (wcStatus == svn_wc_status_deleted) )
				{
					if (m_dwContextMenus & SVNSLC_POPIGNORE)
					{
						CTSVNPathList ignorelist;
						FillListOfSelectedItemPaths(ignorelist);
						// check if all selected entries have the same extension
						bool bSameExt = true;
						CString sExt;
						for (int i=0; i<ignorelist.GetCount(); ++i)
						{
							if (sExt.IsEmpty() && (i==0))
								sExt = ignorelist[i].GetFileExtension();
							else if (sExt.CompareNoCase(ignorelist[i].GetFileExtension())!=0)
								bSameExt = false;
						}
						if (bSameExt)
						{
							if (ignoreSubMenu.CreateMenu())
							{
								CString ignorepath;
								if (ignorelist.GetCount()==1)
									ignorepath = ignorelist[0].GetFileOrDirectoryName();
								else
									ignorepath.Format(IDS_MENUIGNOREMULTIPLE, ignorelist.GetCount());
								ignoreSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNORE, ignorepath);
								ignorepath = _T("*")+sExt;
								ignoreSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNOREMASK, ignorepath);
								CString temp;
								temp.LoadString(IDS_MENUIGNORE);
								popup.InsertMenu((UINT)-1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)ignoreSubMenu.m_hMenu, temp);
							}
						}
						else
						{
							CString temp;
							if (ignorelist.GetCount()==1)
							{
								temp.LoadString(IDS_MENUIGNORE);
							}
							else
							{
								temp.Format(IDS_MENUIGNOREMULTIPLE, ignorelist.GetCount());
							}
							popup.AppendMenuIcon(IDSVNLC_IGNORE, temp, IDI_IGNORE);
						}
					}
				}
			}
			if (((wcStatus == svn_wc_status_conflicted)||(entry->isConflicted)||(entry->tree_conflicted)))
			{
				if ((m_dwContextMenus & SVNSLC_POPCONFLICT)||(m_dwContextMenus & SVNSLC_POPRESOLVE))
					popup.AppendMenu(MF_SEPARATOR);

				if (m_dwContextMenus & SVNSLC_POPCONFLICT)
				{
					popup.AppendMenuIcon(IDSVNLC_EDITCONFLICT, IDS_MENUCONFLICT, IDI_CONFLICT);
					if (entry->isConflicted)
						popup.SetDefaultItem(IDSVNLC_EDITCONFLICT, FALSE);
				}
				if (m_dwContextMenus & SVNSLC_POPRESOLVE)
				{
					popup.AppendMenuIcon(IDSVNLC_RESOLVECONFLICT, IDS_STATUSLIST_CONTEXT_RESOLVED, IDI_RESOLVE);
				}
				if ((m_dwContextMenus & SVNSLC_POPRESOLVE)&&(entry->textstatus == svn_wc_status_conflicted))
				{
					popup.AppendMenuIcon(IDSVNLC_RESOLVETHEIRS, IDS_SVNPROGRESS_MENUUSETHEIRS, IDI_RESOLVE);
					popup.AppendMenuIcon(IDSVNLC_RESOLVEMINE, IDS_SVNPROGRESS_MENUUSEMINE, IDI_RESOLVE);
				}
			}
			if (selectedCount > 0)
			{
				if ((!entry->IsFolder())&&(wcStatus >= svn_wc_status_normal)
					&&(wcStatus!=svn_wc_status_missing)&&(wcStatus!=svn_wc_status_deleted)
					&&(wcStatus!=svn_wc_status_added)&&(wcStatus!=svn_wc_status_ignored))
				{
					popup.AppendMenu(MF_SEPARATOR);
					if ((entry->lock_token.IsEmpty())&&(!entry->IsFolder()))
					{
						if (m_dwContextMenus & SVNSLC_POPLOCK)
						{
							popup.AppendMenuIcon(IDSVNLC_LOCK, IDS_MENU_LOCK, IDI_LOCK);
						}
					}
					if ((!entry->lock_token.IsEmpty())&&(!entry->IsFolder()))
					{
						if (m_dwContextMenus & SVNSLC_POPUNLOCK)
						{
							popup.AppendMenuIcon(IDSVNLC_UNLOCK, IDS_MENU_UNLOCK, IDI_UNLOCK);
						}
					}
				}
				if ((!entry->IsFolder())&&((!entry->lock_token.IsEmpty())||(!entry->lock_remotetoken.IsEmpty())))
				{
					if (m_dwContextMenus & SVNSLC_POPUNLOCKFORCE)
					{
						popup.AppendMenuIcon(IDSVNLC_UNLOCKFORCE, IDS_MENU_UNLOCKFORCE, IDI_UNLOCK);
					}
				}
				if ((!entry->IsFolder())&&(wcStatus >= svn_wc_status_normal)
					&&(wcStatus!=svn_wc_status_missing)&&(wcStatus!=svn_wc_status_deleted)&&(wcStatus!=svn_wc_status_ignored))
				{
					if (m_dwContextMenus & SVNSLC_POPCREATEPATCH)
					{
						popup.AppendMenu(MF_SEPARATOR);
						popup.AppendMenuIcon(IDSVNLC_CREATEPATCH, IDS_MENUCREATEPATCH, IDI_CREATEPATCH);
					}
				}

				if (wcStatus != svn_wc_status_missing && 
					wcStatus != svn_wc_status_deleted &&
					wcStatus != svn_wc_status_unversioned &&
					wcStatus != svn_wc_status_ignored)
				{
					popup.AppendMenu(MF_SEPARATOR);
					popup.AppendMenuIcon(IDSVNLC_PROPERTIES, IDS_STATUSLIST_CONTEXT_PROPERTIES, IDI_PROPERTIES);
				}
				popup.AppendMenu(MF_SEPARATOR);
				popup.AppendMenuIcon(IDSVNLC_COPY, IDS_STATUSLIST_CONTEXT_COPY, IDI_COPYCLIP);
				popup.AppendMenuIcon(IDSVNLC_COPYEXT, IDS_STATUSLIST_CONTEXT_COPYEXT, IDI_COPYCLIP);
				if ((m_dwContextMenus & SVNSLC_POPCHANGELISTS)
					&&(wcStatus != svn_wc_status_unversioned)&&(wcStatus != svn_wc_status_none))
				{
					popup.AppendMenu(MF_SEPARATOR);
					// changelist commands
					size_t numChangelists = GetNumberOfChangelistsInSelection();
					if (numChangelists > 0)
					{
						popup.AppendMenuIcon(IDSVNLC_REMOVEFROMCS, IDS_STATUSLIST_CONTEXT_REMOVEFROMCS);
					}
					if ((!entry->IsFolder())&&(changelistSubMenu.CreateMenu()))
					{
						CString temp;
						temp.LoadString(IDS_STATUSLIST_CONTEXT_CREATECS);
						changelistSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_CREATECS, temp);

						if (entry->changelist.Compare(SVNSLC_IGNORECHANGELIST))
						{
							changelistSubMenu.AppendMenu(MF_SEPARATOR);
							changelistSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_CREATEIGNORECS, SVNSLC_IGNORECHANGELIST);
						}

						if (m_changelists.size() > 0)
						{
							// find the changelist names
							bool bNeedSeparator = true;
							int cmdID = IDSVNLC_MOVETOCS;
							for (std::map<CString, int>::const_iterator it = m_changelists.begin(); it != m_changelists.end(); ++it)
							{
								if ((entry->changelist.Compare(it->first))&&(it->first.Compare(SVNSLC_IGNORECHANGELIST)))
								{
									if (bNeedSeparator)
									{
										changelistSubMenu.AppendMenu(MF_SEPARATOR);
										bNeedSeparator = false;
									}
									changelistSubMenu.AppendMenu(MF_STRING | MF_ENABLED, cmdID, it->first);
									cmdID++;
								}
							}
						}
						temp.LoadString(IDS_STATUSLIST_CONTEXT_MOVETOCS);
						popup.AppendMenu(MF_POPUP|MF_STRING, (UINT_PTR)changelistSubMenu.GetSafeHmenu(), temp);
					}
				}
			}

			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			m_bBlock = TRUE;
			AfxGetApp()->DoWaitCursor(1);
			int iItemCountBeforeMenuCmd = GetItemCount();
			size_t iChangelistCountBeforeMenuCmd = m_changelists.size();
			bool bForce = false;
			switch (cmd)
			{
			case IDSVNLC_COPY:
				CopySelectedEntriesToClipboard(SVNSLC_COLFILEPATH);
				break;
			case IDSVNLC_COPYEXT:
				CopySelectedEntriesToClipboard((DWORD)-1);
				break;
			case IDSVNLC_PROPERTIES:
				{
					CTSVNPathList targetList;
					FillListOfSelectedItemPaths(targetList);
					CEditPropertiesDlg dlg;
					dlg.SetPathList(targetList);
					dlg.DoModal();
					if (dlg.HasChanged())
					{
						// since the user might have changed/removed/added
						// properties recursively, we don't really know
						// which items have changed their status.
						// So tell the parent to do a refresh.
						CWnd* pParent = GetParent();
						if (NULL != pParent && NULL != pParent->GetSafeHwnd())
						{
							pParent->SendMessage(SVNSLNM_NEEDSREFRESH);
						}
					}
				}
				break;
			case IDSVNLC_COMMIT:
				{
					CTSVNPathList targetList;
					FillListOfSelectedItemPaths(targetList);
					CTSVNPath tempFile = CTempFiles::Instance().GetTempFilePath(false);
					VERIFY(targetList.WriteToFile(tempFile.GetWinPathString()));
					CString commandline = CPathUtils::GetAppDirectory();
					commandline += _T("TortoiseProc.exe /command:commit /pathfile:\"");
					commandline += tempFile.GetWinPathString();
					commandline += _T("\"");
					commandline += _T(" /deletepathfile");
					CAppUtils::LaunchApplication(commandline, NULL, false);
				}
				break;
			case IDSVNLC_REVERT:
				{
					// If at least one item is not in the status "added"
					// we ask for a confirmation
					BOOL bConfirm = FALSE;
					POSITION pos = GetFirstSelectedItemPosition();
					int index;
					while ((index = GetNextSelectedItem(pos)) >= 0)
					{
						FileEntry * fentry = GetListEntry(index);
						if (fentry->textstatus != svn_wc_status_added)
						{
							bConfirm = TRUE;
							break;
						}
					}	

					CString str;
					str.Format(IDS_PROC_WARNREVERT,GetSelectedCount());

					if (!bConfirm || CMessageBox::Show(this->m_hWnd, str, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION)==IDYES)
					{
						CTSVNPathList targetList;
						FillListOfSelectedItemPaths(targetList);

						// make sure that the list is reverse sorted, so that
						// children are removed before any parents
						targetList.SortByPathname(true);

						SVN svn;

						// put all reverted files in the trashbin, except the ones with 'added'
						// status because they are not restored by the revert.
						CTSVNPathList delList;
						pos = GetFirstSelectedItemPosition();
						while ((index = GetNextSelectedItem(pos)) >= 0)
						{
							FileEntry * entry2 = GetListEntry(index);
							if ((entry2->status != svn_wc_status_added)&&(entry2->status != svn_wc_status_none)&&(entry2->status != svn_wc_status_unversioned))
								delList.AddPath(entry2->GetPath());
						}
						if (DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\RevertWithRecycleBin"), TRUE)))
							delList.DeleteAllPaths(true, true);

						if (!svn.Revert(targetList, CStringArray(), false))
						{
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
						else
						{
							// since the entries got reverted we need to remove
							// them from the list too, if no remote changes are shown,
							// if the unmodified files are not shown
							// and if the item is not part of a changelist
							SetRedraw(FALSE);
							while ((pos = GetFirstSelectedItemPosition())!=0)
							{
								index = GetNextSelectedItem(pos);
								FileEntry * fentry = m_arStatusArray[m_arListArray[index]];
								if ( fentry->IsFolder() )
								{
									// refresh!
									CWnd* pParent = GetParent();
									if (NULL != pParent && NULL != pParent->GetSafeHwnd())
									{
										pParent->SendMessage(SVNSLNM_NEEDSREFRESH);
									}
									break;
								}

								BOOL bAdded = (fentry->textstatus == svn_wc_status_added);
								fentry->status = svn_wc_status_normal;
								fentry->propstatus = svn_wc_status_normal;
								fentry->textstatus = svn_wc_status_normal;
								fentry->copied = false;
								fentry->isConflicted = false;
								if ((fentry->GetChangeList().IsEmpty()&&(fentry->remotestatus <= svn_wc_status_normal))||(m_dwShow & SVNSLC_SHOWNORMAL))
								{
									if ( bAdded )
									{
										// reverting added items makes them unversioned, not 'normal'
										if (fentry->IsFolder())
											fentry->propstatus = svn_wc_status_none;
										else
											fentry->propstatus = svn_wc_status_unversioned;
										fentry->status = svn_wc_status_unversioned;
										fentry->textstatus = svn_wc_status_unversioned;
										SetItemState(index, 0, LVIS_SELECTED);
										SetEntryCheck(fentry, index, false);
									}
									else if ((fentry->switched)||(m_dwShow & SVNSLC_SHOWNORMAL))
									{
										SetItemState(index, 0, LVIS_SELECTED);
									}
									else
									{
										m_nTotal--;
										if (GetCheck(index))
											m_nSelected--;
										RemoveListEntry(index);
										Invalidate();
									}
								}
								else
								{
									SetItemState(index, 0, LVIS_SELECTED);
								}
							}
							SetRedraw(TRUE);
							SaveColumnWidths();
							Show(m_dwShow, CTSVNPathList(), 0, m_bShowFolders, m_bShowFiles);
							NotifyCheck();
						}
					}
				}
				break;
			case IDSVNLC_COMPARE:
				{
					POSITION pos = GetFirstSelectedItemPosition();
					while ( pos )
					{
						int index = GetNextSelectedItem(pos);
						StartDiff(index);
					}
				}
				break;
			case IDSVNLC_COMPAREWC:
				{
					POSITION pos = GetFirstSelectedItemPosition();
					while ( pos )
					{
						int index = GetNextSelectedItem(pos);
						FileEntry * entry2 = GetListEntry(index);
						ASSERT(entry2 != NULL);
						if (entry2 == NULL)
							continue;
						SVNDiff diff(NULL, m_hWnd, true);
						diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
						svn_revnum_t baseRev = entry2->Revision;
						diff.DiffFileAgainstBase(
							entry2->path, baseRev, entry2->textstatus, entry2->propstatus);
					}
				}
				break;
			case IDSVNLC_GNUDIFF1:
				{
					SVNDiff diff(NULL, this->m_hWnd, true);

					if (entry->remotestatus <= svn_wc_status_normal)
						CAppUtils::StartShowUnifiedDiff(m_hWnd, entry->path, SVNRev::REV_BASE, entry->path, SVNRev::REV_WC);
					else
						CAppUtils::StartShowUnifiedDiff(m_hWnd, entry->path, SVNRev::REV_WC, entry->path, SVNRev::REV_HEAD);
				}
				break;
			case IDSVNLC_UPDATE:
				{
					CTSVNPathList targetList;
					FillListOfSelectedItemPaths(targetList);
					bool bAllExist = true;
					for (int i=0; i<targetList.GetCount(); ++i)
					{
						if (!targetList[i].Exists())
						{
							bAllExist = false;
							break;
						}
					}
					if (bAllExist)
					{
						CSVNProgressDlg dlg;
						dlg.SetCommand(CSVNProgressDlg::SVNProgress_Update);
						dlg.SetPathList(targetList);
						dlg.SetRevision(SVNRev::REV_HEAD);
						dlg.DoModal();
					}
					else
					{
						CString sTempFile = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString();
						targetList.WriteToFile(sTempFile, false);
						CString sCmd;
						sCmd.Format(_T("\"%s\" /command:update /rev /pathfile:\"%s\" /deletepathfile"),
							(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)sTempFile);

						CAppUtils::LaunchApplication(sCmd, NULL, false);
					}
				}
				break;
			case IDSVNLC_LOG:
				{
					CString logPath = filepath.GetWinPathString();
					if (!entry->copyfrom_url.IsEmpty())
						logPath = entry->copyfrom_url;
					CString sCmd;
					sCmd.Format(_T("\"%s\" /command:log /path:\"%s\""),
						(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)logPath);

					if (!filepath.IsUrl())
					{
						sCmd += _T(" /propspath:\"");
						sCmd += filepath.GetWinPathString();
						sCmd += _T("\"");
					}	

					CAppUtils::LaunchApplication(sCmd, NULL, false);
				}
				break;
			case IDSVNLC_BLAME:
				{
					CString blamePath = filepath.GetWinPathString();
					if (!entry->copyfrom_url.IsEmpty())
						blamePath = entry->copyfrom_url;
					CString sCmd;
					sCmd.Format(_T("\"%s\" /command:blame /path:\"%s\""),
						(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)blamePath);

					if (!filepath.IsUrl())
					{
						sCmd += _T(" /propspath:\"");
						sCmd += filepath.GetWinPathString();
						sCmd += _T("\"");
					}	

					CAppUtils::LaunchApplication(sCmd, NULL, false);
				}
				break;
			case IDSVNLC_OPEN:
				{
					CTSVNPath fp = filepath;
					if ((!filepath.Exists())&&(entry->remotestatus == svn_wc_status_added))
					{
						// fetch the file from the repository
						SVN svn;
						CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, filepath);
						if (!svn.Cat(CTSVNPath(entry->GetURL()), SVNRev::REV_HEAD, SVNRev::REV_HEAD, tempfile))
						{
							CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
						fp = tempfile;
					}
					int ret = (int)ShellExecute(this->m_hWnd, NULL, fp.GetWinPath(), NULL, NULL, SW_SHOW);
					if (ret <= HINSTANCE_ERROR)
					{
						CString c = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
						c += fp.GetWinPathString();
						CAppUtils::LaunchApplication(c, NULL, false);
					}
				}
				break;
			case IDSVNLC_OPENWITH:
				{
					CTSVNPath fp = filepath;
					if ((!filepath.Exists())&&(entry->remotestatus == svn_wc_status_added))
					{
						// fetch the file from the repository
						SVN svn;
						CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, filepath);
						if (!svn.Cat(CTSVNPath(entry->GetURL()), SVNRev::REV_HEAD, SVNRev::REV_HEAD, tempfile))
						{
							CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
						fp = tempfile;
					}
					CString c = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
					c += fp.GetWinPathString() + _T(" ");
					CAppUtils::LaunchApplication(c, NULL, false);
				}
				break;
			case IDSVNLC_EXPLORE:
				{
					ShellExecute(this->m_hWnd, _T("explore"), filepath.GetDirectory().GetWinPath(), NULL, NULL, SW_SHOW);
				}
				break;
			case IDSVNLC_CHECKFORMODS:
				{
					CTSVNPathList targetList;
					FillListOfSelectedItemPaths(targetList);

					CString sTempFile = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString();
					targetList.WriteToFile(sTempFile, false);
					CString sCmd;
					sCmd.Format(_T("\"%s\" /command:repostatus /pathfile:\"%s\" /deletepathfile"),
						(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)sTempFile);

					CAppUtils::LaunchApplication(sCmd, NULL, false);
				}
				break;
			case IDSVNLC_CREATEPATCH:
				{
					CTSVNPathList targetList;
					FillListOfSelectedItemPaths(targetList);

					CString sTempFile = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString();
					targetList.WriteToFile(sTempFile, false);
					CString sCmd;
					sCmd.Format(_T("\"%s\" /command:createpatch /pathfile:\"%s\" /deletepathfile /noui"),
						(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)sTempFile);

					CAppUtils::LaunchApplication(sCmd, NULL, false);
				}
				break;
			case IDSVNLC_REMOVE:
				{
					SVN svn;
					CTSVNPathList itemsToRemove;
					FillListOfSelectedItemPaths(itemsToRemove);

					// We must sort items before removing, so that files are always removed
					// *before* their parents
					itemsToRemove.SortByPathname(true);

					bool bSuccess = false;
					if (svn.Remove(itemsToRemove, FALSE, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000)))
					{
						bSuccess = true;
					}
					else
					{
						if ((svn.Err->apr_err == SVN_ERR_UNVERSIONED_RESOURCE) ||
							(svn.Err->apr_err == SVN_ERR_CLIENT_MODIFIED))
						{
							CString msg, yes, no, yestoall;
							msg.Format(IDS_PROC_REMOVEFORCE, (LPCTSTR)svn.GetLastErrorMessage());
							yes.LoadString(IDS_MSGBOX_YES);
							no.LoadString(IDS_MSGBOX_NO);
							yestoall.LoadString(IDS_PROC_YESTOALL);
							UINT ret = CMessageBox::Show(m_hWnd, msg, _T("TortoiseSVN"), 2, IDI_ERROR, yes, no, yestoall);
							if ((ret == 1)||(ret==3))
							{
								if (!svn.Remove(itemsToRemove, TRUE, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000)))
								{
									CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								}
								else
									bSuccess = true;
							}
						}
						else if (svn.Err->apr_err == SVN_ERR_BAD_FILENAME)
						{
							// the file/folder didn't exist (was 'missing')
							// which means the remove succeeded anyway but svn still
							// threw an error - we just ignore the error and
							// assume a normal and successful remove
							bSuccess = true;
						}
						else
							CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
					if (bSuccess)
					{
						// The remove went ok, but we now need to run through the selected items again
						// and update their status
						POSITION pos = GetFirstSelectedItemPosition();
						int index;
						std::vector<int> entriesToRemove;
						while ((index = GetNextSelectedItem(pos)) >= 0)
						{
							FileEntry * e = GetListEntry(index);
							if (!bShift &&
								((e->textstatus == svn_wc_status_unversioned)||
								(e->textstatus == svn_wc_status_none)||
								(e->textstatus == svn_wc_status_ignored)))
							{
								if (GetCheck(index))
									m_nSelected--;
								m_nTotal--;
								entriesToRemove.push_back(index);
							}
							else
							{
								e->textstatus = svn_wc_status_deleted;
								e->status = svn_wc_status_deleted;
								SetEntryCheck(e,index,true);
							}
						}
						for (std::vector<int>::reverse_iterator it = entriesToRemove.rbegin(); it != entriesToRemove.rend(); ++it)
						{
							RemoveListEntry(*it);
						}
					}
					SaveColumnWidths();
					Show(m_dwShow, CTSVNPathList(), 0, m_bShowFolders, m_bShowFiles);
					NotifyCheck();
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
					auto_buffer<TCHAR> buf(len+2);
					_tcscpy_s(buf, len+2, filelist);
					CStringUtils::PipesToNulls(buf, len);
					SHFILEOPSTRUCT fileop;
					fileop.hwnd = this->m_hWnd;
					fileop.wFunc = FO_DELETE;
					fileop.pFrom = buf;
					fileop.pTo = NULL;
					fileop.fAnyOperationsAborted = FALSE;
					fileop.fFlags = FOF_NO_CONNECTED_ELEMENTS | ((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 0 : FOF_ALLOWUNDO);
					fileop.lpszProgressTitle = _T("deleting file");
					int result = SHFileOperation(&fileop);

					if ( (result==0) && (!fileop.fAnyOperationsAborted) )
					{
						SetRedraw(FALSE);
						POSITION pos = NULL;
						CTSVNPathList deletedlist;	// to store list of deleted folders
						while ((pos = GetFirstSelectedItemPosition()) != 0)
						{
							int index = GetNextSelectedItem(pos);
							if (GetCheck(index))
								m_nSelected--;
							m_nTotal--;
							FileEntry * fentry = GetListEntry(index);
							if ((fentry)&&(fentry->isfolder))
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
								FileEntry * entry2 = GetListEntry(i);
								if (folderpath.IsAncestorOf(entry2->path))
								{
									RemoveListEntry(i--);
									nListboxEntries--;
								}
							}
						}
						SetRedraw(TRUE);
					}
				}
				break;
			case IDSVNLC_IGNOREMASK:
				{
					CString name = _T("*")+filepath.GetFileExtension();
					CTSVNPathList ignorelist;
					FillListOfSelectedItemPaths(ignorelist, true);
					std::set<CTSVNPath> parentlist;
					for (int i=0; i<ignorelist.GetCount(); ++i)
					{
						parentlist.insert(ignorelist[i].GetContainingDirectory());
					}
					std::set<CTSVNPath>::iterator it;
					std::vector<CString> toremove;
					SetRedraw(FALSE);
					for (it = parentlist.begin(); it != parentlist.end(); ++it)
					{
						CTSVNPath parentFolder = (*it).GetDirectory();
						SVNProperties props(parentFolder, SVNRev::REV_WC, false);
						CString value;
						for (int i=0; i<props.GetCount(); i++)
						{
							CString propname(props.GetItemName(i).c_str());
							if (propname.CompareNoCase(svnPropIgnore)==0)
							{
								tstring stemp;
								// treat values as normal text even if they're not
								value = CUnicodeUtils::GetUnicode(props.GetItemValue(i).c_str());
							}
						}
						if (value.IsEmpty())
							value = name;
						else
						{
							value = value.Trim(_T("\n\r"));
							value += _T("\n");
							value += name;
							value.Remove('\r');
						}
						if (!props.Add(SVN_PROP_IGNORE, (LPCSTR)CUnicodeUtils::GetUTF8(value)))
						{
							CString temp;
							temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, (LPCTSTR)name);
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						}
						else
						{
							CTSVNPath basepath;
							int nListboxEntries = GetItemCount();
							for (int i=0; i<nListboxEntries; ++i)
							{
								FileEntry * entry2 = GetListEntry(i);
								ASSERT(entry2 != NULL);
								if (entry2 == NULL)
									continue;
								if (basepath.IsEmpty())
									basepath = entry2->basepath;
								// since we ignored files with a mask (e.g. *.exe)
								// we have to find find all files in the same
								// folder (IsAncestorOf() returns TRUE for _all_ children,
								// not just the immediate ones) which match the
								// mask and remove them from the list too.
								if ((entry2->status == svn_wc_status_unversioned)&&(parentFolder.IsAncestorOf(entry2->path)))
								{
									CString f = entry2->path.GetSVNPathString();
									if (f.Mid(parentFolder.GetSVNPathString().GetLength()).ReverseFind('/')<=0)
									{
										if (CStringUtils::WildCardMatch(name, f))
										{
											if (GetCheck(i))
												m_nSelected--;
											m_nTotal--;
											toremove.push_back(f);
										}
									}
								}
							}
							if (!m_bIgnoreRemoveOnly)
							{
								SVNStatus status;
								svn_wc_status2_t * s;
								CTSVNPath svnPath;
								s = status.GetFirstFileStatus(parentFolder, svnPath, false, svn_depth_empty);
								if (s!=0)
								{
									// first check if the folder isn't already present in the list
									bool bFound = false;
									for (int i=0; i<nListboxEntries; ++i)
									{
										FileEntry * entry2 = GetListEntry(i);
										if (entry2->path.IsEquivalentTo(svnPath))
										{
											bFound = true;
											break;
										}
									}
									if (!bFound)
									{
										FileEntry * entry2 = new FileEntry();
										entry2->path = svnPath;
										entry2->basepath = basepath;
										entry2->status = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
										entry2->textstatus = s->text_status;
										entry2->propstatus = s->prop_status;
										entry2->remotestatus = SVNStatus::GetMoreImportant(s->repos_text_status, s->repos_prop_status);
										entry2->remotetextstatus = s->repos_text_status;
										entry2->remotepropstatus = s->repos_prop_status;
										entry2->inunversionedfolder = false;
										entry2->checked = true;
										entry2->inexternal = false;
										entry2->direct = false;
										entry2->isfolder = true;
										entry2->last_commit_date = 0;
										entry2->last_commit_rev = 0;
										entry2->remoterev = 0;
										if (s->entry)
										{
											if (s->entry->url)
											{
												entry2->url = CPathUtils::PathUnescape(s->entry->url);
											}
										}
										if (s->entry && s->entry->present_props)
										{
											entry2->present_props = s->entry->present_props;
										}
										m_arStatusArray.push_back(entry2);
										m_arListArray.push_back(m_arStatusArray.size()-1);
										AddEntry(entry2, langID, GetItemCount());
									}
								}
							}
						}
					}
					for (std::vector<CString>::iterator it2 = toremove.begin(); it2 != toremove.end(); ++it2)
					{
						int nListboxEntries = GetItemCount();
						for (int i=0; i<nListboxEntries; ++i)
						{
							if (GetListEntry(i)->path.GetSVNPathString().Compare(*it2)==0)
							{
								RemoveListEntry(i);
								break;
							}
						}
					}
					SetRedraw(TRUE);
				}
				break;
			case IDSVNLC_IGNORE:
				{
					CTSVNPathList ignorelist;
					std::vector<CString> toremove;
					FillListOfSelectedItemPaths(ignorelist, true);
					SetRedraw(FALSE);
					for (int j=0; j<ignorelist.GetCount(); ++j)
					{
						int nListboxEntries = GetItemCount();
						for (int i=0; i<nListboxEntries; ++i)
						{
							if (GetListEntry(i)->GetPath().IsEquivalentTo(ignorelist[j]))
							{
								selIndex = i;
								break;
							}
						}
						CString name = CPathUtils::PathPatternEscape(ignorelist[j].GetFileOrDirectoryName());
						CTSVNPath parentfolder = ignorelist[j].GetContainingDirectory();
						SVNProperties props(parentfolder, SVNRev::REV_WC, false);
						CString value;
						for (int i=0; i<props.GetCount(); i++)
						{
							CString propname(props.GetItemName(i).c_str());
							if (propname.CompareNoCase(svnPropIgnore)==0)
							{
								tstring stemp;
								// treat values as normal text even if they're not
								value = CUnicodeUtils::GetUnicode(props.GetItemValue(i).c_str());
							}
						}
						if (value.IsEmpty())
							value = name;
						else
						{
							value = value.Trim(_T("\n\r"));
							value += _T("\n");
							value += name;
							value.Remove('\r');
						}
						if (!props.Add(SVN_PROP_IGNORE, (LPCSTR)CUnicodeUtils::GetUTF8(value)))
						{
							CString temp;
							temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, (LPCTSTR)name);
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
						if (GetCheck(selIndex))
							m_nSelected--;
						m_nTotal--;

						// now, if we ignored a folder, remove all its children
						if (ignorelist[j].IsDirectory())
						{
							for (int i=0; i<(int)m_arListArray.size(); ++i)
							{
								FileEntry * entry2 = GetListEntry(i);
								if (entry2->status == svn_wc_status_unversioned)
								{
									if (!ignorelist[j].IsEquivalentTo(entry2->GetPath())&&(ignorelist[j].IsAncestorOf(entry2->GetPath())))
									{
										entry2->status = svn_wc_status_ignored;
										entry2->textstatus = svn_wc_status_ignored;
										if (GetCheck(i))
											m_nSelected--;
										toremove.push_back(entry2->GetPath().GetSVNPathString());
									}
								}
							}
						}

						CTSVNPath basepath = m_arStatusArray[m_arListArray[selIndex]]->basepath;

						FileEntry * entry2 = m_arStatusArray[m_arListArray[selIndex]];
						if ( entry2->status == svn_wc_status_unversioned ) // keep "deleted" items
							toremove.push_back(entry2->GetPath().GetSVNPathString());

						if (!m_bIgnoreRemoveOnly)
						{
							SVNStatus status;
							svn_wc_status2_t * s;
							CTSVNPath svnPath;
							s = status.GetFirstFileStatus(parentfolder, svnPath, false, svn_depth_empty);
							// first check if the folder isn't already present in the list
							bool bFound = false;
							nListboxEntries = GetItemCount();
							for (int i=0; i<nListboxEntries; ++i)
							{
								FileEntry * entry3 = GetListEntry(i);
								if (entry3->path.IsEquivalentTo(svnPath))
								{
									bFound = true;
									break;
								}
							}
							if (!bFound)
							{
								if (s!=0)
								{
									FileEntry * entry3 = new FileEntry();
									entry3->path = svnPath;
									entry3->basepath = basepath;
									entry3->status = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
									entry3->textstatus = s->text_status;
									entry3->propstatus = s->prop_status;
									entry3->remotestatus = SVNStatus::GetMoreImportant(s->repos_text_status, s->repos_prop_status);
									entry3->remotetextstatus = s->repos_text_status;
									entry3->remotepropstatus = s->repos_prop_status;
									entry3->inunversionedfolder = FALSE;
									entry3->checked = true;
									entry3->inexternal = false;
									entry3->direct = false;
									entry3->isfolder = true;
									entry3->last_commit_date = 0;
									entry3->last_commit_rev = 0;
									entry3->remoterev = 0;
									if (s->entry)
									{
										if (s->entry->url)
										{
											entry3->url = CPathUtils::PathUnescape (s->entry->url);
										}
									}
									if (s->entry && s->entry->present_props)
									{
										entry3->present_props = s->entry->present_props;
									}
									m_arStatusArray.push_back(entry3);
									m_arListArray.push_back(m_arStatusArray.size()-1);
									AddEntry(entry3, langID, GetItemCount());
								}
							}
						}
					}
					for (std::vector<CString>::iterator it = toremove.begin(); it != toremove.end(); ++it)
					{
						int nListboxEntries = GetItemCount();
						for (int i=0; i<nListboxEntries; ++i)
						{
							if (GetListEntry(i)->path.GetSVNPathString().Compare(*it)==0)
							{
								RemoveListEntry(i);
								break;
							}
						}
					}
					SetRedraw(TRUE);
				}
				break;
			case IDSVNLC_EDITCONFLICT:
				{
					CString sCmd;
					sCmd.Format(_T("\"%s\" /command:conflicteditor /path:\"%s\""),
						(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), filepath.GetWinPath());
					if (!filepath.IsUrl())
					{
						sCmd += _T(" /propspath:\"");
						sCmd += filepath.GetWinPathString();
						sCmd += _T("\"");
					}	
					CAppUtils::LaunchApplication(sCmd, NULL, false);
				}
				break;
			case IDSVNLC_RESOLVECONFLICT:
			case IDSVNLC_RESOLVEMINE:
			case IDSVNLC_RESOLVETHEIRS:
				{
					svn_wc_conflict_choice_t result = svn_wc_conflict_choose_merged;
					switch (cmd)
					{
					case IDSVNLC_RESOLVETHEIRS:
						result = svn_wc_conflict_choose_theirs_full;
						break;
					case IDSVNLC_RESOLVEMINE:
						result = svn_wc_conflict_choose_mine_full;
						break;
					case IDSVNLC_RESOLVECONFLICT:
						result = svn_wc_conflict_choose_merged;
						break;
					}
					if (CMessageBox::Show(m_hWnd, IDS_PROC_RESOLVE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO)==IDYES)
					{
						SVN svn;
						POSITION pos = GetFirstSelectedItemPosition();
						while (pos != 0)
						{
							int index;
							index = GetNextSelectedItem(pos);
							FileEntry * fentry = m_arStatusArray[m_arListArray[index]];
							if (!svn.Resolve(fentry->GetPath(), result, FALSE))
							{
								CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							}
							else
							{
								fentry->status = svn_wc_status_modified;
								fentry->textstatus = svn_wc_status_modified;
								fentry->isConflicted = false;
							}
						}
						Show(m_dwShow, CTSVNPathList(), 0, m_bShowFolders, m_bShowFiles);
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

					ProjectProperties props;
					props.ReadPropsPathList(itemsToAdd);
					if (svn.Add(itemsToAdd, &props, svn_depth_empty, true, true, true))
					{
						// The add went ok, but we now need to run through the selected items again
						// and update their status
						POSITION pos = GetFirstSelectedItemPosition();
						int index;
						while ((index = GetNextSelectedItem(pos)) >= 0)
						{
							FileEntry * e = GetListEntry(index);
							e->textstatus = svn_wc_status_added;
							e->propstatus = svn_wc_status_none;
							e->status = svn_wc_status_added;
							SetEntryCheck(e,index,true);
						}
					}
					else
					{
						CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
					SaveColumnWidths();
					Show(m_dwShow, CTSVNPathList(), 0, m_bShowFolders, m_bShowFiles);
					NotifyCheck();
				}
				break;
			case IDSVNLC_ADD_RECURSIVE:
				{
					CTSVNPathList itemsToAdd;
					FillListOfSelectedItemPaths(itemsToAdd);

					CAddDlg dlg;
					dlg.m_pathList = itemsToAdd;
					if (dlg.DoModal() == IDOK)
					{
						if (dlg.m_pathList.GetCount() == 0)
							break;
						CSVNProgressDlg progDlg;
						progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Add);
						progDlg.SetPathList(dlg.m_pathList);
						ProjectProperties props;
						props.ReadPropsPathList(dlg.m_pathList);
						progDlg.SetProjectProperties(props);
						progDlg.SetItemCount(dlg.m_pathList.GetCount());
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
			case IDSVNLC_LOCK:
				{
					CTSVNPathList itemsToLock;
					FillListOfSelectedItemPaths(itemsToLock);
					CInputDlg inpDlg;
					inpDlg.m_sTitle.LoadString(IDS_MENU_LOCK);
					CStringUtils::RemoveAccelerators(inpDlg.m_sTitle);
					inpDlg.m_sHintText.LoadString(IDS_LOCK_MESSAGEHINT);
					inpDlg.m_sCheckText.LoadString(IDS_LOCK_STEALCHECK);
					ProjectProperties props;
					props.ReadPropsPathList(itemsToLock);
					props.nMinLogSize = 0;		// the lock message is optional, so no minimum!
					inpDlg.m_pProjectProperties = &props;
					if (inpDlg.DoModal()==IDOK)
					{
						CSVNProgressDlg progDlg;
						progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Lock);
						progDlg.SetOptions(inpDlg.m_iCheck ? ProgOptForce : ProgOptNone);
						progDlg.SetPathList(itemsToLock);
						progDlg.SetCommitMessage(inpDlg.m_sInputText);
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
					progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Unlock);
					progDlg.SetOptions(bForce ? ProgOptForce : ProgOptNone);
					progDlg.SetPathList(itemsToUnlock);
					progDlg.DoModal();
					// refresh!
					CWnd* pParent = GetParent();
					if (NULL != pParent && NULL != pParent->GetSafeHwnd())
					{
						pParent->SendMessage(SVNSLNM_NEEDSREFRESH);
					}
				}
				break;
			case IDSVNLC_REPAIRMOVE:
				{
					POSITION pos = GetFirstSelectedItemPosition();
					int index = GetNextSelectedItem(pos);
					FileEntry * entry1 = NULL;
					FileEntry * entry2 = NULL;
					if (index >= 0)
					{
						entry1 = GetListEntry(index);
						svn_wc_status_kind status1 = svn_wc_status_none;
						svn_wc_status_kind status2 = svn_wc_status_none;
						if (entry1)
						{
							status1 = entry1->status;
							index = GetNextSelectedItem(pos);
							if (index >= 0)
							{
								entry2 = GetListEntry(index);
								if (entry2)
								{
									status2 = entry2->status;
									if (status2 == svn_wc_status_missing && status1 == svn_wc_status_unversioned)
									{
										FileEntry * tempentry = entry1;
										entry1 = entry2;
										entry2 = tempentry;
									}
									// entry1 was renamed to entry2 but outside of Subversion
									// fix this by moving entry2 back to entry1 first,
									// then do an svn-move from entry1 to entry2
									if (MoveFile(entry2->GetPath().GetWinPath(), entry1->GetPath().GetWinPath()))
									{
										SVN svn;
										// now make sure that the target folder is versioned
										CTSVNPath parentPath = entry2->GetPath().GetContainingDirectory();
										FileEntry * parentEntry = GetListEntry(parentPath);
										while (parentEntry && parentEntry->inunversionedfolder)
										{
											parentPath = parentPath.GetContainingDirectory();
											parentEntry = GetListEntry(parentPath);
										}
										if (!parentPath.IsEquivalentTo(entry2->GetPath().GetContainingDirectory()))
										{
											ProjectProperties props;
											props.ReadPropsPathList(CTSVNPathList(entry1->GetPath()));

											svn.Add(CTSVNPathList(entry2->GetPath().GetContainingDirectory()), &props, svn_depth_empty, true, false, true);
										}
										if (!svn.Move(CTSVNPathList(entry1->GetPath()), entry2->GetPath(), true))
										{
											MoveFile(entry1->GetPath().GetWinPath(), entry2->GetPath().GetWinPath());
											CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
										}
										else
										{
											// check the previously unversioned item
											entry1->checked = true;
											// fixing the move was successful. We have to adjust the new status of the
											// files.
											// Since we don't know if the moved/renamed file had local modifications or not,
											// we can't guess the new status. That means we have to refresh...
											CWnd* pParent = GetParent();
											if (NULL != pParent && NULL != pParent->GetSafeHwnd())
											{
												pParent->SendMessage(SVNSLNM_NEEDSREFRESH);
											}
										}
									}
									else
									{
										ShowErrorMessage();
									}
								}
							}
						}
					}
				}
				break;
			case IDSVNLC_REPAIRCOPY:
				{
					POSITION pos = GetFirstSelectedItemPosition();
					int index = GetNextSelectedItem(pos);
					FileEntry * entry1 = NULL;
					FileEntry * entry2 = NULL;
					if (index >= 0)
					{
						entry1 = GetListEntry(index);
						svn_wc_status_kind status1 = svn_wc_status_none;
						svn_wc_status_kind status2 = svn_wc_status_none;
						if (entry1)
						{
							status1 = entry1->status;
							index = GetNextSelectedItem(pos);
							if (index >= 0)
							{
								entry2 = GetListEntry(index);
								if (entry2)
								{
									status2 = entry2->status;
									if (status2 != svn_wc_status_unversioned && status2 != svn_wc_status_none)
									{
										FileEntry * tempentry = entry1;
										entry1 = entry2;
										entry2 = tempentry;
									}
									// entry2 was copied from entry1 but outside of Subversion
									// fix this by moving entry2 out of the way, copy entry1 to entry2
									// and finally move entry2 back over the copy of entry1, overwriting
									// it.
									CString tempfile = entry2->GetPath().GetWinPathString() + _T(".tsvntemp");
									if (MoveFile(entry2->GetPath().GetWinPath(), tempfile))
									{
										SVN svn;
										// now make sure that the target folder is versioned
										CTSVNPath parentPath = entry2->GetPath().GetContainingDirectory();
										FileEntry * parentEntry = GetListEntry(parentPath);
										while (parentEntry && parentEntry->inunversionedfolder)
										{
											parentPath = parentPath.GetContainingDirectory();
											parentEntry = GetListEntry(parentPath);
										}
										if (!parentPath.IsEquivalentTo(entry2->GetPath().GetContainingDirectory()))
										{
											ProjectProperties props;
											props.ReadPropsPathList(CTSVNPathList(entry1->GetPath()));

											svn.Add(CTSVNPathList(entry2->GetPath().GetContainingDirectory()), &props, svn_depth_empty, true, false, true);
										}
										if (!svn.Copy(CTSVNPathList(entry1->GetPath()), entry2->GetPath(), SVNRev(), SVNRev()))
										{
											MoveFile(tempfile, entry2->GetPath().GetWinPath());
											CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
										}
										else
										{
											if (MoveFileEx(tempfile, entry2->GetPath().GetWinPath(), MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING))
											{
												// check the previously unversioned item
												entry1->checked = true;
												// fixing the move was successful. We have to adjust the new status of the
												// files.
												// Since we don't know if the moved/renamed file had local modifications or not,
												// we can't guess the new status. That means we have to refresh...
												CWnd* pParent = GetParent();
												if (NULL != pParent && NULL != pParent->GetSafeHwnd())
												{
													pParent->SendMessage(SVNSLNM_NEEDSREFRESH);
												}
											}
											else
											{
												ShowErrorMessage();
											}
										}
									}
									else
									{
										ShowErrorMessage();
									}
								}
							}
						}
					}
				}
				break;
			case IDSVNLC_REMOVEFROMCS:
				{
					CTSVNPathList changelistItems;
					FillListOfSelectedItemPaths(changelistItems);
					SVN svn;
					SetRedraw(FALSE);
					if (svn.RemoveFromChangeList(changelistItems, CStringArray(), svn_depth_empty))
					{
						// The changelists were removed, but we now need to run through the selected items again
						// and update their changelist
						POSITION pos = GetFirstSelectedItemPosition();
						int index;
						std::vector<int> entriesToRemove;
						while ((index = GetNextSelectedItem(pos)) >= 0)
						{
							FileEntry * e = GetListEntry(index);
							if (e)
							{
								e->changelist.Empty();
								if (e->status == svn_wc_status_normal)
								{
									// remove the entry completely
									entriesToRemove.push_back(index);
								}
								else 
								{
									SetItemGroup(index, 0);
								}
							}
						}
						for (std::vector<int>::reverse_iterator it = entriesToRemove.rbegin(); it != entriesToRemove.rend(); ++it)
						{
							RemoveListEntry(*it);
						}
						// TODO: Should we go through all entries here and check if we also could
						// remove the changelist from m_changelists ?

						Sort();
					}
					else
					{
						CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
					SetRedraw(TRUE);
				}
				break;
			case IDSVNLC_CREATEIGNORECS:
				CreateChangeList(SVNSLC_IGNORECHANGELIST);
				break;
			case IDSVNLC_CREATECS:
				{
					CCreateChangelistDlg dlg;
					if (dlg.DoModal() == IDOK)
					{
						CreateChangeList(dlg.m_sName);
					}
				}
				break;
			default:
				{
					if (cmd < IDSVNLC_MOVETOCS)
						break;
					CTSVNPathList changelistItems;
					FillListOfSelectedItemPaths(changelistItems);

					// find the changelist name
					CString sChangelist;
					int cmdID = IDSVNLC_MOVETOCS;
					SetRedraw(FALSE);
					for (std::map<CString, int>::const_iterator it = m_changelists.begin(); it != m_changelists.end(); ++it)
					{
						if ((it->first.Compare(SVNSLC_IGNORECHANGELIST))&&(entry->changelist.Compare(it->first)))
						{
							if (cmd == cmdID)
							{
								sChangelist = it->first;
							}
							cmdID++;
						}
					}
					if (!sChangelist.IsEmpty())
					{
						SVN svn;
						if (svn.AddToChangeList(changelistItems, sChangelist, svn_depth_empty))
						{
							// The changelists were moved, but we now need to run through the selected items again
							// and update their changelist
							POSITION pos = GetFirstSelectedItemPosition();
							int index;
							while ((index = GetNextSelectedItem(pos)) >= 0)
							{
								FileEntry * e = GetListEntry(index);
								e->changelist = sChangelist;
								if (!e->IsFolder())
								{
									if (m_changelists.find(e->changelist)!=m_changelists.end())
										SetItemGroup(index, m_changelists[e->changelist]);
									else
										SetItemGroup(index, 0);
								}
							}
							Sort();
						}
						else
						{
							CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
					}
					SetRedraw(TRUE);
				}
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
			if (iChangelistCountBeforeMenuCmd != m_changelists.size())
			{
				CWnd* pParent = GetParent();
				if (NULL != pParent && NULL != pParent->GetSafeHwnd())
				{
					pParent->SendMessage(SVNSLNM_CHANGELISTCHANGED, (WPARAM)m_changelists.size());
				}
			}
		} // if (popup.CreatePopupMenu())
	} // if (selIndex >= 0)
}

void CSVNStatusListCtrl::OnContextMenuHeader(CWnd * pWnd, CPoint point)
{
	CHeaderCtrl * pHeaderCtrl = (CHeaderCtrl *)pWnd;
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		pHeaderCtrl->GetItemRect(0, &rect);
		ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	Locker lock(m_critSec);
	CMenu popup;
	if (!popup.CreatePopupMenu())
		return;

	const int columnCount = m_ColumnManager.GetColumnCount();

	CString temp;
	const UINT uCheckedFlags = MF_STRING | MF_ENABLED | MF_CHECKED;
	const UINT uUnCheckedFlags = MF_STRING | MF_ENABLED;

	// build control menu

	temp.LoadString(IDS_STATUSLIST_SHOWGROUPS);
	popup.AppendMenu(IsGroupViewEnabled() ? uCheckedFlags : uUnCheckedFlags, columnCount, temp);

	if (m_ColumnManager.AnyUnusedProperties())
	{
		temp.LoadString(IDS_STATUSLIST_REMOVEUNUSEDPROPS);
		popup.AppendMenu(uUnCheckedFlags, columnCount+1, temp);
	}

	temp.LoadString(IDS_STATUSLIST_RESETCOLUMNORDER);
	popup.AppendMenu(uUnCheckedFlags, columnCount+2, temp);
	popup.AppendMenu(MF_SEPARATOR);

	// standard columns
	for (int i = 1; i < SVNSLC_NUMCOLUMNS; ++i)
	{
		popup.AppendMenu ( m_ColumnManager.IsVisible(i) 
			? uCheckedFlags 
			: uUnCheckedFlags
			, i
			, m_ColumnManager.GetName(i));
	}

	// user-prop columns:
	// find relevant ones and sort 'em

	std::map<CString, int> sortedProps;
	for (int i = SVNSLC_NUMCOLUMNS; i < columnCount; ++i)
		if (m_ColumnManager.IsRelevant(i))
			sortedProps[m_ColumnManager.GetName(i)] = i;

	if (!sortedProps.empty())
	{
		// add 'em to the menu

		popup.AppendMenu(MF_SEPARATOR);

		typedef std::map<CString, int>::const_iterator CIT;
		for ( CIT iter = sortedProps.begin(), end = sortedProps.end()
			; iter != end
			; ++iter)
		{
			popup.AppendMenu ( m_ColumnManager.IsVisible(iter->second) 
				? uCheckedFlags 
				: uUnCheckedFlags
				, iter->second
				, iter->first);
		}
	}

	// show menu & let user pick an entry

	int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
	if ((cmd >= 1)&&(cmd < columnCount))
	{
		m_ColumnManager.SetVisible (cmd, !m_ColumnManager.IsVisible(cmd));
	} 
	else if (cmd == columnCount)
	{
		EnableGroupView(!IsGroupViewEnabled());
	} 
	else if (cmd == columnCount+1)
	{
		m_ColumnManager.RemoveUnusedProps();
	} 
	else if (cmd == columnCount+2)
	{
		m_ColumnManager.ResetColumns (m_dwDefaultColumns);
	}
}

void CSVNStatusListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (pWnd == this)
	{
		OnContextMenuList(pWnd, point);
	} // if (pWnd == this)
	else if (pWnd == GetHeaderCtrl())
	{
		OnContextMenuHeader(pWnd, point);
	}
}

void CSVNStatusListCtrl::CreateChangeList(const CString& name)
{
	CTSVNPathList changelistItems;
	FillListOfSelectedItemPaths(changelistItems);
	SVN svn;
	if (svn.AddToChangeList(changelistItems, name, svn_depth_empty))
	{
		TCHAR groupname[1024];
		LVGROUP grp = {0};
		grp.cbSize = sizeof(LVGROUP);
		grp.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
		_tcsncpy_s(groupname, 1024, name, 1023);
		grp.pszHeader = groupname;
		grp.iGroupId = (int)m_changelists.size();
		grp.uAlign = LVGA_HEADER_LEFT;
		m_changelists[name] = InsertGroup(-1, &grp);

		PrepareGroups(true);

		POSITION pos = GetFirstSelectedItemPosition();
		int index;
		while ((index = GetNextSelectedItem(pos)) >= 0)
		{
			FileEntry * e = GetListEntry(index);
			e->changelist = name;
			SetEntryCheck(e, index, FALSE);
		}

		for (index = 0; index < GetItemCount(); ++index)
		{
			FileEntry * e = GetListEntry(index);
			if (m_changelists.find(e->changelist)!=m_changelists.end())
				SetItemGroup(index, m_changelists[e->changelist]);
			else
				SetItemGroup(index, 0);
		}
	}
	else
	{
		CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
	}
}

void CSVNStatusListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	Locker lock(m_critSec);
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (m_bBlock)
		return;

	UINT hitFlags = 0;
	HitTest(pNMLV->ptAction, &hitFlags);
	if (hitFlags & LVHT_ONITEMSTATEICON)
		return;

	if (pNMLV->iItem < 0)
	{
		if (!IsGroupViewEnabled())
			return;
		POINT pt;
		DWORD ptW = GetMessagePos();
		pt.x = GET_X_LPARAM(ptW);
		pt.y = GET_Y_LPARAM(ptW);
		ScreenToClient(&pt);
		int group = GetGroupFromPoint(&pt);
		if (group < 0)
			return;
		// check/uncheck the whole group depending on the check-state 
		// of the first item in the group
		m_bBlock = true;
		bool bCheck = false;
		bool bFirst = false;
		LVITEM lv;
		for (int i=0; i<GetItemCount(); ++i)
		{
			SecureZeroMemory(&lv, sizeof(LVITEM));
			lv.mask = LVIF_GROUPID;
			lv.iItem = i;
			GetItem(&lv);
			if (lv.iGroupId == group)
			{
				FileEntry * entry = GetListEntry(i);
				if (!bFirst)
				{
					bCheck = !GetCheck(i);
					bFirst = true;
				}
				if (entry)
				{
					bool bOldCheck = !!GetCheck(i);
					SetEntryCheck(entry, i, bCheck);
					if (bCheck != bOldCheck)
					{
						if (bCheck)
							m_nSelected++;
						else
							m_nSelected--;
					}
				}
			}
		}
		GetStatisticsString();
		m_bBlock = false;
		NotifyCheck();
		return;
	}
	FileEntry * entry = GetListEntry(pNMLV->iItem);
	if (entry)
	{
		if (entry->isConflicted)
		{
			CString sCmd;
			sCmd.Format(_T("\"%s\" /command:conflicteditor /path:\"%s\""),
				(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), entry->GetPath().GetWinPath());
			if (!entry->GetPath().IsUrl())
			{
				sCmd += _T(" /propspath:\"");
				sCmd += entry->GetPath().GetWinPathString();
				sCmd += _T("\"");
			}	
			CAppUtils::LaunchApplication(sCmd, NULL, false);
		}
		else
		{
			StartDiff(pNMLV->iItem);
		}
	}
}

void CSVNStatusListCtrl::StartDiff(int fileindex)
{
	if (fileindex < 0)
		return;
	FileEntry * entry = GetListEntry(fileindex);
	ASSERT(entry != NULL);
	if (entry == NULL)
		return;
	if (((entry->status == svn_wc_status_normal)&&(entry->remotestatus <= svn_wc_status_normal))||
		(entry->status == svn_wc_status_unversioned)||(entry->status == svn_wc_status_none))
	{
		CTSVNPath filePath = entry->path;
		if (!filePath.Exists())
		{
			// get the remote file
			filePath = CTempFiles::Instance().GetTempFilePath(false, filePath, SVNRev::REV_HEAD);

			SVN svn;
			CProgressDlg progDlg;
			progDlg.SetTitle(IDS_APPNAME);
			progDlg.SetAnimation(IDR_DOWNLOAD);
			progDlg.SetTime(false);
			svn.SetAndClearProgressInfo(&progDlg, true);	// activate progress bar
			progDlg.ShowModeless(m_hWnd);
			progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)filePath.GetUIPathString());
			if (!svn.Cat(CTSVNPath(entry->GetURL()), SVNRev(SVNRev::REV_HEAD), SVNRev::REV_HEAD, filePath))
			{
				progDlg.Stop();
				svn.SetAndClearProgressInfo((HWND)NULL);
				CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return;
			}
			progDlg.Stop();
			svn.SetAndClearProgressInfo((HWND)NULL);
			SetFileAttributes(filePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
		}
		// open the diff tool
		CString name = filePath.GetUIFileOrDirectoryName();
		CString n1, n2;
		n1.Format(IDS_DIFF_BASENAME, (LPCTSTR)name);
		n2.Format(IDS_DIFF_WCNAME, (LPCTSTR)name);
		CAppUtils::StartExtDiff(filePath, filePath, n1, n2, CAppUtils::DiffFlags().AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000)), 0);
		return;
	}

	SVNDiff diff(NULL, m_hWnd, true);
	diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
	diff.DiffWCFile(
		entry->path, entry->textstatus, entry->propstatus,
		entry->remotetextstatus, entry->remotepropstatus);
}

CString CSVNStatusListCtrl::GetStatisticsString()
{
	CString sNormal, sAdded, sDeleted, sModified, sConflicted, sUnversioned;
	WORD langID = (WORD)(DWORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
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
	sStats.FormatMessage(IDS_COMMITDLG_STATISTICSFORMAT, m_nSelected, GetItemCount());
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

	if (m_pConfirmButton)
	{
		m_pConfirmButton->EnableWindow(m_nSelected>0);
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

CTSVNPath CSVNStatusListCtrl::GetCommonURL(bool bStrict)
{
	if (!bStrict)
	{
		// not strict means that the selected folder has priority
		if (!m_StatusUrlList.GetCommonDirectory().IsEmpty())
			return m_StatusUrlList.GetCommonDirectory();
	}

	CTSVNPathList list;
	int nListItems = GetItemCount();
	for (int i=0; i<nListItems; ++i)
	{
		const FileEntry * entry = GetListEntry(i);
		if (!entry->IsChecked())
			continue;
		const CTSVNPath& baseURL = CTSVNPath(entry->GetURL());
		if (baseURL.IsEmpty())
			continue;			// item has no url
		list.AddPath(baseURL);
	}
	return list.GetCommonRoot();
}

void CSVNStatusListCtrl::SelectAll(bool bSelect, bool bIncludeNoCommits)
{
	CWaitCursor waitCursor;
	// block here so the LVN_ITEMCHANGED messages
	// get ignored
	m_bBlock = TRUE;
	SetRedraw(FALSE);

	int nListItems = GetItemCount();
	if (bSelect)
		m_nSelected = nListItems;
	else
		m_nSelected = 0;
	for (int i=0; i<nListItems; ++i)
	{
		FileEntry * entry = GetListEntry(i);
		ASSERT(entry != NULL);
		if (entry == NULL)
			continue;
		if ((bIncludeNoCommits)||(entry->GetChangeList().Compare(SVNSLC_IGNORECHANGELIST)))
			SetEntryCheck(entry,i,bSelect);
	}
	// unblock before redrawing
	m_bBlock = FALSE;
	SetRedraw(TRUE);
	GetStatisticsString();
	NotifyCheck();
}

void CSVNStatusListCtrl::Check(DWORD dwCheck, bool uncheckNonMatches)
{
	CWaitCursor waitCursor;
	// block here so the LVN_ITEMCHANGED messages
	// get ignored
	m_bBlock = TRUE;
	SetRedraw(FALSE);

	int nListItems = GetItemCount();
	m_nSelected = 0;

	for (int i=0; i<nListItems; ++i)
	{
		FileEntry * entry = GetListEntry(i);
		ASSERT(entry != NULL);
		if (entry == NULL)
			continue;

		DWORD showFlags = GetShowFlagsFromFileEntry(entry);

		if ((showFlags & dwCheck)&&(entry->GetChangeList().Compare(SVNSLC_IGNORECHANGELIST)))
		{
			if ((m_dwShow & SVNSLC_SHOWEXTDISABLED) && (entry->IsFromDifferentRepository() || entry->IsNested()))
				continue;

			SetEntryCheck(entry, i, true);
			m_nSelected++;
		}
		else if (uncheckNonMatches)
			SetEntryCheck(entry, i, false);
	}
	// unblock before redrawing
	m_bBlock = FALSE;
	SetRedraw(TRUE);
	GetStatisticsString();
	NotifyCheck();
}

void CSVNStatusListCtrl::OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	*pResult = 0;
	if (m_bBlock)
		return;
	if (GetListEntry(pGetInfoTip->iItem >= 0))
		if (pGetInfoTip->cchTextMax > GetListEntry(pGetInfoTip->iItem)->path.GetSVNPathString().GetLength())
		{
			if (GetListEntry(pGetInfoTip->iItem)->GetRelativeSVNPath().Compare(GetListEntry(pGetInfoTip->iItem)->path.GetSVNPathString())!= 0)
				_tcsncpy_s(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, GetListEntry(pGetInfoTip->iItem)->path.GetSVNPathString(), pGetInfoTip->cchTextMax);
			else if (GetStringWidth(GetListEntry(pGetInfoTip->iItem)->path.GetSVNPathString()) > GetColumnWidth(pGetInfoTip->iItem))
				_tcsncpy_s(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, GetListEntry(pGetInfoTip->iItem)->path.GetSVNPathString(), pGetInfoTip->cchTextMax);
		}
}

void CSVNStatusListCtrl::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		{
			// This is the prepaint stage for an item. Here's where we set the
			// item's text color. Our return value will tell Windows to draw the
			// item itself, but it will use the new color we set here.

			// Tell Windows to paint the control itself.
			*pResult = CDRF_DODEFAULT;
			if (m_bBlock)
				return;

			COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

			if (m_arListArray.size() > (DWORD_PTR)pLVCD->nmcd.dwItemSpec)
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
						crText = m_Colors.GetColor(CColors::Conflict);
					else
						crText = m_Colors.GetColor(CColors::Added);
					break;
				case svn_wc_status_missing:
				case svn_wc_status_deleted:
				case svn_wc_status_replaced:
					crText = m_Colors.GetColor(CColors::Deleted);
					break;
				case svn_wc_status_modified:
					if (entry->remotestatus == svn_wc_status_modified)
						// indicate a merge (both local and remote changes will require a merge)
						crText = m_Colors.GetColor(CColors::Merged);
					else if (entry->remotestatus == svn_wc_status_deleted)
						// locally modified, but already deleted in the repository
						crText = m_Colors.GetColor(CColors::Conflict);
					else if (entry->textstatus == svn_wc_status_missing)
						crText = m_Colors.GetColor(CColors::Deleted);
					else
						crText = m_Colors.GetColor(CColors::Modified);
					break;
				case svn_wc_status_merged:
					crText = m_Colors.GetColor(CColors::Merged);
					break;
				case svn_wc_status_conflicted:
				case svn_wc_status_obstructed:
					crText = m_Colors.GetColor(CColors::Conflict);
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

				if ((entry->isConflicted) || (entry->tree_conflicted))
					crText = m_Colors.GetColor(CColors::Conflict);

				if ((m_dwShow & SVNSLC_SHOWEXTDISABLED)&&(entry->IsFromDifferentRepository() || entry->IsNested()))
				{
					crText = GetSysColor(COLOR_GRAYTEXT);
				}

				// Store the color back in the NMLVCUSTOMDRAW struct.
				pLVCD->clrText = crText;
			}
		}
		break;
	}
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
	Locker lock(m_critSec);
	DeleteItem(index);
	delete m_arStatusArray[m_arListArray[index]];
	m_arStatusArray.erase(m_arStatusArray.begin()+m_arListArray[index]);
	m_arListArray.erase(m_arListArray.begin()+index);
	for (int i=index; i< (int)m_arListArray.size(); ++i)
	{
		m_arListArray[i]--;
	}
}

// Set a checkbox on an entry in the listbox
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
void CSVNStatusListCtrl::FillListOfSelectedItemPaths(CTSVNPathList& pathList, bool bNoIgnored)
{
	pathList.Clear();

	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		FileEntry * entry = GetListEntry(index);
		if ((bNoIgnored)&&(entry->status == svn_wc_status_ignored))
			continue;
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
	POSITION pos = GetFirstSelectedItemPosition();
	while ( pos )
	{
		int index = GetNextSelectedItem(pos);
		FileEntry * entry = GetListEntry(index);
		if (!entry)
			continue;
		if (entry->isConflicted)
		{
			CString sCmd;
			sCmd.Format(_T("\"%s\" /command:conflicteditor /path:\"%s\""),
				(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), entry->GetPath().GetWinPath());
			if (!entry->GetPath().IsUrl())
			{
				sCmd += _T(" /propspath:\"");
				sCmd += entry->GetPath().GetWinPathString();
				sCmd += _T("\"");
			}	
			CAppUtils::LaunchApplication(sCmd, NULL, false);
		}
		else
		{
			StartDiff(index);
		}
	}
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
		::PostMessage(GetParent()->GetSafeHwnd(), WM_NEXTDLGCTL, GetKeyState(VK_SHIFT)&0x8000, 0);
		return;
	case (VK_ESCAPE):
		::SendMessage(GetParent()->GetSafeHwnd(), WM_CLOSE, 0, 0);
		break;
	}

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSVNStatusListCtrl::PreSubclassWindow()
{
	CListCtrl::PreSubclassWindow();
	EnableToolTips(TRUE);
}

INT_PTR CSVNStatusListCtrl::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	int row, col;
	RECT cellrect;
	row = CellRectFromPoint(point, &cellrect, &col );

	if (row == -1)
		return -1;

	pTI->hwnd = m_hWnd;
	pTI->uId = (UINT)((row<<10)+(col&0x3ff)+1);
	pTI->lpszText = LPSTR_TEXTCALLBACK;

	pTI->rect = cellrect;

	return pTI->uId;
}

int CSVNStatusListCtrl::CellRectFromPoint(CPoint& point, RECT *cellrect, int *col) const
{
	int colnum;

	// Make sure that the ListView is in LVS_REPORT
	if ((GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT)
		return -1;

	// Get the top and bottom row visible
	int row = GetTopIndex();
	int bottom = row + GetCountPerPage();
	if (bottom > GetItemCount())
		bottom = GetItemCount();

	// Get the number of columns
	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();

	// Loop through the visible rows
	for ( ;row <=bottom;row++)
	{
		// Get bounding rect of item and check whether point falls in it.
		CRect rect;
		GetItemRect(row, &rect, LVIR_BOUNDS);
		if (!rect.PtInRect(point))
			continue;
		// Now find the column
		for (colnum = 0; colnum < nColumnCount; colnum++)
		{
			int colwidth = GetColumnWidth(colnum);
			if (point.x >= rect.left && point.x <= (rect.left + colwidth))
			{
				RECT rectClient;
				GetClientRect(&rectClient);
				if (col)
					*col = colnum;
				rect.right = rect.left + colwidth;

				// Make sure that the right extent does not exceed
				// the client area
				if (rect.right > rectClient.right)
					rect.right = rectClient.right;
				*cellrect = rect;
				return row;
			}
			rect.left += colwidth;
		}
	}
	return -1;
}

BOOL CSVNStatusListCtrl::OnToolTipText(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText;
	UINT_PTR nID = pNMHDR->idFrom;

	if (nID == 0)
		return FALSE;

	UINT_PTR row = ((nID-1) >> 10) & 0x3fffff;
	UINT_PTR col = (nID-1) & 0x3ff;

	if (col == 0)
		return FALSE;	// no custom tooltip for the path, we use the infotip there!

	// get the internal column from the visible columns
	int internalcol = 0;
	UINT_PTR currentcol = 0;
    for (;    (currentcol != col) 
           && (internalcol < m_ColumnManager.GetColumnCount()-1)
         ; ++internalcol)
	{
        if (m_ColumnManager.IsVisible (internalcol))
			currentcol++;
	}

	AFX_MODULE_THREAD_STATE* pModuleThreadState = AfxGetModuleThreadState();
	CToolTipCtrl* pToolTip = pModuleThreadState->m_pToolTip;
	pToolTip->SendMessage(TTM_SETMAXTIPWIDTH, 0, 300);

	*pResult = 0;
	if ((internalcol == 2)||(internalcol == 4))
	{
		FileEntry *fentry = GetListEntry(row);
		if (fentry)
		{
			if (fentry->copied)
			{
				CString url;
				url.FormatMessage(IDS_STATUSLIST_COPYFROM, (LPCTSTR)CPathUtils::PathUnescape(fentry->copyfrom_url), (LONG)fentry->copyfrom_rev);
				lstrcpyn(pTTTW->szText, (LPCTSTR)url, 80);
				return TRUE;
			}
			if (fentry->switched)
			{
				CString url;
				url.Format(IDS_STATUSLIST_SWITCHEDTO, (LPCTSTR)CPathUtils::PathUnescape(fentry->url));
				lstrcpyn(pTTTW->szText, (LPCTSTR)url, 80);
				return TRUE;
			}
			if (fentry->keeplocal)
			{
				lstrcpyn(pTTTW->szText, (LPCTSTR)CString(MAKEINTRESOURCE(IDS_STATUSLIST_KEEPLOCAL)), 80);
				return TRUE;
			}
		}
	}

	return FALSE;
}

void CSVNStatusListCtrl::OnPaint()
{
	Default();
	if ((m_bBusy)||(m_bEmpty))
	{
		CString str;
		if (m_bBusy)
		{
			if (m_sBusy.IsEmpty())
				str.LoadString(IDS_STATUSLIST_BUSYMSG);
			else
				str = m_sBusy;
		}
		else
		{
			if (m_sEmpty.IsEmpty())
				str.LoadString(IDS_STATUSLIST_EMPTYMSG);
			else
				str = m_sEmpty;
		}
		COLORREF clrText = ::GetSysColor(COLOR_WINDOWTEXT);
		COLORREF clrTextBk;
		if (IsWindowEnabled())
			clrTextBk = ::GetSysColor(COLOR_WINDOW);
		else
			clrTextBk = ::GetSysColor(COLOR_3DFACE);

		CRect rc;
		GetClientRect(&rc);
		CHeaderCtrl* pHC;
		pHC = GetHeaderCtrl();
		if (pHC != NULL)
		{
			CRect rcH;
			pHC->GetItemRect(0, &rcH);
			rc.top += rcH.bottom;
		}
		CDC* pDC = GetDC();
		{
			CMyMemDC memDC(pDC, &rc);

			memDC.SetTextColor(clrText);
			memDC.SetBkColor(clrTextBk);
			memDC.FillSolidRect(rc, clrTextBk);
			rc.top += 10;
			CGdiObject * oldfont = memDC.SelectStockObject(DEFAULT_GUI_FONT);
			memDC.DrawText(str, rc, DT_CENTER | DT_VCENTER |
				DT_WORDBREAK | DT_NOPREFIX | DT_NOCLIP);
			memDC.SelectObject(oldfont);
		}
		ReleaseDC(pDC);
	}
}

// prevent users from extending our hidden (size 0) columns
void CSVNStatusListCtrl::OnHdnBegintrack(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;
	if ((phdr->iItem < 0)||(phdr->iItem >= SVNSLC_NUMCOLUMNS))
		return;

    if (m_ColumnManager.IsVisible (phdr->iItem))
	{
		return;
	}
	*pResult = 1;
}

// prevent any function from extending our hidden (size 0) columns
void CSVNStatusListCtrl::OnHdnItemchanging(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;
    if ((phdr->iItem < 0)||(phdr->iItem >= m_ColumnManager.GetColumnCount()))
	{
		Default();
		return;
	}

    // visible columns may be modified 

    if (m_ColumnManager.IsVisible (phdr->iItem))
	{
		Default();
		return;
	}

    // columns already marked as "invisible" internally may be (re-)sized to 0

    if (   (phdr->pitem != NULL) 
        && (phdr->pitem->mask == HDI_WIDTH)
        && (phdr->pitem->cxy == 0))
	{
		Default();
		return;
	}

	if (   (phdr->pitem != NULL) 
		&& (phdr->pitem->mask != HDI_WIDTH))
	{
		Default();
		return;
	}

    *pResult = 1;
}

void CSVNStatusListCtrl::OnDestroy()
{
	SaveColumnWidths(true);
	CListCtrl::OnDestroy();
}

void CSVNStatusListCtrl::ShowErrorMessage()
{
	CFormatMessageWrapper errorDetails;
	MessageBox(errorDetails, _T("Error"), MB_OK | MB_ICONINFORMATION );
}

void CSVNStatusListCtrl::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	Locker lock(m_critSec);


	CTSVNPathList pathList;
	FillListOfSelectedItemPaths(pathList);
	if (pathList.GetCount() == 0)
		return;

	CIDropSource* pdsrc = new CIDropSource;
	if (pdsrc == NULL)
		return;
	pdsrc->AddRef();


	SVNDataObject* pdobj = new SVNDataObject(pathList, SVNRev::REV_WC, SVNRev::REV_WC);
	if (pdobj == NULL)
	{
		delete pdsrc;
		return;
	}
	pdobj->AddRef();

	CDragSourceHelper dragsrchelper;

	// Something strange is going on here:
	// InitializeFromWindow() crashes bad if group view is enabled.
	// Since I haven't been able to find out *why* it crashes, I'm disabling
	// the group view right before the call to InitializeFromWindow() and
	// re-enable it again after the call is finished.

	SetRedraw(false);
	BOOL bGroupView = IsGroupViewEnabled();
	if (bGroupView)
		EnableGroupView(false);
	dragsrchelper.InitializeFromWindow(m_hWnd, pNMLV->ptAction, pdobj);
	if (bGroupView)
		EnableGroupView(true);
	SetRedraw(true);
	//dragsrchelper.InitializeFromBitmap()
	pdsrc->m_pIDataObj = pdobj;
	pdsrc->m_pIDataObj->AddRef();


	// Initiate the Drag & Drop
	DWORD dwEffect;
	m_bOwnDrag = true;
	::DoDragDrop(pdobj, pdsrc, DROPEFFECT_MOVE|DROPEFFECT_COPY, &dwEffect);
	m_bOwnDrag = false;
	pdsrc->Release();
	pdobj->Release();

	*pResult = 0;
}

void CSVNStatusListCtrl::SaveColumnWidths(bool bSaveToRegistry /* = false */)
{
	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
        if (m_ColumnManager.IsVisible (col))
            m_ColumnManager.ColumnResized (col);

	if (bSaveToRegistry)
        m_ColumnManager.WriteSettings();
}

bool CSVNStatusListCtrl::EnableFileDrop()
{
	m_bFileDropsEnabled = true;
	return true;
}

bool CSVNStatusListCtrl::HasPath(const CTSVNPath& path)
{
	for (size_t i=0; i < m_arStatusArray.size(); i++)
	{
		FileEntry * entry = m_arStatusArray[i];
		if (entry->GetPath().IsEquivalentTo(path))
			return true;
	}
	return false;
}

bool CSVNStatusListCtrl::IsPathShown(const CTSVNPath& path)
{
	int itemCount = GetItemCount();
	for (int i=0; i < itemCount; i++)
	{
		FileEntry * entry = GetListEntry(i);
		if (entry->GetPath().IsEquivalentTo(path))
			return true;
	}
	return false;
}

BOOL CSVNStatusListCtrl::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case 'A':
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					// select all entries
					for (int i=0; i<GetItemCount(); ++i)
					{
						SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
					}
					return TRUE;
				}
			}
			break;
		case 'C':
		case VK_INSERT:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					// copy all selected paths to the clipboard
					if (GetAsyncKeyState(VK_SHIFT)&0x8000)
						CopySelectedEntriesToClipboard(SVNSLC_COLFILEPATH|SVNSLC_COLSTATUS|SVNSLC_COLURL);
					else
						CopySelectedEntriesToClipboard(SVNSLC_COLFILEPATH);
					return TRUE;
				}
			}
			break;
		}
	}

	return CListCtrl::PreTranslateMessage(pMsg);
}

bool CSVNStatusListCtrl::CopySelectedEntriesToClipboard(DWORD dwCols)
{
	static CString ponly(MAKEINTRESOURCE(IDS_STATUSLIST_PROPONLY));
	static HINSTANCE hResourceHandle(AfxGetResourceHandle());
	WORD langID = (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	CString sClipboard;
	CString temp;
	TCHAR buf[100];
	if (GetSelectedCount() == 0)
		return false;

    // first add the column titles as the first line
    DWORD selection = 0;
    for (int i = 0, count = m_ColumnManager.GetColumnCount(); i < count; ++i)
        if (   ((dwCols == -1) && m_ColumnManager.IsVisible (i))
            || ((i < SVNSLC_NUMCOLUMNS) && (dwCols & (1 << i))))
        {
            if (!sClipboard.IsEmpty())
                sClipboard += _T("\t");
                
            sClipboard += m_ColumnManager.GetName(i);

            if (i < 32)
                selection += 1 << i;
        }

	sClipboard += _T("\r\n");

	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		FileEntry * entry = GetListEntry(index);
        if (selection & SVNSLC_COLFILEPATH)
		{
		    sClipboard += entry->GetDisplayName();
        }
		if (selection & SVNSLC_COLFILENAME)
		{
			sClipboard += _T("\t")+entry->path.GetFileOrDirectoryName();
		}
		if (selection & SVNSLC_COLEXT)
		{
			sClipboard += _T("\t")+entry->path.GetFileExtension();
		}
		if (selection & SVNSLC_COLSTATUS)
		{
			if (entry->isNested)
			{
				temp.LoadString(IDS_STATUSLIST_NESTED);
			}
			else
			{
				SVNStatus::GetStatusString(hResourceHandle, entry->status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
				if ((entry->copied)&&(_tcslen(buf)>1))
					_tcscat_s(buf, 100, _T(" (+)"));
				if ((entry->switched)&&(_tcslen(buf)>1))
					_tcscat_s(buf, 100, _T(" (s)"));
				if ((entry->status == entry->propstatus)&&
					(entry->status != svn_wc_status_normal)&&
					(entry->status != svn_wc_status_unversioned)&&
					(!SVNStatus::IsImportant(entry->textstatus)))
					_tcscat_s(buf, 100, ponly);
				temp = buf;
			}
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLREMOTESTATUS)
		{
			if (entry->isNested)
			{
				temp.LoadString(IDS_STATUSLIST_NESTED);
			}
			else
			{
				SVNStatus::GetStatusString(hResourceHandle, entry->remotestatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
				if ((entry->copied)&&(_tcslen(buf)>1))
					_tcscat_s(buf, 100, _T(" (+)"));
				if ((entry->switched)&&(_tcslen(buf)>1))
					_tcscat_s(buf, 100, _T(" (s)"));
				if ((entry->remotestatus == entry->remotepropstatus)&&
					(entry->remotestatus != svn_wc_status_none)&&
					(entry->remotestatus != svn_wc_status_normal)&&
					(entry->remotestatus != svn_wc_status_unversioned)&&
					(!SVNStatus::IsImportant(entry->remotetextstatus)))
					_tcscat_s(buf, 100, ponly);
				temp = buf;
			}
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLTEXTSTATUS)
		{
			if (entry->isNested)
			{
				temp.LoadString(IDS_STATUSLIST_NESTED);
			}
			else
			{
				SVNStatus::GetStatusString(hResourceHandle, entry->textstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
				if ((entry->copied)&&(_tcslen(buf)>1))
					_tcscat_s(buf, 100, _T(" (+)"));
				if ((entry->switched)&&(_tcslen(buf)>1))
					_tcscat_s(buf, 100, _T(" (s)"));
				temp = buf;
			}
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLPROPSTATUS)
		{
			if (entry->isNested)
			{
				temp.Empty();
			}
			else
			{
				SVNStatus::GetStatusString(hResourceHandle, entry->propstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
				if ((entry->copied)&&(_tcslen(buf)>1))
					_tcscat_s(buf, 100, _T(" (+)"));
				if ((entry->switched)&&(_tcslen(buf)>1))
					_tcscat_s(buf, 100, _T(" (s)"));
				temp = buf;
			}
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLREMOTETEXT)
		{
			if (entry->isNested)
			{
				temp.Empty();
			}
			else
			{
				SVNStatus::GetStatusString(hResourceHandle, entry->remotetextstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
				temp = buf;
			}
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLREMOTEPROP)
		{
			// SVNSLC_COLREMOTEPROP
			if (entry->isNested)
			{
				temp.Empty();
			}
			else
			{
				SVNStatus::GetStatusString(hResourceHandle, entry->remotepropstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
				temp = buf;
			}
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLDEPTH)
		{
			temp = SVNStatus::GetDepthString(entry->depth);
		}
		if (selection & SVNSLC_COLURL)
        {
			sClipboard += _T("\t")+entry->url;
        }
		if (selection & SVNSLC_COLLOCK)
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
						temp = entry->lock_remoteowner;
					else
						temp = entry->lock_owner;
				}
				else if (entry->lock_remotetoken.IsEmpty())
				{
					// broken lock
					temp.LoadString(IDS_STATUSLIST_LOCKBROKEN);
				}
				else
				{
					// stolen lock
					temp.Format(IDS_STATUSLIST_LOCKSTOLEN, (LPCTSTR)entry->lock_remoteowner);
				}
			}
			else
				temp = entry->lock_owner;
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLLOCKCOMMENT)
			sClipboard += _T("\t")+entry->lock_comment;
		if (selection & SVNSLC_COLLOCKDATE)
		{
			TCHAR datebuf[SVN_DATE_BUFFER];
            apr_time_t date = entry->lock_date;
			SVN::formatDate(datebuf, date, true);
			if (date)
				temp = datebuf;
			else
				temp.Empty();
			sClipboard += _T("\t")+temp;
		}

        if (selection & SVNSLC_COLAUTHOR)
			sClipboard += _T("\t")+entry->last_commit_author;
		if (selection & SVNSLC_COLREVISION)
		{
			temp.Format(_T("%ld"), entry->last_commit_rev);
			if (entry->last_commit_rev == 0)
				temp.Empty();
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLREMOTEREVISION)
		{
			temp.Format(_T("%ld"), entry->remoterev);
			if (entry->remoterev <= 0)
				temp.Empty();
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLDATE)
		{
			TCHAR datebuf[SVN_DATE_BUFFER];
			apr_time_t date = entry->last_commit_date;
			SVN::formatDate(datebuf, date, true);
			if (date)
				temp = datebuf;
			else
				temp.Empty();
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLSVNNEEDSLOCK)
		{
            sClipboard += entry->needslock ? _T("\tx") : _T("\t");
		}
		if (selection & SVNSLC_COLCOPYFROM)
		{
			if (m_sRepositoryRoot.Compare(entry->copyfrom_url.Left(m_sRepositoryRoot.GetLength()))==0)
				temp = entry->copyfrom_url.Mid(m_sRepositoryRoot.GetLength());
			else
				temp = entry->copyfrom_url;
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLCOPYFROMREV)
		{
			temp.Format(_T("%ld"), entry->copyfrom_rev);
			if (entry->copyfrom_rev == 0)
				temp.Empty();
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLMODIFICATIONDATE)
		{
			TCHAR datebuf[SVN_DATE_BUFFER];
	        __int64 filetime = entry->GetPath().GetLastWriteTime();
	        if ( (filetime) && (entry->textstatus!=svn_wc_status_deleted) )
	        {
		        FILETIME* f = (FILETIME*)(__int64*)&filetime;
		        SVN::formatDate(datebuf,*f,true);
                temp = datebuf;
            }
			else
				temp.Empty();
			sClipboard += _T("\t")+temp;
		}
		if (selection & SVNSLC_COLSIZE)
		{
			if (!entry->IsFolder())
			{
				TCHAR buf[100];
				__int64 filesize = entry->working_size != (-1) ? entry->working_size : entry->GetPath().GetFileSize();
				StrFormatByteSize64(filesize, buf, 100);
				temp = buf;
			}
			else
				temp.Empty();
			sClipboard += _T("\t")+temp;
		}

        for ( int i = SVNSLC_NUMCOLUMNS, count = m_ColumnManager.GetColumnCount()
            ; i < count
            ; ++i)
        {
            if ((dwCols == -1) && m_ColumnManager.IsVisible (i))
            {
                CString value 
                    = entry->present_props[m_ColumnManager.GetName(i)];
                sClipboard += _T("\t") + value;
            }
        }

		sClipboard += _T("\r\n");
	}

	return CStringUtils::WriteAsciiStringToClipboard(sClipboard);
}

size_t CSVNStatusListCtrl::GetNumberOfChangelistsInSelection()
{
	std::set<CString> changelists;
	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		FileEntry * entry = GetListEntry(index);
		if (!entry->changelist.IsEmpty())
			changelists.insert(entry->changelist);
	}
	return changelists.size();
}

bool CSVNStatusListCtrl::PrepareGroups(bool bForce /* = false */)
{
	if (((m_dwContextMenus & SVNSLC_POPCHANGELISTS) == NULL) && m_externalSet.empty())
		return false;	// don't show groups

	bool bHasChangelistGroups = (m_changelists.size() > 0)||(bForce);
	bool bHasGroups = bHasChangelistGroups|| ((m_externalSet.size()>0) && (m_dwShow & SVNSLC_SHOWINEXTERNALS));
	RemoveAllGroups();
	EnableGroupView(bHasGroups);

	TCHAR groupname[1024];

	m_bHasIgnoreGroup = false;

	// add a new group for each changelist
	int groupindex = 0;

	// now add the items which don't belong to a group
	LVGROUP grp;
	SecureZeroMemory(&grp, sizeof(grp));
	grp.cbSize = sizeof(LVGROUP);
	grp.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
	if (bHasChangelistGroups)
	{
		CString sUnassignedName(MAKEINTRESOURCE(IDS_STATUSLIST_UNASSIGNED_CHANGESET));
		_tcsncpy_s(groupname, 1024, (LPCTSTR)sUnassignedName, 1023);
	}
	else
	{
		CString sNoExternalGroup(MAKEINTRESOURCE(IDS_STATUSLIST_NOEXTERNAL_GROUP));
		_tcsncpy_s(groupname, 1024, (LPCTSTR)sNoExternalGroup, 1023);
	}
	grp.pszHeader = groupname;
	grp.iGroupId = groupindex;
	grp.uAlign = LVGA_HEADER_LEFT;
	InsertGroup(groupindex++, &grp);

	m_bExternalsGroups = false;
	if (bHasChangelistGroups)
	{
		for (std::map<CString,int>::iterator it = m_changelists.begin(); it != m_changelists.end(); ++it)
		{
			if (it->first.Compare(SVNSLC_IGNORECHANGELIST)!=0)
			{
				LVGROUP lvgroup = {0};
				lvgroup.cbSize = sizeof(LVGROUP);
				lvgroup.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
				_tcsncpy_s(groupname, 1024, (LPCTSTR)it->first, 1023);
				lvgroup.pszHeader = groupname;
				lvgroup.iGroupId = groupindex;
				lvgroup.uAlign = LVGA_HEADER_LEFT;
				it->second = InsertGroup(groupindex++, &lvgroup);
			}
			else
				m_bHasIgnoreGroup = true;
		}
	}
	else
	{
		for (std::set<CTSVNPath>::iterator it = m_externalSet.begin(); it != m_externalSet.end(); ++it)
		{
			LVGROUP lvgroup = {0};
			lvgroup.cbSize = sizeof(LVGROUP);
			lvgroup.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
			_tcsncpy_s(groupname, 1024, (LPCTSTR)CString(MAKEINTRESOURCE(IDS_STATUSLIST_EXTERNAL_GROUP)), 1023);
			_tcsncat_s(groupname, 1024, _T(" "), 1023);
			_tcsncat_s(groupname, 1024, (LPCTSTR)it->GetFileOrDirectoryName(), 1023);
			lvgroup.pszHeader = groupname;
			lvgroup.iGroupId = groupindex;
			lvgroup.uAlign = LVGA_HEADER_LEFT;
			InsertGroup(groupindex++, &lvgroup);
			m_bExternalsGroups = true;
		}
	}

	if (m_bHasIgnoreGroup)
	{
		// and now add the group 'ignore-on-commit'
		std::map<CString,int>::iterator it = m_changelists.find(SVNSLC_IGNORECHANGELIST);
		if (it != m_changelists.end())
		{
			grp.cbSize = sizeof(LVGROUP);
			grp.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
			_tcsncpy_s(groupname, 1024, SVNSLC_IGNORECHANGELIST, 1023);
			grp.pszHeader = groupname;
			grp.iGroupId = groupindex;
			grp.uAlign = LVGA_HEADER_LEFT;
			it->second = InsertGroup(groupindex, &grp);
		}
	}

	return bHasGroups;
}

void CSVNStatusListCtrl::NotifyCheck()
{
	CWnd* pParent = GetParent();
	if (NULL != pParent && NULL != pParent->GetSafeHwnd())
	{
		pParent->SendMessage(SVNSLNM_CHECKCHANGED, m_nSelected);
	}
}


//////////////////////////////////////////////////////////////////////////

bool CSVNStatusListCtrlDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD * /*pdwEffect*/, POINTL pt)
{
	if(pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
	{
		HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
		if(hDrop != NULL)
		{
			TCHAR szFileName[MAX_PATH];

			UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

			POINT clientpoint;
			clientpoint.x = pt.x;
			clientpoint.y = pt.y;
			ScreenToClient(m_hTargetWnd, &clientpoint);
			if ((m_pSVNStatusListCtrl->IsGroupViewEnabled())&&(m_pSVNStatusListCtrl->GetGroupFromPoint(&clientpoint, false) >= 0))
			{
				CTSVNPathList changelistItems;
				for(UINT i = 0; i < cFiles; ++i)
				{
					DragQueryFile(hDrop, i, szFileName, sizeof(szFileName)/sizeof(TCHAR));
					CTSVNPath itemPath = CTSVNPath(szFileName);
					if (itemPath.Exists())
						changelistItems.AddPath(itemPath);
				}
				// find the changelist name
				CString sChangelist;
				LONG_PTR nGroup = m_pSVNStatusListCtrl->GetGroupFromPoint(&clientpoint, false);
				for (std::map<CString, int>::iterator it = m_pSVNStatusListCtrl->m_changelists.begin(); it != m_pSVNStatusListCtrl->m_changelists.end(); ++it)
					if (it->second == nGroup)
						sChangelist = it->first;
				if (!sChangelist.IsEmpty())
				{
					SVN svn;
					if (svn.AddToChangeList(changelistItems, sChangelist, svn_depth_empty))
					{
						for (int l=0; l<changelistItems.GetCount(); ++l)
						{
							int index = m_pSVNStatusListCtrl->GetIndex(changelistItems[l]);
							if (index >= 0)
							{
								CSVNStatusListCtrl::FileEntry * e = m_pSVNStatusListCtrl->GetListEntry(index);
								if (e)
								{
									e->changelist = sChangelist;
									if (!e->IsFolder())
									{
										if (m_pSVNStatusListCtrl->m_changelists.find(e->changelist)!=m_pSVNStatusListCtrl->m_changelists.end())
											m_pSVNStatusListCtrl->SetItemGroup(index, m_pSVNStatusListCtrl->m_changelists[e->changelist]);
										else
											m_pSVNStatusListCtrl->SetItemGroup(index, 0);
									}
								}
							}
							else
							{
								HWND hParentWnd = GetParent(m_hTargetWnd);
								if (hParentWnd != NULL)
									::SendMessage(hParentWnd, CSVNStatusListCtrl::SVNSLNM_ADDFILE, 0, (LPARAM)changelistItems[l].GetWinPath());
							}
						}
						m_pSVNStatusListCtrl->Sort();
					}
					else
					{
						CMessageBox::Show(m_pSVNStatusListCtrl->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
				}
				else
				{
					SVN svn;
					if (svn.RemoveFromChangeList(changelistItems, CStringArray(), svn_depth_empty))
					{
						for (int l=0; l<changelistItems.GetCount(); ++l)
						{
							int index = m_pSVNStatusListCtrl->GetIndex(changelistItems[l]);
							if (index >= 0)
							{
								CSVNStatusListCtrl::FileEntry * e = m_pSVNStatusListCtrl->GetListEntry(index);
								if (e)
								{
									e->changelist = sChangelist;
									m_pSVNStatusListCtrl->SetItemGroup(index, 0);
								}
							}
							else
							{
								HWND hParentWnd = GetParent(m_hTargetWnd);
								if (hParentWnd != NULL)
									::SendMessage(hParentWnd, CSVNStatusListCtrl::SVNSLNM_ADDFILE, 0, (LPARAM)changelistItems[l].GetWinPath());
							}
						}
						m_pSVNStatusListCtrl->Sort();
					}
					else
					{
						CMessageBox::Show(m_pSVNStatusListCtrl->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
				}
			}
			else
			{
				for(UINT i = 0; i < cFiles; ++i)
				{
					DragQueryFile(hDrop, i, szFileName, sizeof(szFileName)/sizeof(TCHAR));
					HWND hParentWnd = GetParent(m_hTargetWnd);
					if (hParentWnd != NULL)
						::SendMessage(hParentWnd, CSVNStatusListCtrl::SVNSLNM_ADDFILE, 0, (LPARAM)szFileName);
				}
			}
		}
		GlobalUnlock(medium.hGlobal);
	}
	return true; //let base free the medium
}
HRESULT STDMETHODCALLTYPE CSVNStatusListCtrlDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
	*pdwEffect = DROPEFFECT_MOVE;
	CString sDropDesc;
	if (m_pSVNStatusListCtrl)
	{
		POINT clientpoint;
		clientpoint.x = pt.x;
		clientpoint.y = pt.y;
		ScreenToClient(m_hTargetWnd, &clientpoint);
		if (m_pSVNStatusListCtrl->IsGroupViewEnabled())
		{
			if (m_pSVNStatusListCtrl->m_bOwnDrag)
			{
				LONG_PTR iGroup = m_pSVNStatusListCtrl->GetGroupFromPoint(&clientpoint, false);
				if (iGroup >= 0)
				{
					// find the changelist name
					CString sChangelist;
					for (std::map<CString, int>::iterator it = m_pSVNStatusListCtrl->m_changelists.begin(); it != m_pSVNStatusListCtrl->m_changelists.end(); ++it)
						if (it->second == iGroup)
							sChangelist = it->first;

					if (sChangelist.IsEmpty())
					{
						*pdwEffect = DROPEFFECT_MOVE;
						sDropDesc.LoadString(IDS_DROPDESC_MOVE);
						CString sUnassignedName(MAKEINTRESOURCE(IDS_STATUSLIST_UNASSIGNED_CHANGESET));
						SetDropDescription(DROPIMAGE_MOVE, sDropDesc, sUnassignedName);
					}
					else
					{
						*pdwEffect = DROPEFFECT_MOVE;
						sDropDesc.LoadString(IDS_DROPDESC_MOVE);
						SetDropDescription(DROPIMAGE_MOVE, sDropDesc, sChangelist);
					}
				}
				else
				{
					*pdwEffect = DROPEFFECT_MOVE;
					SetDropDescription(DROPIMAGE_NONE, NULL, NULL);
				}
			}
			else
			{
				sDropDesc.LoadString(IDS_DROPDESC_ADD);
				CString sDialog(MAKEINTRESOURCE(IDS_APPNAME));
				SetDropDescription(DROPIMAGE_COPY, sDropDesc, sDialog);
			}
		}
		else if ((!m_pSVNStatusListCtrl->m_bFileDropsEnabled)||(m_pSVNStatusListCtrl->m_bOwnDrag))
		{
			*pdwEffect = DROPEFFECT_MOVE;
			SetDropDescription(DROPIMAGE_NONE, NULL, NULL);
		}
		else
		{
			sDropDesc.LoadString(IDS_DROPDESC_ADD);
			CString sDialog(MAKEINTRESOURCE(IDS_APPNAME));
			SetDropDescription(DROPIMAGE_COPY, sDropDesc, sDialog);
		}
	}
	else
		SetDropDescription(DROPIMAGE_NONE, NULL, NULL);

	return S_OK;
}