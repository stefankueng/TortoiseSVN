// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - Stefan Kueng

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
#include "MemDC.h"
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
#include "LogDlg.h"
#include "SVNProgressDlg.h"
#include "SysImageList.h"
#include "Svnstatuslistctrl.h"
#include "TSVNPath.h"
#include "Registry.h"
#include "SVNStatus.h"
#include "SVNDiff.h"
#include "InputDlg.h"
#include "ShellUpdater.h"
#include "SVNAdminDir.h"
#include ".\svnstatuslistctrl.h"
#include "DropFiles.h"
#include "EditPropertiesDlg.h"
#include "CreateChangelistDlg.h"

const UINT CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED
					= ::RegisterWindowMessage(_T("SVNSLNM_ITEMCOUNTCHANGED"));
const UINT CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH
					= ::RegisterWindowMessage(_T("SVNSLNM_NEEDSREFRESH"));
const UINT CSVNStatusListCtrl::SVNSLNM_ADDFILE
					= ::RegisterWindowMessage(_T("SVNSLNM_ADDFILE"));
const UINT CSVNStatusListCtrl::SVNSLNM_CHECKCHANGED
					= ::RegisterWindowMessage(_T("SVNSLNM_CHECKCHANGED"));

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
#define IDSVNLC_CHECKGROUP		29
#define IDSVNLC_UNCHECKGROUP	30
// the IDSVNLC_MOVETOCS *must* be the last index, because it contains a dynamic submenu where 
// the submenu items get command ID's sequent to this number
#define IDSVNLC_MOVETOCS		31


BEGIN_MESSAGE_MAP(CSVNStatusListCtrl, CListCtrl)
	ON_NOTIFY(HDN_ITEMCLICKA, 0, OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, OnHdnItemclick)
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


bool	CSVNStatusListCtrl::m_bAscending = false;
int		CSVNStatusListCtrl::m_nSortedColumn = -1;

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
	, m_pDropTarget(NULL)
	, m_bIgnoreRemoveOnly(false)
	, m_bCheckChildrenWithParent(false)
	, m_bUnversionedLast(true)
	, m_bHasChangeLists(false)
	, m_bHasLocks(false)
	, m_bBlock(false)
	, m_bBlockUI(false)
{
	ZeroMemory(m_arColumnWidths, sizeof(m_arColumnWidths));
	m_critSec.Init();
}

CSVNStatusListCtrl::~CSVNStatusListCtrl()
{
	if (m_pDropTarget)
		delete m_pDropTarget;
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

void CSVNStatusListCtrl::Init(DWORD dwColumns, const CString& sColumnInfoContainer, DWORD dwContextMenus /* = SVNSLC_POPALL */, bool bHasCheckboxes /* = true */)
{
	Locker lock(m_critSec);
	m_sColumnInfoContainer = sColumnInfoContainer;
	CRegDWORD regColInfo(_T("Software\\TortoiseSVN\\StatusColumns\\")+sColumnInfoContainer, dwColumns);
	m_dwColumns = regColInfo;
	m_dwContextMenus = dwContextMenus;
	// set the extended style of the listcontrol
	// the style LVS_EX_FULLROWSELECT interferes with the background watermark image but it's more important to be able to select in the whole row.
	CRegDWORD regFullRowSelect(_T("Software\\TortoiseSVN\\FullRowSelect"), TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	exStyle |= (bHasCheckboxes ? LVS_EX_CHECKBOXES : 0);
	SetRedraw(false);
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
	m_ColumnShown[nCol] = true;
	InsertColumn(nCol++, temp);
	temp.LoadString(IDS_STATUSLIST_COLEXT);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLEXT;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLSTATUS);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLSTATUS;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLREMOTESTATUS);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLREMOTESTATUS;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLTEXTSTATUS);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLTEXTSTATUS;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLPROPSTATUS);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLPROPSTATUS;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLREMOTETEXTSTATUS);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLREMOTETEXT;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLREMOTEPROPSTATUS);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLREMOTEPROP;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLURL);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLURL;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLLOCK);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLLOCK;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLLOCKCOMMENT);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLLOCKCOMMENT;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLAUTHOR);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLAUTHOR;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLREVISION);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLREVISION;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLDATE);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLDATE;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLSVNLOCK);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLSVNNEEDSLOCK;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLCOPYFROM);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLCOPYFROM;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);
	nCol++;
	temp.LoadString(IDS_STATUSLIST_COLMODIFICATIONDATE);
	m_ColumnShown[nCol] = m_dwColumns & SVNSLC_COLMODIFICATIONDATE;
	InsertColumn(nCol, temp, LVCFMT_LEFT, m_ColumnShown[nCol] ? -1 : 0);

	CRegString regColOrder(_T("Software\\TortoiseSVN\\StatusColumns\\")+sColumnInfoContainer+_T("_Order"));
	CRegString regColWidths(_T("Software\\TortoiseSVN\\StatusColumns\\")+sColumnInfoContainer+_T("_Width"));
	int arr[SVNSLC_NUMCOLUMNS];
	if (!CString(regColOrder).IsEmpty())
	{
		StringToOrderArray(regColOrder, arr);
		SetColumnOrderArray(SVNSLC_NUMCOLUMNS, arr);
	}
	if (!CString(regColWidths).IsEmpty())
	{
		StringToWidthArray(regColWidths, m_arColumnWidths);
	}
	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = 0; col <= maxcol; col++)
	{
		if (m_ColumnShown[col]==0)
			HideColumn(col);
		if (m_arColumnWidths[col] == 0)
			SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
		else
			SetColumnWidth(col, m_arColumnWidths[col]);
	}
	SetRedraw(true);

	m_bUnversionedRecurse = !!((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\UnversionedRecurse"), TRUE));
}

bool CSVNStatusListCtrl::SetBackgroundImage(UINT nID)
{
	SetTextBkColor(CLR_NONE);
	COLORREF bkColor = GetBkColor();
	// create a bitmap from the icon
	HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(nID), IMAGE_ICON, 128, 128, LR_DEFAULTCOLOR);
	if (!hIcon)
		return false;

	RECT rect = {0};
	rect.right = 128;
	rect.bottom = 128;
	HBITMAP bmp = NULL;

	HWND desktop = ::GetDesktopWindow();
	if (desktop)
	{
		HDC screen_dev = ::GetDC(desktop);
		if (screen_dev)
		{
			// Create a compatible DC
			HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
			if (dst_hdc)
			{
				// Create a new bitmap of icon size
				bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
				if (bmp)
				{
					// Select it into the compatible DC
					HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);
					// Fill the background of the compatible DC with the given colour
					::SetBkColor(dst_hdc, bkColor);
					::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

					// Draw the icon into the compatible DC
					::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);
					::SelectObject(dst_hdc, old_dst_bmp);
				}
				::DeleteDC(dst_hdc);
			}
		}
		::ReleaseDC(desktop, screen_dev); 
	}

	// Restore settings
	DestroyIcon(hIcon);

	if (bmp == NULL)
		return false;

	LVBKIMAGE lv;
	lv.ulFlags = LVBKIF_TYPE_WATERMARK;
	lv.hbm = bmp;
	lv.xOffsetPercent = 100;
	lv.yOffsetPercent = 100;
	SetBkImage(&lv);
	return true;
}

