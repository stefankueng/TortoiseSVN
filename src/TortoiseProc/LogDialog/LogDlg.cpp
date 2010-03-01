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
#include "TortoiseProc.h"
#include "cursor.h"
#include "InputDlg.h"
#include "PropDlg.h"
#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
#include "RepositoryBrowser.h"
#include "CopyDlg.h"
#include "StatGraphDlg.h"
#include "Logdlg.h"
#include "MessageBox.h"
#include "Registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "TempFile.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include "IconMenu.h"
#include "RevisionRangeDlg.h"
#include "BrowseFolder.h"
#include "BlameDlg.h"
#include "Blame.h"
#include "SVNHelpers.h"
#include "SVNStatus.h"
#include "LogDlgHelper.h"
#include "CachedLogInfo.h"
#include "RepositoryInfo.h"
#include "EditPropertiesDlg.h"
#include "LogCacheSettings.h"
#include "SysInfo.h"
#include "SysImageList.h"
#include "svn_props.h"
#include "AsyncCall.h"
#include "svntrace.h"
#include "LogDlgFilter.h"

#if (NTDDI_VERSION < NTDDI_LONGHORN)

enum LISTITEMSTATES_MINE {
	LISS_NORMAL = 1,
	LISS_HOT = 2,
	LISS_SELECTED = 3,
	LISS_DISABLED = 4,
	LISS_SELECTEDNOTFOCUS = 5,
	LISS_HOTSELECTED = 6,
};

#define MCS_NOTRAILINGDATES  0x0040
#define MCS_SHORTDAYSOFWEEK  0x0080
#define MCS_NOSELCHANGEONNAV 0x0100

#define DTM_SETMCSTYLE    (DTM_FIRST + 11)

#endif

#define ICONITEMBORDER 5

const UINT CLogDlg::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);


enum LogDlgContextMenuCommands
{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	ID_COMPARE = 1,
	ID_SAVEAS,
	ID_COMPARETWO,
	ID_UPDATE,
	ID_COPY,
	ID_REVERTREV,
	ID_MERGEREV,
	ID_GNUDIFF1,
	ID_GNUDIFF2,
	ID_FINDENTRY,
	ID_OPEN,
	ID_BLAME,
	ID_REPOBROWSE,
	ID_LOG,
	ID_POPPROPS,
	ID_EDITAUTHOR,
	ID_EDITLOG,
	ID_DIFF,
	ID_OPENWITH,
	ID_COPYCLIPBOARD,
	ID_CHECKOUT,
	ID_REVERTTOREV,
	ID_BLAMECOMPARE,
	ID_BLAMETWO,
	ID_BLAMEDIFF,
	ID_VIEWREV,
	ID_VIEWPATHREV,
	ID_EXPORT,
	ID_COMPAREWITHPREVIOUS,
	ID_BLAMEWITHPREVIOUS,
	ID_GETMERGELOGS,
	ID_REVPROPS
};


IMPLEMENT_DYNAMIC(CLogDlg, CResizableStandAloneDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CLogDlg::IDD, pParent)
	, m_nSearchIndex(0)
	, m_wParam(0)
	, m_nSelectedFilter(LOGFILTER_ALL)
	, m_nSortColumn(0)
	, m_bShowedAll(false)
	, m_bSelect(false)
	, m_regLastStrict(_T("Software\\TortoiseSVN\\LastLogStrict"), FALSE)
	, m_regMaxBugIDColWidth(_T("Software\\TortoiseSVN\\MaxBugIDColWidth"), 200)
	, m_bSelectionMustBeContinuous(false)
	, m_bShowBugtraqColumn(false)
	, m_bStrictStopped(false)
    , m_bSingleRevision(true)
	, m_sLogInfo(_T(""))
	, m_pFindDialog(NULL)
	, m_bCancelled(FALSE)
	, m_pNotifyWindow(NULL)
	, m_bLogThreadRunning(FALSE)
	, m_bAscending(FALSE)
	, m_pStoreSelection(NULL)
	, m_limit(0)
	, m_bIncludeMerges(FALSE)
	, m_hAccel(NULL)
	, m_bRefresh(false)
	, netScheduler(1, 0, true)
	, diskScheduler(1, 0, true)
	, m_pLogListAccServer(NULL)
	, m_pChangedListAccServer(NULL)
	, m_head(-1)
	, m_nSortColumnPathList(0)
	, m_bAscendingPathList(false)
{
	m_bFilterWithRegex = 
        !!CRegDWORD(_T("Software\\TortoiseSVN\\UseRegexFilter"), FALSE);
    m_bFilterCaseSensitively =  
        !!CRegDWORD(_T("Software\\TortoiseSVN\\FilterCaseSensitively"), FALSE);
}

CLogDlg::~CLogDlg()
{
    // prevent further event processing

	InterlockedExchange(&m_bLogThreadRunning, TRUE);
	m_logEntries.ClearAll();
	DestroyIcon(m_hModifiedIcon);
	DestroyIcon(m_hReplacedIcon);
	DestroyIcon(m_hAddedIcon);
	DestroyIcon(m_hDeletedIcon);
	DestroyIcon(m_hMergedIcon);
	if ( m_pStoreSelection )
	{
		delete m_pStoreSelection;
		m_pStoreSelection = NULL;
	}
	if (m_boldFont)
		DeleteObject(m_boldFont);
	if (m_pLogListAccServer)
		m_pLogListAccServer->Release();
	if (m_pChangedListAccServer)
		m_pChangedListAccServer->Release();
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGLIST, m_LogList);
	DDX_Control(pDX, IDC_LOGMSG, m_ChangedFileListCtrl);
	DDX_Control(pDX, IDC_PROGRESS, m_LogProgress);
	DDX_Control(pDX, IDC_SPLITTERTOP, m_wndSplitter1);
	DDX_Control(pDX, IDC_SPLITTERBOTTOM, m_wndSplitter2);
	DDX_Check(pDX, IDC_CHECK_STOPONCOPY, m_bStrict);
	DDX_Text(pDX, IDC_SEARCHEDIT, m_sFilterText);
	DDX_Control(pDX, IDC_DATEFROM, m_DateFrom);
	DDX_Control(pDX, IDC_DATETO, m_DateTo);
	DDX_Control(pDX, IDC_SHOWPATHS, m_cShowPaths);
	DDX_Control(pDX, IDC_GETALL, m_btnShow);
	DDX_Text(pDX, IDC_LOGINFO, m_sLogInfo);
	DDX_Check(pDX, IDC_INCLUDEMERGE, m_bIncludeMerges);
	DDX_Control(pDX, IDC_SEARCHEDIT, m_cFilter);
}

BEGIN_MESSAGE_MAP(CLogDlg, CResizableStandAloneDialog)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage) 
	ON_BN_CLICKED(IDC_GETALL, OnBnClickedGetall)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGMSG, OnNMDblclkChangedFileList)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGLIST, OnNMDblclkLoglist)
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOGLIST, OnLvnItemchangedLoglist)
	ON_NOTIFY(EN_LINK, IDC_MSGVIEW, OnEnLinkMsgview)
	ON_BN_CLICKED(IDC_STATBUTTON, OnBnClickedStatbutton)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LOGLIST, OnNMCustomdrawLoglist)
	ON_REGISTERED_MESSAGE(CFilterEdit::WM_FILTEREDIT_INFOCLICKED, OnClickedInfoIcon)
	ON_REGISTERED_MESSAGE(CFilterEdit::WM_FILTEREDIT_CANCELCLICKED, OnClickedCancelFilter)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LOGLIST, OnLvnGetdispinfoLoglist)
	ON_EN_CHANGE(IDC_SEARCHEDIT, OnEnChangeSearchedit)
	ON_WM_TIMER()
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETO, OnDtnDatetimechangeDateto)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATEFROM, OnDtnDatetimechangeDatefrom)
	ON_BN_CLICKED(IDC_NEXTHUNDRED, OnBnClickedNexthundred)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LOGMSG, OnNMCustomdrawChangedFileList)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LOGMSG, OnLvnGetdispinfoChangedFileList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LOGLIST, OnLvnColumnclick)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LOGMSG, OnLvnColumnclickChangedFileList)
	ON_BN_CLICKED(IDC_SHOWPATHS, OnBnClickedHidepaths)
	ON_NOTIFY(LVN_ODFINDITEM, IDC_LOGLIST, OnLvnOdfinditemLoglist)
	ON_BN_CLICKED(IDC_CHECK_STOPONCOPY, &CLogDlg::OnBnClickedCheckStoponcopy)
	ON_NOTIFY(DTN_DROPDOWN, IDC_DATEFROM, &CLogDlg::OnDtnDropdownDatefrom)
	ON_NOTIFY(DTN_DROPDOWN, IDC_DATETO, &CLogDlg::OnDtnDropdownDateto)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_INCLUDEMERGE, &CLogDlg::OnBnClickedIncludemerge)
	ON_BN_CLICKED(IDC_REFRESH, &CLogDlg::OnBnClickedRefresh)
	ON_COMMAND(ID_LOGDLG_REFRESH,&CLogDlg::OnRefresh)
	ON_COMMAND(ID_LOGDLG_FIND,&CLogDlg::OnFind)
	ON_COMMAND(ID_LOGDLG_FOCUSFILTER,&CLogDlg::OnFocusFilter)
	ON_COMMAND(ID_EDIT_COPY, &CLogDlg::OnEditCopy)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LOGLIST, &CLogDlg::OnLvnKeydownLoglist)
	ON_NOTIFY(NM_CLICK, IDC_LOGLIST, &CLogDlg::OnNMClickLoglist)
END_MESSAGE_MAP()

void CLogDlg::SetParams(const CTSVNPath& path, SVNRev pegrev, SVNRev startrev, SVNRev endrev, int limit, BOOL bStrict /* = FALSE */, BOOL bSaveStrict /* = TRUE */)
{
	m_path = path;
	m_pegrev = pegrev;
	m_startrev = startrev;
	m_LogRevision = startrev;
	m_endrev = endrev;
	m_hasWC = !path.IsUrl();
	m_bStrict = bStrict;
	m_bSaveStrict = bSaveStrict;
	m_limit = limit;
	if (::IsWindow(m_hWnd))
		UpdateData(FALSE);
}

void CLogDlg::SetFilter(const CString& findstr, LONG findtype, bool findregex)
{
	m_sFilterText = findstr;
	if (findtype)
		m_nSelectedFilter = findtype;
	m_bFilterWithRegex = findregex;
}

BOOL CLogDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	ExtendFrameIntoClientArea(IDC_LOGMSG, 0, IDC_LOGMSG, IDC_LOGMSG);
	m_aeroControls.SubclassControl(this, IDC_LOGINFO);
	m_aeroControls.SubclassControl(this, IDC_SHOWPATHS);
	m_aeroControls.SubclassControl(this, IDC_STATBUTTON);
	m_aeroControls.SubclassControl(this, IDC_CHECK_STOPONCOPY);
	m_aeroControls.SubclassControl(this, IDC_INCLUDEMERGE);
	m_aeroControls.SubclassControl(this, IDC_PROGRESS);
	m_aeroControls.SubclassControl(this, IDC_GETALL);
	m_aeroControls.SubclassControl(this, IDC_NEXTHUNDRED);
	m_aeroControls.SubclassControl(this, IDC_REFRESH);
	m_aeroControls.SubclassOkCancelHelp(this);

	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);

	// use the default GUI font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)
	HFONT hFont = (HFONT)m_LogList.SendMessage(WM_GETFONT);
	LOGFONT lf = {0};
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont = CreateFontIndirect(&lf);

	EnableToolTips();
	m_LogList.SetTooltipProvider(this);

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_LOGDLG));

	// use the state of the "stop on copy/rename" option from the last time
	if (!m_bStrict)
		m_bStrict = m_regLastStrict;
	UpdateData(FALSE);
	CString temp;
	if (m_limit)
		temp.Format(IDS_LOG_SHOWNEXT, m_limit);
	else
		temp.Format(IDS_LOG_SHOWNEXT, (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100));

	SetDlgItemText(IDC_NEXTHUNDRED, temp);

	// set the font to use in the log message view, configured in the settings dialog
	CAppUtils::CreateFontForLogs(m_logFont);
	GetDlgItem(IDC_MSGVIEW)->SetFont(&m_logFont);
	// automatically detect URLs in the log message and turn them into links
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_AUTOURLDETECT, TRUE, NULL);
	// make the log message rich edit control send a message when the mouse pointer is over a link
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_SETEVENTMASK, NULL, ENM_LINK);
	DWORD dwStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES;

	// we *could* enable checkboxes on pre Vista OS too, but those don't have
	// the LVS_EX_AUTOCHECKSELECT style. Without that style, users could get
	// very confused because selected items are not checked.
	// Also, while handling checkboxes is implemented, most code paths in this
	// file still only work on the selected items, not the checked ones.
	if (m_bSelect && SysInfo::Instance().IsVistaOrLater())
		dwStyle |= LVS_EX_CHECKBOXES | 0x08000000 /*LVS_EX_AUTOCHECKSELECT*/;
	m_LogList.SetExtendedStyle(dwStyle);

	int checkState = (int)DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\LogShowPaths"), BST_UNCHECKED));
	m_cShowPaths.SetCheck(checkState);

	// load the icons for the action columns
	m_hModifiedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hAddedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hMergedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMERGED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	// if there is a working copy, load the project properties
	// to get information about the bugtraq: integration
	if (m_hasWC)
		m_ProjectProperties.ReadProps(m_path);

	// the bugtraq issue id column is only shown if the bugtraq:url or bugtraq:regex is set
	if ((!m_ProjectProperties.sUrl.IsEmpty())||(!m_ProjectProperties.GetCheckRe().IsEmpty()))
		m_bShowBugtraqColumn = true;

	theme.SetWindowTheme(m_LogList.GetSafeHwnd(), L"Explorer", NULL);
	theme.SetWindowTheme(m_ChangedFileListCtrl.GetSafeHwnd(), L"Explorer", NULL);

	// set up the columns
	int c = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_LogList.DeleteColumn(c--);
	temp.LoadString(IDS_LOG_REVISION);
	m_LogList.InsertColumn(0, temp);
	
	// make the revision column right aligned
	LVCOLUMN Column;
	Column.mask = LVCF_FMT;
	Column.fmt = LVCFMT_RIGHT;
	m_LogList.SetColumn(0, &Column); 
	
	temp.LoadString(IDS_LOG_ACTIONS);
	m_LogList.InsertColumn(1, temp);
	temp.LoadString(IDS_LOG_AUTHOR);
	m_LogList.InsertColumn(2, temp);
	temp.LoadString(IDS_LOG_DATE);
	m_LogList.InsertColumn(3, temp);
	if (m_bShowBugtraqColumn)
	{
		temp = m_ProjectProperties.sLabel;
		if (temp.IsEmpty())
			temp.LoadString(IDS_LOG_BUGIDS);
		m_LogList.InsertColumn(4, temp);
	}
	temp.LoadString(IDS_LOG_MESSAGE);
	m_LogList.InsertColumn(m_bShowBugtraqColumn ? 5 : 4, temp);
	m_LogList.SetRedraw(false);
	ResizeAllListCtrlCols(true);
	m_LogList.SetRedraw(true);

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	m_ChangedFileListCtrl.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
	m_ChangedFileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	c = ((CHeaderCtrl*)(m_ChangedFileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ChangedFileListCtrl.DeleteColumn(c--);
	temp.LoadString(IDS_PROGRS_PATH);
	m_ChangedFileListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGRS_ACTION);
	m_ChangedFileListCtrl.InsertColumn(1, temp);
	temp.LoadString(IDS_LOG_COPYFROM);
	m_ChangedFileListCtrl.InsertColumn(2, temp);
	temp.LoadString(IDS_LOG_REVISION);
	m_ChangedFileListCtrl.InsertColumn(3, temp);
	m_ChangedFileListCtrl.SetRedraw(false);
	CAppUtils::ResizeAllListCtrlCols(&m_ChangedFileListCtrl);
	m_ChangedFileListCtrl.SetRedraw(true);


	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
	m_sMessageBuf.Preallocate(100000);

	// set the dialog title to "Log - path/to/whatever/we/show/the/log/for"
	SetDlgTitle(false);

	m_tooltips.Create(this);
	CheckRegexpTooltip();

	SetSplitterRange();
	
	// the filter control has a 'cancel' button (the red 'X'), we need to load its bitmap
	m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED);
	m_cFilter.SetInfoIcon(IDI_LOGFILTER);
	m_cFilter.SetValidator(this);
	m_cFilter.SetWindowText(m_sFilterText);

	AdjustControlSize(IDC_SHOWPATHS);
	AdjustControlSize(IDC_CHECK_STOPONCOPY);
	AdjustControlSize(IDC_INCLUDEMERGE);

	GetClientRect(m_DlgOrigRect);
	m_LogList.GetClientRect(m_LogListOrigRect);
	GetDlgItem(IDC_MSGVIEW)->GetClientRect(m_MsgViewOrigRect);
	m_ChangedFileListCtrl.GetClientRect(m_ChgOrigRect);

	m_DateFrom.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);
	m_DateTo.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);

	// resizable stuff
	AddAnchor(IDC_FROMLABEL, TOP_LEFT);
	AddAnchor(IDC_DATEFROM, TOP_LEFT);
	AddAnchor(IDC_TOLABEL, TOP_LEFT);
	AddAnchor(IDC_DATETO, TOP_LEFT);

	SetFilterCueText();
	AddAnchor(IDC_SEARCHEDIT, TOP_LEFT, TOP_RIGHT);
	
	AddMainAnchors();

	AddAnchor(IDC_LOGINFO, BOTTOM_LEFT, BOTTOM_RIGHT);	
	AddAnchor(IDC_SHOWPATHS, BOTTOM_LEFT);	
	AddAnchor(IDC_CHECK_STOPONCOPY, BOTTOM_LEFT);
	AddAnchor(IDC_INCLUDEMERGE, BOTTOM_LEFT);
	AddAnchor(IDC_GETALL, BOTTOM_LEFT);
	AddAnchor(IDC_NEXTHUNDRED, BOTTOM_LEFT);
	AddAnchor(IDC_REFRESH, BOTTOM_LEFT);
	AddAnchor(IDC_STATBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	SetPromptParentWindow(m_hWnd);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("LogDlg"));

	DWORD yPos1 = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1"));
	DWORD yPos2 = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2"));
	RECT rcDlg, rcLogList, rcChgMsg;
	GetClientRect(&rcDlg);
	m_LogList.GetWindowRect(&rcLogList);
	ScreenToClient(&rcLogList);
	m_ChangedFileListCtrl.GetWindowRect(&rcChgMsg);
	ScreenToClient(&rcChgMsg);
	if (yPos1)
	{
		RECT rectSplitter;
		m_wndSplitter1.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos1 - rectSplitter.top;

		if ((rcLogList.bottom + delta > rcLogList.top)&&(rcLogList.bottom + delta < rcChgMsg.bottom - 30))
		{
			m_wndSplitter1.SetWindowPos(NULL, rectSplitter.left, yPos1, 0, 0, SWP_NOSIZE);
			DoSizeV1(delta);
		}
	}
	if (yPos2)
	{
		RECT rectSplitter;
		m_wndSplitter2.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos2 - rectSplitter.top;

		if ((rcChgMsg.top + delta < rcChgMsg.bottom)&&(rcChgMsg.top + delta > rcLogList.top + 30))
		{
			m_wndSplitter2.SetWindowPos(NULL, rectSplitter.left, yPos2, 0, 0, SWP_NOSIZE);
			DoSizeV2(delta);
		}
	}

	
	if (m_bSelect)
	{
		// the dialog is used to select revisions
		if (m_bSelectionMustBeContinuous)
			DialogEnableWindow(IDOK, (m_LogList.GetSelectedCount()!=0)&&(IsSelectionContinuous()));
		else
			DialogEnableWindow(IDOK, m_LogList.GetSelectedCount()!=0);
	}
	else
	{
		// the dialog is used to just view log messages
		GetDlgItemText(IDOK, temp);
		SetDlgItemText(IDCANCEL, temp);
		GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
	}
	
	// set the choices for the "Show All" button
	temp.LoadString(IDS_LOG_SHOWALL);
	m_btnShow.AddEntry(temp);
	temp.LoadString(IDS_LOG_SHOWRANGE);
	m_btnShow.AddEntry(temp);
	m_btnShow.SetCurrentEntry((LONG)CRegDWORD(_T("Software\\TortoiseSVN\\ShowAllEntry")));

	m_mergedRevs.clear();

	// set up the accessibility callback
	m_pLogListAccServer = ListViewAccServer::CreateProvider(m_LogList.GetSafeHwnd(), this);
	m_pChangedListAccServer = ListViewAccServer::CreateProvider(m_ChangedFileListCtrl.GetSafeHwnd(), this);

	// first start a thread to obtain the log messages without
	// blocking the dialog
	InterlockedExchange(&m_bLogThreadRunning, TRUE);

	new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
	GetDlgItem(IDC_LOGLIST)->SetFocus();
	return FALSE;
}