BOOL CSVNStatusListCtrl::GetStatus(const CTSVNPathList& pathList, bool bUpdate /* = FALSE */, bool bShowIgnores /* = false */)
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
	m_changelists.clear();
	for (size_t i=0; i < m_arStatusArray.size(); i++)
	{
		FileEntry * entry = m_arStatusArray[i];
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

		SVNStatus status(m_pbCanceled);
		if(m_nTargetCount > 1 && pathList.AreAllPathsFilesInOneDirectory())
		{
			// This is a special case, where we've been given a list of files
			// all from one folder
			// We handle them by setting a status filter, then requesting the SVN status of
			// all the files in the directory, filtering for the ones we're interested in
			status.SetFilter(pathList);

			// if all selected entries are files, we don't do a recursive status
			// fetching. But if only one is a directory, we have to recurse.
			svn_depth_t depth = svn_depth_files;
			// We have set a filter. If all selected items were files or we fetch
			// the status not recursively, then we have to include
			// ignored items too - the user has selected them
			bool bShowIgnoresRight = true;
			for (int fcindex=0; fcindex<pathList.GetCount(); ++fcindex)
			{
				if (pathList[fcindex].IsDirectory())
				{
					depth = svn_depth_infinity;
					bShowIgnoresRight = false;
					break;
				}
			}
			if(!FetchStatusForSingleTarget(config, status, pathList.GetCommonDirectory(), bUpdate, sUUID, arExtPaths, true, depth, bShowIgnoresRight))
			{
				bRet = FALSE;
			}
		}
		else
		{
			for(int nTarget = 0; nTarget < m_nTargetCount; nTarget++)
			{
				// note:
				// if a target specified multiple times (either directly or included
				// in the status of a parent item) it will also show up multiple
				// times in the list!
				if(!FetchStatusForSingleTarget(config, status, pathList[nTarget], bUpdate, sUUID, arExtPaths, false, svn_depth_infinity, bShowIgnores))
				{
					bRet = FALSE;
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

	m_bBlock = FALSE;
	m_bBusy = false;
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

	m_HeadRev = SVNRev(status.headrev);
	if (s!=0)
	{
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
			if (SVNStatus::GetAllStatus(workingTarget) != svn_wc_status_unversioned)
			{
				return true;	// ignore nested layouts
			}
		}
		if (status.IsExternal(svnPath))
		{
			m_bHasExternals = TRUE;
		}

		AddNewFileEntry(s, svnPath, workingTarget, true, m_bHasExternals, bEntryFromDifferentRepo);

		if (((wcFileStatus == svn_wc_status_unversioned)||((wcFileStatus == svn_wc_status_ignored)&&(m_bShowIgnores))) && svnPath.IsDirectory())
		{
			// we have an unversioned folder -> get all files in it recursively!
			AddUnversionedFolder(svnPath, workingTarget.GetContainingDirectory(), &config);
		}

		// for folders, get all statuses inside it too
		if(workingTarget.IsDirectory())
		{
			ReadRemainingItemsStatus(status, workingTarget, strCurrentRepositoryUUID, arExtPaths, &config, bAllDirect);
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
	entry->differentrepo = bEntryfromDifferentRepo;
	entry->direct = bDirectItem;
	entry->copied = !!pSVNStatus->copied;
	entry->switched = !!pSVNStatus->switched;

	entry->last_commit_date = pSVNStatus->ood_last_cmt_date;
	if ((entry->last_commit_date == NULL)&&(pSVNStatus->entry))
		entry->last_commit_date = pSVNStatus->entry->cmt_date;
	entry->last_commit_rev = pSVNStatus->ood_last_cmt_rev;
	if ((entry->last_commit_rev == -1)&&(pSVNStatus->entry))
		entry->last_commit_rev = pSVNStatus->entry->cmt_rev;
	if (pSVNStatus->ood_last_cmt_author)
		entry->last_commit_author = CUnicodeUtils::GetUnicode(pSVNStatus->ood_last_cmt_author);
	if ((entry->last_commit_author.IsEmpty())&&(pSVNStatus->entry)&&(pSVNStatus->entry->cmt_author))
		entry->last_commit_author = CUnicodeUtils::GetUnicode(pSVNStatus->entry->cmt_author);

	if (pSVNStatus->entry)
		entry->isConflicted = pSVNStatus->entry->conflict_wrk ? true : false;

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

	if (pSVNStatus->entry)
	{
		entry->isfolder = (pSVNStatus->entry->kind == svn_node_dir);
		entry->Revision = pSVNStatus->entry->revision;
		entry->keeplocal = !!pSVNStatus->entry->keep_local;
		entry->working_size = pSVNStatus->entry->working_size;
		entry->depth = pSVNStatus->entry->depth;

		if (pSVNStatus->entry->url)
		{
			entry->url = CUnicodeUtils::GetUnicode(CPathUtils::PathUnescape(pSVNStatus->entry->url));
		}
		if (pSVNStatus->entry->copyfrom_url)
		{
			entry->copyfrom_url = CUnicodeUtils::GetUnicode(CPathUtils::PathUnescape(pSVNStatus->entry->copyfrom_url));
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
		entry->needslock = (pSVNStatus->entry->present_props && (strstr(pSVNStatus->entry->present_props, "svn:needs-lock")!=NULL) );
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
			if (SVNStatus::GetAllStatus(svnPath) != svn_wc_status_unversioned)
			{
				FileEntry * entry = new FileEntry();
				entry->path = svnPath;
				entry->basepath = basePath;
				entry->inunversionedfolder = true;
				entry->isfolder = true;
				entry->isNested = true;
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
					if ((SVNStatus::IsImportant(wcFileStatus))&&
						(!lastexternalpath.IsEmpty())&&
						(lastexternalpath.IsAncestorOf(svnPath)))
					{
						bEntryfromDifferentRepo = true;
						m_bHasExternalsFromDifferentRepos = TRUE;
					}
				}
			}
		}
		if (status.IsExternal(svnPath))
		{
			arExtPaths.AddPath(svnPath);
			m_bHasExternals = TRUE;
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
		if (((wcFileStatus == svn_wc_status_unversioned)&&(!bMatchIgnore))||
			((wcFileStatus == svn_wc_status_ignored)&&(m_bShowIgnores))||
			((wcFileStatus == svn_wc_status_unversioned)&&(bMatchIgnore)&&(m_bShowIgnores)))
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
DWORD CSVNStatusListCtrl::GetShowFlagsFromSVNStatus(svn_wc_status_kind status)
{
	switch (status)
	{
	case svn_wc_status_none:
	case svn_wc_status_unversioned:
		return SVNSLC_SHOWUNVERSIONED;
	case svn_wc_status_ignored:
		if (!m_bShowIgnores)
			return SVNSLC_SHOWDIRECTS;
		return SVNSLC_SHOWDIRECTS|SVNSLC_SHOWIGNORED;
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
	Locker lock(m_critSec);
	WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	CWinApp * pApp = AfxGetApp();
	if (pApp)
		pApp->DoWaitCursor(1);
	m_dwShow = dwShow;
	m_bShowFolders = bShowFolders;
	m_nSelected = 0;
	int nTopIndex = GetTopIndex();
	POSITION posSelectedEntry = GetFirstSelectedItemPosition();
	int nSelectedEntry = 0;
	if (posSelectedEntry)
		nSelectedEntry = GetNextSelectedItem(posSelectedEntry);
	SetRedraw(FALSE);
	DeleteAllItems();

	PrepareGroups();

	m_arListArray.clear();

	m_arListArray.reserve(m_arStatusArray.size());
	SetItemCount(m_arStatusArray.size());

	int listIndex = 0;
	for (size_t i=0; i < m_arStatusArray.size(); ++i)
	{
		FileEntry * entry = m_arStatusArray[i];
		if ((entry->inexternal) && (!(dwShow & SVNSLC_SHOWINEXTERNALS)))
			continue;
		if ((entry->differentrepo || entry->isNested) && (! (dwShow & SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO)))
			continue;
		if (entry->IsFolder() && (!bShowFolders))
			continue;	// don't show folders if they're not wanted.
		svn_wc_status_kind status = SVNStatus::GetMoreImportant(entry->status, entry->remotestatus);
		DWORD showFlags = GetShowFlagsFromSVNStatus(status);
		if (entry->IsLocked())
			showFlags |= SVNSLC_SHOWLOCKS;
		if (entry->switched)
			showFlags |= SVNSLC_SHOWSWITCHED;

		// status_ignored is a special case - we must have the 'direct' flag set to add a status_ignored item
		if (status != svn_wc_status_ignored || (entry->direct) || (dwShow & SVNSLC_SHOWIGNORED))
		{
			if ((!entry->IsFolder()) && (status == svn_wc_status_deleted) && (dwShow & SVNSLC_SHOWREMOVEDANDPRESENT))
			{
				if (PathFileExists(entry->GetPath().GetWinPath()))
				{
					m_arListArray.push_back(i);
					if ((dwCheck & SVNSLC_SHOWREMOVEDANDPRESENT)||((dwCheck & SVNSLC_SHOWDIRECTS)&&(entry->direct)))
					{
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
					entry->checked = true;
				}
				AddEntry(entry, langID, listIndex++);
			}
			else if ((dwShow & showFlags)||((dwShow & SVNSLC_SHOWDIRECTFOLDER)&&(entry->direct)&&entry->IsFolder()))
			{
				m_arListArray.push_back(i);
				if ((dwCheck & showFlags)||((dwCheck & SVNSLC_SHOWDIRECTS)&&(entry->direct)))
				{
					entry->checked = true;
				}
				AddEntry(entry, langID, listIndex++);
			}
		}
	}

	SetItemCount(listIndex);

	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = 0; col <= maxcol; col++)
	{
		if (m_arColumnWidths[col] == 0)
			SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
		else
			SetColumnWidth(col, m_arColumnWidths[col]);
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

	if (pApp)
		pApp->DoWaitCursor(-1);

	m_bEmpty = (GetItemCount() == 0);
	Invalidate();
}

void CSVNStatusListCtrl::Show(DWORD dwShow, const CTSVNPathList& checkedList, bool bShowFolders /* = true */)
{
	Locker lock(m_critSec);
	WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	CWinApp * pApp = AfxGetApp();
	if (pApp)
		pApp->DoWaitCursor(1);
	m_dwShow = dwShow;
	m_bShowFolders = bShowFolders;
	m_nSelected = 0;
	int nTopIndex = GetTopIndex();
	POSITION posSelectedEntry = GetFirstSelectedItemPosition();
	int nSelectedEntry = 0;
	if (posSelectedEntry)
		nSelectedEntry = GetNextSelectedItem(posSelectedEntry);
	SetRedraw(FALSE);
	DeleteAllItems();

	PrepareGroups();

	m_arListArray.clear();

	m_arListArray.reserve(m_arStatusArray.size());
	SetItemCount(m_arStatusArray.size());

	int listIndex = 0;
	for (size_t i=0; i < m_arStatusArray.size(); ++i)
	{
		FileEntry * entry = m_arStatusArray[i];
		if ((entry->inexternal) && (!(dwShow & SVNSLC_SHOWINEXTERNALS)))
			continue;
		if ((entry->differentrepo || entry->isNested) && (! (dwShow & SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO)))
			continue;
		if (entry->IsFolder() && (!bShowFolders))
			continue;	// don't show folders if they're not wanted.
		svn_wc_status_kind status = SVNStatus::GetMoreImportant(entry->status, entry->remotestatus);
		DWORD showFlags = GetShowFlagsFromSVNStatus(status);
		if (entry->IsLocked())
			showFlags |= SVNSLC_SHOWLOCKS;

		// status_ignored is a special case - we must have the 'direct' flag set to add a status_ignored item
		if (status != svn_wc_status_ignored || (entry->direct) || (dwShow & SVNSLC_SHOWIGNORED))
		{
			for (int npath = 0; npath < checkedList.GetCount(); ++npath)
			{
				if (entry->GetPath().IsEquivalentTo(checkedList[npath]))
				{
					entry->checked = true;
					break;
				}
			}
			if ((!entry->IsFolder()) && (status == svn_wc_status_deleted) && (dwShow & SVNSLC_SHOWREMOVEDANDPRESENT))
			{
				if (PathFileExists(entry->GetPath().GetWinPath()))
				{
					m_arListArray.push_back(i);
					AddEntry(entry, langID, listIndex++);
				}
			}
			else if ((dwShow & showFlags)||((dwShow & SVNSLC_SHOWDIRECTFILES)&&(entry->direct)&&(!entry->IsFolder())))
			{
				m_arListArray.push_back(i);
				AddEntry(entry, langID, listIndex++);
			}
			else if ((dwShow & showFlags)||((dwShow & SVNSLC_SHOWDIRECTFOLDER)&&(entry->direct)&&entry->IsFolder()))
			{
				m_arListArray.push_back(i);
				AddEntry(entry, langID, listIndex++);
			}
			else if (entry->switched)
			{
				m_arListArray.push_back(i);
				AddEntry(entry, langID, listIndex++);
			}
		}
	}

	SetItemCount(listIndex);

	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = 0; col <= maxcol; col++)
	{
		if (m_arColumnWidths[col] == 0)
			SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
		else
			SetColumnWidth(col, m_arColumnWidths[col]);
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

	if (pApp)
		pApp->DoWaitCursor(-1);

	m_bEmpty = (GetItemCount() == 0);
	Invalidate();
}

void CSVNStatusListCtrl::AddEntry(FileEntry * entry, WORD langID, int listIndex)
{
	static CString ponly(MAKEINTRESOURCE(IDS_STATUSLIST_PROPONLY));
	static HINSTANCE hResourceHandle(AfxGetResourceHandle());

	CString path = entry->GetPath().GetSVNPathString();
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
	CString entryname = entry->GetDisplayName();
	int icon_idx = 0;
	if (entry->isfolder)
		icon_idx = m_nIconFolder;
	else
	{
		icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(entry->path);
	}
	// relative path
	InsertItem(index, entryname, icon_idx);
	// SVNSLC_COLEXT
	SetItemText(index, nCol++, entry->path.GetFileExtension());
	// SVNSLC_COLSTATUS
	if (entry->isNested)
	{
		CString sTemp(MAKEINTRESOURCE(IDS_STATUSLIST_NESTED));
		SetItemText(index, nCol++, sTemp);
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
		SetItemText(index, nCol++, buf);
	}
	// SVNSLC_COLREMOTESTATUS
	if (entry->isNested)
	{
		CString sTemp(MAKEINTRESOURCE(IDS_STATUSLIST_NESTED));
		SetItemText(index, nCol++, sTemp);
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
		CString sTemp(MAKEINTRESOURCE(IDS_STATUSLIST_NESTED));
		SetItemText(index, nCol++, sTemp);
	}
	else
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->textstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		if ((entry->copied)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (+)"));
		if ((entry->switched)&&(_tcslen(buf)>1))
			_tcscat_s(buf, 100, _T(" (s)"));
		SetItemText(index, nCol++, buf);
	}
	// SVNSLC_COLPROPSTATUS
	if (entry->isNested)
	{
		SetItemText(index, nCol++, _T(""));
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
		SetItemText(index, nCol++, _T(""));
	}
	else
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->remotetextstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	// SVNSLC_COLREMOTEPROP
	if (entry->isNested)
	{
		SetItemText(index, nCol++, _T(""));
	}
	else
	{
		SVNStatus::GetStatusString(hResourceHandle, entry->remotepropstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
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
	// SVNSLC_COLLOCKCOMMENT
	SetItemText(index, nCol++, entry->lock_comment);
	// SVNSLC_COLAUTHOR
	SetItemText(index, nCol++, entry->last_commit_author);
	// SVNSLC_COLREVISION
	CString temp;
	temp.Format(_T("%ld"), entry->last_commit_rev);
	if (entry->last_commit_rev > 0)
		SetItemText(index, nCol++, temp);
	else
		SetItemText(index, nCol++, _T(""));
	// SVNSLC_COLDATE
	TCHAR datebuf[SVN_DATE_BUFFER];
	apr_time_t date = entry->last_commit_date;
	SVN::formatDate(datebuf, date, true);
	if (date)
		SetItemText(index, nCol++, datebuf);
	else
		SetItemText(index, nCol++, _T(""));
	// SVNSLC_COLSVNNEEDSLOCK
	BOOL bFoundSVNNeedsLock = (entry->present_props.Find(_T("svn:needs-lock"))!=-1);
	CString strSVNNeedsLock = (bFoundSVNNeedsLock) ? _T("*") : _T("");
	SetItemText(index, nCol++, strSVNNeedsLock);
	// SVNSLC_COLCOPYFROM
	if (m_sURL.Compare(entry->copyfrom_url.Left(m_sURL.GetLength()))==0)
		temp = entry->copyfrom_url.Mid(m_sURL.GetLength());
	else
		temp = entry->copyfrom_url;
	SetItemText(index, nCol++, temp);
	// SVNSLC_COLMODIFICATIONDATE
	__int64 filetime = entry->GetPath().GetLastWriteTime();
	if ( (filetime) && (entry->status!=svn_wc_status_deleted) )
	{
		FILETIME* f = (FILETIME*)(__int64*)&filetime;
		TCHAR datebuf[SVN_DATE_BUFFER];
		SVN::formatDate(datebuf,*f,true);
		SetItemText(index, nCol++, datebuf);
	}
	else
	{
		SetItemText(index, nCol++, _T(""));
	}

	SetCheck(index, entry->checked);
	if (entry->checked)
		m_nSelected++;
	if (m_changelists.find(entry->changelist) != m_changelists.end())
		SetItemGroup(index, m_changelists[entry->changelist]);
	else
		SetItemGroup(index, 0);
	m_bBlock = FALSE;
}

bool CSVNStatusListCtrl::SetItemGroup(int item, int groupindex)
{
	if ((m_dwContextMenus & SVNSLC_POPCHANGELISTS) == NULL)
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
	std::sort(m_arStatusArray.begin(), m_arStatusArray.end(), SortCompare);
	SaveColumnWidths();
	Show(m_dwShow, 0, m_bShowFolders);
}

bool CSVNStatusListCtrl::SortCompare(const FileEntry* entry1, const FileEntry* entry2)
{
	#define SGN(x) ((x)==0?0:((x)>0?1:-1))

	int result = 0;
	switch (m_nSortedColumn)
	{
	case 16:
		{
			if (result == 0)
			{
				__int64 writetime1 = entry1->GetPath().GetLastWriteTime();
				__int64 writetime2 = entry2->GetPath().GetLastWriteTime();

				FILETIME* filetime1 = (FILETIME*)(__int64*)&writetime1;
				FILETIME* filetime2 = (FILETIME*)(__int64*)&writetime2;
	
				result = CompareFileTime(filetime1,filetime2);
			}
		}
	case 15:
		{
			if (result == 0)
			{
				result = entry1->copyfrom_url.CompareNoCase(entry2->copyfrom_url);
			}
		}
	case 14:
		{
			if (result == 0)
			{
				result = SGN(entry1->needslock - entry2->needslock);
			}
		}
	case 13:
		{
			if (result == 0)
			{
				result = SGN(entry1->last_commit_date - entry2->last_commit_date);
			}
		}
	case 12:
		{
			if (result == 0)
			{
				result = entry1->last_commit_rev - entry2->last_commit_rev;
			}
		}
	case 11:
		{
			if (result == 0)
			{
				result = entry1->last_commit_author.CompareNoCase(entry2->last_commit_author);
			}
		}
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
	case 0:		// path column
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
	m_nSortedColumn = phdr->iItem;
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

#define ISCHECKED(x) ((x) ? ((((x)&LVIS_STATEIMAGEMASK)>>12)-1) : FALSE)
	if ((m_bBlock)&&(m_bBlockUI))
	{
		// if we're blocked, prevent changing of the check state
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
	if ( (entry->status == svn_wc_status_deleted) || (m_bCheckChildrenWithParent) || (bShift) )
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
					if ((testEntry->status == svn_wc_status_deleted)||(m_bCheckChildrenWithParent))
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
	else if (entry->status == svn_wc_status_deleted)
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
					if (testEntry->status == svn_wc_status_deleted)
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
							FileEntryVector::iterator itMatchingItem;
							itMatchingItem = std::lower_bound(m_arStatusArray.begin(), itFirstUnversionedEntry, entry, EntryPathCompareNoCase);

							// adjust the case of the filename
							if (MoveFileEx(entry->path.GetWinPath(), (*itMatchingItem)->path.GetWinPath(), MOVEFILE_REPLACE_EXISTING))
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
				}
				break;
			} // switch (entry->status)
		} // if (entry)
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

int CSVNStatusListCtrl::GetGroupFromPoint(POINT * ppt)
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
	while ((flags & LVHT_BELOW) == 0)
	{
		nItem = HitTest(pt, &flags);
		if (flags & LVHT_ONITEM)
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
					LVITEM lv = {0};
					lv.mask = LVIF_GROUPID;
					lv.iItem = nItem;
					GetItem(&lv);
					if (lv.iGroupId != groupID)
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

void CSVNStatusListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	bool XPorLater = false;
	OSVERSIONINFOEX inf;
	ZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	if (fullver >= 0x0501)
		XPorLater = true;
	bool bShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);

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
		if ((GetSelectedCount() == 0)&&(XPorLater))
		{
			// nothing selected could mean the context menu is requested for
			// a group header
			POINT clientpoint = point;
			ScreenToClient(&clientpoint);
			if ((IsGroupViewEnabled())&&(GetGroupFromPoint(&clientpoint) >= 0))
			{
				CMenu popup;
				if (popup.CreatePopupMenu())
				{
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
								ZeroMemory(&lv, sizeof(LVITEM));
								lv.mask = LVIF_GROUPID;
								lv.iItem = i;
								GetItem(&lv);
								if (lv.iGroupId == group)
								{
									FileEntry * entry = GetListEntry(i);
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
							NotifyCheck();
							m_bBlock = false;
						}
						break;
					}
				}
			}
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
			CMenu popup;
			CMenu changelistSubMenu;
			if (popup.CreatePopupMenu())
			{
				CString temp;
				if (wcStatus >= svn_wc_status_normal)
				{
					if (m_dwContextMenus & SVNSLC_POPCOMPAREWITHBASE)
					{
						temp.LoadString(IDS_LOG_COMPAREWITHBASE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMPARE, temp);
						popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
					}
								
					if (GetSelectedCount() == 1)
					{
						bool bEntryAdded = false;
						if (entry->remotestatus <= svn_wc_status_normal)
						{
							if (wcStatus > svn_wc_status_normal)
							{
								if ((m_dwContextMenus & SVNSLC_POPGNUDIFF)&&(wcStatus != svn_wc_status_deleted)&&(wcStatus != svn_wc_status_missing))
								{
									temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
									popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_GNUDIFF1, temp);
									bEntryAdded = true;
								}
							}
							if (wcStatus > svn_wc_status_normal)
							{
								if (m_dwContextMenus & SVNSLC_POPCOMMIT)
								{
									temp.LoadString(IDS_STATUSLIST_CONTEXT_COMMIT);
									popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMMIT, temp);
									popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
									bEntryAdded = true;
								}
							}
						}
						else if (wcStatus != svn_wc_status_deleted)
						{
							if (m_dwContextMenus & SVNSLC_POPCOMPARE)
							{
								temp.LoadString(IDS_LOG_POPUP_COMPARE);
								popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMPARE, temp);
								popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
								bEntryAdded = true;
							}
							if (m_dwContextMenus & SVNSLC_POPGNUDIFF)
							{
								temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
								popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_GNUDIFF1, temp);
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
							temp.LoadString(IDS_STATUSLIST_CONTEXT_COMMIT);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMMIT, temp);
							popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
						}
					}
				}
				if (GetSelectedCount() > 0)
				{
					if ((GetSelectedCount() == 2)&&(m_dwContextMenus & SVNSLC_POPREPAIRMOVE))
					{
						POSITION pos = GetFirstSelectedItemPosition();
						int index = GetNextSelectedItem(pos);
						if (index >= 0)
						{
							FileEntry * entry = GetListEntry(index);
							svn_wc_status_kind status1 = svn_wc_status_none;
							svn_wc_status_kind status2 = svn_wc_status_none;
							if (entry)
								status1 = entry->status;
							index = GetNextSelectedItem(pos);
							if (index >= 0)
							{
								entry = GetListEntry(index);
								if (entry)
									status2 = entry->status;
								if ((status1 == svn_wc_status_missing && status2 == svn_wc_status_unversioned) ||
									(status2 == svn_wc_status_missing && status1 == svn_wc_status_unversioned))
								{
									temp.LoadString(IDS_STATUSLIST_CONTEXT_REPAIRMOVE);
									popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_REPAIRMOVE, temp);
								}
							}
						}
					}
					if (wcStatus > svn_wc_status_normal)
					{
						if (m_dwContextMenus & SVNSLC_POPREVERT)
						{
							// reverting missing folders is not possible
							if (!entry->IsFolder() || (wcStatus != svn_wc_status_missing))
							{
								temp.LoadString(IDS_MENUREVERT);
								popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_REVERT, temp);
							}
						}
					}
					if (entry->remotestatus > svn_wc_status_normal)
					{
						if (m_dwContextMenus & SVNSLC_POPUPDATE)
						{
							temp.LoadString(IDS_MENUUPDATE);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_UPDATE, temp);
						}
					}
				}
				if ((GetSelectedCount() == 1)&&(wcStatus >= svn_wc_status_normal)
					&&(wcStatus != svn_wc_status_ignored))
				{
					if (m_dwContextMenus & SVNSLC_POPSHOWLOG)
					{
						temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_LOG, temp);
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
				if (GetSelectedCount() > 0)
				{
					if (((wcStatus == svn_wc_status_unversioned)||(wcStatus == svn_wc_status_ignored))&&(m_dwContextMenus & SVNSLC_POPDELETE))
					{
						temp.LoadString(IDS_MENUREMOVE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_DELETE, temp);
					}
					if ((wcStatus != svn_wc_status_unversioned)&&(wcStatus != svn_wc_status_ignored)&&(wcStatus != svn_wc_status_deleted)&&(wcStatus != svn_wc_status_added)&&(m_dwContextMenus & SVNSLC_POPDELETE))
					{
						if (bShift)
							temp.LoadString(IDS_MENUREMOVEKEEP);
						else
							temp.LoadString(IDS_MENUREMOVE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_REMOVE, temp);
					}
					if ((wcStatus == svn_wc_status_unversioned)||(wcStatus == svn_wc_status_deleted))
					{
						if (m_dwContextMenus & SVNSLC_POPADD)
						{
							temp.LoadString(IDS_STATUSLIST_CONTEXT_ADD);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_ADD, temp);
						}
					}
					if ((wcStatus == svn_wc_status_unversioned))
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
								CMenu submenu;
								if (submenu.CreateMenu())
								{
									CString ignorepath;
									if (ignorelist.GetCount()==1)
										ignorepath = ignorelist[0].GetFileOrDirectoryName();
									else
										ignorepath.Format(IDS_MENUIGNOREMULTIPLE, ignorelist.GetCount());
									submenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNORE, ignorepath);
									ignorepath = _T("*")+sExt;
									submenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNOREMASK, ignorepath);
									temp.LoadString(IDS_MENUIGNORE);
									popup.InsertMenu((UINT)-1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)submenu.m_hMenu, temp);
								}
							}
							else
							{
								if (ignorelist.GetCount()==1)
								{
									temp.LoadString(IDS_MENUIGNORE);
								}
								else
								{
									temp.Format(IDS_MENUIGNOREMULTIPLE, ignorelist.GetCount());
								}
								popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNORE, temp);
							}
						}
					}
				}
				if (((wcStatus == svn_wc_status_conflicted)||(entry->isConflicted)))
				{
					if ((m_dwContextMenus & SVNSLC_POPCONFLICT)||(m_dwContextMenus & SVNSLC_POPRESOLVE))
						popup.AppendMenu(MF_SEPARATOR);

					if ((m_dwContextMenus & SVNSLC_POPCONFLICT)&&(entry->textstatus == svn_wc_status_conflicted))
					{
						temp.LoadString(IDS_MENUCONFLICT);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_EDITCONFLICT, temp);
					}
					if (m_dwContextMenus & SVNSLC_POPRESOLVE)
					{
						temp.LoadString(IDS_STATUSLIST_CONTEXT_RESOLVED);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_RESOLVECONFLICT, temp);
					}
					if ((m_dwContextMenus & SVNSLC_POPRESOLVE)&&(entry->textstatus == svn_wc_status_conflicted))
					{
						temp.LoadString(IDS_SVNPROGRESS_MENUUSETHEIRS);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_RESOLVETHEIRS, temp);
						temp.LoadString(IDS_SVNPROGRESS_MENUUSEMINE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_RESOLVEMINE, temp);
					}
				}
				if (GetSelectedCount() > 0)
				{
					if ((!entry->IsFolder())&&(wcStatus >= svn_wc_status_normal)
						&&(wcStatus!=svn_wc_status_missing)&&(wcStatus!=svn_wc_status_deleted)
						&&(wcStatus!=svn_wc_status_added))
					{
						popup.AppendMenu(MF_SEPARATOR);
						if ((entry->lock_token.IsEmpty())&&(!entry->IsFolder()))
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
					}
					if ((!entry->IsFolder())&&((!entry->lock_token.IsEmpty())||(!entry->lock_remotetoken.IsEmpty())))
					{
						if (m_dwContextMenus & SVNSLC_POPUNLOCKFORCE)
						{
							temp.LoadString(IDS_MENU_UNLOCKFORCE);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_UNLOCKFORCE, temp);
						}
					}

					if (wcStatus != svn_wc_status_missing && wcStatus != svn_wc_status_deleted &&wcStatus!=svn_wc_status_unversioned)
					{
						popup.AppendMenu(MF_SEPARATOR);
						temp.LoadString(IDS_STATUSLIST_CONTEXT_PROPERTIES);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_PROPERTIES, temp);
					}
					popup.AppendMenu(MF_SEPARATOR);
					temp.LoadString(IDS_STATUSLIST_CONTEXT_COPY);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COPY, temp);
					temp.LoadString(IDS_STATUSLIST_CONTEXT_COPYEXT);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COPYEXT, temp);
					if ((m_dwContextMenus & SVNSLC_POPCHANGELISTS)&&(XPorLater))
					{
						popup.AppendMenu(MF_SEPARATOR);
						// changelist commands
						int numChangelists = GetNumberOfChangelistsInSelection();
						if (numChangelists > 0)
						{
							temp.LoadString(IDS_STATUSLIST_CONTEXT_REMOVEFROMCS);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_REMOVEFROMCS, temp);
						}
						if (changelistSubMenu.CreateMenu())
						{
							temp.LoadString(IDS_STATUSLIST_CONTEXT_CREATECS);
							changelistSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_CREATECS, temp);
							if (m_changelists.size() > 0)
							{
								changelistSubMenu.AppendMenu(MF_SEPARATOR);
								// find the changelist names
								int cmdID = IDSVNLC_MOVETOCS;
								for (std::map<CString, int>::const_iterator it = m_changelists.begin(); it != m_changelists.end(); ++it)
								{
									changelistSubMenu.AppendMenu(MF_STRING | MF_ENABLED, cmdID, it->first);
									cmdID++;
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
				bool bForce = false;
				switch (cmd)
				{
				case IDSVNLC_COPY:
					CopySelectedEntriesToClipboard(0);
					break;
				case IDSVNLC_COPYEXT:
					CopySelectedEntriesToClipboard(m_dwColumns);
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
							if (!svn.Revert(targetList, FALSE))
							{
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							}
							else
							{
								// since the entries got reverted we need to remove
								// them from the list too, if no remote changes are shown
								// and if the unmodified files are not shown
								POSITION pos;
								while ((pos = GetFirstSelectedItemPosition())!=0)
								{
									int index;
									index = GetNextSelectedItem(pos);
									FileEntry * fentry = m_arStatusArray[m_arListArray[index]];
									BOOL bAdded = (fentry->textstatus == svn_wc_status_added);
									fentry->status = svn_wc_status_normal;
									fentry->propstatus = svn_wc_status_normal;
									fentry->textstatus = svn_wc_status_normal;
									fentry->copied = false;
									fentry->isConflicted = false;
									if ((fentry->remotestatus <= svn_wc_status_normal)||(m_dwShow & SVNSLC_SHOWNORMAL))
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
										}
									}
									else
									{
										SetItemState(index, 0, LVIS_SELECTED);
									}
								}
								SaveColumnWidths();
								Show(m_dwShow, 0, m_bShowFolders);
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
				case IDSVNLC_GNUDIFF1:
					{
						SVNDiff diff(NULL, this->m_hWnd, true);

						if (entry->remotestatus <= svn_wc_status_normal)
							diff.ShowUnifiedDiff(entry->path, SVNRev::REV_BASE, entry->path, SVNRev::REV_WC);
						else
							diff.ShowUnifiedDiff(entry->path, SVNRev::REV_WC, entry->path, SVNRev::REV_HEAD);
					}
					break;
				case IDSVNLC_UPDATE:
					{
						CTSVNPathList targetList;
						FillListOfSelectedItemPaths(targetList);
						CSVNProgressDlg dlg;
						dlg.SetParams(CSVNProgressDlg::SVNProgress_Update, 0, targetList);
						dlg.DoModal();
					}
					break;
				case IDSVNLC_LOG:
					{
						CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
						int limit = (int)(LONG)reg;
						CLogDlg dlg;
						dlg.SetParams(filepath, SVNRev(), SVNRev::REV_HEAD, 1, limit);
						dlg.DoModal();
					}
					break;
				case IDSVNLC_OPEN:
					{
						int ret = (int)ShellExecute(this->m_hWnd, NULL, filepath.GetWinPath(), NULL, NULL, SW_SHOW);
						if (ret <= HINSTANCE_ERROR)
						{
							CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
							cmd += filepath.GetWinPathString();
							CAppUtils::LaunchApplication(cmd, NULL, false);
						}
					}
					break;
				case IDSVNLC_OPENWITH:
					{
						CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
						cmd += filepath.GetWinPathString();
						CAppUtils::LaunchApplication(cmd, NULL, false);
					}
					break;
				case IDSVNLC_EXPLORE:
					{
						ShellExecute(this->m_hWnd, _T("explore"), filepath.GetDirectory().GetWinPath(), NULL, NULL, SW_SHOW);
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
						if (svn.Remove(itemsToRemove, FALSE, bShift))
						{
							bSuccess = true;
						}
						else
						{
							if ((svn.Err->apr_err == SVN_ERR_UNVERSIONED_RESOURCE) ||
								(svn.Err->apr_err == SVN_ERR_CLIENT_MODIFIED))
							{
								CString msg, yes, no, yestoall;
								msg.Format(IDS_PROC_REMOVEFORCE, svn.GetLastErrorMessage());
								yes.LoadString(IDS_MSGBOX_YES);
								no.LoadString(IDS_MSGBOX_NO);
								yestoall.LoadString(IDS_PROC_YESTOALL);
								UINT ret = CMessageBox::Show(m_hWnd, msg, _T("TortoiseSVN"), 2, IDI_ERROR, yes, no, yestoall);
								if ((ret == 1)||(ret==3))
								{
									if (!svn.Remove(itemsToRemove, TRUE, bShift))
									{
										CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
									}
									else
										bSuccess = true;
								}
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
									RemoveListEntry(index);
								}
								else
								{
									e->textstatus = svn_wc_status_deleted;
									e->status = svn_wc_status_deleted;
									SetEntryCheck(e,index,true);
								}
							}
						}
						SaveColumnWidths();
						Show(m_dwShow, 0, m_bShowFolders);
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
						TCHAR * buf = new TCHAR[len+2];
						_tcscpy_s(buf, len+2, filelist);
						for (int i=0; i<len; ++i)
							if (buf[i] == '|')
								buf[i] = 0;
						SHFILEOPSTRUCT fileop;
						fileop.hwnd = this->m_hWnd;
						fileop.wFunc = FO_DELETE;
						fileop.pFrom = buf;
						fileop.pTo = NULL;
						fileop.fFlags = FOF_NO_CONNECTED_ELEMENTS | (bShift ? 0 : FOF_ALLOWUNDO);
						fileop.lpszProgressTitle = _T("deleting file");
						int result = SHFileOperation(&fileop);
						delete [] buf;

						if ( (result==0) && (!fileop.fAnyOperationsAborted) )
						{
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
						CTSVNPathList ignorelist;
						FillListOfSelectedItemPaths(ignorelist, true);
						std::set<CTSVNPath> parentlist;
						for (int i=0; i<ignorelist.GetCount(); ++i)
						{
							parentlist.insert(ignorelist[i].GetContainingDirectory());
						}
						std::set<CTSVNPath>::iterator it;
						std::vector<CString> toremove;
						for (it = parentlist.begin(); it != parentlist.end(); ++it)
						{
							CTSVNPath parentFolder = (*it).GetDirectory();
							SVNProperties props(parentFolder);
							CStringA value;
							for (int i=0; i<props.GetCount(); i++)
							{
								CString propname(props.GetItemName(i).c_str());
								if (propname.CompareNoCase(_T("svn:ignore"))==0)
								{
									stdstring stemp;
									// treat values as normal text even if they're not
									value = (char *)props.GetItemValue(i).c_str();
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
							if (!props.Add(_T("svn:ignore"), (LPCSTR)value))
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
											entry->last_commit_date = 0;
											entry->last_commit_rev = 0;
											if (s->entry)
											{
												if (s->entry->url)
												{
													entry->url = CUnicodeUtils::GetUnicode(CPathUtils::PathUnescape(s->entry->url));
												}
											}
											if (s->entry && s->entry->present_props)
											{
												entry->present_props = s->entry->present_props;
											}
											m_arStatusArray.push_back(entry);
											m_arListArray.push_back(m_arStatusArray.size()-1);
											AddEntry(entry, langID, GetItemCount());
										}
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
					}
					break;
				case IDSVNLC_IGNORE:
					{
						CTSVNPathList ignorelist;
						std::vector<CString> toremove;
						FillListOfSelectedItemPaths(ignorelist, true);
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
							CString name = ignorelist[j].GetFileOrDirectoryName();
							CTSVNPath parentfolder = ignorelist[j].GetContainingDirectory();
							SVNProperties props(parentfolder);
							CStringA value;
							for (int i=0; i<props.GetCount(); i++)
							{
								CString propname(props.GetItemName(i).c_str());
								if (propname.CompareNoCase(_T("svn:ignore"))==0)
								{
									stdstring stemp;
									// treat values as normal text even if they're not
									value = (char *)props.GetItemValue(i).c_str();
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
							if (!props.Add(_T("svn:ignore"), (LPCSTR)value))
							{
								CString temp;
								temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, name);
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
									FileEntry * entry = GetListEntry(i);
									if (entry->status == svn_wc_status_unversioned)
									{
										if (!ignorelist[j].IsEquivalentTo(entry->GetPath())&&(ignorelist[j].IsAncestorOf(entry->GetPath())))
										{
											entry->status = svn_wc_status_ignored;
											entry->textstatus = svn_wc_status_ignored;
											if (GetCheck(i))
												m_nSelected--;
											toremove.push_back(entry->GetPath().GetSVNPathString());
										}
									}
								}
							}

							CTSVNPath basepath = m_arStatusArray[m_arListArray[selIndex]]->basepath;
							toremove.push_back(m_arStatusArray[m_arListArray[selIndex]]->GetPath().GetSVNPathString());
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
										entry->last_commit_date = 0;
										entry->last_commit_rev = 0;
										if (s->entry)
										{
											if (s->entry->url)
											{
												entry->url = CUnicodeUtils::GetUnicode(CPathUtils::PathUnescape(s->entry->url));
											}
										}
										if (s->entry && s->entry->present_props)
										{
											entry->present_props = s->entry->present_props;
										}
										m_arStatusArray.push_back(entry);
										m_arListArray.push_back(m_arStatusArray.size()-1);
										AddEntry(entry, langID, GetItemCount());
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
					}
					break;
				case IDSVNLC_EDITCONFLICT:
					SVNDiff::StartConflictEditor(filepath);
					break;
				case IDSVNLC_RESOLVECONFLICT:
					{
						if (CMessageBox::Show(m_hWnd, IDS_PROC_RESOLVE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO)==IDYES)
						{
							SVN svn;
							POSITION pos = GetFirstSelectedItemPosition();
							while (pos != 0)
							{
								int index;
								index = GetNextSelectedItem(pos);
								FileEntry * fentry = m_arStatusArray[m_arListArray[index]];
								if (!svn.Resolve(fentry->GetPath(), FALSE))
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
							Show(m_dwShow, 0, m_bShowFolders);
						}
					}
					break;
				case IDSVNLC_RESOLVETHEIRS:
					{
						if (CMessageBox::Show(m_hWnd, IDS_PROC_RESOLVE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO)==IDYES)
						{
							SVN svn;
							POSITION pos = GetFirstSelectedItemPosition();
							while (pos != 0)
							{
								int index;
								index = GetNextSelectedItem(pos);
								FileEntry * fentry = m_arStatusArray[m_arListArray[index]];
								CTSVNPath theirs(fentry->GetPath().GetDirectory());
								SVNStatus stat;
								stat.GetStatus(fentry->GetPath());
								if (stat.status)
								{
									if ((stat.status->entry)&&(stat.status->entry->conflict_new))
									{
										theirs.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new));
									}
									else break;
								}
								else
								{
									CMessageBox::Show(m_hWnd, stat.GetLastErrorMsg(), _T("TortoiseSVN"), MB_ICONERROR);
									break;
								}
								CopyFile(theirs.GetWinPath(), fentry->GetPath().GetWinPath(), FALSE);
								if (!svn.Resolve(fentry->GetPath(), FALSE))
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
							Show(m_dwShow, 0, m_bShowFolders);
						}
					}
					break;
				case IDSVNLC_RESOLVEMINE:
					{
						if (CMessageBox::Show(m_hWnd, IDS_PROC_RESOLVE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO)==IDYES)
						{
							SVN svn;
							POSITION pos = GetFirstSelectedItemPosition();
							while (pos != 0)
							{
								int index;
								index = GetNextSelectedItem(pos);
								FileEntry * fentry = m_arStatusArray[m_arListArray[index]];
								CTSVNPath mine(fentry->GetPath().GetDirectory());
								SVNStatus stat;
								stat.GetStatus(fentry->GetPath());
								if (stat.status)
								{
									if (stat.status->entry)
									{
										if (stat.status->entry->conflict_wrk)
										{
											mine.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk));
										}
										else if (stat.status->entry->conflict_old)
										{
											// if the conflict_wrk entry is empty, that means the file is binary
											// and there is no 'merged' file with the conflict markers in it which would
											// have replaced the original file. In that case, the original file is left
											// untouched, and we need to use that untouched file as 'mine'.
											mine.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->name));
										}
										else
											break;
									}
								}
								else
								{
									CMessageBox::Show(m_hWnd, stat.GetLastErrorMsg(), _T("TortoiseSVN"), MB_ICONERROR);
									break;
								}
								CopyFile(mine.GetWinPath(), fentry->GetPath().GetWinPath(), FALSE);
								if (!svn.Resolve(fentry->GetPath(), FALSE))
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
							Show(m_dwShow, 0, m_bShowFolders);
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
						if (svn.Add(itemsToAdd, &props, FALSE, TRUE, TRUE))
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
						Show(m_dwShow, 0, m_bShowFolders);
						NotifyCheck();
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
						progDlg.SetParams(CSVNProgressDlg::SVNProgress_Lock, inpDlg.m_iCheck ? ProgOptLockForce : 0, itemsToLock, CString(), inpDlg.m_sInputText);
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
					progDlg.SetParams(CSVNProgressDlg::SVNProgress_Unlock, bForce ? ProgOptLockForce : 0, itemsToUnlock);
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
										if (!svn.Move(CTSVNPathList(entry1->GetPath()), entry2->GetPath(), TRUE))
										{
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
										LPVOID lpMsgBuf;
										FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
											FORMAT_MESSAGE_FROM_SYSTEM | 
											FORMAT_MESSAGE_IGNORE_INSERTS,
											NULL,
											GetLastError(),
											MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
											(LPTSTR) &lpMsgBuf,
											0,
											NULL 
											);
										MessageBox((LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
										LocalFree( lpMsgBuf );
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
						if (svn.RemoveFromChangeList(changelistItems, CString()))
						{
							// The changelists were removed, but we now need to run through the selected items again
							// and update their changelist
							POSITION pos = GetFirstSelectedItemPosition();
							int index;
							while ((index = GetNextSelectedItem(pos)) >= 0)
							{
								FileEntry * e = GetListEntry(index);
								e->changelist.Empty();
								SetItemGroup(index, 0);
							}
							// TODO: Should we go through all entries here and check if we also could
							// remove the changeset from m_changelists ?
						}
						else
						{
							CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
					}
					break;
				case IDSVNLC_CREATECS:
					{
						CTSVNPathList changelistItems;
						FillListOfSelectedItemPaths(changelistItems);
						CCreateChangelistDlg dlg;
						if (dlg.DoModal() == IDOK)
						{
							SVN svn;
							if (svn.AddToChangeList(changelistItems, dlg.m_sName))
							{
								// The changelists were removed, but we now need to run through the selected items again
								// and update their changelist
								PrepareGroups(true);
								if (m_changelists.size() == 0)
								{
									// there are no groups defined yet.
									// before we can add our new group, we must assign all entries
									// to the null-group (not assigned to a changeset)
									for (int ii = 0; ii < GetItemCount(); ++ii)
										SetItemGroup(ii, 0);
								}
								TCHAR groupname[1024];
								LVGROUP grp = {0};
								grp.cbSize = sizeof(LVGROUP);
								grp.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
								_tcsncpy_s(groupname, 1024, dlg.m_sName, 1023);
								grp.pszHeader = groupname;
								grp.iGroupId = m_changelists.size()+1;
								grp.uAlign = LVGA_HEADER_LEFT;
								m_changelists[dlg.m_sName] = InsertGroup(-1, &grp);
								
								POSITION pos = GetFirstSelectedItemPosition();
								int index;
								while ((index = GetNextSelectedItem(pos)) >= 0)
								{
									FileEntry * e = GetListEntry(index);
									e->changelist = dlg.m_sName;
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
						for (std::map<CString, int>::const_iterator it = m_changelists.begin(); it != m_changelists.end(); ++it)
						{
							if (cmd == cmdID)
							{
								sChangelist = it->first;
							}
							cmdID++;
						}
						if (!sChangelist.IsEmpty())
						{
							SVN svn;
							if (svn.AddToChangeList(changelistItems, sChangelist))
							{
								// The changelists were moved, but we now need to run through the selected items again
								// and update their changelist
								POSITION pos = GetFirstSelectedItemPosition();
								int index;
								while ((index = GetNextSelectedItem(pos)) >= 0)
								{
									FileEntry * e = GetListEntry(index);
									e->changelist = sChangelist;
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
			} // if (popup.CreatePopupMenu())
		} // if (selIndex >= 0)
	} // if (pWnd == this)
	else if (pWnd == GetHeaderCtrl())
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
		if (popup.CreatePopupMenu())
		{
			CString temp;
			int nCol = 1;
			UINT uCheckedFlags = MF_STRING | MF_ENABLED | MF_CHECKED;
			UINT uUnCheckedFlags = MF_STRING | MF_ENABLED;
			if (XPorLater)
			{
				temp.LoadString(IDS_STATUSLIST_SHOWGROUPS);
				popup.AppendMenu(IsGroupViewEnabled() ? uCheckedFlags : uUnCheckedFlags, SVNSLC_NUMCOLUMNS, temp);
				popup.AppendMenu(MF_SEPARATOR);
			}
			temp.LoadString(IDS_STATUSLIST_COLEXT);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLSTATUS);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLREMOTESTATUS);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLTEXTSTATUS);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLPROPSTATUS);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLREMOTETEXTSTATUS);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLREMOTEPROPSTATUS);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLURL);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLLOCK);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLLOCKCOMMENT);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLAUTHOR);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLREVISION);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLDATE);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLSVNLOCK);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLCOPYFROM);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);
			temp.LoadString(IDS_STATUSLIST_COLMODIFICATIONDATE);
			popup.AppendMenu(m_ColumnShown[nCol] ? uCheckedFlags : uUnCheckedFlags, nCol++, temp);

			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			if ((cmd >= 1)&&(cmd < SVNSLC_NUMCOLUMNS))
			{
				if (m_ColumnShown[cmd])
				{
					HideColumn(cmd);
				}
				else
				{
					ShowColumn(cmd);
				}
			}
			if (cmd == SVNSLC_NUMCOLUMNS)
			{
				EnableGroupView(!IsGroupViewEnabled());
			}
		}
	}
}