void CLogDlg::SetDlgTitle(bool bOffline)
{
	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);

	if (bOffline)
	{
		CString sTemp;
		if (m_path.IsUrl())
			sTemp.FormatMessage(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle, (LPCTSTR)m_path.GetUIPathString());
		else if (m_path.IsDirectory())
			sTemp.FormatMessage(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle, (LPCTSTR)m_path.GetWinPathString());
		else
			sTemp.FormatMessage(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle, (LPCTSTR)m_path.GetFilename());
		SetWindowText(sTemp);
	}
	else
	{
		if (m_path.IsUrl())
			SetWindowText(m_sTitle + _T(" - ") + m_path.GetUIPathString());
		else if (m_path.IsDirectory())
			SetWindowText(m_sTitle + _T(" - ") + m_path.GetWinPathString());
		else
			SetWindowText(m_sTitle + _T(" - ") + m_path.GetFilename());
	}
}

void CLogDlg::CheckRegexpTooltip()
{
	CWnd *pWnd = GetDlgItem(IDC_SEARCHEDIT);

    m_tooltips.DelTool(pWnd);
    m_tooltips.AddTool(pWnd, m_bFilterWithRegex ? IDS_LOG_FILTER_REGEX_TT
                                                : IDS_LOG_FILTER_SUBSTRING_TT);
}

void CLogDlg::EnableOKButton()
{
	if (m_bSelect)
	{
		// the dialog is used to select revisions
		if (m_bSelectionMustBeContinuous)
			DialogEnableWindow(IDOK, (m_LogList.GetSelectedCount()!=0)&&(IsSelectionContinuous()));
		else
			DialogEnableWindow(IDOK, m_LogList.GetSelectedCount()!=0);
	}
	else
		DialogEnableWindow(IDOK, TRUE);
}

void CLogDlg::FillLogMessageCtrl(bool bShow /* = true*/)
{
	// we fill here the log message rich edit control,
	// and also populate the changed files list control
	// according to the selected revision(s).

	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	// empty the log message view
	pMsgView->SetWindowText(_T(" "));
	// empty the changed files list
	m_ChangedFileListCtrl.SetRedraw(FALSE);
    m_currentChangedArray.RemoveAll();
	m_ChangedFileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	m_ChangedFileListCtrl.SetItemCountEx(0);

	// if we're not here to really show a selected revision, just
	// get out of here after clearing the views, which is what is intended
	// if that flag is not set.
	if (!bShow)
	{
		// force a redraw
		m_ChangedFileListCtrl.Invalidate();
		m_ChangedFileListCtrl.SetRedraw(TRUE);
		return;
	}

	// depending on how many revisions are selected, we have to do different
	// tasks.
	int selCount = m_LogList.GetSelectedCount();
	if (selCount == 0)
	{
		// if nothing is selected, we have nothing more to do
		m_ChangedFileListCtrl.SetRedraw(TRUE);
		return;
	}
	else if (selCount == 1)
	{
        m_currentChangedPathList.Clear();
        m_bSingleRevision = true;

		// if one revision is selected, we have to fill the log message view
		// with the corresponding log message, and also fill the changed files
		// list fully.
		POSITION pos = m_LogList.GetFirstSelectedItemPosition();
		size_t selIndex = m_LogList.GetNextSelectedItem(pos);
        if (selIndex >= m_logEntries.GetVisibleCount())
		{
			m_ChangedFileListCtrl.SetRedraw(TRUE);
			return;
		}
		m_nSearchIndex = (int)selIndex;
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (selIndex);

		// set the log message text
		pMsgView->SetWindowText(pLogEntry->GetMessage());
		// turn bug ID's into links if the bugtraq: properties have been set
		// and we can find a match of those in the log message
		m_ProjectProperties.FindBugID(pLogEntry->GetMessage(), pMsgView);
		// underline all revisions mentioned in the message
		CAppUtils::UnderlineRegexMatches(pMsgView, m_ProjectProperties.sLogRevRegex, _T("\\d+"));
		if (((DWORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\StyleCommitMessages"), TRUE))==TRUE)
			CAppUtils::FormatTextInRichEditControl(pMsgView);
        m_currentChangedArray = pLogEntry->GetChangedPaths();

        // fill in the changed files list control
		if ((m_cShowPaths.GetState() & 0x0003)==BST_CHECKED)
            m_currentChangedArray.RemoveIrrelevantPaths();
	}
	else
	{
		// more than one revision is selected:
		// the log message view must be emptied
		// the changed files list contains all the changed paths from all
		// selected revisions, with 'doubles' removed

        m_bSingleRevision = false;
        m_currentChangedArray.RemoveAll();
		m_currentChangedPathList = GetChangedPathsFromSelectedRevisions(true);
	}
	
	// sort according to the settings
	if (m_nSortColumnPathList > 0)
    {
        m_currentChangedArray.Sort (m_nSortColumnPathList, m_bAscendingPathList);
        SetSortArrow(&m_ChangedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
    }
	else
		SetSortArrow(&m_ChangedFileListCtrl, -1, false);

	// redraw the views
	if (m_bSingleRevision)
	{
		m_ChangedFileListCtrl.SetItemCountEx((int)m_currentChangedArray.GetCount());
		m_ChangedFileListCtrl.RedrawItems(0, (int)m_currentChangedArray.GetCount());
	}
	else if (m_currentChangedPathList.GetCount())
	{
		m_ChangedFileListCtrl.SetItemCountEx((int)m_currentChangedPathList.GetCount());
		m_ChangedFileListCtrl.RedrawItems(0, (int)m_currentChangedPathList.GetCount());
	}
	else
	{
		m_ChangedFileListCtrl.SetItemCountEx(0);
		m_ChangedFileListCtrl.Invalidate();
	}

    CAppUtils::ResizeAllListCtrlCols(&m_ChangedFileListCtrl);
	m_ChangedFileListCtrl.SetRedraw(TRUE);
}

void CLogDlg::OnBnClickedGetall()
{
	GetAll();
}

void CLogDlg::GetAll(bool bForceAll /* = false */)
{
	// fetch all requested log messages, either the specified range or
	// really *all* available log messages.
	UpdateData();
	INT_PTR entry = m_btnShow.GetCurrentEntry();
	if (bForceAll)
		entry = 0;
	switch (entry)
	{
	case 0:	// show all
		m_endrev = 0;
		m_startrev = m_LogRevision;
		if (m_bStrict)
			m_bShowedAll = true;
		break;
	case 1: // show range
		{
			// ask for a revision range
			CRevisionRangeDlg dlg;
			dlg.SetStartRevision(m_startrev);
			dlg.SetEndRevision( (m_endrev>=0) ? m_endrev : 0);
			if (dlg.DoModal()!=IDOK)
				return;
			m_endrev = dlg.GetEndRevision();
			m_startrev = dlg.GetStartRevision();
			if (((m_endrev.IsNumber())&&(m_startrev.IsNumber()))||
				(m_endrev.IsHead()||m_startrev.IsHead()))
			{
				if (((LONG)m_startrev < (LONG)m_endrev)||
					(m_endrev.IsHead()))
				{
					svn_revnum_t temp = m_startrev;
					m_startrev = m_endrev;
					m_endrev = temp;
				}
			}
			m_bShowedAll = false;
		}
		break;
	}
	m_ChangedFileListCtrl.SetItemCountEx(0);
	m_ChangedFileListCtrl.Invalidate();
	// We need to create CStoreSelection on the heap or else
	// the variable will run out of the scope before the
	// thread ends. Therefore we let the thread delete
	// the instance.
    AutoStoreSelection();

    m_LogList.SetItemCountEx(0);
	m_LogList.Invalidate();
	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	if ((m_startrev > m_head)&&(m_head > 0))
	{
		CString sTemp;
		sTemp.FormatMessage(IDS_ERR_NOSUCHREVISION, m_startrev.ToString());
		m_LogList.ShowText(sTemp, true);
	    return;
	}
	pMsgView->SetWindowText(_T(""));

	SetSortArrow(&m_LogList, -1, true);

	m_logEntries.ClearAll();

	m_bCancelled = FALSE;
	m_limit = 0;

	InterlockedExchange(&m_bLogThreadRunning, TRUE);
	new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

void CLogDlg::OnBnClickedRefresh()
{
	m_limit = 0;
	Refresh (true);
}

void CLogDlg::Refresh (bool autoGoOnline)
{
	//does the user force the cache to refresh (shift or control key down)?
    m_bRefresh =   (GetKeyState (VK_CONTROL) < 0) 
                || (GetKeyState (VK_SHIFT) < 0);

	// refreshing means re-downloading the already shown log messages
	UpdateData();
	if ((m_limit == 0)||(m_bStrict)||(int(m_logEntries.size()-1) > m_limit))
	{
		if (m_logEntries.size() != 0)
		{
			m_endrev = m_logEntries[m_logEntries.size()-1]->GetRevision();
		}
	}
	m_startrev = -1;
	m_bCancelled = FALSE;
	m_wcRev = SVNRev();

	// We need to create CStoreSelection on the heap or else
	// the variable will run out of the scope before the
	// thread ends. Therefore we let the thread delete
	// the instance.
    AutoStoreSelection();

    m_ChangedFileListCtrl.SetItemCountEx(0);
	m_ChangedFileListCtrl.Invalidate();
	m_LogList.SetItemCountEx(0);
	m_LogList.Invalidate();
	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	pMsgView->SetWindowText(_T(""));

	SetSortArrow(&m_LogList, -1, true);
	m_logEntries.ClearAll();

    // reset the cached HEAD property & go on-line

    if (autoGoOnline)
    {
	    SetDlgTitle (false);
        GetLogCachePool()->GetRepositoryInfo().ResetHeadRevision (m_sUUID, m_sRepositoryRoot);
    }

	InterlockedExchange(&m_bLogThreadRunning, TRUE);
	new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

void CLogDlg::OnBnClickedNexthundred()
{
	UpdateData();
	// we have to fetch the next X log messages.
	if (m_logEntries.size() < 1)
	{
		// since there weren't any log messages fetched before, just
		// fetch all since we don't have an 'anchor' to fetch the 'next'
		// messages from.
		return GetAll(true);
	}
	svn_revnum_t rev = m_logEntries[m_logEntries.size()-1]->GetRevision();

	if (rev < 1)
		return;		// do nothing! No more revisions to get
	m_startrev = rev;
	m_endrev = 0;
	m_bCancelled = FALSE;

    // rev is is revision we already have and we will receive it again
    // -> fetch one extra revision to get NumberOfLogs *new* revisions

	m_limit = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100) +1;
	InterlockedExchange(&m_bLogThreadRunning, TRUE);
	SetSortArrow(&m_LogList, -1, true);
	// We need to create CStoreSelection on the heap or else
	// the variable will run out of the scope before the
	// thread ends. Therefore we let the thread delete
	// the instance.
    AutoStoreSelection();

	// since we fetch the log from the last revision we already have,
	// we have to remove that revision entry to avoid getting it twice
	m_logEntries.RemoveLast();
	new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

BOOL CLogDlg::Cancel()
{
	return m_bCancelled;
}

void CLogDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos1 = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1"));
		CRegDWORD regPos2 = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2"));
		RECT rectSplitter;
		m_wndSplitter1.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos1 = rectSplitter.top;
		m_wndSplitter2.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos2 = rectSplitter.top;
	}
}

void CLogDlg::OnCancel()
{
	// canceling means stopping the working thread if it's still running.
	// we do this by using the Subversion cancel callback.

	m_bCancelled = true;
    bool threadsStillRunning
        =    !netScheduler.WaitForEmptyQueueOrTimeout(2000)
		  || !diskScheduler.WaitForEmptyQueueOrTimeout(2000);

	// But canceling can also mean just to close the dialog, depending on the
	// text shown on the cancel button (it could simply read "OK").

	CString temp, temp2;
	GetDlgItemText(IDCANCEL, temp);
	temp2.LoadString(IDS_MSGBOX_CANCEL);
    if ((temp.Compare(temp2)==0))
    {
        // "Cancel" was hit. We told the SVN ops to stop
		// end the process the hard way
		if (threadsStillRunning)
		{
			// end the process the hard way
			TerminateProcess(GetCurrentProcess(), 0);
		}
		else
		{
			__super::OnCancel();
		}
	}

    // we hit "Ok"

	UpdateData();
	if (m_bSaveStrict)
		m_regLastStrict = m_bStrict;
	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowAllEntry"));
	reg = (DWORD)m_btnShow.GetCurrentEntry();

	CRegDWORD reg2 = CRegDWORD(_T("Software\\TortoiseSVN\\LogShowPaths"));
	reg2 = (DWORD)m_cShowPaths.GetCheck();
	SaveSplitterPos();

    // can we close the app cleanly?

    if (threadsStillRunning)
    {
		// end the process the hard way
		TerminateProcess(GetCurrentProcess(), 0);
	}
    else
    {
	    __super::OnCancel();
    }
}

BOOL CLogDlg::Log(svn_revnum_t rev, const CString& author, const CString& message, apr_time_t time, BOOL haschildren)
{
	// this is the callback function which receives the data for every revision we ask the log for
	// we store this information here one by one.

	__time64_t ttime = time / 1000000L;
    m_logEntries.Add ( rev
                     , ttime
                     , author
                     , message
                     , &m_ProjectProperties
                     , haschildren != FALSE);

    // end of child list

	if (rev == SVN_INVALID_REVNUM)
		return TRUE;

    // update progress

    if (m_startrev == -1)
		m_startrev = rev;

    if (m_limit != 0)
	{
        m_LogProgress.SetPos ((int)m_logEntries.size() - m_prevLogEntriesSize);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, m_logEntries.size() - m_prevLogEntriesSize, m_limit);
		}
	}
	else if (m_startrev.IsNumber() && m_endrev.IsNumber())
    {
        svn_revnum_t range = (svn_revnum_t)m_startrev - (svn_revnum_t)m_endrev;
        if ((rev > m_temprev) || (m_temprev - rev) > (range / 1000))
        {
            m_temprev = rev;
		    m_LogProgress.SetPos((svn_revnum_t)m_startrev-rev+(svn_revnum_t)m_endrev);
			if (m_pTaskbarList)
			{
				m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
				m_pTaskbarList->SetProgressValue(m_hWnd, rev, (svn_revnum_t)m_endrev-(svn_revnum_t)m_startrev);
			}
        }
    }

    // clean-up

	return TRUE;
}

//this is the thread function which calls the subversion function
void CLogDlg::LogThread()
{
	InterlockedExchange(&m_bLogThreadRunning, TRUE);

	new async::CAsyncCall(this, &CLogDlg::StatusThread, &diskScheduler);

	//disable the "Get All" button while we're receiving
	//log messages.
	DialogEnableWindow(IDC_GETALL, FALSE);
	DialogEnableWindow(IDC_NEXTHUNDRED, FALSE);
	DialogEnableWindow(IDC_SHOWPATHS, FALSE);
	DialogEnableWindow(IDC_CHECK_STOPONCOPY, FALSE);
	DialogEnableWindow(IDC_INCLUDEMERGE, FALSE);
	DialogEnableWindow(IDC_STATBUTTON, FALSE);
	DialogEnableWindow(IDC_REFRESH, FALSE);
	
	CString temp;
	temp.LoadString(IDS_PROGRESSWAIT);
	m_LogList.ShowText(temp, true);
	// change the text of the close button to "Cancel" since now the thread
	// is running, and simply closing the dialog doesn't work.
	if (!GetDlgItem(IDOK)->IsWindowVisible())
	{
		temp.LoadString(IDS_MSGBOX_CANCEL);
		SetDlgItemText(IDCANCEL, temp);
	}
	// We use a progress bar while getting the logs
	m_LogProgress.SetRange32(0, 100);
	m_LogProgress.SetPos(0);
	GetDlgItem(IDC_PROGRESS)->ShowWindow(TRUE);
	
    // we need the UUID to unambiguously identify the log cache
    BOOL succeeded = true;
    if (LogCache::CSettings::GetEnabled())
    {
        m_sUUID = GetLogCachePool()->GetRepositoryInfo().GetRepositoryUUID (m_path);
        if (m_sUUID.IsEmpty())
            succeeded = false;
    }

	// get the repository root url, because the changed-files-list has the
	// paths shown there relative to the repository root.
	CTSVNPath rootpath;
    if (succeeded)
    {
        // e.g. when we show the "next 100", we already have proper 
        // start and end revs.
        // -> we don't need to look for the head revision in these cases

        if ((m_startrev == SVNRev::REV_HEAD) || (m_endrev == SVNRev::REV_HEAD) || (m_head < 0))
        {
            // expensive repository lookup 
            // (if maxHeadAge has expired, which is the default setting)

    	    svn_revnum_t head = -1;
            succeeded = GetRootAndHead(m_path, rootpath, head);
			m_head = head;
            if (m_startrev == SVNRev::REV_HEAD) 
			{
	            m_startrev = head;
			}
            if (m_endrev == SVNRev::REV_HEAD)
	            m_endrev = head;
        }
        else
        {
            // far less expensive root lookup

            rootpath.SetFromSVN (GetRepositoryRoot (m_path));
            succeeded = !rootpath.IsEmpty();
        }
    }

    m_sRepositoryRoot = rootpath.GetSVNPathString();
    m_sURL = m_path.GetSVNPathString();

    // if the log dialog is started from a working copy, we need to turn that
    // local path into an url here
    if (succeeded)
    {
        if (!m_path.IsUrl())
        {
	        m_sURL = GetURLFromPath(m_path);
        }
		m_sURL = CPathUtils::PathUnescape(m_sURL);
        m_sRelativeRoot = m_sURL.Mid(CPathUtils::PathUnescape(m_sRepositoryRoot).GetLength());
    }

    if (succeeded && !m_mergePath.IsEmpty() && m_mergedRevs.empty())
    {
	    // in case we got a merge path set, retrieve the merge info
	    // of that path and check whether one of the merge URLs
	    // match the URL we show the log for.
	    SVNPool localpool(pool);
	    svn_error_clear(Err);
	    apr_hash_t * mergeinfo = NULL;

        const char* svnPath = m_mergePath.GetSVNApiPath(localpool);
        SVNTRACE (
            Err = svn_client_mergeinfo_get_merged (&mergeinfo, svnPath, SVNRev(SVNRev::REV_WC), m_pctx, localpool),
            svnPath
        )
	    if (Err == NULL)
	    {
		    // now check the relative paths
		    apr_hash_index_t *hi;
		    const void *key;
		    void *val;

		    if (mergeinfo)
		    {
				CStringA sUrl = CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(m_sURL));

			    for (hi = apr_hash_first(localpool, mergeinfo); hi; hi = apr_hash_next(hi))
			    {
				    apr_hash_this(hi, &key, NULL, &val);
				    if (sUrl.Compare((char*)key) == 0)
				    {
					    apr_array_header_t * arr = (apr_array_header_t*)val;
					    if (val)
					    {
						    for (long i=0; i<arr->nelts; ++i)
						    {
							    svn_merge_range_t * pRange = APR_ARRAY_IDX(arr, i, svn_merge_range_t*);
							    if (pRange)
							    {
								    for (svn_revnum_t re = pRange->start+1; re <= pRange->end; ++re)
								    {
									    m_mergedRevs.insert(re);
								    }
							    }
						    }
					    }
					    break;
				    }
			    }
		    }
	    }
    }

    m_LogProgress.SetPos(1);
	m_prevLogEntriesSize = m_logEntries.size();
	if (m_pTaskbarList)
	{
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
	}
    if (m_limit != 0)
	    m_LogProgress.SetRange32(0, m_limit);
    else
	    m_LogProgress.SetRange32(m_endrev, m_startrev);
	
    if (!m_pegrev.IsValid())
	    m_pegrev = m_startrev;
    size_t startcount = m_logEntries.size();
    m_bStrictStopped = false;

    std::auto_ptr<const CCacheLogQuery> cachedData;
    if (succeeded)
    {
        cachedData = ReceiveLog (CTSVNPathList(m_path), m_pegrev, m_startrev, m_endrev, m_limit, !!m_bStrict, !!m_bIncludeMerges, m_bRefresh);
        if ((cachedData.get() == NULL)&&(!m_path.IsUrl()))
        {
	        // try again with REV_WC as the start revision, just in case the path doesn't
	        // exist anymore in HEAD
	        cachedData = ReceiveLog(CTSVNPathList(m_path), SVNRev(), SVNRev::REV_WC, m_endrev, m_limit, !!m_bStrict, !!m_bIncludeMerges, m_bRefresh);
        }

        // Err will also be set if the user cancelled.

        succeeded = Err == NULL;

        // make sure the m_logEntries is consistent

        if (cachedData.get() != NULL)
            m_logEntries.Finalize (cachedData, m_sRelativeRoot, !LogCache::CSettings::GetEnabled());
        else
            m_logEntries.ClearAll();
    }
	m_LogList.ClearText();
    if (!succeeded)
	{
		temp.LoadString(IDS_LOG_CLEARERROR);
		m_LogList.ShowText(GetLastErrorMessage() + _T("\n\n") + temp, true);
		FillLogMessageCtrl(false);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
		}
	}

	if (   m_bStrict 
        && (m_logEntries.GetMinRevision() > 1) 
        && (m_limit > 0
               ? (startcount + m_limit > m_logEntries.size()) 
               : (m_endrev < m_logEntries.GetMinRevision())))
    {
		m_bStrictStopped = true;
    }
	m_LogList.SetItemCountEx(ShownCountWithStopped());

    m_tFrom = m_logEntries.GetMinDate();
	m_tTo = m_logEntries.GetMaxDate();
    m_timFrom = m_tFrom;
	m_timTo = m_tTo;
	m_DateFrom.SetRange(&m_timFrom, &m_timTo);
	m_DateTo.SetRange(&m_timFrom, &m_timTo);
	m_DateFrom.SetTime(&m_timFrom);
	m_DateTo.SetTime(&m_timTo);

	DialogEnableWindow(IDC_GETALL, TRUE);
	
	if (!m_bShowedAll)
		DialogEnableWindow(IDC_NEXTHUNDRED, TRUE);
	DialogEnableWindow(IDC_SHOWPATHS, TRUE);
	DialogEnableWindow(IDC_CHECK_STOPONCOPY, TRUE);
	DialogEnableWindow(IDC_INCLUDEMERGE, TRUE);
	DialogEnableWindow(IDC_STATBUTTON, TRUE);
	DialogEnableWindow(IDC_REFRESH, TRUE);

	LogCache::CRepositoryInfo& cachedProperties = GetLogCachePool()->GetRepositoryInfo();
	SetDlgTitle(cachedProperties.IsOffline (m_sUUID, m_sRepositoryRoot, false));

	GetDlgItem(IDC_PROGRESS)->ShowWindow(FALSE);
	if (m_pTaskbarList)
	{
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
	}
	m_bCancelled = true;
	InterlockedExchange(&m_bLogThreadRunning, FALSE);
	if ( m_pStoreSelection == NULL )
	{
		// If no selection has been set then this must be the first time
		// the revisions are shown. Let's preselect the topmost revision.
		if ( m_LogList.GetItemCount()>0 )
		{
			m_LogList.SetSelectionMark(0);
			m_LogList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	if (!GetDlgItem(IDOK)->IsWindowVisible())
	{
		temp.LoadString(IDS_MSGBOX_OK);
		SetDlgItemText(IDCANCEL, temp);
	}
	RefreshCursor();
	// make sure the filter is applied (if any) now, after we refreshed/fetched
	// the log messages
	PostMessage(WM_TIMER, LOGFILTER_TIMER);
}

//this is the thread function which calls the subversion function
void CLogDlg::StatusThread()
{
	bool bAllowStatusCheck = !!(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogStatusCheck"), TRUE);
	if ((bAllowStatusCheck)&&(!m_wcRev.IsValid()))
	{
		// fetch the revision the wc path is on so we can mark it
		CTSVNPath revWCPath = m_ProjectProperties.GetPropsPath();
		if (!m_path.IsUrl())
			revWCPath = m_path;

		svn_revnum_t minrev, maxrev;
		bool switched, modified, sparse;
		SVN().GetWCRevisionStatus(revWCPath, true, minrev, maxrev, switched, modified, sparse);
		if (maxrev)
		{
			m_wcRev = maxrev;
			// force a redraw of the log list control to make sure the wc rev is
			// redrawn in bold
			m_LogList.Invalidate(FALSE);
		}
	}
}

void CLogDlg::CopySelectionToClipBoard()
{
	CString sClipdata;
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		CString sRev;
		sRev.LoadString(IDS_LOG_REVISION);
		CString sAuthor;
		sAuthor.LoadString(IDS_LOG_AUTHOR);
		CString sDate;
		sDate.LoadString(IDS_LOG_DATE);
		CString sMessage;
		sMessage.LoadString(IDS_LOG_MESSAGE);
		while (pos)
		{
			CString sLogCopyText;
			CString sPaths;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
            const CLogChangedPathArray& cpatharray = pLogEntry->GetChangedPaths();
			for (size_t cpPathIndex = 0; cpPathIndex < cpatharray.GetCount(); ++cpPathIndex)
			{
				const CLogChangedPath& cpath = cpatharray[cpPathIndex];
				sPaths += cpath.GetActionString() + _T(" : ") + cpath.GetPath();
				if (cpath.GetCopyFromPath().IsEmpty())
					sPaths += _T("\r\n");
				else
				{
					CString sCopyFrom;
					sCopyFrom.Format(_T(" (%s: %s, %s, %ld)\r\n"), (LPCTSTR)CString(MAKEINTRESOURCE(IDS_LOG_COPYFROM)), 
						(LPCTSTR)cpath.GetCopyFromPath(), 
						(LPCTSTR)CString(MAKEINTRESOURCE(IDS_LOG_REVISION)), 
						cpath.GetCopyFromRev());
					sPaths += sCopyFrom;
				}
			}
			sPaths.Trim();
			sLogCopyText.Format(_T("%s: %d\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n"),
				(LPCTSTR)sRev, pLogEntry->GetRevision(),
				(LPCTSTR)sAuthor, (LPCTSTR)pLogEntry->GetAuthor(),
				(LPCTSTR)sDate, (LPCTSTR)pLogEntry->GetDateString(),
				(LPCTSTR)sMessage, (LPCTSTR)pLogEntry->GetMessage(),
				(LPCTSTR)sPaths);
			sClipdata +=  sLogCopyText;
		}
		CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
	}
}

void CLogDlg::CopyChangedSelectionToClipBoard()
{
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos == NULL)
		return;	// nothing is selected, get out of here

	CString sPaths;

	PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_LogList.GetNextSelectedItem(pos));
	if (pos)
	{
		POSITION pos2 = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos2)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
			sPaths += m_currentChangedPathList[nItem].GetSVNPathString();
			sPaths += _T("\r\n");
		}
	}
	else
	{
		// only one revision is selected in the log dialog top pane
		// but multiple items could be selected  in the changed items list
		POSITION pos2 = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos2)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
            const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();

			if ((m_cShowPaths.GetState() & 0x0003)==BST_CHECKED)
			{
				// some items are hidden! So find out which item the user really selected
				int selRealIndex = -1;
                for (size_t hiddenindex=0; hiddenindex<paths.GetCount(); ++hiddenindex)
				{
					if (paths[hiddenindex].IsRelevantForStartPath())
						selRealIndex++;
					if (selRealIndex == nItem)
					{
						nItem = static_cast<int>(hiddenindex);
						break;
					}
				}
			}

            const CLogChangedPath& changedlogpath = paths[nItem];
			sPaths += changedlogpath.GetPath();
			sPaths += _T("\r\n");
		}
	}
	sPaths.Trim();
	CStringUtils::WriteAsciiStringToClipboard(sPaths, GetSafeHwnd());
}