void CSVNStatusListCtrl::ShowColumn(int col)
{
	m_ColumnShown[col] = 1;
	SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);

	// restore position
	int arr[SVNSLC_NUMCOLUMNS] = {0};
	int arr2[SVNSLC_NUMCOLUMNS] = {0};
	GetColumnOrderArray(arr, SVNSLC_NUMCOLUMNS);
	int arrIndex = 0;
	for (int i = 0; i < SVNSLC_NUMCOLUMNS; i++) 
	{
		if (m_ColumnShown[arr[i]] == 0)
		{
			arr2[arrIndex++] = arr[i];
		}
	}
	bool bInserted = false;
	for (int i = 0; i < SVNSLC_NUMCOLUMNS; i++) 
	{
		if (m_ColumnShown[arr[i]])
		{
			if ((arr[i]>col)&&(!bInserted))
			{
				arr2[arrIndex++] = col;
				bInserted = true;
			}
			if (arr[i] != col)
				arr2[arrIndex++] = arr[i];
		}
	}
	if ( !bInserted )
	{
		arr2[arrIndex++] = col;
	}

	SetColumnOrderArray(SVNSLC_NUMCOLUMNS, arr2);


	DWORD dwColumns = 0;
	for (int i=0; i<SVNSLC_NUMCOLUMNS; ++i)
	{
		if (m_ColumnShown[i])
			dwColumns |= (1<<i);
	}
	m_dwColumns = dwColumns;
}

void CSVNStatusListCtrl::HideColumn(int col)
{
	SetColumnWidth(col,0);
	m_ColumnShown[col] = 0;
	// move to front of list
	int arr[SVNSLC_NUMCOLUMNS] = {0};
	int arr2[SVNSLC_NUMCOLUMNS] = {0};
	GetColumnOrderArray(arr, SVNSLC_NUMCOLUMNS);
	int arrIndex = 0;
	for (int i = 0; i < SVNSLC_NUMCOLUMNS; i++) 
	{
		if (m_ColumnShown[arr[i]] == 0)
		{
			arr2[arrIndex++] = arr[i];
		}
	}
	for (int i = 0; i < SVNSLC_NUMCOLUMNS; i++) 
	{
		if (m_ColumnShown[arr[i]])
		{
			arr2[arrIndex++] = arr[i];
		}
	}

	SetColumnOrderArray(SVNSLC_NUMCOLUMNS, arr2);

	DWORD dwColumns = 0;
	for (int i=0; i<SVNSLC_NUMCOLUMNS; ++i)
	{
		if (m_ColumnShown[i])
			dwColumns |= (1<<i);
	}
	m_dwColumns = dwColumns;
}

void CSVNStatusListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	Locker lock(m_critSec);
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (m_bBlock)
		return;
	if (pNMLV->iItem < 0)
	{
		if (!IsGroupViewEnabled())
			return;
		POINT pt;
		GetCursorPos(&pt);
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
			ZeroMemory(&lv, sizeof(LVITEM));
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
			SVNDiff::StartConflictEditor(entry->GetPath());
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
	if ((entry->status == svn_wc_status_normal)&&(entry->remotestatus <= svn_wc_status_normal))
		return;		// normal files won't show anything interesting in a diff
	if (entry->status == svn_wc_status_unversioned)
		return;		// we don't compare new files with nothing

	SVNDiff diff(NULL, m_hWnd, true);
	diff.DiffWCFile(entry->path, entry->textstatus, entry->propstatus, entry->remotetextstatus, entry->remotepropstatus);
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
	sStats.Format(IDS_COMMITDLG_STATISTICSFORMAT, m_nSelected, GetItemCount());
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

void CSVNStatusListCtrl::SelectAll(bool bSelect)
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
		SetEntryCheck(entry,i,bSelect);
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

				if (entry->isConflicted)
					crText = m_Colors.GetColor(CColors::Conflict);

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
		if (rect.PtInRect(point))
		{
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
	}
	return -1;
}