BOOL CLogDlg::IsDiffPossible(const CLogChangedPath& changedpath, svn_revnum_t rev)
{
	if ((rev > 1)&&(changedpath.GetAction() != LOGACTIONS_DELETED))
	{
		if (changedpath.GetAction() == LOGACTIONS_ADDED) // file is added
		{
			if (changedpath.GetCopyFromRev() == 0)
				return FALSE; // but file was not added with history
		}
		return TRUE;
	}
	return FALSE;
}

void CLogDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// we have two separate context menus:
	// one shown on the log message list control,
	// the other shown in the changed-files list control
	int selCount = m_LogList.GetSelectedCount();
	if (pWnd == &m_LogList)
	{
		ShowContextMenuForRevisions(pWnd, point);
	}
	else if (pWnd == &m_ChangedFileListCtrl)
	{
		ShowContextMenuForChangedpaths(pWnd, point);
	}
	else if ((selCount == 1)&&(pWnd == GetDlgItem(IDC_MSGVIEW)))
	{
		POSITION pos = m_LogList.GetFirstSelectedItemPosition();
		int selIndex = -1;
		if (pos)
			selIndex = m_LogList.GetNextSelectedItem(pos);
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			GetDlgItem(IDC_MSGVIEW)->GetClientRect(&rect);
			ClientToScreen(&rect);
			point = rect.CenterPoint();
		}
		CString sMenuItemText;
		CMenu popup;
		if (popup.CreatePopupMenu())
		{
			// add the 'default' entries
			sMenuItemText.LoadString(IDS_SCIEDIT_COPY);
			popup.AppendMenu(MF_STRING | MF_ENABLED, WM_COPY, sMenuItemText);
			sMenuItemText.LoadString(IDS_SCIEDIT_SELECTALL);
			popup.AppendMenu(MF_STRING | MF_ENABLED, EM_SETSEL, sMenuItemText);

			if (selIndex >= 0)
			{
				popup.AppendMenu(MF_SEPARATOR);
				sMenuItemText.LoadString(IDS_LOG_POPUP_EDITLOG);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITAUTHOR, sMenuItemText);
			}

			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			switch (cmd)
			{
			case 0:
				break;	// no command selected
			case EM_SETSEL:
			case WM_COPY:
				::SendMessage(GetDlgItem(IDC_MSGVIEW)->GetSafeHwnd(), cmd, 0, -1);
				break;
			case ID_EDITAUTHOR:
				EditLogMessage(selIndex);
				break;
			}
		}
	}
}

bool CLogDlg::IsSelectionContinuous()
{
	if ( m_LogList.GetSelectedCount()==1 )
	{
		// if only one revision is selected, the selection is of course
		// continuous
		return true;
	}

	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	bool bContinuous = m_logEntries.GetVisibleCount() == m_logEntries.size();
	if (bContinuous)
	{
		int itemindex = m_LogList.GetNextSelectedItem(pos);
		while (pos)
		{
			int nextindex = m_LogList.GetNextSelectedItem(pos);
			if (nextindex - itemindex > 1)
			{
				bContinuous = false;
				break;
			}
			itemindex = nextindex;
		}
	}
	return bContinuous;
}

LRESULT CLogDlg::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_pFindDialog != NULL);

    if (m_pFindDialog->IsTerminating())
    {
	    // invalidate the handle identifying the dialog box.
        m_pFindDialog = NULL;
        return 0;
    }

    if(m_pFindDialog->FindNext())
    {
        //read data from dialog
        CString findText = m_pFindDialog->GetFindString();
        bool bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
		bool bFound = false;
		tr1::wregex pat;
		bool bRegex = ValidateRegexp(findText, pat, bMatchCase);

        bool scanRelevantPathsOnly = (m_cShowPaths.GetState() & 0x0003)==BST_CHECKED;
        CLogDlgFilter filter ( findText
                             , bRegex
                             , LOGFILTER_ALL
                             , bMatchCase
                             , m_tFrom
                             , m_tTo
                             , scanRelevantPathsOnly
                             , -1);

        for (size_t i = m_nSearchIndex; i < m_logEntries.GetVisibleCount() && !bFound; i++)
		{
            if (filter (*m_logEntries.GetVisible (i)))
            {
			    m_LogList.EnsureVisible((int)i, FALSE);
			    m_LogList.SetItemState(m_LogList.GetSelectionMark(), 0, LVIS_SELECTED);
			    m_LogList.SetItemState((int)i, LVIS_SELECTED, LVIS_SELECTED);
			    m_LogList.SetSelectionMark((int)i);

			    FillLogMessageCtrl();
			    UpdateData(FALSE);

    			m_nSearchIndex = (int)(i+1);
                break;
            }
		}
    } // if(m_pFindDialog->FindNext()) 
	UpdateLogInfoLabel();
    return 0;
}

void CLogDlg::OnOK()
{
	// since the log dialog is also used to select revisions for other
	// dialogs, we have to do some work before closing this dialog
	if ((GetDlgItem(IDOK)->IsWindowVisible()) && (GetFocus() != GetDlgItem(IDOK)))
		return;	// if the "OK" button doesn't have the focus, do nothing: this prevents closing the dialog when pressing enter
	if (!GetDlgItem(IDOK)->IsWindowVisible() && GetFocus() != GetDlgItem(IDCANCEL))
		return; // the Cancel button works as the OK button. But if the cancel button has not the focus, do nothing.

	CString temp;
	m_bCancelled = true;
    if (   !netScheduler.WaitForEmptyQueueOrTimeout(0)
		|| !diskScheduler.WaitForEmptyQueueOrTimeout(0))
    {
		return;
    }

	m_selectedRevs.Clear();
	m_selectedRevsOneRange.Clear();
	if (m_pNotifyWindow)
	{
		int selIndex = m_LogList.GetSelectionMark();
		if (selIndex >= 0)
		{	
		    PLOGENTRYDATA pLogEntry = NULL;
			POSITION pos = m_LogList.GetFirstSelectedItemPosition();
            pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
			m_selectedRevs.AddRevision(pLogEntry->GetRevision());
			svn_revnum_t lowerRev = pLogEntry->GetRevision();
			svn_revnum_t higherRev = lowerRev;
			while (pos)
			{
                pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
				svn_revnum_t rev = pLogEntry->GetRevision();
				m_selectedRevs.AddRevision(pLogEntry->GetRevision());
				if (lowerRev > rev)
					lowerRev = rev;
				if (higherRev < rev)
					higherRev = rev;
			}
			if (m_sFilterText.IsEmpty() && m_nSortColumn == 0 && IsSelectionContinuous())
			{
				m_selectedRevsOneRange.AddRevRange(lowerRev, higherRev);
			}
			BOOL bSentMessage = FALSE;
			if (m_LogList.GetSelectedCount() == 1)
			{
				// if only one revision is selected, check if the path/url with which the dialog was started
				// was directly affected in that revision. If it was, then check if our path was copied from somewhere.
				// if it was copied, use the copy from revision as lowerRev
                if ((pLogEntry)&&(lowerRev == higherRev))
				{
					CString sUrl = m_path.GetSVNPathString();
					if (!m_path.IsUrl())
					{
						sUrl = GetURLFromPath(m_path);
					}
					sUrl = sUrl.Mid(m_sRepositoryRoot.GetLength());

                    const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();
					for (size_t cp = 0; cp < paths.GetCount(); ++cp)
					{
						const CLogChangedPath& pData = paths[cp];
						if (sUrl.Compare(pData.GetPath()) == 0)
						{
							if (!pData.GetCopyFromPath().IsEmpty())
							{
								lowerRev = pData.GetCopyFromRev();
								m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTSTART), lowerRev);
								m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTEND), higherRev);
								m_pNotifyWindow->SendMessage(WM_REVLIST, m_selectedRevs.GetCount(), (LPARAM)&m_selectedRevs);
								bSentMessage = TRUE;
							}
						}
					}
				}
			}
			if ( !bSentMessage )
			{
				m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTSTART | MERGE_REVSELECTMINUSONE), lowerRev);
				m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTEND | MERGE_REVSELECTMINUSONE), higherRev);
				m_pNotifyWindow->SendMessage(WM_REVLIST, m_selectedRevs.GetCount(), (LPARAM)&m_selectedRevs);
				if (m_selectedRevsOneRange.GetCount())
					m_pNotifyWindow->SendMessage(WM_REVLISTONERANGE, 0, (LPARAM)&m_selectedRevsOneRange);
			}
		}
	}
	UpdateData();
	if (m_bSaveStrict)
		m_regLastStrict = m_bStrict;
	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowAllEntry"));
	reg = (DWORD)m_btnShow.GetCurrentEntry();
	CRegDWORD reg2 = CRegDWORD(_T("Software\\TortoiseSVN\\LogHidePaths"));
	reg2 = (DWORD)m_cShowPaths.GetCheck();
	SaveSplitterPos();

	CString buttontext;
	GetDlgItemText(IDOK, buttontext);
	temp.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(buttontext) != 0)
		__super::OnOK();	// only exit if the button text matches, and that will match only if the thread isn't running anymore
}

void CLogDlg::OnNMDblclkChangedFileList(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// a double click on an entry in the changed-files list has happened
	*pResult = 0;

	DiffSelectedFile();
}

void CLogDlg::DiffSelectedFile()
{
	if ((m_bLogThreadRunning)||(m_LogList.HasText()))
		return;
	UpdateLogInfoLabel();
	INT_PTR selIndex = m_ChangedFileListCtrl.GetSelectionMark();
	if (selIndex < 0)
		return;
	if (m_ChangedFileListCtrl.GetSelectedCount() == 0)
		return;
	// find out if there's an entry selected in the log list
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_LogList.GetNextSelectedItem(pos));
	svn_revnum_t rev1 = pLogEntry->GetRevision();
	svn_revnum_t rev2 = rev1;
	if (pos)
	{
		while (pos)
		{
			// there's at least a second entry selected in the log list: several revisions selected!
			pLogEntry = m_logEntries.GetVisible(m_LogList.GetNextSelectedItem(pos));
			if (pLogEntry)
			{
				rev1 = max(rev1,(long)pLogEntry->GetRevision());
				rev2 = min(rev2,(long)pLogEntry->GetRevision());
			}
		}
		rev2--;
		// now we have both revisions selected in the log list, so we can do a diff of the selected
		// entry in the changed files list with these two revisions.
		DoDiffFromLog(selIndex, rev1, rev2, false, false);
	}
	else
	{
        const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();
		rev2 = rev1-1;
		// nothing or only one revision selected in the log list

        if ((m_cShowPaths.GetState() & 0x0003)==BST_CHECKED)
		{
			// some items are hidden! So find out which item the user really clicked on
			INT_PTR selRealIndex = -1;
            for (size_t hiddenindex=0; hiddenindex<paths.GetCount(); ++hiddenindex)
			{
                if (paths[hiddenindex].IsRelevantForStartPath())
					selRealIndex++;
				if (selRealIndex == selIndex)
				{
					selIndex = hiddenindex;
					break;
				}
			}
		}

		const CLogChangedPath& changedpath = m_currentChangedArray[selIndex];

		if (IsDiffPossible(changedpath, rev1))
		{
			// diffs with renamed files are possible
			if (!changedpath.GetCopyFromPath().IsEmpty())
				rev2 = changedpath.GetCopyFromRev();
			else
			{
				// if the path was modified but the parent path was 'added with history'
				// then we have to use the copy from revision of the parent path
				CTSVNPath cpath = CTSVNPath(changedpath.GetPath());
                for (size_t flist = 0; flist < paths.GetCount(); ++flist)
				{
                    const CLogChangedPath& path = paths[flist];
					CTSVNPath p = CTSVNPath(path.GetPath());
					if (p.IsAncestorOf(cpath))
					{
						if (!path.GetCopyFromPath().IsEmpty())
							rev2 = path.GetCopyFromRev();
					}
				}
			}
			DoDiffFromLog(selIndex, rev1, rev2, false, false);
		}
		else 
		{
			CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(changedpath.GetPath()));
			CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(changedpath.GetPath()));
			SVNRev r = rev1;
			// deleted files must be opened from the revision before the deletion
			if (changedpath.GetAction() == LOGACTIONS_DELETED)
				r = rev1-1;
			m_bCancelled = false;

			CProgressDlg progDlg;
			progDlg.SetTitle(IDS_APPNAME);
			progDlg.SetAnimation(IDR_DOWNLOAD);
			CString sInfoLine;
			sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)(m_sRepositoryRoot + changedpath.GetPath()), (LPCTSTR)r.ToString());
			progDlg.SetLine(1, sInfoLine, true);
			SetAndClearProgressInfo(&progDlg);
			progDlg.ShowModeless(m_hWnd);

			if (!Cat(CTSVNPath(m_sRepositoryRoot + changedpath.GetPath()), r, r, tempfile))
			{
				m_bCancelled = false;
				if (!Cat(CTSVNPath(m_sRepositoryRoot + changedpath.GetPath()), SVNRev::REV_HEAD, r, tempfile))
				{
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
					CMessageBox::Show(m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return;
				}
			}
			progDlg.Stop();
			SetAndClearProgressInfo((HWND)NULL);

			CString sName1, sName2;
			sName1.Format(_T("%s - Revision %ld"), (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath.GetPath()), (svn_revnum_t)rev1);
			sName2.Format(_T("%s - Revision %ld"), (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath.GetPath()), (svn_revnum_t)rev1-1);
			CAppUtils::DiffFlags flags;
			flags.AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			if (changedpath.GetAction() == LOGACTIONS_DELETED)
				CAppUtils::StartExtDiff(tempfile, tempfile2, sName2, sName1, flags, 0);
			else
				CAppUtils::StartExtDiff(tempfile2, tempfile, sName2, sName1, flags, 0);
		}
	}
}