BOOL CSVNStatusListCtrl::OnToolTipText(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText;
	UINT nID = pNMHDR->idFrom;

	if (nID == 0)
		return FALSE;

	int row = ((nID-1) >> 10) & 0x3fffff;
	int col = (nID-1) & 0x3ff;

	if (col == 0)
		return FALSE;	// no custom tooltip for the path, we use the infotip there!

	// get the internal column from the visible columns
	int internalcol = 0;
	int currentcol = 0;
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLEXT)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLSTATUS)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLREMOTESTATUS)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLTEXTSTATUS)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLPROPSTATUS)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLREMOTETEXT)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLREMOTEPROP)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLURL)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLLOCK)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLLOCKCOMMENT)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLSVNNEEDSLOCK)
			currentcol++;
	}
	if (currentcol != col)
	{
		internalcol++;
		if (m_dwColumns & SVNSLC_COLMODIFICATIONDATE)
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
				url.Format(IDS_STATUSLIST_COPYFROM, (LPCTSTR)CPathUtils::PathUnescape(fentry->copyfrom_url), (LONG)fentry->copyfrom_rev);
				lstrcpyn(pTTTW->szText, url, 80);
				return TRUE;
			}
			if (fentry->switched)
			{
				CString url;
				url.Format(IDS_STATUSLIST_SWITCHEDTO, CPathUtils::PathUnescape(fentry->url));
				lstrcpyn(pTTTW->szText, url, 80);
				return TRUE;
			}
			if (fentry->keeplocal)
			{
				lstrcpyn(pTTTW->szText, CString(MAKEINTRESOURCE(IDS_STATUSLIST_KEEPLOCAL)), 80);
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
			CMemDC memDC(pDC, &rc);

			memDC.SetTextColor(clrText);
			memDC.SetBkColor(clrTextBk);
			memDC.FillSolidRect(rc, clrTextBk);
			rc.top += 10;
			CGdiObject * oldfont = memDC.SelectStockObject(ANSI_VAR_FONT);
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
	if (m_ColumnShown[phdr->iItem])
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
	if ((phdr->iItem < 0)||(phdr->iItem >= SVNSLC_NUMCOLUMNS))
		return;
	if (m_ColumnShown[phdr->iItem])
	{
		return;
	}
	*pResult = 1;
}