void CLogDlg::OnNMDblclkLoglist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// a double click on an entry in the revision list has happened
	*pResult = 0;

	if (m_LogList.HasText())
	{
		m_LogList.ClearText();
        FillLogMessageCtrl();
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
		}
		return;
	}
	if (CRegDWORD(_T("Software\\TortoiseSVN\\DiffByDoubleClickInLog"), FALSE))
		DiffSelectedRevWithPrevious();
}

void CLogDlg::DiffSelectedRevWithPrevious()
{
	if ((m_bLogThreadRunning)||(m_LogList.HasText()))
		return;
	UpdateLogInfoLabel();
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex < 0)
		return;
	int selCount = m_LogList.GetSelectedCount();
	if (selCount != 1)
		return;

	// Find selected entry in the log list
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
	long rev1 = pLogEntry->GetRevision();
	long rev2 = rev1-1;
	CTSVNPath path = m_path;

	// See how many files under the relative root were changed in selected revision
	int nChanged = 0;
	size_t lastChangedIndex = (size_t)(-1);

    const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();
    for (size_t c = 0; c < paths.GetCount(); ++c)
        if (paths[c].IsRelevantForStartPath())
		{
			++nChanged;
			lastChangedIndex = c;
		}

	if (m_path.IsDirectory() && nChanged == 1) 
	{
		// We're looking at the log for a directory and only one file under dir was changed in the revision
		// Do diff on that file instead of whole directory

        const CLogChangedPath& cpath = pLogEntry->GetChangedPaths()[lastChangedIndex];
		path.SetFromWin (cpath.GetPath());
	} 

	m_bCancelled = FALSE;
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);

	if (PromptShown())
	{
		SVNDiff diff(this, m_hWnd, true);
		diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		diff.SetHEADPeg(m_LogRevision);
		diff.ShowCompare(path, rev2, path, rev1, SVNRev(), false, false, m_path.IsDirectory() && (nChanged != 1) ? svn_node_dir : svn_node_file);
	}
	else
	{
		CAppUtils::StartShowCompare(m_hWnd, path, rev2, path, rev1, SVNRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), false, false, m_path.IsDirectory() && (nChanged != 1) ? svn_node_dir : svn_node_file);
	}

	theApp.DoWaitCursor(-1);
	EnableOKButton();
}

void CLogDlg::DoDiffFromLog(INT_PTR selIndex, svn_revnum_t rev1, svn_revnum_t rev2, bool blame, bool unified)
{
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	//get the filename
	CString filepath;
	if (SVN::PathIsURL(m_path))
	{
		filepath = m_path.GetSVNPathString();
	}
	else
	{
		filepath = GetURLFromPath(m_path);
		if (filepath.IsEmpty())
		{
			theApp.DoWaitCursor(-1);
			CString temp;
			temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
			CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
			TRACE(_T("could not retrieve the URL of the file!\n"));
			EnableOKButton();
			return;		//exit
		}
	}
	m_bCancelled = FALSE;
	filepath = GetRepositoryRoot(CTSVNPath(filepath));

	svn_node_kind_t nodekind = svn_node_unknown;
	CString firstfile, secondfile;
	if (m_LogList.GetSelectedCount()==1)
	{
		const CLogChangedPath& changedpath = m_currentChangedArray[selIndex];
		nodekind = changedpath.GetNodeKind();
		firstfile = changedpath.GetPath();
		secondfile = firstfile;
		if ((rev2 == rev1-1)&&(changedpath.GetCopyFromRev() > 0)) // is it an added file with history?
		{
			secondfile = changedpath.GetCopyFromPath();
			rev2 = changedpath.GetCopyFromRev();
		}
	}
	else
	{
		firstfile = m_currentChangedPathList[selIndex].GetSVNPathString();
		secondfile = firstfile;
	}

	firstfile = filepath + firstfile.Trim();
	secondfile = filepath + secondfile.Trim();

	SVNDiff diff(this, this->m_hWnd, true);
	diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
	// since we have the direct urls here and revisions, we set the head peg
	// revision to rev1.
	// see this thread for why: http://tortoisesvn.tigris.org/ds/viewMessage.do?dsForumId=4061&dsMessageId=2096879
	diff.SetHEADPeg(rev1);
	if (unified)
	{
		if (PromptShown())
			diff.ShowUnifiedDiff(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1);
		else
			CAppUtils::StartShowUnifiedDiff(m_hWnd, CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1, SVNRev(), m_LogRevision);
	}
	else
	{
		if (diff.ShowCompare(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1, SVNRev(), false, blame, nodekind))
		{
			if (firstfile.Compare(secondfile)==0)
			{
				svn_revnum_t baseRev = 0;
				diff.DiffProps(CTSVNPath(firstfile), rev2, rev1, baseRev);
			}
		}
	}
	theApp.DoWaitCursor(-1);
	EnableOKButton();
}

BOOL CLogDlg::Open(bool bOpenWith,CString changedpath, svn_revnum_t rev)
{
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	CString filepath;
	if (SVN::PathIsURL(m_path))
	{
		filepath = m_path.GetSVNPathString();
	}
	else
	{
		filepath = GetURLFromPath(m_path);
		if (filepath.IsEmpty())
		{
			theApp.DoWaitCursor(-1);
			CString temp;
			temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
			CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
			TRACE(_T("could not retrieve the URL of the file!\n"));
			EnableOKButton();
			return FALSE;
		}
	}
	m_bCancelled = false;
	filepath = GetRepositoryRoot(CTSVNPath(filepath));
	filepath += changedpath;

	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_APPNAME);
	progDlg.SetAnimation(IDR_DOWNLOAD);
	CString sInfoLine;
	sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)filepath, (LPCTSTR)SVNRev(rev).ToString());
	progDlg.SetLine(1, sInfoLine, true);
	SetAndClearProgressInfo(&progDlg);
	progDlg.ShowModeless(m_hWnd);

	CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(filepath), rev);
	m_bCancelled = false;
	if (!Cat(CTSVNPath(filepath), SVNRev(rev), rev, tempfile))
	{
		progDlg.Stop();
		SetAndClearProgressInfo((HWND)NULL);
		CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		EnableOKButton();
		theApp.DoWaitCursor(-1);
		return FALSE;
	}
	progDlg.Stop();
	SetAndClearProgressInfo((HWND)NULL);
	SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
	if (!bOpenWith)
	{
		int ret = (int)ShellExecute(this->m_hWnd, NULL, tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
		if (ret <= HINSTANCE_ERROR)
			bOpenWith = true;
	}
	if (bOpenWith)
	{
		CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
		cmd += tempfile.GetWinPathString() + _T(" ");
		CAppUtils::LaunchApplication(cmd, NULL, false);
	}
	EnableOKButton();
	theApp.DoWaitCursor(-1);
	return TRUE;
}

void CLogDlg::EditAuthor(const std::vector<PLOGENTRYDATA>& logs)
{
	CString url;
	CString name;
	if (logs.size() == 0)
		return;
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	if (SVN::PathIsURL(m_path))
		url = m_path.GetSVNPathString();
	else
		url = GetURLFromPath(m_path);
	
	name = SVN_PROP_REVISION_AUTHOR;

	CString value = RevPropertyGet(name, CTSVNPath(url), logs[0]->GetRevision());
	CString sOldValue = value;
	value.Replace(_T("\n"), _T("\r\n"));
	CInputDlg dlg(this);
	dlg.m_sHintText.LoadString(IDS_LOG_AUTHOR);
	dlg.m_sInputText = value;
	dlg.m_sTitle.LoadString(IDS_LOG_AUTHOREDITTITLE);
	dlg.m_pProjectProperties = &m_ProjectProperties;
	dlg.m_bUseLogWidth = false;
	if (dlg.DoModal() == IDOK)
	{
		if(sOldValue.CompareNoCase(dlg.m_sInputText))	
		{
		dlg.m_sInputText.Replace(_T("\r"), _T(""));

		LogCache::CCachedLogInfo* toUpdate 
			= GetLogCache (CTSVNPath (m_sRepositoryRoot));

		CProgressDlg progDlg;
		progDlg.SetTitle(IDS_APPNAME);
		progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
		progDlg.SetTime(true);
		progDlg.SetShowProgressBar(true);
		progDlg.ShowModeless(m_hWnd);
		for (DWORD i=0; i<logs.size(); ++i)
		{
			if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTSVNPath(url), logs[i]->GetRevision()))
			{
				progDlg.Stop();
				CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				break;
			}
			else
			{

				logs[i]->SetAuthor (dlg.m_sInputText);
				m_LogList.Invalidate();

				// update the log cache 

				if (toUpdate != NULL)
				{
					// log caching is active

					LogCache::CCachedLogInfo newInfo;
					newInfo.Insert ( logs[i]->GetRevision()
						, (const char*) CUnicodeUtils::GetUTF8 (logs[i]->GetAuthor())
						, ""
						, 0
						, LogCache::CRevisionInfoContainer::HAS_AUTHOR);

					toUpdate->Update (newInfo);
                    toUpdate->Save();
				}
			}
			progDlg.SetProgress64(i, logs.size());
		}
		progDlg.Stop();
		}
	}
	theApp.DoWaitCursor(-1);
	EnableOKButton();
}

void CLogDlg::EditLogMessage(int index)
{
	CString url;
	CString name;
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	if (SVN::PathIsURL(m_path))
		url = m_path.GetSVNPathString();
	else
		url = GetURLFromPath(m_path);

	name = SVN_PROP_REVISION_LOG;

	PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
	m_bCancelled = FALSE;
	CString value = RevPropertyGet(name, CTSVNPath(url), pLogEntry->GetRevision());
	CString sOldValue = value;
	value.Replace(_T("\n"), _T("\r\n"));
	CInputDlg dlg(this);
	dlg.m_sHintText.LoadString(IDS_LOG_MESSAGE);
	dlg.m_sInputText = value;
	dlg.m_sTitle.LoadString(IDS_LOG_MESSAGEEDITTITLE);
	dlg.m_pProjectProperties = &m_ProjectProperties;
	dlg.m_bUseLogWidth = true;
	if (dlg.DoModal() == IDOK)
	{
		if(sOldValue.CompareNoCase(dlg.m_sInputText))	
		{
		dlg.m_sInputText.Replace(_T("\r"), _T(""));
		if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTSVNPath(url), pLogEntry->GetRevision()))
		{
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		else
		{
            pLogEntry->SetMessage (dlg.m_sInputText, &m_ProjectProperties);

			CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
			pMsgView->SetWindowText(_T(" "));
			pMsgView->SetWindowText(dlg.m_sInputText);
			m_ProjectProperties.FindBugID(dlg.m_sInputText, pMsgView);
			m_LogList.Invalidate();
        
            // update the log cache 

            LogCache::CCachedLogInfo* toUpdate 
                = GetLogCache (CTSVNPath (m_sRepositoryRoot));
            if (toUpdate != NULL)
            {
                // log caching is active

                LogCache::CCachedLogInfo newInfo;
                newInfo.Insert ( pLogEntry->GetRevision()
                               , ""
                               , (const char*) CUnicodeUtils::GetUTF8 (pLogEntry->GetMessage())
                               , 0
                               , LogCache::CRevisionInfoContainer::HAS_COMMENT);

                toUpdate->Update (newInfo);
                toUpdate->Save();
            }
        }
		}
	}
	theApp.DoWaitCursor(-1);
	EnableOKButton();
}

BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
	// Skip Ctrl-C when copying text out of the log message or search filter
	BOOL bSkipAccelerator = ( pMsg->message == WM_KEYDOWN && pMsg->wParam=='C' && (GetFocus()==GetDlgItem(IDC_MSGVIEW) || GetFocus()==GetDlgItem(IDC_SEARCHEDIT) ) && GetKeyState(VK_CONTROL)&0x8000 );
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam==VK_RETURN)
	{
		if (GetAsyncKeyState(VK_CONTROL)&0x8000)
		{
			if (DWORD(CRegStdDWORD(_T("Software\\TortoiseSVN\\CtrlEnter"), TRUE)))
			{
				if ( GetDlgItem(IDOK)->IsWindowVisible() )
				{
					GetDlgItem(IDOK)->SetFocus();
					PostMessage(WM_COMMAND, IDOK);
				}
				else
				{
					GetDlgItem(IDCANCEL)->SetFocus();
					PostMessage(WM_COMMAND, IDOK);
				}
			}
			return TRUE;
		}
		if (GetFocus()==GetDlgItem(IDC_LOGLIST))
		{
			if (CRegDWORD(_T("Software\\TortoiseSVN\\DiffByDoubleClickInLog"), FALSE))
			{
				DiffSelectedRevWithPrevious();
				return TRUE;
			}
		}
		if (GetFocus()==GetDlgItem(IDC_LOGMSG))
		{
			DiffSelectedFile();
			return TRUE;
		}
	}
	if (m_hAccel && !bSkipAccelerator)
	{
		int ret = TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
		if (ret)
			return TRUE;
	}
	
	m_tooltips.RelayEvent(pMsg);
	return __super::PreTranslateMessage(pMsg);
}

BOOL CLogDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bLogThreadRunning)
	{
		if (!IsCursorOverWindowBorder() && ((pWnd)&&(pWnd != GetDlgItem(IDCANCEL))))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	if ((pWnd) && (pWnd == GetDlgItem(IDC_MSGVIEW)))
		return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);

	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CLogDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CLogDlg::ToggleCheckbox(size_t item)
{
	if (!SysInfo::Instance().IsVistaOrLater())
	{
		if (item < m_logEntries.GetVisibleCount())
		{
			PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(item);
			if (pLogEntry)
			{
				pLogEntry->SetChecked (!pLogEntry->GetChecked());
				m_LogList.RedrawItems ((int)item, (int)item);
			}
		}
	}
}

void CLogDlg::OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if ((m_bLogThreadRunning)||(m_LogList.HasText()))
		return;
	if (pNMLV->iItem >= 0)
	{
		if (pNMLV->iSubItem != 0)
			return;

        size_t item = pNMLV->iItem;
		if ((item == m_logEntries.GetVisibleCount())&&(m_bStrict)&&(m_bStrictStopped))
		{
			// remove the selected state
			if (pNMLV->uChanged & LVIF_STATE)
			{
				m_LogList.SetItemState(pNMLV->iItem, 0, LVIS_SELECTED);
				FillLogMessageCtrl();
				UpdateData(FALSE);
				UpdateLogInfoLabel();
			}
			return;
		}
		if (pNMLV->uChanged & LVIF_STATE)
		{
			FillLogMessageCtrl();
			UpdateData(FALSE);
			if (SysInfo::Instance().IsVistaOrLater())
			{
				if (item < m_logEntries.GetVisibleCount())
				{
					PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(item);
					if (pLogEntry)
					{
						pLogEntry->SetChecked ((pNMLV->uNewState & LVIS_SELECTED) != 0);
					}
				}
			}
		}
	}
	else
	{
		FillLogMessageCtrl();
		UpdateData(FALSE);
	}
	EnableOKButton();
	UpdateLogInfoLabel();
}