bool CSVNStatusListCtrl::StringToOrderArray(const CString& OrderString, int OrderArray[])
{
	TCHAR * endchar;
	for (int i=0; i<SVNSLC_NUMCOLUMNS; ++i)
	{
		CString hex = OrderString.Mid(i*2, 2);
		if ( hex.IsEmpty() )
		{
			// This case only occurs when upgrading from an older
			// TSVN version in which there were fewer columns.
			OrderArray[i] = i;
		}
		else
		{
			OrderArray[i] = _tcstol(hex, &endchar, 16);
		}
	}
	return true;
}

CString CSVNStatusListCtrl::OrderArrayToString(int OrderArray[])
{
	CString sResult;
	TCHAR buf[5];
	for (int i=0; i<SVNSLC_NUMCOLUMNS; ++i)
	{
		_stprintf_s(buf, 5, _T("%02X"), OrderArray[i]);
		sResult += buf;
	}
	return sResult;
}

bool CSVNStatusListCtrl::StringToWidthArray(const CString& WidthString, int WidthArray[])
{
	TCHAR * endchar;
	for (int i=0; i<SVNSLC_NUMCOLUMNS; ++i)
	{
		CString hex = WidthString.Mid(i*8, 8);
		if ( hex.IsEmpty() )
		{
			// This case only occurs when upgrading from an older
			// TSVN version in which there were fewer columns.
			WidthArray[i] = 0;
		}
		else
		{
			WidthArray[i] = _tcstol(hex, &endchar, 16);
		}
	}
	return true;
}

CString CSVNStatusListCtrl::WidthArrayToString(int WidthArray[])
{
	CString sResult;
	TCHAR buf[10];
	for (int i=0; i<SVNSLC_NUMCOLUMNS; ++i)
	{
		_stprintf_s(buf, 10, _T("%08X"), WidthArray[i]);
		sResult += buf;
	}
	return sResult;
}

void CSVNStatusListCtrl::OnDestroy()
{
	SaveColumnWidths(true);
	CListCtrl::OnDestroy();
}

void CSVNStatusListCtrl::OnBeginDrag(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	Locker lock(m_critSec);
	CDropFiles dropFiles; // class for creating DROPFILES struct

	int index;
	POSITION pos = GetFirstSelectedItemPosition();
	while ( (index = GetNextSelectedItem(pos)) >= 0 )
	{
		FileEntry * fentry = m_arStatusArray[m_arListArray[index]];
		CTSVNPath path = fentry->GetPath();
		dropFiles.AddFile( path.GetWinPathString() );
	}

	if ( dropFiles.GetCount()>0 )
	{
		dropFiles.CreateStructure();
	}

	*pResult = 0;
}

void CSVNStatusListCtrl::SaveColumnWidths(bool bSaveToRegistry /* = false */)
{
	if (bSaveToRegistry)
	{
		int arr[SVNSLC_NUMCOLUMNS];
		GetColumnOrderArray(arr, SVNSLC_NUMCOLUMNS);
		CString sOrder = OrderArrayToString(arr);
		CRegString regColOrder(_T("Software\\TortoiseSVN\\StatusColumns\\")+m_sColumnInfoContainer+_T("_Order"));
		regColOrder = sOrder;
	}

	CRegString regColWidth(_T("Software\\TortoiseSVN\\StatusColumns\\")+m_sColumnInfoContainer+_T("_Width"));
	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
	{
		m_arColumnWidths[col] = GetColumnWidth(col);
	}
	if (bSaveToRegistry)
	{
		CString sWidths = WidthArrayToString(m_arColumnWidths);
		regColWidth = sWidths;

		CRegDWORD regColInfo(_T("Software\\TortoiseSVN\\StatusColumns\\")+m_sColumnInfoContainer);
		regColInfo = m_dwColumns;
	}
}