void CLogDlg::OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	ENLINK *pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink->msg != WM_LBUTTONUP)
		return;

	CString url, msg;
	GetDlgItemText(IDC_MSGVIEW, msg);
	msg.Replace(_T("\r\n"), _T("\n"));
	url = msg.Mid(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax-pEnLink->chrg.cpMin);
	if (!::PathIsURL(url))
	{
		// not a full url: either a bug ID or a revision.
		std::set<CString> bugIDs = m_ProjectProperties.FindBugIDs(msg);
		bool bBugIDFound = false;
		for (std::set<CString>::iterator it = bugIDs.begin(); it != bugIDs.end(); ++it)
		{
			if (it->Compare(url) == 0)
			{
				url = m_ProjectProperties.GetBugIDUrl(url);
				url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
				bBugIDFound = true;
				break;
			}
		}
		if (!bBugIDFound)
		{
			// now check whether it matches a revision
			const tr1::wregex regMatch(m_ProjectProperties.sLogRevRegex, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
			const tr1::wsregex_iterator end;
			wstring s = msg;
			for (tr1::wsregex_iterator it(s.begin(), s.end(), regMatch); it != end; ++it)
			{
				wstring matchedString = (*it)[0];
				const tr1::wregex regRevMatch(_T("\\d+"));
				wstring ss = matchedString;
				for (tr1::wsregex_iterator it2(ss.begin(), ss.end(), regRevMatch); it2 != end; ++it2)
				{
					wstring matchedRevString = (*it2)[0];
					if (url.Compare(matchedRevString.c_str()) == 0)
					{
						svn_revnum_t rev = _ttol(matchedRevString.c_str());
						ATLTRACE(_T("found revision %ld\n"), rev);
						// do we already show this revision? If yes, just select that revision and 'scroll' to it
						for (size_t i=0; i<m_logEntries.GetVisibleCount(); ++i)
						{
							PLOGENTRYDATA data = m_logEntries.GetVisible(i);
							if (!data)
								continue;
							if (data->GetRevision() != rev)
								continue;
							int selMark = m_LogList.GetSelectionMark();
							if (selMark>=0)
							{
								m_LogList.SetItemState(selMark, 0, LVIS_SELECTED);
							}
							m_LogList.EnsureVisible((int)i, FALSE);
							m_LogList.SetSelectionMark((int)i);
							m_LogList.SetItemState((int)i, LVIS_SELECTED, LVIS_SELECTED);
							return;
						}
						try
						{
							CLogCacheUtility logUtil(GetLogCachePool()->GetCache(m_sUUID, m_sRepositoryRoot), &m_ProjectProperties);
							if (logUtil.IsCached(rev))
							{
								PLOGENTRYDATA pLogItem = logUtil.GetRevisionData(rev);
								if (pLogItem)
								{
									// insert the data
                                    m_logEntries.Sort(CLogDataVector::RevisionCol, false);
                                    m_logEntries.AddSorted (pLogItem, &m_ProjectProperties);

									int selMark = m_LogList.GetSelectionMark();
									// now start filter the log list
                                    SortAndFilter (rev);
									m_LogList.SetItemCountEx(ShownCountWithStopped());
									m_LogList.RedrawItems(0, ShownCountWithStopped());
									if (selMark >= 0)
										m_LogList.SetSelectionMark(selMark);
									m_LogList.Invalidate();

									for (size_t i=0; i<m_logEntries.GetVisibleCount(); ++i)
									{
										PLOGENTRYDATA data = m_logEntries.GetVisible(i);
										if (!data)
											continue;
										if (data->GetRevision() != rev)
											continue;
										if (selMark>=0)
										{
											m_LogList.SetItemState(selMark, 0, LVIS_SELECTED);
										}
										m_LogList.EnsureVisible((int)i, FALSE);
										m_LogList.SetSelectionMark((int)i);
										m_LogList.SetItemState((int)i, LVIS_SELECTED, LVIS_SELECTED);
										return;
									}
								}
							}
						}
						catch (CException* e)
						{
							e->Delete();
						}

						// if we get here, then the linked revision is not shown in this dialog:
						// start a new log dialog for the repository root and this revision
						CString sCmd;
						sCmd.Format(_T("%s /command:log /path:\"%s\" /startrev:%ld /propspath:\"%s\""),
							(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
							(LPCTSTR)m_sRepositoryRoot, rev, (LPCTSTR)m_path.GetWinPath());
						CAppUtils::LaunchApplication(sCmd, NULL, false);
						return;
					}
				}
			}
		}
	}

    if (!url.IsEmpty())
		ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
}

void CLogDlg::OnBnClickedStatbutton()
{
	if ((m_bLogThreadRunning)||(m_LogList.HasText()))
		return;
    if (m_logEntries.GetVisibleCount() == 0)
		return;		// nothing is shown, so no statistics.

    // the statistics dialog expects the log entries to be sorted by date
    // and we must remove duplicate entries created by merge info etc.

    typedef std::map<__time64_t, PLOGENTRYDATA> TMap;
    TMap revsByDate;

    std::set<svn_revnum_t> revisionsCovered;
    for (size_t i=0; i<m_logEntries.GetVisibleCount(); ++i)
    {
		PLOGENTRYDATA entry = m_logEntries.GetVisible(i);
        if (revisionsCovered.insert (entry->GetRevision()).second)
            revsByDate.insert (std::make_pair (entry->GetDate(), entry));
    }

    // create arrays which are aware of the current filter
	CStringArray m_arAuthorsFiltered;
	CDWordArray m_arDatesFiltered;
	CDWordArray m_arFileChangesFiltered;
    for ( TMap::const_reverse_iterator iter = revsByDate.rbegin()
        , end = revsByDate.rend()
        ; iter != end
        ; ++iter)
	{
		PLOGENTRYDATA pLogEntry = iter->second;
		CString strAuthor = pLogEntry->GetAuthor();
		if ( strAuthor.IsEmpty() )
		{
			strAuthor.LoadString(IDS_STATGRAPH_EMPTYAUTHOR);
		}
		m_arAuthorsFiltered.Add(strAuthor);
		m_arDatesFiltered.Add(static_cast<DWORD>(pLogEntry->GetDate()));
        m_arFileChangesFiltered.Add (static_cast<DWORD>(pLogEntry->GetChangedPaths().GetCount()));
	}
	CStatGraphDlg dlg;
	dlg.m_parAuthors = &m_arAuthorsFiltered;
	dlg.m_parDates = &m_arDatesFiltered;
	dlg.m_parFileChanges = &m_arFileChangesFiltered;
	dlg.m_path = m_path;
	dlg.DoModal();
	OnTimer(LOGFILTER_TIMER);
}

void CLogDlg::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	if (m_bLogThreadRunning)
		return;

	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT:
		{
			// This is the prepaint stage for an item. Here's where we set the
			// item's text color. 
			
			// Tell Windows to send draw notifications for each subitem.
			*pResult = CDRF_NOTIFYSUBITEMDRAW;

			COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

			if (m_logEntries.GetVisibleCount() > pLVCD->nmcd.dwItemSpec)
			{
				PLOGENTRYDATA data = m_logEntries.GetVisible (pLVCD->nmcd.dwItemSpec);
				if (data)
				{
                    if (data->GetChangedPaths().ContainsSelfCopy())
					{
						// only change the background color if the item is not 'hot' (on vista with themes enabled)
						if (!theme.IsAppThemed() || !SysInfo::Instance().IsVistaOrLater() || ((pLVCD->nmcd.uItemState & CDIS_HOT)==0))
							pLVCD->clrTextBk = GetSysColor(COLOR_MENU);
					}
                    if (data->GetChangedPaths().ContainsCopies())
						crText = m_Colors.GetColor(CColors::Modified);
					if ((data->GetDepth())||(m_mergedRevs.find(data->GetRevision()) != m_mergedRevs.end()))
						crText = GetSysColor(COLOR_GRAYTEXT);
					if (data->GetRevision() == m_wcRev)
					{
						SelectObject(pLVCD->nmcd.hdc, m_boldFont);
						// We changed the font, so we're returning CDRF_NEWFONT. This
						// tells the control to recalculate the extent of the text.
						*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
					}
				}
			}
			if (m_logEntries.GetVisibleCount() == pLVCD->nmcd.dwItemSpec)
			{
				if (m_bStrictStopped)
					crText = GetSysColor(COLOR_GRAYTEXT);
			}
			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT|CDDS_ITEM|CDDS_SUBITEM:
		{
			if ((m_bStrictStopped)&&(m_logEntries.GetVisibleCount() == pLVCD->nmcd.dwItemSpec))
			{
				pLVCD->nmcd.uItemState &= ~(CDIS_SELECTED|CDIS_FOCUS);
			}
			if (pLVCD->iSubItem == 1)
			{
				*pResult = CDRF_DODEFAULT;

				if (m_logEntries.GetVisibleCount() <= pLVCD->nmcd.dwItemSpec)
					return;

				int		nIcons = 0;
				int		iconwidth = ::GetSystemMetrics(SM_CXSMICON);
				int		iconheight = ::GetSystemMetrics(SM_CYSMICON);

				PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (pLVCD->nmcd.dwItemSpec);

				// Get the selected state of the
				// item being drawn.
				LVITEM   rItem;
				SecureZeroMemory(&rItem, sizeof(LVITEM));
				rItem.mask  = LVIF_STATE;
				rItem.iItem = (int)pLVCD->nmcd.dwItemSpec;
				rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
				m_LogList.GetItem(&rItem);

				CRect rect;
				m_LogList.GetSubItemRect((int)pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);

				// Fill the background
				if (theme.IsAppThemed() && SysInfo::Instance().IsVistaOrLater())
				{
					theme.Open(m_hWnd, L"Explorer");
					int state = LISS_NORMAL;
					if (rItem.state & LVIS_SELECTED)
					{
						if (::GetFocus() == m_LogList.m_hWnd)
							state |= LISS_SELECTED;
						else
							state |= LISS_SELECTEDNOTFOCUS;
					}
					else
					{
                        if (pLogEntry->GetChangedPaths().ContainsSelfCopy())
						{
							// unfortunately, the pLVCD->nmcd.uItemState does not contain valid
							// information at this drawing stage. But we can check the whether the
							// previous stage changed the background color of the item
							if (pLVCD->clrTextBk == GetSysColor(COLOR_MENU))
							{
								HBRUSH brush;
								brush = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
								if (brush)
								{
									::FillRect(pLVCD->nmcd.hdc, &rect, brush);
									::DeleteObject(brush);
								}
							}
						}
					}

					if (theme.IsBackgroundPartiallyTransparent(LVP_LISTDETAIL, state))
						theme.DrawParentBackground(m_hWnd, pLVCD->nmcd.hdc, &rect);

					theme.DrawBackground(pLVCD->nmcd.hdc, LVP_LISTDETAIL, state, &rect, NULL);
				}
				else
				{
					HBRUSH brush;
					if (rItem.state & LVIS_SELECTED)
					{
						if (::GetFocus() == m_LogList.m_hWnd)
							brush = ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
						else
							brush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
					}
					else
					{
						if (pLogEntry->GetChangedPaths().ContainsSelfCopy())
							brush = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
						else
							brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
					}
					if (brush == NULL)
						return;

					::FillRect(pLVCD->nmcd.hdc, &rect, brush);
					::DeleteObject(brush);
				}

				// Draw the icon(s) into the compatible DC

                DWORD actions = pLogEntry->GetChangedPaths().GetActions();
				if (actions & LOGACTIONS_MODIFIED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hModifiedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (actions & LOGACTIONS_ADDED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hAddedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (actions & LOGACTIONS_DELETED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hDeletedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (actions & LOGACTIONS_REPLACED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hReplacedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if ((pLogEntry->GetDepth())||(m_mergedRevs.find(pLogEntry->GetRevision()) != m_mergedRevs.end()))
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hMergedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				*pResult = CDRF_SKIPDEFAULT;
				return;
			}
		}
		break;
	}
	*pResult = CDRF_DODEFAULT;
}
void CLogDlg::OnNMCustomdrawChangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	if (m_bLogThreadRunning)
		return;

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
		bool bGrayed = false;
		if ((m_cShowPaths.GetState() & 0x0003)==BST_UNCHECKED)
		{
			if (m_currentChangedArray.GetCount() > pLVCD->nmcd.dwItemSpec)
			{
                if (!m_currentChangedArray[pLVCD->nmcd.dwItemSpec].IsRelevantForStartPath())
				{
					crText = GetSysColor(COLOR_GRAYTEXT);
					bGrayed = true;
				}
			}
			else if ((DWORD_PTR)m_currentChangedPathList.GetCount() > pLVCD->nmcd.dwItemSpec)
			{
				if (m_currentChangedPathList[pLVCD->nmcd.dwItemSpec].GetSVNPathString().Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)!=0)
				{
					crText = GetSysColor(COLOR_GRAYTEXT);
					bGrayed = true;
				}
			}
		}

		if ((!bGrayed)&&(m_currentChangedArray.GetCount() > pLVCD->nmcd.dwItemSpec))
		{
			DWORD action = m_currentChangedArray[pLVCD->nmcd.dwItemSpec].GetAction();
			if (action == LOGACTIONS_MODIFIED)
				crText = m_Colors.GetColor(CColors::Modified);
			if (action == LOGACTIONS_REPLACED)
				crText = m_Colors.GetColor(CColors::Deleted);
			if (action == LOGACTIONS_ADDED)
				crText = m_Colors.GetColor(CColors::Added);
			if (action == LOGACTIONS_DELETED)
				crText = m_Colors.GetColor(CColors::Deleted);
		}

		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = crText;
	}
}

void CLogDlg::DoSizeV1(int delta)
{
	RemoveMainAnchors();

	// first, reduce the middle section to a minimum.
	// if that is not sufficient, minimize the lower section 

	CRect changeListViewRect;
	m_ChangedFileListCtrl.GetClientRect (changeListViewRect);
	CRect messageViewRect;
	GetDlgItem(IDC_MSGVIEW)->GetClientRect (messageViewRect);

	int messageViewDelta = max (-delta, 20 - messageViewRect.Height());
	int changeFileListDelta = -delta - messageViewDelta;

	// set new sizes & positions

	CSplitterControl::ChangeHeight(&m_LogList, delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), messageViewDelta, CW_TOPALIGN);
	CSplitterControl::ChangePos(GetDlgItem(IDC_MSGVIEW), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_SPLITTERBOTTOM), 0, -changeFileListDelta);
	CSplitterControl::ChangeHeight(&m_ChangedFileListCtrl, changeFileListDelta, CW_BOTTOMALIGN);

	AddMainAnchors();
	ArrangeLayout();
	AdjustMinSize();
	SetSplitterRange();
	m_LogList.Invalidate();
	m_ChangedFileListCtrl.Invalidate();
	GetDlgItem(IDC_MSGVIEW)->Invalidate();
}

void CLogDlg::DoSizeV2(int delta)
{
	RemoveMainAnchors();

	// first, reduce the middle section to a minimum.
	// if that is not sufficient, minimize the top section 

	CRect logViewRect;
	m_LogList.GetClientRect (logViewRect);
	CRect messageViewRect;
	GetDlgItem(IDC_MSGVIEW)->GetClientRect (messageViewRect);

	int messageViewDelta = max (delta, 20 - messageViewRect.Height());
	int logListDelta = delta - messageViewDelta;

	// set new sizes & positions

	CSplitterControl::ChangeHeight(&m_LogList, logListDelta, CW_TOPALIGN);
	CSplitterControl::ChangePos(GetDlgItem(IDC_SPLITTERTOP), 0, logListDelta);

	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), messageViewDelta, CW_TOPALIGN);
	CSplitterControl::ChangePos(GetDlgItem(IDC_MSGVIEW), 0, logListDelta);

	CSplitterControl::ChangeHeight(&m_ChangedFileListCtrl, -delta, CW_BOTTOMALIGN);

	AddMainAnchors();
	ArrangeLayout();
	AdjustMinSize();
	SetSplitterRange();
	GetDlgItem(IDC_MSGVIEW)->Invalidate();
	m_ChangedFileListCtrl.Invalidate();
}

void CLogDlg::AdjustMinSize()
{
	// adjust the minimum size of the dialog to prevent the resizing from
	// moving the list control too far down.
	CRect rcChgListView;
	m_ChangedFileListCtrl.GetClientRect(rcChgListView);
	CRect rcLogList;
	m_LogList.GetClientRect(rcLogList);
	CRect rcLogMsg;
	GetDlgItem(IDC_MSGVIEW)->GetClientRect(rcLogMsg);
 
 	SetMinTrackSize(CSize(m_DlgOrigRect.Width(), 
 		m_DlgOrigRect.Height()-m_ChgOrigRect.Height()-m_LogListOrigRect.Height()-m_MsgViewOrigRect.Height()
		+rcLogMsg.Height()+abs(rcChgListView.Height()-rcLogList.Height())+60));
}

LRESULT CLogDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_SPLITTERTOP)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSizeV1(pHdr->delta);
		}
		else if (wParam == IDC_SPLITTERBOTTOM)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSizeV2(pHdr->delta);
		}
		break;
	}

	return CResizableDialog::DefWindowProc(message, wParam, lParam);
}

void CLogDlg::SetSplitterRange()
{
	if ((m_LogList)&&(m_ChangedFileListCtrl))
	{
		CRect rcTop;
		m_LogList.GetWindowRect(rcTop);
		ScreenToClient(rcTop);

		CRect rcMiddle;
		GetDlgItem(IDC_MSGVIEW)->GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);

		CRect rcBottom;
		m_ChangedFileListCtrl.GetWindowRect(rcBottom);
		ScreenToClient(rcBottom);

		m_wndSplitter1.SetRange(rcTop.top+20, rcBottom.bottom-50);
		m_wndSplitter2.SetRange(rcTop.top+50, rcBottom.bottom-20);
	}
}

LRESULT CLogDlg::OnClickedInfoIcon(WPARAM /*wParam*/, LPARAM lParam)
{
	RECT * rect = (LPRECT)lParam;
	CPoint point;
	CString temp;
	point = CPoint(rect->left, rect->bottom);
#define LOGMENUFLAGS(x) (MF_STRING | MF_ENABLED | (m_nSelectedFilter == x ? MF_CHECKED : MF_UNCHECKED))
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		temp.LoadString(IDS_LOG_FILTER_ALL);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_ALL), LOGFILTER_ALL, temp);

		popup.AppendMenu(MF_SEPARATOR, NULL);
		
		temp.LoadString(IDS_LOG_FILTER_MESSAGES);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_MESSAGES), LOGFILTER_MESSAGES, temp);
		temp.LoadString(IDS_LOG_FILTER_PATHS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_PATHS), LOGFILTER_PATHS, temp);
		temp.LoadString(IDS_LOG_FILTER_AUTHORS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_AUTHORS), LOGFILTER_AUTHORS, temp);
		temp.LoadString(IDS_LOG_FILTER_REVS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_REVS), LOGFILTER_REVS, temp);
		temp.LoadString(IDS_LOG_FILTER_BUGIDS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_BUGID), LOGFILTER_BUGID, temp);
		
		popup.AppendMenu(MF_SEPARATOR, NULL);

		temp.LoadString(IDS_LOG_FILTER_REGEX);
		popup.AppendMenu(MF_STRING | MF_ENABLED | (m_bFilterWithRegex ? MF_CHECKED : MF_UNCHECKED), LOGFILTER_REGEX, temp);
		temp.LoadString(IDS_LOG_FILTER_CASESENSITIVE);
		popup.AppendMenu(MF_STRING | MF_ENABLED | (m_bFilterCaseSensitively ? MF_CHECKED : MF_UNCHECKED), LOGFILTER_CASE, temp);

		m_tooltips.Pop();
		int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (selection)
		{
        case 0 : 
            break;

        case LOGFILTER_REGEX :
			m_bFilterWithRegex = !m_bFilterWithRegex;
			CRegDWORD(_T("Software\\TortoiseSVN\\UseRegexFilter")) = m_bFilterWithRegex;
			CheckRegexpTooltip();
            break;

        case LOGFILTER_CASE:
			m_bFilterCaseSensitively = !m_bFilterCaseSensitively;
			CRegDWORD(_T("Software\\TortoiseSVN\\FilterCaseSensitively")) = m_bFilterCaseSensitively;
            break;

        default:

            m_nSelectedFilter = selection;
		    SetFilterCueText();
		}

        if (selection != 0)
			SetTimer(LOGFILTER_TIMER, 1000, NULL);
	}
	return 0L;
}

LRESULT CLogDlg::OnClickedCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	KillTimer(LOGFILTER_TIMER);

	m_sFilterText.Empty();
	UpdateData(FALSE);
	theApp.DoWaitCursor(1);
    AutoStoreSelection();
	FillLogMessageCtrl(false);

	// reset the time filter too
    m_logEntries.ClearFilter();
    m_timFrom = m_logEntries.GetMinDate();
	m_timTo = m_logEntries.GetMaxDate();
	m_DateFrom.SetTime(&m_timFrom);
	m_DateTo.SetTime(&m_timTo);
	m_DateFrom.SetRange(&m_timFrom, &m_timTo);
	m_DateTo.SetRange(&m_timFrom, &m_timTo);

    m_LogList.SetItemCountEx(0);
    m_LogList.SetItemCountEx(ShownCountWithStopped());
	m_LogList.RedrawItems(0, ShownCountWithStopped());
	m_LogList.SetRedraw(false);
	ResizeAllListCtrlCols(true);
	m_LogList.SetRedraw(true);

    theApp.DoWaitCursor(-1);
	GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_SEARCHEDIT)->SetFocus();

    AutoRestoreSelection();

	return 0L;	
}


void CLogDlg::SetFilterCueText()
{
	CString temp;
	switch (m_nSelectedFilter)
	{
	case LOGFILTER_ALL:
		temp.LoadString(IDS_LOG_FILTER_ALL);
		break;
	case LOGFILTER_MESSAGES:
		temp.LoadString(IDS_LOG_FILTER_MESSAGES);
		break;
	case LOGFILTER_PATHS:
		temp.LoadString(IDS_LOG_FILTER_PATHS);
		break;
	case LOGFILTER_AUTHORS:
		temp.LoadString(IDS_LOG_FILTER_AUTHORS);
		break;
	case LOGFILTER_REVS:
		temp.LoadString(IDS_LOG_FILTER_REVS);
		break;
	}
	// to make the cue banner text appear more to the right of the edit control
	temp = _T("   ")+temp;
	m_cFilter.SetCueBanner(temp);
}

void CLogDlg::OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = 0;

	// Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	// Which item number?
	size_t itemid = pItem->iItem;
	PLOGENTRYDATA pLogEntry = NULL;
	if (itemid < m_logEntries.GetVisibleCount())
		pLogEntry = m_logEntries.GetVisible (pItem->iItem);

	if (pItem->mask & LVIF_INDENT)
	{
		pItem->iIndent = 0;
	}
	if (pItem->mask & LVIF_IMAGE)
	{
		pItem->mask |= LVIF_STATE;
		pItem->stateMask = LVIS_STATEIMAGEMASK;

		if (pLogEntry)
		{
			if (pLogEntry->GetChecked())
			{
				//Turn check box on
				pItem->state = INDEXTOSTATEIMAGEMASK(2);
			}
			else
			{
				//Turn check box off
				pItem->state = INDEXTOSTATEIMAGEMASK(1);
			}
		}
	}
	if (pItem->mask & LVIF_TEXT)
	{
		// By default, clear text buffer.
		lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);

		bool bOutOfRange = pItem->iItem >= ShownCountWithStopped();

		if (m_bLogThreadRunning || bOutOfRange)
			return;


		// Which column?
		switch (pItem->iSubItem)
		{
		case 0:	//revision
			if (pLogEntry)
			{
				_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld"), pLogEntry->GetRevision());
				// to make the child entries indented, add spaces
				size_t len = _tcslen(pItem->pszText);
				TCHAR * pBuf = pItem->pszText + len;
				DWORD nSpaces = m_logEntries.GetMaxDepth() - pLogEntry->GetDepth();
				while ((pItem->cchTextMax >= (int)len)&&(nSpaces))
				{
					*pBuf = ' ';
					pBuf++;
					nSpaces--;
				}
				*pBuf = 0;
			}
			break;
		case 1: //action -- no text in the column
			break;
		case 2: //author
			if (pLogEntry)
				lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetAuthor(), pItem->cchTextMax);
			break;
		case 3: //date
			if (pLogEntry)
				lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetDateString(), pItem->cchTextMax);
			break;
		case 4: //message or bug id
			if (m_bShowBugtraqColumn)
			{
				if (pLogEntry)
					lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetBugIDs(), pItem->cchTextMax);
				break;
			}
			// fall through here!
		case 5:
			if (pLogEntry)
			{
				// Add as many characters as possible from the short log message
				// to the list control. If the message is longer than
				// allowed width, add "..." as an indication.
				const int dots_len = 3;
				if (pLogEntry->GetShortMessage().GetLength() >= pItem->cchTextMax && pItem->cchTextMax > dots_len)
				{
					lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetShortMessage(), pItem->cchTextMax - dots_len);
					lstrcpyn(pItem->pszText + pItem->cchTextMax - dots_len - 1, _T("..."), dots_len + 1);
				}
				else
					lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetShortMessage(), pItem->cchTextMax);
			}
			else if ((itemid == m_logEntries.GetVisibleCount()) && m_bStrict && m_bStrictStopped)
			{
				CString sTemp;
				sTemp.LoadString(IDS_LOG_STOPONCOPY_HINT);
				lstrcpyn(pItem->pszText, sTemp, pItem->cchTextMax);
			}
			break;
		default:
			ASSERT(false);
		}
	}
}

void CLogDlg::OnLvnGetdispinfoChangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	//Create a pointer to the item
	LV_ITEM* pItem= &(pDispInfo)->item;

	*pResult = 0;
	if (m_bLogThreadRunning)
	{
		if (pItem->mask & LVIF_TEXT)
			lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		return;
	}
    if (m_bSingleRevision && ((size_t)pItem->iItem >= m_currentChangedArray.GetCount()))
	{
		if (pItem->mask & LVIF_TEXT)
			lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		return;
	}
	if (!m_bSingleRevision && (pItem->iItem >= m_currentChangedPathList.GetCount()))
	{
		if (pItem->mask & LVIF_TEXT)
			lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		return;
	}

	//Does the list need text information?
	if (pItem->mask & LVIF_TEXT)
	{
		//Which column?
		switch (pItem->iSubItem)
		{
		case 0: //path
    		lstrcpyn ( pItem->pszText
                     , (LPCTSTR) (m_bSingleRevision
                           ? m_currentChangedArray[pItem->iItem].GetPath()
                           : m_currentChangedPathList[pItem->iItem].GetSVNPathString())
                     , pItem->cchTextMax);
			break;

        case 1:	//Action
			lstrcpyn ( pItem->pszText
                     , m_currentChangedArray.GetCount() > (size_t)pItem->iItem
                           ? (LPCTSTR)m_currentChangedArray[pItem->iItem].GetActionString()
                           : _T("")
                     , pItem->cchTextMax);
			break;

		case 2: //copyfrom path
			lstrcpyn ( pItem->pszText
                     , m_currentChangedArray.GetCount() > (size_t)pItem->iItem
                           ? (LPCTSTR)m_currentChangedArray[pItem->iItem].GetCopyFromPath()
                           : _T("")
                     , pItem->cchTextMax);
			break;

		case 3: //revision
            svn_revnum_t revision = 0;
			if (m_currentChangedArray.GetCount() > (size_t)pItem->iItem)
                revision = m_currentChangedArray[pItem->iItem].GetCopyFromRev();

			if (revision == 0)
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			else
				_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld"), revision);
			break;
		}
	}
	if (pItem->mask & LVIF_IMAGE)
	{
		int icon_idx = 0;
		if (m_currentChangedArray.GetCount() > (size_t)pItem->iItem)
		{
            const CLogChangedPath& cpath = m_currentChangedArray[pItem->iItem];
			if (cpath.GetNodeKind() == svn_node_dir)
				icon_idx = m_nIconFolder;
			else
				icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(CTSVNPath(cpath.GetPath()));
		}
		else
		{
			icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(m_currentChangedPathList[pItem->iItem]);
		}
		pDispInfo->item.iImage = icon_idx;
	}

	*pResult = 0;
}

void CLogDlg::OnEnChangeSearchedit()
{
	UpdateData();
	if (m_sFilterText.IsEmpty())
	{
        AutoStoreSelection();

        // clear the filter, i.e. make all entries appear
		theApp.DoWaitCursor(1);
		KillTimer(LOGFILTER_TIMER);
		FillLogMessageCtrl(false);
        m_logEntries.Filter (m_tFrom, m_tTo);
		m_LogList.SetItemCountEx(0);
		m_LogList.SetItemCountEx(ShownCountWithStopped());
		m_LogList.RedrawItems(0, ShownCountWithStopped());
		m_LogList.SetRedraw(false);
		ResizeAllListCtrlCols(true);
		m_LogList.SetRedraw(true);
		theApp.DoWaitCursor(-1);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
		DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning)||(m_logEntries.GetVisibleCount() == 0))));

        AutoRestoreSelection();
		return;
	}
	if (Validate(m_sFilterText))
		SetTimer(LOGFILTER_TIMER, 1000, NULL);
	else
		KillTimer(LOGFILTER_TIMER);
}

bool CLogDlg::ValidateRegexp(LPCTSTR regexp_str, tr1::wregex& pat, bool bMatchCase /* = false */)
{
	try
	{
		tr1::regex_constants::syntax_option_type type = tr1::regex_constants::ECMAScript;
		if (!bMatchCase)
			type |= tr1::regex_constants::icase;
		pat = tr1::wregex(regexp_str, type);
		return true;
	}
	catch (exception) {}
	return false;
}

bool CLogDlg::Validate(LPCTSTR string)
{
	if (!m_bFilterWithRegex)
		return true;
	tr1::wregex pat;
	return ValidateRegexp(string, pat, false);
}

void CLogDlg::RecalculateShownList(svn_revnum_t revToKeep)
{
    // actually filter the data

    bool scanRelevantPathsOnly = (m_cShowPaths.GetState() & 0x0003)==BST_CHECKED;
    CLogDlgFilter filter ( m_sFilterText
                         , m_bFilterWithRegex
                         , m_nSelectedFilter
                         , m_bFilterCaseSensitively
                         , m_tFrom
                         , m_tTo
                         , scanRelevantPathsOnly
                         , revToKeep);
    m_logEntries.Filter (filter);
}

void CLogDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == LOGFILTER_TIMER)
	{
		if (m_bLogThreadRunning)
		{
			// thread still running! So just restart the timer.
			SetTimer(LOGFILTER_TIMER, 1000, NULL);
			return;
		}
		CWnd * focusWnd = GetFocus();
		bool bSetFocusToFilterControl = ((focusWnd != GetDlgItem(IDC_DATEFROM))&&(focusWnd != GetDlgItem(IDC_DATETO))
			&& (focusWnd != GetDlgItem(IDC_LOGLIST)));
		if (m_sFilterText.IsEmpty())
		{
			DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning)||(m_logEntries.GetVisibleCount() == 0))));
			// do not return here!
			// we also need to run the filter if the filter text is empty:
			// 1. to clear an existing filter
			// 2. to rebuild the filtered list after sorting
		}
		theApp.DoWaitCursor(1);
        AutoStoreSelection();
		KillTimer(LOGFILTER_TIMER);

		// now start filter the log list
        SortAndFilter();

		m_LogList.SetItemCountEx(0);
		m_LogList.SetItemCountEx(ShownCountWithStopped());
		m_LogList.RedrawItems(0, ShownCountWithStopped());
		m_LogList.SetRedraw(false);
		ResizeAllListCtrlCols(true);
		m_LogList.SetRedraw(true);
		m_LogList.Invalidate();
		if ( m_LogList.GetItemCount()==1 )
		{
			m_LogList.SetSelectionMark(0);
			m_LogList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
		}
		theApp.DoWaitCursor(-1);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
		if (bSetFocusToFilterControl)
			GetDlgItem(IDC_SEARCHEDIT)->SetFocus();

        AutoRestoreSelection();
	} // if (nIDEvent == LOGFILTER_TIMER)
	DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning)||(m_logEntries.GetVisibleCount() == 0))));
	__super::OnTimer(nIDEvent);
}

void CLogDlg::OnDtnDatetimechangeDateto(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	CTime _time;
	m_DateTo.GetTime(_time);
	try
	{
		CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 23, 59, 59);
		if (time.GetTime() != m_tTo)
		{
			m_tTo = (DWORD)time.GetTime();
			SetTimer(LOGFILTER_TIMER, 10, NULL);
		}
	}
	catch (CAtlException)
	{
	}
	
	*pResult = 0;
}

void CLogDlg::OnDtnDatetimechangeDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	CTime _time;
	m_DateFrom.GetTime(_time);
	try
	{
		CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 0, 0, 0);
		if (time.GetTime() != m_tFrom)
		{
			m_tFrom = (DWORD)time.GetTime();
			SetTimer(LOGFILTER_TIMER, 10, NULL);
		}
	}
	catch (CAtlException)
	{
	}
	
	*pResult = 0;
}

BOOL CLogDlg::IsEntryInDateRange(int i)
{
	__time64_t time = m_logEntries[i]->GetDate();
	if ((time >= m_tFrom)&&(time <= m_tTo))
		return TRUE;
	return FALSE;
}

CTSVNPathList CLogDlg::GetChangedPathsFromSelectedRevisions(bool bRelativePaths /* = false */, bool bUseFilter /* = true */)
{
	CTSVNPathList pathList;
	if (m_sRepositoryRoot.IsEmpty() && (bRelativePaths == false))
	{
		m_sRepositoryRoot = GetRepositoryRoot(m_path);
	}
	if (m_sRepositoryRoot.IsEmpty() && (bRelativePaths == false))
		return pathList;
	
    quick_hash_set<LogCache::index_t> pathIDsAdded;
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		while (pos)
		{
			size_t nextpos = m_LogList.GetNextSelectedItem(pos);
			if (nextpos >= m_logEntries.GetVisibleCount())
				continue;

			PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (nextpos);
			const CLogChangedPathArray& cpatharray = pLogEntry->GetChangedPaths();
			for (size_t cpPathIndex = 0; cpPathIndex<cpatharray.GetCount(); ++cpPathIndex)
			{
				const CLogChangedPath& cpath = cpatharray[cpPathIndex];

                LogCache::index_t pathID = cpath.GetCachedPath().GetIndex();
                if (pathIDsAdded.contains (pathID))
                    continue;

                pathIDsAdded.insert (pathID);

                CTSVNPath path;
				if (!bRelativePaths)
					path.SetFromSVN(m_sRepositoryRoot);
				path.AppendPathString(cpath.GetPath());
				if (   !bUseFilter
					|| ((m_cShowPaths.GetState() & 0x0003)!=BST_CHECKED)
                    || cpath.IsRelevantForStartPath())
					pathList.AddPath(path);
				
			}
		}
	}
	return pathList;
}

void CLogDlg::SortByColumn(int nSortColumn, bool bAscending)
{
    if (nSortColumn > 5)
    {
		ATLASSERT(0);
		return;
    }

    // sort by message instead of bug id, if bug ids are not visible
    if ((nSortColumn == 4) && !m_bShowBugtraqColumn)
        ++nSortColumn;

    m_logEntries.Sort (CLogDataVector::SortColumn (nSortColumn), bAscending);
}

void CLogDlg::SortAndFilter (svn_revnum_t revToKeep)
{
    SortByColumn (m_nSortColumn, m_bAscending);
    RecalculateShownList (revToKeep);
}

void CLogDlg::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if ((m_bLogThreadRunning)||(m_LogList.HasText()))
		return;		//no sorting while the arrays are filled
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int nColumn = pNMLV->iSubItem;
	m_bAscending = nColumn == m_nSortColumn ? !m_bAscending : TRUE;
	m_nSortColumn = nColumn;
	SortAndFilter();
	SetSortArrow(&m_LogList, m_nSortColumn, !!m_bAscending);
	
    // clear the selection states
	for (POSITION pos = m_LogList.GetFirstSelectedItemPosition(); pos != NULL; )
		m_LogList.SetItemState(m_LogList.GetNextSelectedItem(pos), 0, LVIS_SELECTED);

    m_LogList.SetSelectionMark(-1);

    m_LogList.Invalidate();
	UpdateLogInfoLabel();
	// the "next 100" button only makes sense if the log messages
	// are sorted by revision in descending order
	if ((m_nSortColumn)||(m_bAscending))
	{
		DialogEnableWindow(IDC_NEXTHUNDRED, false);
	}
	else
	{
		DialogEnableWindow(IDC_NEXTHUNDRED, true);
	}
	*pResult = 0;
}

void CLogDlg::SetSortArrow(CListCtrl * control, int nColumn, bool bAscending)
{
	if (control == NULL)
		return;
	// set the sort arrow
	CHeaderCtrl * pHeader = control->GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	if (nColumn >= 0)
	{
		pHeader->GetItem(nColumn, &HeaderItem);
		HeaderItem.fmt |= (bAscending ? HDF_SORTUP : HDF_SORTDOWN);
		pHeader->SetItem(nColumn, &HeaderItem);
	}
}
void CLogDlg::OnLvnColumnclickChangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
	if ((m_bLogThreadRunning)||(m_LogList.HasText()))
		return;		//no sorting while the arrays are filled

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int nColumn = pNMLV->iSubItem;

	// multi-rev selection shows paths only -> can only sort by column
	if ((m_currentChangedPathList.GetCount() > 0) && (nColumn > 0))
		return;

	m_bAscendingPathList = nColumn == m_nSortColumnPathList ? !m_bAscendingPathList : TRUE;
	m_nSortColumnPathList = nColumn;
	if (m_currentChangedArray.GetCount() > 0)
		m_currentChangedArray.Sort (m_nSortColumnPathList, m_bAscendingPathList);
	else
		m_currentChangedPathList.SortByPathname (!m_bAscendingPathList);

	SetSortArrow(&m_ChangedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
	m_ChangedFileListCtrl.Invalidate();
	*pResult = 0;
}

void CLogDlg::ResizeAllListCtrlCols(bool bOnlyVisible)
{
	const int nMinimumWidth = ICONITEMBORDER+16*5;
	int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int nItemCount = m_LogList.GetItemCount();
	TCHAR textbuf[MAX_PATH];
	CHeaderCtrl * pHdrCtrl = (CHeaderCtrl*)(m_LogList.GetDlgItem(0));
	if (pHdrCtrl)
	{
		size_t startRow = 0;
		size_t endRow = nItemCount;
		if (bOnlyVisible)
		{
			startRow = m_LogList.GetTopIndex();
			endRow = startRow + m_LogList.GetCountPerPage();
		}
		for (int col = 0; col <= maxcol; col++)
		{
			HDITEM hdi = {0};
			hdi.mask = HDI_TEXT;
			hdi.pszText = textbuf;
			hdi.cchTextMax = sizeof(textbuf);
			pHdrCtrl->GetItem(col, &hdi);
			int cx = m_LogList.GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin
			for (size_t index = startRow; index<endRow; ++index)
			{
				// get the width of the string and add 14 pixels for the column separator and margins
				int linewidth = m_LogList.GetStringWidth(m_LogList.GetItemText((int)index, col)) + 14;
				if (index < m_logEntries.GetVisibleCount())
				{
					PLOGENTRYDATA pCurLogEntry = m_logEntries.GetVisible(index);
					if ((pCurLogEntry)&&(pCurLogEntry->GetRevision() == m_wcRev))
					{
						HFONT hFont = (HFONT)m_LogList.SendMessage(WM_GETFONT);
						// set the bold font and ask for the string width again
						m_LogList.SendMessage(WM_SETFONT, (WPARAM)m_boldFont, NULL);
						linewidth = m_LogList.GetStringWidth(m_LogList.GetItemText((int)index, col)) + 14;
						// restore the system font
						m_LogList.SendMessage(WM_SETFONT, (WPARAM)hFont, NULL);
					}
				}
				if (index == 0)
				{
					// add the image size
					CImageList * pImgList = m_LogList.GetImageList(LVSIL_SMALL);
					if ((pImgList)&&(pImgList->GetImageCount()))
					{
						IMAGEINFO imginfo;
						pImgList->GetImageInfo(0, &imginfo);
						linewidth += (imginfo.rcImage.right - imginfo.rcImage.left);
						linewidth += 3;	// 3 pixels between icon and text
					}
				}
				if (cx < linewidth)
					cx = linewidth;
			}
			// Adjust columns "Actions" containing icons
			if (col == 1)
			{
				if (cx < nMinimumWidth)
				{
					cx = nMinimumWidth;
				}
			}
			if ((col == 0)&&(m_bSelect)&&(SysInfo::Instance().IsVistaOrLater()))
			{
				cx += 16;	// add space for the checkbox
			}
			// keep the bug id column small
			if ((col == 4)&&(m_bShowBugtraqColumn))
			{
				if (cx > (int)(DWORD)m_regMaxBugIDColWidth)
				{
					cx = (int)(DWORD)m_regMaxBugIDColWidth;
				}
			}

			m_LogList.SetColumnWidth(col, cx);
		}
	}
}

void CLogDlg::OnBnClickedHidepaths()
{
	FillLogMessageCtrl();
	m_ChangedFileListCtrl.Invalidate();
}


void CLogDlg::OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);
	*pResult = -1;
	
	if (pFindInfo->lvfi.flags & LVFI_PARAM)
		return;	
	if ((pFindInfo->iStart < 0)||(pFindInfo->iStart >= (int)m_logEntries.GetVisibleCount()))
		return;
	if (pFindInfo->lvfi.psz == 0)
		return;
		
	CString sCmp = pFindInfo->lvfi.psz;
	CString sRev;	
	for (size_t i=pFindInfo->iStart; i<m_logEntries.GetVisibleCount(); ++i)
	{
		PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(i);
		sRev.Format(_T("%ld"), pLogEntry->GetRevision());
		if (pFindInfo->lvfi.flags & LVFI_PARTIAL)
		{
			if (sCmp.Compare(sRev.Left(sCmp.GetLength()))==0)
			{
				*pResult = i;
				return;
			}
		}
		else
		{
			if (sCmp.Compare(sRev)==0)
			{
				*pResult = i;
				return;
			}
		}
	}
	if (pFindInfo->lvfi.flags & LVFI_WRAP)
	{
		for (int i=0; i<pFindInfo->iStart; ++i)
		{
			PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(i);
			sRev.Format(_T("%ld"), pLogEntry->GetRevision());
			if (pFindInfo->lvfi.flags & LVFI_PARTIAL)
			{
				if (sCmp.Compare(sRev.Left(sCmp.GetLength()))==0)
				{
					*pResult = i;
					return;
				}
			}
			else
			{
				if (sCmp.Compare(sRev)==0)
				{
					*pResult = i;
					return;
				}
			}
		}
	}

	*pResult = -1;
}

void CLogDlg::OnBnClickedCheckStoponcopy()
{
	if (!GetDlgItem(IDC_GETALL)->IsWindowEnabled())
		return;

	// ignore old fetch limits when switching
	// between copy-following and stop-on-copy
	// (otherwise stop-on-copy will limit what
	// we see immediately after switching to
	// copy-following)

	m_endrev = 0;

	// now, restart the query

	Refresh();
}

void CLogDlg::OnBnClickedIncludemerge()
{
	m_endrev = 0;

	m_limit = 0;
	Refresh();
}

void CLogDlg::UpdateLogInfoLabel()
{
	svn_revnum_t rev1 = 0;
	svn_revnum_t rev2 = 0;
	long selectedrevs = 0;
	long changedPaths = 0;
	if (m_logEntries.GetVisibleCount())
	{
		PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(0);
		rev1 = pLogEntry->GetRevision();
		pLogEntry = m_logEntries.GetVisible (m_logEntries.GetVisibleCount()-1);
		rev2 = pLogEntry->GetRevision();
		selectedrevs = m_LogList.GetSelectedCount();

		if (m_bSingleRevision)
		{
			changedPaths = m_currentChangedArray.GetCount();
		}
		else if (m_currentChangedPathList.GetCount())
		{
			changedPaths = m_currentChangedPathList.GetCount();
		}
	}
	CString sTemp;
	sTemp.FormatMessage(IDS_LOG_LOGINFOSTRING, m_logEntries.GetVisibleCount(), rev2, rev1, selectedrevs, changedPaths);
	m_sLogInfo = sTemp;
	UpdateData(FALSE);
	GetDlgItem(IDC_LOGINFO)->Invalidate();
}