bool CSVNStatusListCtrl::EnableFileDrop()
{
	if (m_pDropTarget)
		return false;
	m_pDropTarget = new CSVNStatusListCtrlDropTarget(m_hWnd);
	RegisterDragDrop(m_hWnd,m_pDropTarget);
	// create the supported formats:
	FORMATETC ftetc={0};
	ftetc.dwAspect = DVASPECT_CONTENT;
	ftetc.lindex = -1;
	ftetc.tymed = TYMED_HGLOBAL;
	ftetc.cfFormat=CF_HDROP;
	m_pDropTarget->AddSuportedFormat(ftetc);
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
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					// copy all selected paths to the clipboard
					if (GetAsyncKeyState(VK_SHIFT)&0x8000)
						CopySelectedEntriesToClipboard(SVNSLC_COLSTATUS|SVNSLC_COLURL);
					else
						CopySelectedEntriesToClipboard(0);
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
	WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());

	CString sClipboard;
	CString temp;
	TCHAR buf[100];
	if (GetSelectedCount() == 0)
		return false;
	// first add the column titles as the first line
	temp.LoadString(IDS_STATUSLIST_COLFILE);
	sClipboard = temp;
	if (dwCols & SVNSLC_COLEXT)
	{
		temp.LoadString(IDS_STATUSLIST_COLEXT);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLSTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLSTATUS);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLTEXTSTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLTEXTSTATUS);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLREMOTESTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLREMOTESTATUS);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLPROPSTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLPROPSTATUS);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLREMOTETEXT)
	{
		temp.LoadString(IDS_STATUSLIST_COLREMOTETEXTSTATUS);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLREMOTEPROP)
	{
		temp.LoadString(IDS_STATUSLIST_COLREMOTEPROPSTATUS);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLURL)
	{
		temp.LoadString(IDS_STATUSLIST_COLURL);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLLOCK)
	{
		temp.LoadString(IDS_STATUSLIST_COLLOCK);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLLOCKCOMMENT)
	{
		temp.LoadString(IDS_STATUSLIST_COLLOCKCOMMENT);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLAUTHOR)
	{
		temp.LoadString(IDS_STATUSLIST_COLAUTHOR);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLREVISION)
	{
		temp.LoadString(IDS_STATUSLIST_COLREVISION);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLDATE)
	{
		temp.LoadString(IDS_STATUSLIST_COLDATE);
		sClipboard += _T("\t")+temp;
	}
	if (dwCols & SVNSLC_COLCOPYFROM)
	{
		temp.LoadString(IDS_STATUSLIST_COLCOPYFROM);
		sClipboard += _T("\t")+temp;
	}
	sClipboard += _T("\r\n");

	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		FileEntry * entry = GetListEntry(index);
		sClipboard += entry->GetDisplayName();
		if (dwCols & SVNSLC_COLEXT)
		{
			sClipboard += _T("\t")+entry->path.GetFileExtension();
		}
		if (dwCols & SVNSLC_COLSTATUS)
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
		if (dwCols & SVNSLC_COLTEXTSTATUS)
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
		if (dwCols & SVNSLC_COLREMOTESTATUS)
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
		if (dwCols & SVNSLC_COLPROPSTATUS)
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
		if (dwCols & SVNSLC_COLREMOTETEXT)
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
		if (dwCols & SVNSLC_COLREMOTEPROP)
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
		if (dwCols & SVNSLC_COLURL)
			sClipboard += _T("\t")+entry->url;
		if (dwCols & SVNSLC_COLLOCK)
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
					temp.Format(IDS_STATUSLIST_LOCKSTOLEN, entry->lock_remoteowner);
				}
			}
			else
				temp = entry->lock_owner;
			sClipboard += _T("\t")+temp;
		}
		if (dwCols & SVNSLC_COLLOCKCOMMENT)
			sClipboard += _T("\t")+entry->lock_comment;
		if (dwCols & SVNSLC_COLAUTHOR)
			sClipboard += _T("\t")+entry->last_commit_author;
		if (dwCols & SVNSLC_COLREVISION)
		{
			temp.Format(_T("%ld"), entry->last_commit_rev);
			if (entry->last_commit_rev == 0)
				temp.Empty();
			sClipboard += _T("\t")+temp;
		}
		if (dwCols & SVNSLC_COLDATE)
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
		if (dwCols & SVNSLC_COLCOPYFROM)
		{
			if (m_sURL.Compare(entry->copyfrom_url.Left(m_sURL.GetLength()))==0)
				temp = entry->copyfrom_url.Mid(m_sURL.GetLength());
			else
				temp = entry->copyfrom_url;
			sClipboard += _T("\t")+temp;
		}
		sClipboard += _T("\r\n");
	}

	return CStringUtils::WriteAsciiStringToClipboard(CStringA(sClipboard));
}

int CSVNStatusListCtrl::GetNumberOfChangelistsInSelection()
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
	if ((m_dwContextMenus & SVNSLC_POPCHANGELISTS) == NULL)
		return false;	// don't show groups

	bool bHasGroups = (m_changelists.size() > 0)||(bForce);
	RemoveAllGroups();
	EnableGroupView(bHasGroups);

	TCHAR groupname[1024];

	LVGROUP grp = {0};
	grp.cbSize = sizeof(LVGROUP);
	grp.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
	CString sUnassignedName(MAKEINTRESOURCE(IDS_STATUSLIST_UNASSIGNED_CHANGESET));
	_tcsncpy_s(groupname, 1024, (LPCTSTR)sUnassignedName, 1023);
	grp.pszHeader = groupname;
	grp.iGroupId = 0;
	grp.uAlign = LVGA_HEADER_LEFT;
	InsertGroup(0, &grp);

	// add a new group for each changelist
	int groupindex = 1;
	for (std::map<CString,int>::iterator it = m_changelists.begin(); it != m_changelists.end(); ++it)
	{
		LVGROUP grp = {0};
		grp.cbSize = sizeof(LVGROUP);
		grp.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
		_tcsncpy_s(groupname, 1024, it->first, 1023);
		grp.pszHeader = groupname;
		grp.iGroupId = groupindex;
		grp.uAlign = LVGA_HEADER_LEFT;
		it->second = InsertGroup(groupindex++, &grp);
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