void CLogDlg::ShowContextMenuForRevisions(CWnd* /*pWnd*/, CPoint point)
{
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex < 0)
		return;	// nothing selected, nothing to do with a context menu

	// if the user selected the info text telling about not all revisions shown due to
	// the "stop on copy/rename" option, we also don't show the context menu
	if ((m_bStrictStopped)&&(selIndex == (int)m_logEntries.GetVisibleCount()))
		return;

	// if the context menu is invoked through the keyboard, we have to use
	// a calculated position on where to anchor the menu on
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		m_LogList.GetItemRect(selIndex, &rect, LVIR_LABEL);
		m_LogList.ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	m_nSearchIndex = selIndex;
	m_bCancelled = FALSE;

	// calculate some information the context menu commands can use
	CString pathURL = GetURLFromPath(m_path);
	CString relPathURL = pathURL.Mid(m_sRepositoryRoot.GetLength());
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	int indexNext = m_LogList.GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;
	PLOGENTRYDATA pSelLogEntry = m_logEntries.GetVisible(indexNext);
	SVNRev revSelected = pSelLogEntry->GetRevision();
	SVNRev revPrevious = svn_revnum_t(revSelected)-1;

    const CLogChangedPathArray& paths = pSelLogEntry->GetChangedPaths();
	if (paths.GetCount() <= 2)
	{
        for (size_t i=0; i<paths.GetCount(); ++i)
		{
            const CLogChangedPath& changedpath = paths[i];
			if (changedpath.GetCopyFromRev() && (changedpath.GetPath().Compare(relPathURL)==0))
				revPrevious = changedpath.GetCopyFromRev();
		}
	}
	SVNRev revSelected2;
	if (pos)
	{
		PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
		revSelected2 = pLogEntry->GetRevision();
	}
	bool bAllFromTheSameAuthor = true;
	CString firstAuthor;
    std::vector<PLOGENTRYDATA> selEntries;
	SVNRev revLowest, revHighest;
	SVNRevRangeArray revisionRanges;
	{
		POSITION pos2 = m_LogList.GetFirstSelectedItemPosition();
		PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_LogList.GetNextSelectedItem(pos2));
		revisionRanges.AddRevision(pLogEntry->GetRevision());
		selEntries.push_back(pLogEntry);
		firstAuthor = pLogEntry->GetAuthor();
		revLowest = pLogEntry->GetRevision();
		revHighest = pLogEntry->GetRevision();
		while (pos2)
		{
			pLogEntry = m_logEntries.GetVisible(m_LogList.GetNextSelectedItem(pos2));
			revisionRanges.AddRevision(pLogEntry->GetRevision());
			selEntries.push_back(pLogEntry);
			if (firstAuthor.Compare(pLogEntry->GetAuthor()))
				bAllFromTheSameAuthor = false;
			revLowest = (svn_revnum_t(pLogEntry->GetRevision()) > svn_revnum_t(revLowest) ? revLowest : pLogEntry->GetRevision());
			revHighest = (svn_revnum_t(pLogEntry->GetRevision()) < svn_revnum_t(revHighest) ? revHighest : pLogEntry->GetRevision());
		}
	}



	//entry is selected, now show the popup menu
	CIconMenu popup;
	if (popup.CreatePopupMenu())
	{
		if (m_LogList.GetSelectedCount() == 1)
		{
			if (!m_path.IsDirectory())
			{
				if (m_hasWC)
				{
					popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
					popup.AppendMenuIcon(ID_BLAMECOMPARE, IDS_LOG_POPUP_BLAMECOMPARE, IDI_BLAME);
				}
				popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
				popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
				popup.AppendMenu(MF_SEPARATOR, NULL);
				popup.AppendMenuIcon(ID_SAVEAS, IDS_LOG_POPUP_SAVE, IDI_SAVEAS);
				popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
				popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
				popup.AppendMenuIcon(ID_BLAME, IDS_LOG_POPUP_BLAME, IDI_BLAME);
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
			else
			{
				if (m_hasWC)
				{
					popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
					// TODO:
					// TortoiseMerge could be improved to take a /blame switch
					// and then not 'cat' the files from a unified diff but
					// blame then.
					// But until that's implemented, the context menu entry for
					// this feature is commented out.
					//popup.AppendMenu(ID_BLAMECOMPARE, IDS_LOG_POPUP_BLAMECOMPARE, IDI_BLAME);
				}
				popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
				popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
				popup.AppendMenuIcon(ID_BLAMEWITHPREVIOUS, IDS_LOG_POPUP_BLAMEWITHPREVIOUS, IDI_BLAME);
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
			if (!m_ProjectProperties.sWebViewerRev.IsEmpty())
			{
				popup.AppendMenuIcon(ID_VIEWREV, IDS_LOG_POPUP_VIEWREV);
			}
			if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
			{
				popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);
			}
			if ((!m_ProjectProperties.sWebViewerPathRev.IsEmpty())||
				(!m_ProjectProperties.sWebViewerRev.IsEmpty()))
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}

			popup.AppendMenuIcon(ID_REPOBROWSE, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
			popup.AppendMenuIcon(ID_COPY, IDS_LOG_POPUP_COPY, IDI_COPY);
			if (m_hasWC)
				popup.AppendMenuIcon(ID_UPDATE, IDS_LOG_POPUP_UPDATE, IDI_UPDATE);
			if (m_hasWC)
				popup.AppendMenuIcon(ID_REVERTTOREV, IDS_LOG_POPUP_REVERTTOREV, IDI_REVERT);
			if (m_hasWC)
				popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);
			if (m_hasWC)
				popup.AppendMenuIcon(ID_MERGEREV, IDS_LOG_POPUP_MERGEREV, IDI_MERGE);
			if (m_path.IsDirectory())
			{
				popup.AppendMenuIcon(ID_CHECKOUT, IDS_MENUCHECKOUT, IDI_CHECKOUT);
				popup.AppendMenuIcon(ID_EXPORT, IDS_MENUEXPORT, IDI_EXPORT);
			}
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}
		else if (m_LogList.GetSelectedCount() >= 2)
		{
			bool bAddSeparator = false;
			if (IsSelectionContinuous() || (m_LogList.GetSelectedCount() == 2))
			{
				popup.AppendMenuIcon(ID_COMPARETWO, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
			}
			if (m_LogList.GetSelectedCount() == 2)
			{
				popup.AppendMenuIcon(ID_BLAMETWO, IDS_LOG_POPUP_BLAMEREVS, IDI_BLAME);
				popup.AppendMenuIcon(ID_GNUDIFF2, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
				bAddSeparator = true;
			}
			if (m_hasWC)
			{
				popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREVS, IDI_REVERT);
				if (m_hasWC)
					popup.AppendMenuIcon(ID_MERGEREV, IDS_LOG_POPUP_MERGEREVS, IDI_MERGE);
				bAddSeparator = true;
			}
			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if ((selEntries.size() > 0)&&(bAllFromTheSameAuthor))
		{
			popup.AppendMenuIcon(ID_EDITAUTHOR, IDS_LOG_POPUP_EDITAUTHOR);
		}
		if (m_LogList.GetSelectedCount() == 1)
		{
			popup.AppendMenuIcon(ID_EDITLOG, IDS_LOG_POPUP_EDITLOG);
			popup.AppendMenuIcon(ID_REVPROPS, IDS_REPOBROWSE_SHOWREVPROP, IDI_PROPERTIES); // "Show Revision Properties"
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}
		if (m_LogList.GetSelectedCount() != 0)
		{
			popup.AppendMenuIcon(ID_COPYCLIPBOARD, IDS_LOG_POPUP_COPYTOCLIPBOARD, IDI_COPYCLIP);
		}
		popup.AppendMenuIcon(ID_FINDENTRY, IDS_LOG_POPUP_FIND, IDI_FILTEREDIT);

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		DialogEnableWindow(IDOK, FALSE);
		SetPromptApp(&theApp);
		theApp.DoWaitCursor(1);
		bool bOpenWith = false;
		switch (cmd)
		{
		case ID_GNUDIFF1:
			{
				if (PromptShown())
				{
					SVNDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowUnifiedDiff(m_path, revPrevious, m_path, revSelected);
				}
				else
					CAppUtils::StartShowUnifiedDiff(m_hWnd, m_path, revPrevious, m_path, revSelected, SVNRev(), m_LogRevision);
			}
			break;
		case ID_GNUDIFF2:
			{
				if (PromptShown())
				{
					SVNDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowUnifiedDiff(m_path, revSelected2, m_path, revSelected);
				}
				else
					CAppUtils::StartShowUnifiedDiff(m_hWnd, m_path, revSelected2, m_path, revSelected, SVNRev(), m_LogRevision);
			}
			break;
		case ID_REVERTREV:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseSVN"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}
				CString msg;
				msg.Format(IDS_LOG_REVERT_CONFIRM, m_path.GetWinPath());
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					CSVNProgressDlg dlg;
					dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
					dlg.SetPathList(CTSVNPathList(m_path));
					dlg.SetUrl(pathURL);
					dlg.SetSecondUrl(pathURL);
					revisionRanges.AdjustForMerge(true);
					dlg.SetRevisionRanges(revisionRanges);
					dlg.SetPegRevision(m_LogRevision);
					dlg.DoModal();
				}
			}
			break;
		case ID_MERGEREV:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseSVN"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}

				CString path = m_path.GetWinPathString();
				bool bGotSavePath = false;
				if ((m_LogList.GetSelectedCount() == 1)&&(!m_path.IsDirectory()))
				{
					bGotSavePath = CAppUtils::FileOpenSave(path, NULL, IDS_LOG_MERGETO, IDS_COMMONFILEFILTER, true, GetSafeHwnd());
				}
				else
				{
					CBrowseFolder folderBrowser;
					folderBrowser.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_MERGETO)));
					bGotSavePath = (folderBrowser.Show(GetSafeHwnd(), path, path) == CBrowseFolder::OK);
				}
				if (bGotSavePath)
				{
					svn_revnum_t	minrev;
					svn_revnum_t	maxrev;
					bool			bswitched;
					bool			bmodified;
					bool			bSparse;

					if (GetWCRevisionStatus(CTSVNPath(path), true, minrev, maxrev, bswitched, bmodified, bSparse))
					{
						if (bmodified)
						{
							if (CMessageBox::Show(this->m_hWnd, IDS_MERGE_WCDIRTYASK, IDS_APPNAME, MB_YESNO | MB_ICONWARNING) != IDYES)
							{
								break;
							}
						}
					}
					CSVNProgressDlg dlg;
					dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
					dlg.SetPathList(CTSVNPathList(CTSVNPath(path)));
					dlg.SetUrl(pathURL);
					dlg.SetSecondUrl(pathURL);
					revisionRanges.AdjustForMerge(false);
					dlg.SetRevisionRanges(revisionRanges);
					dlg.SetPegRevision(m_LogRevision);
					dlg.DoModal();
				}
			}
			break;
		case ID_REVERTTOREV:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseSVN"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}

				CString msg;
				msg.Format(IDS_LOG_REVERTTOREV_CONFIRM, m_path.GetWinPath());
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					CSVNProgressDlg dlg;
					dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
					dlg.SetPathList(CTSVNPathList(m_path));
					dlg.SetUrl(pathURL);
					dlg.SetSecondUrl(pathURL);
					SVNRevRangeArray revarray;
					revarray.AddRevRange(SVNRev::REV_HEAD, revSelected);
					dlg.SetRevisionRanges(revarray);
					dlg.SetPegRevision(m_LogRevision);
					dlg.DoModal();
				}
			}
			break;
		case ID_COPY:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseSVN"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}

				CCopyDlg dlg;
				dlg.m_URL = pathURL;
				dlg.m_path = m_path;
				dlg.m_CopyRev = revSelected;
				if (dlg.DoModal() == IDOK)
				{
					// should we show a progress dialog here? Copies are done really fast
					// and without much network traffic.
					if (!Copy(CTSVNPathList(CTSVNPath(pathURL)), CTSVNPath(dlg.m_URL), dlg.m_CopyRev, dlg.m_CopyRev, dlg.m_sLogMessage))
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					else
						CMessageBox::Show(this->m_hWnd, IDS_LOG_COPY_SUCCESS, IDS_APPNAME, MB_ICONINFORMATION);
				}
			} 
			break;
		case ID_COMPARE:
			{
				//user clicked on the menu item "compare with working copy"
				if (PromptShown())
				{
					SVNDiff diff(this, m_hWnd, true);
					diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(m_path, SVNRev::REV_WC, m_path, revSelected);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, m_path, SVNRev::REV_WC, m_path, revSelected, SVNRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			}
			break;
		case ID_COMPARETWO:
			{
				SVNRev r1 = revSelected;
				SVNRev r2 = revSelected2;
				if (m_LogList.GetSelectedCount() > 2)
				{
					r1 = revHighest;
					r2 = revLowest;
				}
				svn_node_kind_t nodekind = svn_node_unknown;
				if (!m_path.IsUrl())
				{
					nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
				}
				//user clicked on the menu item "compare revisions"
				if (PromptShown())
				{
					SVNDiff diff(this, m_hWnd, true);
					diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTSVNPath(pathURL), r2, CTSVNPath(pathURL), r1, SVNRev(), false, false, nodekind);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), r2, CTSVNPath(pathURL), r1, 
												SVNRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), 
												false, false, nodekind);
			}
			break;
		case ID_COMPAREWITHPREVIOUS:
			{
				svn_node_kind_t nodekind = svn_node_unknown;
				if (!m_path.IsUrl())
				{
					nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
				}
				if (PromptShown())
				{
					SVNDiff diff(this, m_hWnd, true);
					diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, SVNRev(), false, false, nodekind);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, 
												SVNRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000),
												false, false, nodekind);
			}
			break;
		case ID_BLAMECOMPARE:
			{
				//user clicked on the menu item "compare with working copy"
				//now first get the revision which is selected
				if (PromptShown())
				{
					SVNDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(m_path, SVNRev::REV_BASE, m_path, revSelected, SVNRev(), false, true);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, m_path, SVNRev::REV_BASE, m_path, revSelected, SVNRev(), m_LogRevision, false, false, true);
			}
			break;
		case ID_BLAMETWO:
			{
				//user clicked on the menu item "compare and blame revisions"
				svn_node_kind_t nodekind = svn_node_unknown;
				if (!m_path.IsUrl())
				{
					nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
				}
				if (PromptShown())
				{
					SVNDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTSVNPath(pathURL), revSelected2, CTSVNPath(pathURL), revSelected, SVNRev(), false, true, nodekind);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), revSelected2, CTSVNPath(pathURL), revSelected, 
												SVNRev(), m_LogRevision, false, false, true, nodekind);
			}
			break;
		case ID_BLAMEWITHPREVIOUS:
			{
				//user clicked on the menu item "Compare and Blame with previous revision"
				svn_node_kind_t nodekind = svn_node_unknown;
				if (!m_path.IsUrl())
				{
					nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
				}
				if (PromptShown())
				{
					SVNDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, SVNRev(), false, true, nodekind);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, 
												SVNRev(), m_LogRevision, false, false, true, nodekind);
			}
			break;
		case ID_SAVEAS:
			{
				//now first get the revision which is selected
				CString revFilename;
				if (m_hasWC)
				{
					CString strWinPath = m_path.GetWinPathString();
					int rfind = strWinPath.ReverseFind('.');
					if (rfind > 0)
						revFilename.Format(_T("%s-%s%s"), (LPCTSTR)strWinPath.Left(rfind), (LPCTSTR)revSelected.ToString(), (LPCTSTR)strWinPath.Mid(rfind));
					else
						revFilename.Format(_T("%s-%s"), (LPCTSTR)strWinPath, (LPCTSTR)revSelected.ToString());
				}
				if (CAppUtils::FileOpenSave(revFilename, NULL, IDS_LOG_POPUP_SAVE, IDS_COMMONFILEFILTER, false, m_hWnd))
				{
					CTSVNPath tempfile;
					tempfile.SetFromWin(revFilename);
					CProgressDlg progDlg;
					progDlg.SetTitle(IDS_APPNAME);
					progDlg.SetAnimation(IDR_DOWNLOAD);
					CString sInfoLine;
					sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LPCTSTR)revSelected.ToString());
					progDlg.SetLine(1, sInfoLine, true);
					SetAndClearProgressInfo(&progDlg);
					progDlg.ShowModeless(m_hWnd);
					if (!Cat(m_path, SVNRev(SVNRev::REV_HEAD), revSelected, tempfile))
					{
						// try again with another peg revision
						if (!Cat(m_path, revSelected, revSelected, tempfile))
						{
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							EnableOKButton();
							break;
						}
					}
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
				}
			}
			break;
		case ID_OPENWITH:
			bOpenWith = true;
		case ID_OPEN:
			{
				CProgressDlg progDlg;
				progDlg.SetTitle(IDS_APPNAME);
				progDlg.SetAnimation(IDR_DOWNLOAD);
				CString sInfoLine;
				sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LPCTSTR)revSelected.ToString());
				progDlg.SetLine(1, sInfoLine, true);
				SetAndClearProgressInfo(&progDlg);
				progDlg.ShowModeless(m_hWnd);
				CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, m_path, revSelected);
				bool bSuccess = true;
				if (!Cat(m_path, SVNRev(SVNRev::REV_HEAD), revSelected, tempfile))
				{
					bSuccess = false;
					// try again, but with the selected revision as the peg revision
					if (!Cat(m_path, revSelected, revSelected, tempfile))
					{
						progDlg.Stop();
						SetAndClearProgressInfo((HWND)NULL);
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						EnableOKButton();
						break;
					}
					bSuccess = true;
				}
				if (bSuccess)
				{
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
					SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
					int ret = 0;
					if (!bOpenWith)
						ret = (int)ShellExecute(this->m_hWnd, NULL, tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
					if ((ret <= HINSTANCE_ERROR)||bOpenWith)
					{
						CString c = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
						c += tempfile.GetWinPathString() + _T(" ");
						CAppUtils::LaunchApplication(c, NULL, false);
					}
				}
			}
			break;
		case ID_BLAME:
			{
				CBlameDlg dlg;
				dlg.EndRev = revSelected;
				if (dlg.DoModal() == IDOK)
				{
					CBlame blame;
					CString tempfile;
					CString logfile;
					tempfile = blame.BlameToTempFile(m_path, dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, _T(""), dlg.m_bIncludeMerge, TRUE, TRUE);
					if (!tempfile.IsEmpty())
					{
						if (dlg.m_bTextView)
						{
							//open the default text editor for the result file
							CAppUtils::StartTextViewer(tempfile);
						}
						else
						{
							CString sParams = _T("/path:\"") + m_path.GetSVNPathString() + _T("\" ");
							if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(m_path.GetFileOrDirectoryName()),sParams, dlg.StartRev, dlg.EndRev))
							{
								break;
							}
						}
					}
					else
					{
						CMessageBox::Show(this->m_hWnd, blame.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
				}
			}
			break;
		case ID_UPDATE:
			{
				CString sCmd;
				sCmd.Format(_T("%s /command:update /path:\"%s\" /rev:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)m_path.GetWinPath(), (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_FINDENTRY:
			{
				m_nSearchIndex = m_LogList.GetSelectionMark();
				if (m_nSearchIndex < 0)
					m_nSearchIndex = 0;
				if (m_pFindDialog)
				{
					break;
				}
				else
				{
					m_pFindDialog = new CFindReplaceDialog();
					m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
				}
			}
			break;
		case ID_REPOBROWSE:
			{
				CString sCmd;
				sCmd.Format(_T("%s /command:repobrowser /path:\"%s\" /rev:%s"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)pathURL, (LPCTSTR)revSelected.ToString());

				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_EDITLOG:
			{
				EditLogMessage(selIndex);
			}
			break;
		case ID_EDITAUTHOR:
			{
				EditAuthor(selEntries);
			}
			break;
		case ID_REVPROPS:
			{
				CEditPropertiesDlg dlg;
				dlg.SetProjectProperties(&m_ProjectProperties);
				CTSVNPathList escapedlist;
				dlg.SetPathList(CTSVNPathList(CTSVNPath(pathURL)));
				dlg.SetRevision(revSelected);
				dlg.RevProps(true);
				dlg.DoModal();
			}
			break;
		case ID_COPYCLIPBOARD:
			{
				CopySelectionToClipBoard();
			}
			break;
		case ID_EXPORT:
			{
				CString sCmd;
				sCmd.Format(_T("%s /command:export /path:\"%s\" /revision:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)pathURL, (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_CHECKOUT:
			{
				CString sCmd;
				CString url = _T("tsvn:")+pathURL;
				sCmd.Format(_T("%s /command:checkout /url:\"%s\" /revision:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)url, (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_VIEWREV:
			{
				CString url = m_ProjectProperties.sWebViewerRev;
				url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
				url.Replace(_T("%REVISION%"), revSelected.ToString());
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
		case ID_VIEWPATHREV:
			{
				CString relurl = pathURL;
				CString sRoot = GetRepositoryRoot(CTSVNPath(relurl));
				relurl = relurl.Mid(sRoot.GetLength());
				CString url = m_ProjectProperties.sWebViewerPathRev;
				url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
				url.Replace(_T("%REVISION%"), revSelected.ToString());
				url.Replace(_T("%PATH%"), relurl);
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
		default:
			break;
		} // switch (cmd)
		theApp.DoWaitCursor(-1);
		EnableOKButton();
	} // if (popup.CreatePopupMenu())
}

void CLogDlg::ShowContextMenuForChangedpaths(CWnd* /*pWnd*/, CPoint point)
{
	INT_PTR selIndex = m_ChangedFileListCtrl.GetSelectionMark();
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		m_ChangedFileListCtrl.GetItemRect((int)selIndex, &rect, LVIR_LABEL);
		m_ChangedFileListCtrl.ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	if (selIndex < 0)
		return;
	int s = m_LogList.GetSelectionMark();
	if (s < 0)
		return;
	std::vector<CString> changedpaths;
	std::vector<size_t> changedlogpathindices;
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos == NULL)
		return;	// nothing is selected, get out of here

	PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
	svn_revnum_t rev1 = pLogEntry->GetRevision();
	svn_revnum_t rev2 = rev1;
	bool bOneRev = true;
	if (pos)
	{
		while (pos)
		{
			pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
			if (pLogEntry)
			{
				rev1 = max(rev1,(svn_revnum_t)pLogEntry->GetRevision());
				rev2 = min(rev2,(svn_revnum_t)pLogEntry->GetRevision());
				bOneRev = false;
			}				
		}
		if (!bOneRev)
			rev2--;
		POSITION pos2 = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos2)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
			changedpaths.push_back(m_currentChangedPathList[nItem].GetSVNPathString());
		}
	}
	else
	{
		// only one revision is selected in the log dialog top pane
		// but multiple items could be selected  in the changed items list
		rev2 = rev1-1;

		POSITION pos2 = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos2)
		{
            const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();

            int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
			if ((m_cShowPaths.GetState() & 0x0003)==BST_CHECKED)
			{
				// some items are hidden! So find out which item the user really clicked on
				INT_PTR selRealIndex = -1;
				for (INT_PTR hiddenindex=0; hiddenindex<(INT_PTR)paths.GetCount(); ++hiddenindex)
				{
                    if (paths[hiddenindex].IsRelevantForStartPath())
						selRealIndex++;
					if (selRealIndex == nItem)
					{
						nItem = static_cast<int>(hiddenindex);
						break;
					}
				}
			}

			const CLogChangedPath& changedlogpath = paths[nItem];
			if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
			{
				if (!changedlogpath.GetCopyFromPath().IsEmpty())
					rev2 = changedlogpath.GetCopyFromRev();
				else
				{
					// if the path was modified but the parent path was 'added with history'
					// then we have to use the copy from revision of the parent path
					CTSVNPath cpath = CTSVNPath(changedlogpath.GetPath());
					for (size_t flist = 0; flist < paths.GetCount(); ++flist)
					{
						CTSVNPath p = CTSVNPath(paths[flist].GetPath());
						if (p.IsAncestorOf(cpath))
						{
							if (!paths[flist].GetCopyFromPath().IsEmpty())
								rev2 = paths[flist].GetCopyFromRev();
						}
					}
				}
			}

			changedpaths.push_back(changedlogpath.GetPath());
            changedlogpathindices.push_back (static_cast<size_t>(nItem));
		}
	}

	//entry is selected, now show the popup menu
	CIconMenu popup;
	if (popup.CreatePopupMenu())
	{
		bool bEntryAdded = false;
		if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
		{
			if ((!bOneRev)||(IsDiffPossible (pLogEntry->GetChangedPaths()[changedlogpathindices[0]], rev1)))
			{
				popup.AppendMenuIcon(ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
				popup.AppendMenuIcon(ID_BLAMEDIFF, IDS_LOG_POPUP_BLAMEDIFF, IDI_BLAME);
				popup.SetDefaultItem(ID_DIFF, FALSE);
				popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
				bEntryAdded = true;
			}
			else if (bOneRev)
			{
				popup.AppendMenuIcon(ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
				popup.SetDefaultItem(ID_DIFF, FALSE);
				bEntryAdded = true;
			}
			if ((rev2 == rev1-1)||(changedpaths.size() == 1))
			{
				if (bEntryAdded)
					popup.AppendMenu(MF_SEPARATOR, NULL);
				popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
				popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
				popup.AppendMenuIcon(ID_BLAME, IDS_LOG_POPUP_BLAME, IDI_BLAME);
				popup.AppendMenu(MF_SEPARATOR, NULL);
				if (m_hasWC)
					popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);
				popup.AppendMenuIcon(ID_POPPROPS, IDS_REPOBROWSE_SHOWPROP, IDI_PROPERTIES);			// "Show Properties"
				popup.AppendMenuIcon(ID_LOG, IDS_MENULOG, IDI_LOG);						// "Show Log"				
				popup.AppendMenuIcon(ID_GETMERGELOGS, IDS_LOG_POPUP_GETMERGELOGS, IDI_LOG);		// "Show merge log"
				popup.AppendMenuIcon(ID_SAVEAS, IDS_LOG_POPUP_SAVE, IDI_SAVEAS);
				bEntryAdded = true;
				if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
				{
					popup.AppendMenu(MF_SEPARATOR, NULL);
					popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);
				}
				if (popup.GetDefaultItem(0,FALSE)==-1)
					popup.SetDefaultItem(ID_OPEN, FALSE);
			}
		}
		else if (changedlogpathindices.size())
		{
			// more than one entry is selected
			popup.AppendMenuIcon(ID_SAVEAS, IDS_LOG_POPUP_SAVE);
			bEntryAdded = true;
		}

		if (!bEntryAdded)
			return;
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		bool bOpenWith = false;
		bool bMergeLog = false;
		m_bCancelled = false;
		switch (cmd)
		{
		case ID_DIFF:
			{
				if ((!bOneRev)|| IsDiffPossible (pLogEntry->GetChangedPaths()[changedlogpathindices[0]], rev1))
					DoDiffFromLog(selIndex, rev1, rev2, false, false);
				else
					DiffSelectedFile();
			}
			break;
		case ID_BLAMEDIFF:
			{
				DoDiffFromLog(selIndex, rev1, rev2, true, false);
			}
			break;
		case ID_GNUDIFF1:
			{
				DoDiffFromLog(selIndex, rev1, rev2, false, true);
			}
			break;
		case ID_REVERTREV:
			{
			    const CLogChangedPath& changedlogpath 
                    = pLogEntry->GetChangedPaths()[changedlogpathindices[0]];

                SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				CString sUrl;
				if (SVN::PathIsURL(m_path))
				{
					sUrl = m_path.GetSVNPathString();
				}
				else
				{
					sUrl = GetURLFromPath(m_path);
					if (sUrl.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, m_path.GetWinPath());
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						EnableOKButton();
						theApp.DoWaitCursor(-1);
						break;		//exit
					}
				}
				// find the working copy path of the selected item from the URL
				m_bCancelled = false;
				CString sUrlRoot = GetRepositoryRoot(CTSVNPath(sUrl));

				CString fileURL = changedpaths[0];
				fileURL = sUrlRoot + fileURL.Trim();
				// firstfile = (e.g.) http://mydomain.com/repos/trunk/folder/file1
				// sUrl = http://mydomain.com/repos/trunk/folder
				CString sUnescapedUrl = CPathUtils::PathUnescape(sUrl);
				// find out until which char the urls are identical
				int i=0;
				while ((i<fileURL.GetLength())&&(i<sUnescapedUrl.GetLength())&&(fileURL[i]==sUnescapedUrl[i]))
					i++;
				int leftcount = m_path.GetWinPathString().GetLength()-(sUnescapedUrl.GetLength()-i);
				CString wcPath = m_path.GetWinPathString().Left(leftcount);
				wcPath += fileURL.Mid(i);
				wcPath.Replace('/', '\\');
				CSVNProgressDlg dlg;
				if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
				{
					// a deleted path! Since the path isn't there anymore, merge
					// won't work. So just do a copy url->wc
					dlg.SetCommand(CSVNProgressDlg::SVNProgress_Copy);
					dlg.SetPathList(CTSVNPathList(CTSVNPath(fileURL)));
					dlg.SetUrl(wcPath);
					dlg.SetRevision(rev2);
				}
				else
				{
					if (!PathFileExists(wcPath))
					{
						// seems the path got renamed
						// tell the user how to work around this.
						CMessageBox::Show(this->m_hWnd, IDS_LOG_REVERTREV_ERROR, IDS_APPNAME, MB_ICONERROR);
						EnableOKButton();
						theApp.DoWaitCursor(-1);
						break;		//exit
					}
					dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
					dlg.SetPathList(CTSVNPathList(CTSVNPath(wcPath)));
					dlg.SetUrl(fileURL);
					dlg.SetSecondUrl(fileURL);
					SVNRevRangeArray revarray;
					revarray.AddRevRange(rev1, rev2);
					dlg.SetRevisionRanges(revarray);
				}
				CString msg;
				msg.Format(IDS_LOG_REVERT_CONFIRM, (LPCTSTR)wcPath);
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					dlg.DoModal();
				}
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_POPPROPS:
			{
				DialogEnableWindow(IDOK, FALSE);
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				CString filepath;
				if (SVN::PathIsURL(m_path))
				{
					filepath = m_path.GetSVNPathString();
				}
				else
				{
					filepath = GetURLFromPath(m_path);
					if (filepath.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the file!\n"));
						EnableOKButton();
						break;
					}
				}
				filepath = GetRepositoryRoot(CTSVNPath(filepath));
				filepath += changedpaths[0];
				CPropDlg dlg;
				dlg.m_rev = rev1;
				dlg.m_Path = CTSVNPath(filepath);
				dlg.DoModal();
				EnableOKButton();
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_SAVEAS:
			{
				DialogEnableWindow(IDOK, FALSE);
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				CString filepath;
				if (SVN::PathIsURL(m_path))
				{
					filepath = m_path.GetSVNPathString();
				}
				else
				{
					filepath = GetURLFromPath(m_path);
					if (filepath.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the file!\n"));
						EnableOKButton();
						break;
					}
				}
				m_bCancelled = false;
				CString sRoot = GetRepositoryRoot(CTSVNPath(filepath));
				// if more than one entry is selected, we save them
				// one by one into a folder the user has selected
				bool bTargetSelected = false;
				CTSVNPath TargetPath;
				if (m_ChangedFileListCtrl.GetSelectedCount() > 1)
				{
					CBrowseFolder browseFolder;
					browseFolder.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_SAVEFOLDERTOHINT)));
					browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
					CString strSaveAsDirectory;
					if (browseFolder.Show(GetSafeHwnd(), strSaveAsDirectory) == CBrowseFolder::OK) 
					{
						TargetPath = CTSVNPath(strSaveAsDirectory);
						bTargetSelected = true;
					}
				}
				else
				{
					// Display the Open dialog box. 
					CString revFilename;
					CString temp;
					temp = CPathUtils::GetFileNameFromPath(changedpaths[0]);
					int rfind = temp.ReverseFind('.');
					if (rfind > 0)
						revFilename.Format(_T("%s-%ld%s"), (LPCTSTR)temp.Left(rfind), rev1, (LPCTSTR)temp.Mid(rfind));
					else
						revFilename.Format(_T("%s-%ld"), (LPCTSTR)temp, rev1);
					bTargetSelected = CAppUtils::FileOpenSave(revFilename, NULL, IDS_LOG_POPUP_SAVE, IDS_COMMONFILEFILTER, false, m_hWnd);
					TargetPath.SetFromWin(revFilename);
				}
				if (bTargetSelected)
				{
					CProgressDlg progDlg;
					progDlg.SetTitle(IDS_APPNAME);
					progDlg.SetAnimation(IDR_DOWNLOAD);
					for ( size_t i = 0; i < changedlogpathindices.size(); ++i)
					{
			            const CLogChangedPath& changedlogpath 
                            = pLogEntry->GetChangedPaths()[changedlogpathindices[i]];

						SVNRev getrev = (changedlogpath.GetAction() == LOGACTIONS_DELETED) ? rev2 : rev1;

						CString sInfoLine;
						sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)filepath, (LPCTSTR)getrev.ToString());
						progDlg.SetLine(1, sInfoLine, true);
						SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);

						CTSVNPath tempfile = TargetPath;
						if (changedpaths.size() > 1)
						{
							// if multiple items are selected, then the TargetPath
							// points to a folder and we have to append the filename
							// to save to to that folder.
							CString sName = changedlogpath.GetPath();
							int slashpos = sName.ReverseFind('/');
							if (slashpos >= 0)
								sName = sName.Mid(slashpos);
							tempfile.AppendPathString(sName);
							// TODO: one problem here:
							// a user could have selected multiple items which
							// have the same filename but reside in different
							// directories, e.g.
							// /folder1/file1
							// /folder2/file1
							// in that case, the second 'file1' will overwrite
							// the already saved 'file1'.
							// 
							// we could maybe find the common root of all selected
							// items and then create sub folders to save those files
							// there.
							// But I think we should just leave it that way: to check
							// out multiple items at once, the better way is still to
							// use the export command from the top pane of the log dialog.
						}
						filepath = sRoot + changedlogpath.GetPath();
						if (!Cat(CTSVNPath(filepath), getrev, getrev, tempfile))
						{
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							tempfile.Delete(false);
							EnableOKButton();
							theApp.DoWaitCursor(-1);
							break;
						}
					}
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
				}
				EnableOKButton();
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_OPENWITH:
			bOpenWith = true;
		case ID_OPEN:
			{
				SVNRev getrev = pLogEntry->GetChangedPaths()[selIndex].GetAction() == LOGACTIONS_DELETED ? rev2 : rev1;
				Open(bOpenWith,changedpaths[0],getrev);
			}
			break;
		case ID_BLAME:
			{
				CString filepath;
				if (SVN::PathIsURL(m_path))
				{
					filepath = m_path.GetSVNPathString();
				}
				else
				{
					filepath = GetURLFromPath(m_path);
					if (filepath.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the file!\n"));
						EnableOKButton();
						break;
					}
				}
				filepath = GetRepositoryRoot(CTSVNPath(filepath));
				filepath += changedpaths[0];
				CBlameDlg dlg;
				dlg.EndRev = rev1;
				if (dlg.DoModal() == IDOK)
				{
					CBlame blame;
					CString tempfile;
					CString logfile;
					tempfile = blame.BlameToTempFile(CTSVNPath(filepath), dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, _T(""), dlg.m_bIncludeMerge, TRUE, TRUE);
					if (!tempfile.IsEmpty())
					{
						if (dlg.m_bTextView)
						{
							//open the default text editor for the result file
							CAppUtils::StartTextViewer(tempfile);
						}
						else
						{
							CString sParams = _T("/path:\"") + filepath + _T("\" ");
							if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(filepath),sParams, dlg.StartRev, dlg.EndRev))
							{
								break;
							}
						}
					}
					else
					{
						CMessageBox::Show(this->m_hWnd, blame.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
				}
			}
			break;
		case ID_GETMERGELOGS:
			bMergeLog = true;
			// fall through
		case ID_LOG:
			{
			    const CLogChangedPath& changedlogpath 
                    = pLogEntry->GetChangedPaths()[changedlogpathindices[0]];

				DialogEnableWindow(IDOK, FALSE);
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				CString filepath;
				if (SVN::PathIsURL(m_path))
				{
					filepath = m_path.GetSVNPathString();
				}
				else
				{
					filepath = GetURLFromPath(m_path);
					if (filepath.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the file!\n"));
						EnableOKButton();
						break;
					}
				}
				m_bCancelled = false;
				filepath = GetRepositoryRoot(CTSVNPath(filepath));
				filepath += changedpaths[0];
				svn_revnum_t logrev = rev1;
				CString sCmd;
				if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
				{
					// if the item got deleted in this revision,
					// fetch the log from the previous revision where it
					// still existed.
					sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /startrev:%ld /pegrev:%ld"), (LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)filepath, logrev, logrev-1);
				}
				else
					sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /pegrev:%ld"), (LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)filepath, logrev);

				if (bMergeLog)
					sCmd += _T(" /merge");
				CAppUtils::LaunchApplication(sCmd, NULL, false);
				EnableOKButton();
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_VIEWPATHREV:
			{
				PLOGENTRYDATA pLogEntry2 = m_logEntries.GetVisible (m_LogList.GetSelectionMark());
				SVNRev rev = pLogEntry2->GetRevision();
				CString relurl = changedpaths[0];
				CString url = m_ProjectProperties.sWebViewerPathRev;
				url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
				url.Replace(_T("%REVISION%"), rev.ToString());
				url.Replace(_T("%PATH%"), relurl);
				relurl = relurl.Mid(relurl.Find('/'));
				url.Replace(_T("%PATH1%"), relurl);
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
		default:
			break;
		} // switch (cmd)
	} // if (popup.CreatePopupMenu())
}

void CLogDlg::OnDtnDropdownDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// the date control should not show the "today" button
	CMonthCalCtrl * pCtrl = m_DateFrom.GetMonthCalCtrl();
	if (pCtrl)
		SetWindowLongPtr(pCtrl->GetSafeHwnd(), GWL_STYLE, LONG_PTR(pCtrl->GetStyle() | MCS_NOTODAY));
	*pResult = 0;
}

void CLogDlg::OnDtnDropdownDateto(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// the date control should not show the "today" button
	CMonthCalCtrl * pCtrl = m_DateTo.GetMonthCalCtrl();
	if (pCtrl)
		SetWindowLongPtr(pCtrl->GetSafeHwnd(), GWL_STYLE, LONG_PTR(pCtrl->GetStyle() | MCS_NOTODAY));
	*pResult = 0;
}

void CLogDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	//set range
	SetSplitterRange();
}

void CLogDlg::OnRefresh()
{
	if (GetDlgItem(IDC_GETALL)->IsWindowEnabled())
	{
		m_limit = 0;
		Refresh (true);
	}
}

void CLogDlg::OnFind()
{
	if (!m_pFindDialog)
	{
		m_pFindDialog = new CFindReplaceDialog();
		m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
	}
}

void CLogDlg::OnFocusFilter()
{
	GetDlgItem(IDC_SEARCHEDIT)->SetFocus();	
}

void CLogDlg::OnEditCopy()
{
	if (GetFocus() == &m_ChangedFileListCtrl)
		CopyChangedSelectionToClipBoard();
	else
		CopySelectionToClipBoard();
}

void CLogDlg::OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	// If user press space, toggle flag on selected item
	if (pLVKeyDown->wVKey == VK_SPACE)
	{
		// Toggle checked for the focused item.
		int nFocusedItem = m_LogList.GetNextItem(-1, LVNI_FOCUSED);
		if (nFocusedItem >= 0)
			ToggleCheckbox(nFocusedItem);
	}

	*pResult = 0;
}

void CLogDlg::OnNMClickLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	UINT flags = 0;
	CPoint point(pNMItemActivate->ptAction);

	//Make the hit test...
	int item = m_LogList.HitTest(point, &flags); 

	if (item != -1)
	{
		//We hit one item... did we hit state image (check box)?
		//This test only works if we are in list or report mode.
		if( (flags & LVHT_ONITEMSTATEICON) != 0)
		{
			ToggleCheckbox(item);
		}
	}

	*pResult = 0;
}

CString CLogDlg::GetToolTipText(int nItem, int nSubItem)
{
	if ((nSubItem == 1) && (m_logEntries.GetVisibleCount() > (size_t)nItem))
	{
		PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (nItem);

		CString sToolTipText;

		// Draw the icon(s) into the compatible DC

        DWORD actions = pLogEntry->GetChangedPaths().GetActions();
		if (actions & LOGACTIONS_MODIFIED)
		{
            sToolTipText += CLogChangedPath::GetActionString (LOGACTIONS_MODIFIED);
		}

		if (actions & LOGACTIONS_ADDED)
		{
			if (!sToolTipText.IsEmpty())
				sToolTipText += _T("\r\n");
			sToolTipText += CLogChangedPath::GetActionString (LOGACTIONS_ADDED);
		}

		if (actions & LOGACTIONS_DELETED)
		{
			if (!sToolTipText.IsEmpty())
				sToolTipText += _T("\r\n");
			sToolTipText += CLogChangedPath::GetActionString (LOGACTIONS_DELETED);
		}

		if (actions & LOGACTIONS_REPLACED)
		{
			if (!sToolTipText.IsEmpty())
				sToolTipText += _T("\r\n");
			sToolTipText += CLogChangedPath::GetActionString (LOGACTIONS_REPLACED);
		}

		if ((pLogEntry->GetDepth())||(m_mergedRevs.find(pLogEntry->GetRevision()) != m_mergedRevs.end()))
		{
			if (!sToolTipText.IsEmpty())
				sToolTipText += _T("\r\n");
			sToolTipText += CString(MAKEINTRESOURCE(IDS_LOG_ALREADYMERGED));
		}

		if (!sToolTipText.IsEmpty())
		{
			CString sTitle(MAKEINTRESOURCE(IDS_LOG_ACTIONS));
			sToolTipText = sTitle + _T(":\r\n") + sToolTipText; 
		}
		return sToolTipText;
	}
	return CString();
}

// selection management

void CLogDlg::AutoStoreSelection()
{
    if (m_pStoreSelection == NULL)
        m_pStoreSelection = new CStoreSelection(this);
}

void CLogDlg::AutoRestoreSelection()
{
    if (m_pStoreSelection != NULL)
    {
        delete m_pStoreSelection;
        m_pStoreSelection = NULL;

        FillLogMessageCtrl();
        UpdateLogInfoLabel();
    }
}

CString CLogDlg::GetListviewHelpString(HWND hControl, int index)
{
	CString sHelpText;
	if (hControl == m_LogList.GetSafeHwnd())
	{
		if (m_logEntries.GetVisibleCount() > (size_t)index)
		{
			PLOGENTRYDATA data = m_logEntries.GetVisible(index);
			if (data)
			{
				if ((data->GetDepth())||(m_mergedRevs.find(data->GetRevision()) != m_mergedRevs.end()))
				{
					// this revision was already merged
					sHelpText = CString(MAKEINTRESOURCE(IDS_ACC_LOGENTRYALREADYMERGED));
				}
				if (data->GetRevision() == m_wcRev)
				{
					// the working copy is at this revision
					if (!sHelpText.IsEmpty())
						sHelpText += _T(", ");
					sHelpText += CString(MAKEINTRESOURCE(IDS_ACC_WCISATTHISREVISION));
				}
			}
		}
	}
	else if (hControl == m_ChangedFileListCtrl.GetSafeHwnd())
	{
		// currently the changed files list control only colors items for faster
		// indication of the information that's already there. So we don't
		// provide this info again in the help string.
	}

	return sHelpText;
}

void CLogDlg::AddMainAnchors()
{
	AddAnchor(IDC_LOGLIST, TOP_LEFT, MIDDLE_RIGHT);
	AddAnchor(IDC_SPLITTERTOP, MIDDLE_LEFT, MIDDLE_RIGHT);
	AddAnchor(IDC_MSGVIEW, MIDDLE_LEFT, MIDDLE_RIGHT);
	AddAnchor(IDC_SPLITTERBOTTOM, MIDDLE_LEFT, MIDDLE_RIGHT);
	AddAnchor(IDC_LOGMSG, MIDDLE_LEFT, BOTTOM_RIGHT);
}

void CLogDlg::RemoveMainAnchors()
{
	RemoveAnchor(IDC_LOGLIST);
	RemoveAnchor(IDC_SPLITTERTOP);
	RemoveAnchor(IDC_MSGVIEW);
	RemoveAnchor(IDC_SPLITTERBOTTOM);
	RemoveAnchor(IDC_LOGMSG);
}
