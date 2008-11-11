// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
	, m_logcounter(0)
	, m_nSearchIndex(0)
	, m_wParam(0)
	, m_nSelectedFilter(LOGFILTER_ALL)
	, m_bNoDispUpdates(FALSE)
	, m_currentChangedArray(NULL)
	, m_nSortColumn(0)
	, m_bShowedAll(false)
	, m_bSelect(false)
	, m_regLastStrict(_T("Software\\TortoiseSVN\\LastLogStrict"), FALSE)
	, m_regMaxBugIDColWidth(_T("Software\\TortoiseSVN\\MaxBugIDColWidth"), 200)
	, m_bSelectionMustBeContinuous(false)
	, m_bShowBugtraqColumn(false)
	, m_lowestRev(-1)
	, m_bStrictStopped(false)
	, m_sLogInfo(_T(""))
	, m_pFindDialog(NULL)
	, m_bCancelled(FALSE)
	, m_pNotifyWindow(NULL)
	, m_bThreadRunning(FALSE)
	, m_bAscending(FALSE)
	, m_pStoreSelection(NULL)
	, m_limit(0)
	, m_childCounter(0)
	, m_maxChild(0)
	, m_bIncludeMerges(FALSE)
	, m_hAccel(NULL)
	, m_bVista(false)
{
	m_bFilterWithRegex = !!CRegDWORD(_T("Software\\TortoiseSVN\\UseRegexFilter"), TRUE);
	// use the default GUI font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf = {0};
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont = CreateFontIndirect(&lf);
}

CLogDlg::~CLogDlg()
{
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
    m_CurrentFilteredChangedArray.RemoveAll();
	m_logEntries.ClearAll();
	DestroyIcon(m_hModifiedIcon);
	DestroyIcon(m_hReplacedIcon);
	DestroyIcon(m_hAddedIcon);
	DestroyIcon(m_hDeletedIcon);
	if ( m_pStoreSelection )
	{
		delete m_pStoreSelection;
		m_pStoreSelection = NULL;
	}
	if (m_boldFont)
		DeleteObject(m_boldFont);
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
	DDX_Control(pDX, IDC_HIDEPATHS, m_cHidePaths);
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
	ON_MESSAGE(WM_FILTEREDIT_INFOCLICKED, OnClickedInfoIcon)
	ON_MESSAGE(WM_FILTEREDIT_CANCELCLICKED, OnClickedCancelFilter)
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
	ON_BN_CLICKED(IDC_HIDEPATHS, OnBnClickedHidepaths)
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

BOOL CLogDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_LOGDLG));

	OSVERSIONINFOEX inf;
	SecureZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	m_bVista = (fullver >= 0x0600);

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
	m_LogList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES);

	// the "hide unrelated paths" checkbox should be indeterminate
	m_cHidePaths.SetCheck(BST_INDETERMINATE);

	// load the icons for the action columns
	m_hModifiedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hAddedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	
	// if there is a working copy, load the project properties
	// to get information about the bugtraq: integration
	if (m_hasWC)
		m_ProjectProperties.ReadProps(m_path);

	// the bugtraq issue id column is only shown if the bugtraq:url or bugtraq:regex is set
	if ((!m_ProjectProperties.sUrl.IsEmpty())||(!m_ProjectProperties.sCheckRe.IsEmpty()))
		m_bShowBugtraqColumn = true;

	theme.SetWindowTheme(m_LogList.GetSafeHwnd(), L"Explorer", NULL);
	theme.SetWindowTheme(m_ChangedFileListCtrl.GetSafeHwnd(), L"Explorer", NULL);

	// set up the columns
	m_LogList.DeleteAllItems();
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
	ResizeAllListCtrlCols();
	m_LogList.SetRedraw(true);

	m_ChangedFileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	m_ChangedFileListCtrl.DeleteAllItems();
	c = ((CHeaderCtrl*)(m_ChangedFileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ChangedFileListCtrl.DeleteColumn(c--);
	temp.LoadString(IDS_PROGRS_ACTION);
	m_ChangedFileListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGRS_PATH);
	m_ChangedFileListCtrl.InsertColumn(1, temp);
	temp.LoadString(IDS_LOG_COPYFROM);
	m_ChangedFileListCtrl.InsertColumn(2, temp);
	temp.LoadString(IDS_LOG_REVISION);
	m_ChangedFileListCtrl.InsertColumn(3, temp);
	m_ChangedFileListCtrl.SetRedraw(false);
	CAppUtils::ResizeAllListCtrlCols(&m_ChangedFileListCtrl);
	m_ChangedFileListCtrl.SetRedraw(true);


	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

	m_logcounter = 0;
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
	
	AdjustControlSize(IDC_HIDEPATHS);
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
	
	AddAnchor(IDC_LOGLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTERTOP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MSGVIEW, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTERBOTTOM, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGMSG, BOTTOM_LEFT, BOTTOM_RIGHT);

	AddAnchor(IDC_LOGINFO, BOTTOM_LEFT, BOTTOM_RIGHT);	
	AddAnchor(IDC_HIDEPATHS, BOTTOM_LEFT);	
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
			m_wndSplitter1.SetWindowPos(NULL, 0, yPos1, 0, 0, SWP_NOSIZE);
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
			m_wndSplitter2.SetWindowPos(NULL, 0, yPos2, 0, 0, SWP_NOSIZE);
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

	// first start a thread to obtain the log messages without
	// blocking the dialog
	m_tTo = 0;
	m_tFrom = (DWORD)-1;
	InterlockedExchange(&m_bThreadRunning, TRUE);
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
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
			sTemp.Format(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle, (LPCTSTR)m_path.GetUIPathString());
		else if (m_path.IsDirectory())
			sTemp.Format(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle, (LPCTSTR)m_path.GetWinPathString());
		else
			sTemp.Format(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle, (LPCTSTR)m_path.GetFilename());
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
	// Since tooltip describes regexp features, show it only if regexps are enabled.
	if (m_bFilterWithRegex)
	{
		m_tooltips.AddTool(pWnd, IDS_LOG_FILTER_REGEX_TT);
	}
	else
		m_tooltips.DelTool(pWnd);
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
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	m_currentChangedArray = NULL;
	m_ChangedFileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	m_ChangedFileListCtrl.DeleteAllItems();
	m_ChangedFileListCtrl.SetItemCountEx(0);

	// if we're not here to really show a selected revision, just
	// get out of here after clearing the views, which is what is intended
	// if that flag is not set.
	if (!bShow)
	{
		// force a redraw
		m_ChangedFileListCtrl.Invalidate();
		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		m_ChangedFileListCtrl.SetRedraw(TRUE);
		return;
	}

	// depending on how many revisions are selected, we have to do different
	// tasks.
	int selCount = m_LogList.GetSelectedCount();
	if (selCount == 0)
	{
		// if nothing is selected, we have nothing more to do
		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		m_ChangedFileListCtrl.SetRedraw(TRUE);
		return;
	}
	else if (selCount == 1)
	{
		// if one revision is selected, we have to fill the log message view
		// with the corresponding log message, and also fill the changed files
		// list fully.
		POSITION pos = m_LogList.GetFirstSelectedItemPosition();
		int selIndex = m_LogList.GetNextSelectedItem(pos);
		if (selIndex >= m_arShownList.GetCount())
		{
			InterlockedExchange(&m_bNoDispUpdates, FALSE);
			m_ChangedFileListCtrl.SetRedraw(TRUE);
			return;
		}
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(selIndex));

		// set the log message text
		pMsgView->SetWindowText(pLogEntry->sMessage);
		// turn bug ID's into links if the bugtraq: properties have been set
		// and we can find a match of those in the log message
		m_ProjectProperties.FindBugID(pLogEntry->sMessage, pMsgView);
		CAppUtils::FormatTextInRichEditControl(pMsgView);
		m_currentChangedArray = pLogEntry->pArChangedPaths;
		if (m_currentChangedArray == NULL)
		{
			InterlockedExchange(&m_bNoDispUpdates, FALSE);
			m_ChangedFileListCtrl.SetRedraw(TRUE);
			return;
		}
		// fill in the changed files list control
		if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
		{
			m_CurrentFilteredChangedArray.RemoveAll();
			for (INT_PTR c = 0; c < m_currentChangedArray->GetCount(); ++c)
			{
				LogChangedPath * cpath = m_currentChangedArray->GetAt(c);
				if (cpath == NULL)
					continue;
				if (m_currentChangedArray->GetAt(c)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
				{
					m_CurrentFilteredChangedArray.Add(cpath);
				}
			}
			m_currentChangedArray = &m_CurrentFilteredChangedArray;
		}
	}
	else
	{
		// more than one revision is selected:
		// the log message view must be emptied
		// the changed files list contains all the changed paths from all
		// selected revisions, with 'doubles' removed
		m_currentChangedPathList = GetChangedPathsFromSelectedRevisions(true);
	}
	
	// redraw the views
	InterlockedExchange(&m_bNoDispUpdates, FALSE);
	if (m_currentChangedArray)
	{
		m_ChangedFileListCtrl.SetItemCountEx(m_currentChangedArray->GetCount());
		m_ChangedFileListCtrl.RedrawItems(0, m_currentChangedArray->GetCount());
	}
	else if (m_currentChangedPathList.GetCount())
	{
		m_ChangedFileListCtrl.SetItemCountEx(m_currentChangedPathList.GetCount());
		m_ChangedFileListCtrl.RedrawItems(0, m_currentChangedPathList.GetCount());
	}
	else
	{
		m_ChangedFileListCtrl.SetItemCountEx(0);
		m_ChangedFileListCtrl.Invalidate();
	}
	CAppUtils::ResizeAllListCtrlCols(&m_ChangedFileListCtrl);
	// sort according to the settings
	if (m_nSortColumnPathList > 0)
		SetSortArrow(&m_ChangedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
	else
		SetSortArrow(&m_ChangedFileListCtrl, -1, false);
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
			{
				return;
			}
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
	m_pStoreSelection = new CStoreSelection(this);
	m_LogList.SetItemCountEx(0);
	m_LogList.Invalidate();
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	pMsgView->SetWindowText(_T(""));

	SetSortArrow(&m_LogList, -1, true);

	m_LogList.DeleteAllItems();
	m_arShownList.RemoveAll();
	m_logEntries.ClearAll();

	m_logcounter = 0;
	m_bCancelled = FALSE;
	m_tTo = 0;
	m_tFrom = (DWORD)-1;
	m_limit = 0;

	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
	InterlockedExchange(&m_bNoDispUpdates, FALSE);
}

void CLogDlg::OnBnClickedRefresh()
{
	m_limit = 0;
	Refresh (true);
}

void CLogDlg::Refresh (bool autoGoOnline)
{
	// refreshing means re-downloading the already shown log messages
	UpdateData();
	m_maxChild = 0;
	m_childCounter = 0;

	if ((m_limit == 0)||(m_bStrict)||(int(m_logEntries.size()-1) > m_limit))
	{
		if (m_logEntries.size() != 0)
		{
			m_endrev = m_logEntries[m_logEntries.size()-1]->Rev;
		}
	}
	m_startrev = -1;
	m_bCancelled = FALSE;

	// We need to create CStoreSelection on the heap or else
	// the variable will run out of the scope before the
	// thread ends. Therefore we let the thread delete
	// the instance.
	m_pStoreSelection = new CStoreSelection(this);
	m_ChangedFileListCtrl.SetItemCountEx(0);
	m_ChangedFileListCtrl.Invalidate();
	m_LogList.SetItemCountEx(0);
	m_LogList.Invalidate();
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	pMsgView->SetWindowText(_T(""));

	SetSortArrow(&m_LogList, -1, true);

	m_LogList.DeleteAllItems();
	m_arShownList.RemoveAll();
	m_logEntries.ClearAll();

    // reset the cached HEAD property & go on-line

    if (autoGoOnline)
    {
	    SetDlgTitle (false);
        GetLogCachePool()->GetRepositoryInfo().ResetHeadRevision (m_sUUID, m_sRepositoryRoot);
    }

	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
	InterlockedExchange(&m_bNoDispUpdates, FALSE);
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
	svn_revnum_t rev = m_logEntries[m_logEntries.size()-1]->Rev;

	if (rev < 1)
		return;		// do nothing! No more revisions to get
	m_startrev = rev;
	m_endrev = 0;
	m_bCancelled = FALSE;

    // rev is is revision we already have and we will receive it again
    // -> fetch one extra revision to get NumberOfLogs *new* revisions

	m_limit = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100) +1;
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	SetSortArrow(&m_LogList, -1, true);
	InterlockedExchange(&m_bThreadRunning, TRUE);
	// We need to create CStoreSelection on the heap or else
	// the variable will run out of the scope before the
	// thread ends. Therefore we let the thread delete
	// the instance.
	m_pStoreSelection = new CStoreSelection(this);

	// since we fetch the log from the last revision we already have,
	// we have to remove that revision entry to avoid getting it twice
	m_logEntries.pop_back();
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
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
	// But canceling can also mean just to close the dialog, depending on the
	// text shown on the cancel button (it could simply read "OK").
	CString temp, temp2;
	GetDlgItemText(IDOK, temp);
	temp2.LoadString(IDS_MSGBOX_CANCEL);
	if ((temp.Compare(temp2)==0)||(m_bThreadRunning))
	{
		m_bCancelled = true;
		return;
	}
	UpdateData();
	if (m_bSaveStrict)
		m_regLastStrict = m_bStrict;
	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowAllEntry"));
	reg = m_btnShow.GetCurrentEntry();
	SaveSplitterPos();
	__super::OnCancel();
}

CString CLogDlg::MakeShortMessage(const CString& message)
{
	bool bFoundShort = true;
	CString sShortMessage = m_ProjectProperties.GetLogSummary(message);
	if (sShortMessage.IsEmpty())
	{
		bFoundShort = false;
		sShortMessage = message;
	}
	// Remove newlines and tabs 'cause those are not shown nicely in the list control
	sShortMessage.Replace(_T("\r"), _T(""));
	sShortMessage.Replace(_T("\t"), _T(" "));
	
	// Suppose the first empty line separates 'summary' from the rest of the message.
	int found = sShortMessage.Find(_T("\n\n"));
	// To avoid too short 'short' messages 
	// (e.g. if the message looks something like "Bugfix:\n\n*done this\n*done that")
	// only use the empty newline as a separator if it comes after at least 15 chars.
	if ((!bFoundShort)&&(found >= 15))
	{
		sShortMessage = sShortMessage.Left(found);
	}
	sShortMessage.Replace('\n', ' ');
	return sShortMessage;
}

BOOL CLogDlg::Log(svn_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies, DWORD actions, BOOL haschildren)
{
	if (rev == SVN_INVALID_REVNUM)
	{
		m_childCounter--;
		return TRUE;
	}
	// this is the callback function which receives the data for every revision we ask the log for
	// we store this information here one by one.
	m_logcounter += 1;
	if (m_startrev == -1)
		m_startrev = rev;
	if (m_limit != 0)
	{
		m_limitcounter--;
		m_LogProgress.SetPos(m_limit - m_limitcounter);
	}
	else if (m_startrev.IsNumber() && m_startrev.IsNumber())
		m_LogProgress.SetPos((svn_revnum_t)m_startrev-rev+(svn_revnum_t)m_endrev);
	__time64_t ttime = time/1000000L;
	if (m_tTo < (DWORD)ttime)
		m_tTo = (DWORD)ttime;
	if (m_tFrom > (DWORD)ttime)
		m_tFrom = (DWORD)ttime;
	if ((m_lowestRev > rev)||(m_lowestRev < 0))
		m_lowestRev = rev;
	// Add as many characters from the log message to the list control
	PLOGENTRYDATA pLogItem = new LOGENTRYDATA;
	pLogItem->bCopies = !!copies;
	
	// find out if this item was copied in the revision
	BOOL copiedself = FALSE;
	if (copies)
	{
		for (INT_PTR cpPathIndex = 0; cpPathIndex < cpaths->GetCount(); ++cpPathIndex)
		{
			LogChangedPath * cpath = cpaths->GetAt(cpPathIndex);
			if (!cpath->sCopyFromPath.IsEmpty() && (cpath->sPath.Compare(m_sSelfRelativeURL) == 0))
			{
				// note: this only works if the log is fetched top-to-bottom
				// but since we do that, it shouldn't be a problem
				m_sSelfRelativeURL = cpath->sCopyFromPath;
				copiedself = TRUE;
				break;
			}
		}
	}
	pLogItem->bCopiedSelf = copiedself;
	pLogItem->tmDate = ttime;
	pLogItem->sAuthor = author;
	pLogItem->sDate = date;
	pLogItem->sShortMessage = MakeShortMessage(message);
	pLogItem->dwFileChanges = filechanges;
	pLogItem->actions = actions;
	pLogItem->haschildren = haschildren;
	pLogItem->childStackDepth = m_childCounter;
	m_maxChild = max(m_childCounter, m_maxChild);
	if (haschildren)
		m_childCounter++;
	pLogItem->sBugIDs = m_ProjectProperties.FindBugID(message).Trim();
	
	// split multi line log entries and concatenate them
	// again but this time with \r\n as line separators
	// so that the edit control recognizes them
	try
	{
		if (message.GetLength()>0)
		{
			m_sMessageBuf = message;
			m_sMessageBuf.Replace(_T("\n\r"), _T("\n"));
			m_sMessageBuf.Replace(_T("\r\n"), _T("\n"));
			if (m_sMessageBuf.Right(1).Compare(_T("\n"))==0)
				m_sMessageBuf = m_sMessageBuf.Left(m_sMessageBuf.GetLength()-1);
		}
		else
			m_sMessageBuf.Empty();
        pLogItem->sMessage = m_sMessageBuf;
        pLogItem->Rev = rev;

        // move-construct path array

        pLogItem->pArChangedPaths = new LogChangedPathArray (*cpaths);
        cpaths->RemoveAll();
	}
	catch (CException * e)
	{
		::MessageBox(NULL, _T("not enough memory!"), _T("TortoiseSVN"), MB_ICONERROR);
		e->Delete();
		m_bCancelled = TRUE;
	}
	m_logEntries.push_back(pLogItem);
	m_arShownList.Add(pLogItem);
	
	return TRUE;
}

//this is the thread function which calls the subversion function
UINT CLogDlg::LogThreadEntry(LPVOID pVoid)
{
	return ((CLogDlg*)pVoid)->LogThread();
}


//this is the thread function which calls the subversion function
UINT CLogDlg::LogThread()
{
	InterlockedExchange(&m_bThreadRunning, TRUE);

    //does the user force the cache to refresh (shift or control key down)?
    bool refresh =    (GetKeyState (VK_CONTROL) < 0) 
                   || (GetKeyState (VK_SHIFT) < 0);

	//disable the "Get All" button while we're receiving
	//log messages.
	DialogEnableWindow(IDC_GETALL, FALSE);
	DialogEnableWindow(IDC_NEXTHUNDRED, FALSE);
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
	svn_revnum_t r = -1;
	
	// get the repository root url, because the changed-files-list has the
	// paths shown there relative to the repository root.
	CTSVNPath rootpath;
    BOOL succeeded = GetRootAndHead(m_path, rootpath, r);

    m_sRepositoryRoot = rootpath.GetSVNPathString();
    m_sURL = m_path.GetSVNPathString();

    // we need the UUID to unambigously identify the log cache
    if (LogCache::CSettings::GetEnabled())
        m_sUUID = GetLogCachePool()->GetRepositoryInfo().GetRepositoryUUID (rootpath);

    // if the log dialog is started from a working copy, we need to turn that
    // local path into an url here
    if (succeeded)
    {
        if (!m_path.IsUrl())
        {
	        m_sURL = GetURLFromPath(m_path);

	        // The URL is escaped because SVN::logReceiver
	        // returns the path in a native format
	        m_sURL = CPathUtils::PathUnescape(m_sURL);
        }
        m_sRelativeRoot = m_sURL.Mid(CPathUtils::PathUnescape(m_sRepositoryRoot).GetLength());
		m_sSelfRelativeURL = m_sRelativeRoot;
    }

    if (succeeded && !m_mergePath.IsEmpty() && m_mergedRevs.empty())
    {
	    // in case we got a merge path set, retrieve the merge info
	    // of that path and check whether one of the merge URLs
	    // match the URL we show the log for.
	    SVNPool localpool(pool);
	    svn_error_clear(Err);
	    apr_hash_t * mergeinfo = NULL;
	    if (svn_client_mergeinfo_get_merged (&mergeinfo, m_mergePath.GetSVNApiPath(localpool), SVNRev(SVNRev::REV_WC), m_pctx, localpool) == NULL)
	    {
		    // now check the relative paths
		    apr_hash_index_t *hi;
		    const void *key;
		    void *val;

		    if (mergeinfo)
		    {
			    for (hi = apr_hash_first(localpool, mergeinfo); hi; hi = apr_hash_next(hi))
			    {
				    apr_hash_this(hi, &key, NULL, &val);
				    if (m_sURL.Compare(CUnicodeUtils::GetUnicode((char*)key)) == 0)
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
    if (m_startrev == SVNRev::REV_HEAD)
    {
	    m_startrev = r;
    }
    if (m_endrev == SVNRev::REV_HEAD)
    {
	    m_endrev = r;
    }

    if (m_limit != 0)
    {
	    m_limitcounter = m_limit;
	    m_LogProgress.SetRange32(0, m_limit);
    }
    else
	    m_LogProgress.SetRange32(m_endrev, m_startrev);
	
    if (!m_pegrev.IsValid())
	    m_pegrev = m_startrev;
    size_t startcount = m_logEntries.size();
    m_lowestRev = -1;
    m_bStrictStopped = false;

    if (succeeded)
    {
        succeeded = ReceiveLog (CTSVNPathList(m_path), m_pegrev, m_startrev, m_endrev, m_limit, m_bStrict, m_bIncludeMerges, refresh);
        if ((!succeeded)&&(!m_path.IsUrl()))
        {
	        // try again with REV_WC as the start revision, just in case the path doesn't
	        // exist anymore in HEAD
	        succeeded = ReceiveLog(CTSVNPathList(m_path), SVNRev(), SVNRev::REV_WC, m_endrev, m_limit, m_bStrict, m_bIncludeMerges, refresh);
        }
    }
	m_LogList.ClearText();
    if (!succeeded)
	{
		m_LogList.ShowText(GetLastErrorMessage(), true);
		FillLogMessageCtrl(false);
	}
	else
	{
		if (!m_wcRev.IsValid())
		{
			// fetch the revision the wc path is on so we can mark it
			CTSVNPath revWCPath = m_ProjectProperties.GetPropsPath();
			if (!m_path.IsUrl())
				revWCPath = m_path;
			if (DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\RecursiveLogRev"), FALSE)))
			{
				svn_revnum_t minrev, maxrev;
				bool switched, modified, sparse;
				GetWCRevisionStatus(revWCPath, true, minrev, maxrev, switched, modified, sparse);
				if (maxrev)
					m_wcRev = maxrev;
			}
			else
			{
				CTSVNPath dummypath;
				SVNStatus status;
				svn_wc_status2_t * stat = status.GetFirstFileStatus(revWCPath, dummypath, false, svn_depth_empty);
				if (stat && stat->entry && stat->entry->cmt_rev)
					m_wcRev = stat->entry->cmt_rev;
				if (stat && stat->entry && (stat->entry->kind == svn_node_dir))
					m_wcRev = stat->entry->revision;
			}
		}
	}
    if (m_bStrict && (m_lowestRev>1) && ((m_limit>0) ? ((startcount + m_limit)>m_logEntries.size()) : (m_endrev<m_lowestRev)))
		m_bStrictStopped = true;
	m_LogList.SetItemCountEx(ShownCountWithStopped());

	m_timFrom = (__time64_t(m_tFrom));
	m_timTo = (__time64_t(m_tTo));
	m_DateFrom.SetRange(&m_timFrom, &m_timTo);
	m_DateTo.SetRange(&m_timFrom, &m_timTo);
	m_DateFrom.SetTime(&m_timFrom);
	m_DateTo.SetTime(&m_timTo);

	DialogEnableWindow(IDC_GETALL, TRUE);
	
	if (!m_bShowedAll)
		DialogEnableWindow(IDC_NEXTHUNDRED, TRUE);
	DialogEnableWindow(IDC_CHECK_STOPONCOPY, TRUE);
	DialogEnableWindow(IDC_INCLUDEMERGE, TRUE);
	DialogEnableWindow(IDC_STATBUTTON, TRUE);
	DialogEnableWindow(IDC_REFRESH, TRUE);

	LogCache::CRepositoryInfo& cachedProperties = GetLogCachePool()->GetRepositoryInfo();
	SetDlgTitle(cachedProperties.IsOffline (m_sUUID, m_sRepositoryRoot, false));

	GetDlgItem(IDC_PROGRESS)->ShowWindow(FALSE);
	m_bCancelled = true;
	InterlockedExchange(&m_bThreadRunning, FALSE);
	m_LogList.RedrawItems(0, m_arShownList.GetCount());
	m_LogList.SetRedraw(false);
	ResizeAllListCtrlCols();
	m_LogList.SetRedraw(true);
	if ( m_pStoreSelection )
	{
		// Deleting the instance will restore the
		// selection of the CLogDlg.
		delete m_pStoreSelection;
		m_pStoreSelection = NULL;
	}
	else
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
	return 0;
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
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
			for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
			{
				LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
				sPaths += cpath->GetAction() + _T(" : ") + cpath->sPath;
				if (cpath->sCopyFromPath.IsEmpty())
					sPaths += _T("\r\n");
				else
				{
					CString sCopyFrom;
					sCopyFrom.Format(_T(" (%s: %s, %s, %ld)\r\n"), (LPCTSTR)CString(MAKEINTRESOURCE(IDS_LOG_COPYFROM)), 
						(LPCTSTR)cpath->sCopyFromPath, 
						(LPCTSTR)CString(MAKEINTRESOURCE(IDS_LOG_REVISION)), 
						cpath->lCopyFromRev);
					sPaths += sCopyFrom;
				}
			}
			sPaths.Trim();
			sLogCopyText.Format(_T("%s: %d\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n"),
				(LPCTSTR)sRev, pLogEntry->Rev,
				(LPCTSTR)sAuthor, (LPCTSTR)pLogEntry->sAuthor,
				(LPCTSTR)sDate, (LPCTSTR)pLogEntry->sDate,
				(LPCTSTR)sMessage, (LPCTSTR)pLogEntry->sMessage,
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

	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
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
			LogChangedPath * changedlogpath = pLogEntry->pArChangedPaths->GetAt(nItem);

			if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
			{
				// some items are hidden! So find out which item the user really selected
				INT_PTR selRealIndex = -1;
				for (INT_PTR hiddenindex=0; hiddenindex<pLogEntry->pArChangedPaths->GetCount(); ++hiddenindex)
				{
					if (pLogEntry->pArChangedPaths->GetAt(hiddenindex)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
						selRealIndex++;
					if (selRealIndex == nItem)
					{
						changedlogpath = pLogEntry->pArChangedPaths->GetAt(hiddenindex);
						break;
					}
				}
			}
			if (changedlogpath)
			{
				sPaths += changedlogpath->sPath;
				sPaths += _T("\r\n");
			}
		}
	}
	sPaths.Trim();
	CStringUtils::WriteAsciiStringToClipboard(sPaths, GetSafeHwnd());
}

BOOL CLogDlg::IsDiffPossible(LogChangedPath * changedpath, svn_revnum_t rev)
{
	CString added, deleted;
	if (changedpath == NULL)
		return false;

	if ((rev > 1)&&(changedpath->action != LOGACTIONS_DELETED))
	{
		if (changedpath->action == LOGACTIONS_ADDED) // file is added
		{
			if (changedpath->lCopyFromRev == 0)
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
	bool bContinuous = (m_arShownList.GetCount() == (INT_PTR)m_logEntries.size());
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
        CString FindText = m_pFindDialog->GetFindString();
        bool bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
		bool bFound = false;
		tr1::wregex pat;
		bool bRegex = ValidateRegexp(FindText, pat, bMatchCase);

		tr1::regex_constants::match_flag_type flags = tr1::regex_constants::match_not_null;

		int i;
		for (i = this->m_nSearchIndex; i<m_arShownList.GetCount()&&!bFound; i++)
		{
			if (bRegex)
			{
				PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));

				if (regex_search(wstring((LPCTSTR)pLogEntry->sMessage), pat, flags))
				{
					bFound = true;
					break;
				}
				LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
				for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
				{
					LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
					if (regex_search(wstring((LPCTSTR)cpath->sCopyFromPath), pat, flags))
					{
						bFound = true;
						--i;
						break;
					}
					if (regex_search(wstring((LPCTSTR)cpath->sPath), pat, flags))
					{
						bFound = true;
						--i;
						break;
					}
				}
			}
			else
			{
				if (bMatchCase)
				{
					if (m_logEntries[i]->sMessage.Find(FindText) >= 0)
					{
						bFound = true;
						break;
					}
					PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
					LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						if (cpath->sCopyFromPath.Find(FindText)>=0)
						{
							bFound = true;
							--i;
							break;
						}
						if (cpath->sPath.Find(FindText)>=0)
						{
							bFound = true;
							--i;
							break;
						}
					}
				}
				else
				{
				    PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
					CString msg = pLogEntry->sMessage;
					msg = msg.MakeLower();
					CString find = FindText.MakeLower();
					if (msg.Find(find) >= 0)
					{
						bFound = TRUE;
						break;
					}
					LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						CString lowerpath = cpath->sCopyFromPath;
						lowerpath.MakeLower();
						if (lowerpath.Find(find)>=0)
						{
							bFound = TRUE;
							--i;
							break;
						}
						lowerpath = cpath->sPath;
						lowerpath.MakeLower();
						if (lowerpath.Find(find)>=0)
						{
							bFound = TRUE;
							--i;
							break;
						}
					}
				} 
			}
		} // for (i = this->m_nSearchIndex; i<m_arShownList.GetItemCount()&&!bFound; i++)
		if (bFound)
		{
			this->m_nSearchIndex = (i+1);
			m_LogList.EnsureVisible(i, FALSE);
			m_LogList.SetItemState(m_LogList.GetSelectionMark(), 0, LVIS_SELECTED);
			m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_LogList.SetSelectionMark(i);
			FillLogMessageCtrl();
			UpdateData(FALSE);
			m_nSearchIndex++;
			if (m_nSearchIndex >= m_arShownList.GetCount())
				m_nSearchIndex = (int)m_arShownList.GetCount()-1;
		}
    } // if(m_pFindDialog->FindNext()) 
	UpdateLogInfoLabel();
    return 0;
}

void CLogDlg::OnOK()
{
	// since the log dialog is also used to select revisions for other
	// dialogs, we have to do some work before closing this dialog
	if (GetFocus() != GetDlgItem(IDOK))
		return;	// if the "OK" button doesn't have the focus, do nothing: this prevents closing the dialog when pressing enter
	if (!GetDlgItem(IDOK)->IsWindowVisible() && GetFocus() != GetDlgItem(IDCANCEL))
		return; // the Cancel button works as the OK button. But if the cancel button has not the focus, do nothing.

	CString temp;
	CString buttontext;
	GetDlgItemText(IDOK, buttontext);
	temp.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(buttontext) != 0)
		__super::OnOK();	// only exit if the button text matches, and that will match only if the thread isn't running anymore
	m_bCancelled = TRUE;
	m_selectedRevs.Clear();
	m_selectedRevsOneRange.Clear();
	if (m_pNotifyWindow)
	{
		int selIndex = m_LogList.GetSelectionMark();
		if (selIndex >= 0)
		{	
		    PLOGENTRYDATA pLogEntry = NULL;
			POSITION pos = m_LogList.GetFirstSelectedItemPosition();
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			m_selectedRevs.AddRevision(pLogEntry->Rev);
			svn_revnum_t lowerRev = pLogEntry->Rev;
			svn_revnum_t higherRev = lowerRev;
			while (pos)
			{
			    pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
				svn_revnum_t rev = pLogEntry->Rev;
				m_selectedRevs.AddRevision(pLogEntry->Rev);
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
				if ((pLogEntry)&&(pLogEntry->pArChangedPaths)&&(lowerRev == higherRev))
				{
					CString sUrl = m_path.GetSVNPathString();
					if (!m_path.IsUrl())
					{
						sUrl = GetURLFromPath(m_path);
					}
					sUrl = sUrl.Mid(m_sRepositoryRoot.GetLength());
					for (int cp = 0; cp < pLogEntry->pArChangedPaths->GetCount(); ++cp)
					{
						LogChangedPath * pData = pLogEntry->pArChangedPaths->GetAt(cp);
						if (pData)
						{
							if (sUrl.Compare(pData->sPath) == 0)
							{
								if (!pData->sCopyFromPath.IsEmpty())
								{
									lowerRev = pData->lCopyFromRev;
									m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTSTART), lowerRev);
									m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTEND), higherRev);
									m_pNotifyWindow->SendMessage(WM_REVLIST, m_selectedRevs.GetCount(), (LPARAM)&m_selectedRevs);
									bSentMessage = TRUE;
								}
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
	reg = m_btnShow.GetCurrentEntry();
	SaveSplitterPos();
}

void CLogDlg::OnNMDblclkChangedFileList(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// a double click on an entry in the changed-files list has happened
	*pResult = 0;

	DiffSelectedFile();
}

void CLogDlg::DiffSelectedFile()
{
	if ((m_bThreadRunning)||(m_LogList.HasText()))
		return;
	UpdateLogInfoLabel();
	INT_PTR selIndex = m_ChangedFileListCtrl.GetSelectionMark();
	if (selIndex < 0)
		return;
	if (m_ChangedFileListCtrl.GetSelectedCount() == 0)
		return;
	// find out if there's an entry selected in the log list
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
	svn_revnum_t rev1 = pLogEntry->Rev;
	svn_revnum_t rev2 = rev1;
	if (pos)
	{
		while (pos)
		{
			// there's at least a second entry selected in the log list: several revisions selected!
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			if (pLogEntry)
			{
				rev1 = max(rev1,(long)pLogEntry->Rev);
				rev2 = min(rev2,(long)pLogEntry->Rev);
			}
		}
		rev2--;
		// now we have both revisions selected in the log list, so we can do a diff of the selected
		// entry in the changed files list with these two revisions.
		DoDiffFromLog(selIndex, rev1, rev2, false, false);
	}
	else
	{
		rev2 = rev1-1;
		// nothing or only one revision selected in the log list
		LogChangedPath * changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);

		if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
		{
			// some items are hidden! So find out which item the user really clicked on
			INT_PTR selRealIndex = -1;
			for (INT_PTR hiddenindex=0; hiddenindex<pLogEntry->pArChangedPaths->GetCount(); ++hiddenindex)
			{
				if (pLogEntry->pArChangedPaths->GetAt(hiddenindex)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
					selRealIndex++;
				if (selRealIndex == selIndex)
				{
					selIndex = hiddenindex;
					changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);
					break;
				}
			}
		}

		if (IsDiffPossible(changedpath, rev1))
		{
			// diffs with renamed files are possible
			if ((changedpath)&&(!changedpath->sCopyFromPath.IsEmpty()))
				rev2 = changedpath->lCopyFromRev;
			else
			{
				// if the path was modified but the parent path was 'added with history'
				// then we have to use the copy from revision of the parent path
				CTSVNPath cpath = CTSVNPath(changedpath->sPath);
				for (int flist = 0; flist < pLogEntry->pArChangedPaths->GetCount(); ++flist)
				{
					CTSVNPath p = CTSVNPath(pLogEntry->pArChangedPaths->GetAt(flist)->sPath);
					if (p.IsAncestorOf(cpath))
					{
						if (!pLogEntry->pArChangedPaths->GetAt(flist)->sCopyFromPath.IsEmpty())
							rev2 = pLogEntry->pArChangedPaths->GetAt(flist)->lCopyFromRev;
					}
				}
			}
			DoDiffFromLog(selIndex, rev1, rev2, false, false);
		}
		else 
		{
			CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(changedpath->sPath));
			CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(changedpath->sPath));
			SVNRev r = rev1;
			// deleted files must be opened from the revision before the deletion
			if (changedpath->action == LOGACTIONS_DELETED)
				r = rev1-1;
			m_bCancelled = false;

			CProgressDlg progDlg;
			progDlg.SetTitle(IDS_APPNAME);
			progDlg.SetAnimation(IDR_DOWNLOAD);
			CString sInfoLine;
			sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)(m_sRepositoryRoot + changedpath->sPath), (LPCTSTR)r.ToString());
			progDlg.SetLine(1, sInfoLine, true);
			SetAndClearProgressInfo(&progDlg);
			progDlg.ShowModeless(m_hWnd);

			if (!Cat(CTSVNPath(m_sRepositoryRoot + changedpath->sPath), r, r, tempfile))
			{
				m_bCancelled = false;
				if (!Cat(CTSVNPath(m_sRepositoryRoot + changedpath->sPath), SVNRev::REV_HEAD, r, tempfile))
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
			sName1.Format(_T("%s - Revision %ld"), (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath->sPath), (svn_revnum_t)rev1);
			sName2.Format(_T("%s - Revision %ld"), (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath->sPath), (svn_revnum_t)rev1-1);
			CAppUtils::DiffFlags flags;
			flags.AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			if (changedpath->action == LOGACTIONS_DELETED)
				CAppUtils::StartExtDiff(tempfile, tempfile2, sName2, sName1, flags);
			else
				CAppUtils::StartExtDiff(tempfile2, tempfile, sName2, sName1, flags);
		}
	}
}


void CLogDlg::OnNMDblclkLoglist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// a double click on an entry in the revision list has happened
	*pResult = 0;

  if (CRegDWORD(_T("Software\\TortoiseSVN\\DiffByDoubleClickInLog"), FALSE))
	  DiffSelectedRevWithPrevious();
}

void CLogDlg::DiffSelectedRevWithPrevious()
{
	if ((m_bThreadRunning)||(m_LogList.HasText()))
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
	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
	long rev1 = pLogEntry->Rev;
	long rev2 = rev1-1;
	CTSVNPath path = m_path;

	// See how many files under the relative root were changed in selected revision
	int nChanged = 0;
	LogChangedPath * changed = NULL;
	for (INT_PTR c = 0; c < pLogEntry->pArChangedPaths->GetCount(); ++c)
	{
		LogChangedPath * cpath = pLogEntry->pArChangedPaths->GetAt(c);
		if (cpath  &&  cpath -> sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
		{
			++nChanged;
			changed = cpath;
		}
	}

	if (m_path.IsDirectory() && nChanged == 1) 
	{
		// We're looking at the log for a directory and only one file under dir was changed in the revision
		// Do diff on that file instead of whole directory
		path.AppendPathString(changed->sPath.Mid(m_sRelativeRoot.GetLength()));
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
		diff.ShowCompare(path, rev2, path, rev1);
	}
	else
	{
		CAppUtils::StartShowCompare(m_hWnd, path, rev2, path, rev1, SVNRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
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
			theApp.DoWaitCursor(-11);
			return;		//exit
		}
	}
	m_bCancelled = FALSE;
	filepath = GetRepositoryRoot(CTSVNPath(filepath));

	CString firstfile, secondfile;
	if (m_LogList.GetSelectedCount()==1)
	{
		int s = m_LogList.GetSelectionMark();
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(s));
		LogChangedPath * changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);
		firstfile = changedpath->sPath;
		secondfile = firstfile;
		if ((rev2 == rev1-1)&&(changedpath->lCopyFromRev > 0)) // is it an added file with history?
		{
			secondfile = changedpath->sCopyFromPath;
			rev2 = changedpath->lCopyFromRev;
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
	diff.SetHEADPeg(m_LogRevision);
	if (unified)
	{
		if (PromptShown())
			diff.ShowUnifiedDiff(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1);
		else
			CAppUtils::StartShowUnifiedDiff(m_hWnd, CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1, SVNRev(), m_LogRevision);
	}
	else
	{
		if (diff.ShowCompare(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1, SVNRev(), false, blame))
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
	sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)filepath, (LPCTSTR)SVNRev(rev).ToString());
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

void CLogDlg::EditAuthor(const CLogDataVector& logs)
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
	{
		url = GetURLFromPath(m_path);
	}
	name = SVN_PROP_REVISION_AUTHOR;

	CString value = RevPropertyGet(name, CTSVNPath(url), logs[0]->Rev);
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
			if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTSVNPath(url), logs[i]->Rev))
			{
				progDlg.Stop();
				CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				break;
			}
			else
			{

				logs[i]->sAuthor = dlg.m_sInputText;
				m_LogList.Invalidate();

				// update the log cache 

				if (toUpdate != NULL)
				{
					// log caching is active

					LogCache::CCachedLogInfo newInfo;
					newInfo.Insert ( logs[i]->Rev
						, (const char*) CUnicodeUtils::GetUTF8 (logs[i]->sAuthor)
						, ""
						, 0
						, LogCache::CRevisionInfoContainer::HAS_AUTHOR);

					toUpdate->Update (newInfo);
				}
			}
			progDlg.SetProgress64(i, logs.size());
		}
		progDlg.Stop();
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
	{
		url = GetURLFromPath(m_path);
	}
	name = SVN_PROP_REVISION_LOG;

	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(index));
	m_bCancelled = FALSE;
	CString value = RevPropertyGet(name, CTSVNPath(url), pLogEntry->Rev);
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
		dlg.m_sInputText.Replace(_T("\r"), _T(""));
		if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTSVNPath(url), pLogEntry->Rev))
		{
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		else
		{
			pLogEntry->sShortMessage = MakeShortMessage(dlg.m_sInputText);
			// split multi line log entries and concatenate them
			// again but this time with \r\n as line separators
			// so that the edit control recognizes them
			if (dlg.m_sInputText.GetLength()>0)
			{
				m_sMessageBuf = dlg.m_sInputText;
				dlg.m_sInputText.Replace(_T("\n\r"), _T("\n"));
				dlg.m_sInputText.Replace(_T("\r\n"), _T("\n"));
				if (dlg.m_sInputText.Right(1).Compare(_T("\n"))==0)
					dlg.m_sInputText = dlg.m_sInputText.Left(dlg.m_sInputText.GetLength()-1);
			} 
			else
				dlg.m_sInputText.Empty();
			pLogEntry->sMessage = dlg.m_sInputText;
			pLogEntry->sBugIDs = m_ProjectProperties.FindBugID(dlg.m_sInputText);
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
                newInfo.Insert ( pLogEntry->Rev
                               , ""
                               , (const char*) CUnicodeUtils::GetUTF8 (pLogEntry->sMessage)
                               , 0
                               , LogCache::CRevisionInfoContainer::HAS_COMMENT);

                toUpdate->Update (newInfo);
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
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam=='\r')
	{
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
	if (m_bThreadRunning)
	{
		// only show the wait cursor over the list control
		if ((pWnd)&&
			((pWnd == GetDlgItem(IDC_LOGLIST))||
			(pWnd == GetDlgItem(IDC_MSGVIEW))||
			(pWnd == GetDlgItem(IDC_LOGMSG))))
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

void CLogDlg::OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if ((m_bThreadRunning)||(m_LogList.HasText()))
		return;
	if (pNMLV->iItem >= 0)
	{
		m_nSearchIndex = pNMLV->iItem;
		if (pNMLV->iSubItem != 0)
			return;
		if ((pNMLV->iItem == m_arShownList.GetCount())&&(m_bStrict)&&(m_bStrictStopped))
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
	ENLINK *pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink->msg == WM_LBUTTONUP)
	{
		CString url, msg;
		GetDlgItemText(IDC_MSGVIEW, msg);
		msg.Replace(_T("\r\n"), _T("\n"));
		url = msg.Mid(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax-pEnLink->chrg.cpMin);
		if (!::PathIsURL(url))
		{
			url = m_ProjectProperties.GetBugIDUrl(url);
			url = GetAbsoluteUrlFromRelativeUrl(url);
		}
		if (!url.IsEmpty())
			ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
	}
	*pResult = 0;
}

void CLogDlg::OnBnClickedStatbutton()
{
	if ((m_bThreadRunning)||(m_LogList.HasText()))
		return;
	if (m_arShownList.IsEmpty())
		return;		// nothing is shown, so no statistics.
	// the statistics dialog expects the log entries to be sorted by date
	SortByColumn(3, false);
	CPtrArray shownlist;
	RecalculateShownList(&shownlist);
	// create arrays which are aware of the current filter
	CStringArray m_arAuthorsFiltered;
	CDWordArray m_arDatesFiltered;
	CDWordArray m_arFileChangesFiltered;
	for (INT_PTR i=0; i<shownlist.GetCount(); ++i)
	{
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(shownlist.GetAt(i));
		CString strAuthor = pLogEntry->sAuthor;
		if ( strAuthor.IsEmpty() )
		{
			strAuthor.LoadString(IDS_STATGRAPH_EMPTYAUTHOR);
		}
		m_arAuthorsFiltered.Add(strAuthor);
		m_arDatesFiltered.Add(static_cast<DWORD>(pLogEntry->tmDate));
		m_arFileChangesFiltered.Add(pLogEntry->dwFileChanges);
	}
	CStatGraphDlg dlg;
	dlg.m_parAuthors = &m_arAuthorsFiltered;
	dlg.m_parDates = &m_arDatesFiltered;
	dlg.m_parFileChanges = &m_arFileChangesFiltered;
	dlg.m_path = m_path;
	dlg.DoModal();
	// restore the previous sorting
	SortByColumn(m_nSortColumn, m_bAscending);
	OnTimer(LOGFILTER_TIMER);
}

void CLogDlg::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	if (m_bNoDispUpdates)
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

			if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
			{
				PLOGENTRYDATA data = (PLOGENTRYDATA)m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec);
				if (data)
				{
					if (data->bCopiedSelf)
					{
						// only change the background color if the item is not 'hot' (on vista with themes enabled)
						if (!theme.IsAppThemed() || !m_bVista || ((pLVCD->nmcd.uItemState & CDIS_HOT)==0))
							pLVCD->clrTextBk = GetSysColor(COLOR_MENU);
					}
					if (data->bCopies)
						crText = m_Colors.GetColor(CColors::Modified);
					if ((data->childStackDepth)||(m_mergedRevs.find(data->Rev) != m_mergedRevs.end()))
						crText = GetSysColor(COLOR_GRAYTEXT);
					if (data->Rev == m_wcRev)
					{
						SelectObject(pLVCD->nmcd.hdc, m_boldFont);
						// We changed the font, so we're returning CDRF_NEWFONT. This
						// tells the control to recalculate the extent of the text.
						*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
					}
				}
			}
			if (m_arShownList.GetCount() == (INT_PTR)pLVCD->nmcd.dwItemSpec)
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
			if ((m_bStrictStopped)&&(m_arShownList.GetCount() == (INT_PTR)pLVCD->nmcd.dwItemSpec))
			{
				pLVCD->nmcd.uItemState &= ~(CDIS_SELECTED|CDIS_FOCUS);
			}
			if (pLVCD->iSubItem == 1)
			{
				*pResult = CDRF_DODEFAULT;

				if (m_arShownList.GetCount() <= (INT_PTR)pLVCD->nmcd.dwItemSpec)
					return;

				int		nIcons = 0;
				int		iconwidth = ::GetSystemMetrics(SM_CXSMICON);
				int		iconheight = ::GetSystemMetrics(SM_CYSMICON);

				PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec));

				// Get the selected state of the
				// item being drawn.
				LVITEM   rItem;
				SecureZeroMemory(&rItem, sizeof(LVITEM));
				rItem.mask  = LVIF_STATE;
				rItem.iItem = pLVCD->nmcd.dwItemSpec;
				rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
				m_LogList.GetItem(&rItem);

				CRect rect;
				m_LogList.GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);

				// Fill the background
				if (theme.IsAppThemed() && m_bVista)
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
						if (pLogEntry->bCopiedSelf)
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
						if (pLogEntry->bCopiedSelf)
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
				if (pLogEntry->actions & LOGACTIONS_MODIFIED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hModifiedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->actions & LOGACTIONS_ADDED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hAddedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->actions & LOGACTIONS_DELETED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hDeletedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->actions & LOGACTIONS_REPLACED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hReplacedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
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

	if (m_bNoDispUpdates)
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
		if ((m_cHidePaths.GetState() & 0x0003)==BST_INDETERMINATE)
		{
			if ((m_currentChangedArray)&&((m_currentChangedArray->GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)))
			{
				if (m_currentChangedArray->GetAt(pLVCD->nmcd.dwItemSpec)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)!=0)
				{
					crText = GetSysColor(COLOR_GRAYTEXT);
					bGrayed = true;
				}
			}
			else if (m_currentChangedPathList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
			{
				if (m_currentChangedPathList[pLVCD->nmcd.dwItemSpec].GetSVNPathString().Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)!=0)
				{
					crText = GetSysColor(COLOR_GRAYTEXT);
					bGrayed = true;
				}
			}
		}

		if ((!bGrayed)&&(m_currentChangedArray)&&(m_currentChangedArray->GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec))
		{
			DWORD action = m_currentChangedArray->GetAt(pLVCD->nmcd.dwItemSpec)->action;
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
	RemoveAnchor(IDC_LOGLIST);
	RemoveAnchor(IDC_SPLITTERTOP);
	RemoveAnchor(IDC_MSGVIEW);
	RemoveAnchor(IDC_SPLITTERBOTTOM);
	RemoveAnchor(IDC_LOGMSG);
	CSplitterControl::ChangeHeight(&m_LogList, delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), -delta, CW_BOTTOMALIGN);
	AddAnchor(IDC_LOGLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTERTOP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MSGVIEW, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTERBOTTOM, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGMSG, BOTTOM_LEFT, BOTTOM_RIGHT);
	ArrangeLayout();
	AdjustMinSize();
	SetSplitterRange();
	m_LogList.Invalidate();
	GetDlgItem(IDC_MSGVIEW)->Invalidate();
}

void CLogDlg::DoSizeV2(int delta)
{
	RemoveAnchor(IDC_LOGLIST);
	RemoveAnchor(IDC_SPLITTERTOP);
	RemoveAnchor(IDC_MSGVIEW);
	RemoveAnchor(IDC_SPLITTERBOTTOM);
	RemoveAnchor(IDC_LOGMSG);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(&m_ChangedFileListCtrl, -delta, CW_BOTTOMALIGN);
	AddAnchor(IDC_LOGLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTERTOP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MSGVIEW, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTERBOTTOM, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGMSG, BOTTOM_LEFT, BOTTOM_RIGHT);
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

	SetMinTrackSize(CSize(m_DlgOrigRect.Width(), 
		m_DlgOrigRect.Height()-m_ChgOrigRect.Height()-m_LogListOrigRect.Height()-m_MsgViewOrigRect.Height()
		+rcChgListView.Height()+rcLogList.Height()+60));
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
		m_wndSplitter1.SetRange(rcTop.top+30, rcMiddle.bottom-20);
		CRect rcBottom;
		m_ChangedFileListCtrl.GetWindowRect(rcBottom);
		ScreenToClient(rcBottom);
		m_wndSplitter2.SetRange(rcMiddle.top+30, rcBottom.bottom-20);
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

		m_tooltips.Pop();
		int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		if (selection != 0)
		{

			if (selection == LOGFILTER_REGEX)
			{
				m_bFilterWithRegex = !m_bFilterWithRegex;
				CRegDWORD b = CRegDWORD(_T("Software\\TortoiseSVN\\UseRegexFilter"), TRUE);
				b = m_bFilterWithRegex;
				CheckRegexpTooltip();
			}
			else
			{
				m_nSelectedFilter = selection;
				SetFilterCueText();
			}
			SetTimer(LOGFILTER_TIMER, 1000, NULL);
		}
	}
	return 0L;
}

LRESULT CLogDlg::OnClickedCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	KillTimer(LOGFILTER_TIMER);

	m_sFilterText.Empty();
	UpdateData(FALSE);
	theApp.DoWaitCursor(1);
	CStoreSelection storeselection(this);
	FillLogMessageCtrl(false);
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	m_arShownList.RemoveAll();

	// reset the time filter too
	m_timFrom = (__time64_t(m_tFrom));
	m_timTo = (__time64_t(m_tTo));
	m_DateFrom.SetTime(&m_timFrom);
	m_DateTo.SetTime(&m_timTo);
	m_DateFrom.SetRange(&m_timFrom, &m_timTo);
	m_DateTo.SetRange(&m_timFrom, &m_timTo);

	for (DWORD i=0; i<m_logEntries.size(); ++i)
	{
		m_arShownList.Add(m_logEntries[i]);
	}
	InterlockedExchange(&m_bNoDispUpdates, FALSE);
	m_LogList.DeleteAllItems();
	m_LogList.SetItemCountEx(ShownCountWithStopped());
	m_LogList.RedrawItems(0, ShownCountWithStopped());
	m_LogList.SetRedraw(false);
	ResizeAllListCtrlCols();
	m_LogList.SetRedraw(true);
	theApp.DoWaitCursor(-1);
	GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
	UpdateLogInfoLabel();
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

	// Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	// Do the list need text information?
	if (!(pItem->mask & LVIF_TEXT))
		return;

	// By default, clear text buffer.
	lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);

	bool bOutOfRange = pItem->iItem >= ShownCountWithStopped();
	
	*pResult = 0;
	if (m_bNoDispUpdates || m_bThreadRunning || bOutOfRange)
		return;

	// Which item number?
	int itemid = pItem->iItem;
	PLOGENTRYDATA pLogEntry = NULL;
	if (itemid < m_arShownList.GetCount())
		pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(pItem->iItem));
    
	// Which column?
	switch (pItem->iSubItem)
	{
	case 0:	//revision
		if (pLogEntry)
		{
			_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld"), pLogEntry->Rev);
			// to make the child entries indented, add spaces
			size_t len = _tcslen(pItem->pszText);
			TCHAR * pBuf = pItem->pszText + len;
			DWORD nSpaces = m_maxChild-pLogEntry->childStackDepth;
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
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->sAuthor, pItem->cchTextMax);
		break;
	case 3: //date
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->sDate, pItem->cchTextMax);
		break;
	case 4: //message or bug id
		if (m_bShowBugtraqColumn)
		{
			if (pLogEntry)
				lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->sBugIDs, pItem->cchTextMax);
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
			if (pLogEntry->sShortMessage.GetLength() >= pItem->cchTextMax && pItem->cchTextMax > dots_len)
			{
				lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->sShortMessage, pItem->cchTextMax - dots_len);
				lstrcpyn(pItem->pszText + pItem->cchTextMax - dots_len - 1, _T("..."), dots_len + 1);
			}
			else
				lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->sShortMessage, pItem->cchTextMax);
		}
		else if ((itemid == m_arShownList.GetCount()) && m_bStrict && m_bStrictStopped)
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

void CLogDlg::OnLvnGetdispinfoChangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	//Create a pointer to the item
	LV_ITEM* pItem= &(pDispInfo)->item;

	*pResult = 0;
	if ((m_bNoDispUpdates)||(m_bThreadRunning))
	{
		if (pItem->mask & LVIF_TEXT)
			lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		return;
	}
	if ((m_currentChangedArray!=NULL)&&(pItem->iItem >= m_currentChangedArray->GetCount()))
	{
		if (pItem->mask & LVIF_TEXT)
			lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		return;
	}
	if ((m_currentChangedArray==NULL)&&(pItem->iItem >= m_currentChangedPathList.GetCount()))
	{
		if (pItem->mask & LVIF_TEXT)
			lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		return;
	}
	LogChangedPath * lcpath = NULL;
	if (m_currentChangedArray)
		lcpath = m_currentChangedArray->GetAt(pItem->iItem);
	//Does the list need text information?
	if (pItem->mask & LVIF_TEXT)
	{
		//Which column?
		switch (pItem->iSubItem)
		{
		case 0:	//Action
			if (lcpath)
				lstrcpyn(pItem->pszText, (LPCTSTR)lcpath->GetAction(), pItem->cchTextMax);
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);				
			break;
		case 1: //path
			if (lcpath)
				lstrcpyn(pItem->pszText, (LPCTSTR)lcpath->sPath, pItem->cchTextMax);
			else
				lstrcpyn(pItem->pszText, (LPCTSTR)m_currentChangedPathList[pItem->iItem].GetSVNPathString(), pItem->cchTextMax);
			break;
		case 2: //copyfrom path
			if (lcpath)
				lstrcpyn(pItem->pszText, (LPCTSTR)lcpath->sCopyFromPath, pItem->cchTextMax);
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 3: //revision
			if ((lcpath==NULL)||(lcpath->sCopyFromPath.IsEmpty()))
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			else
				_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld"), lcpath->lCopyFromRev);
			break;
		}
	}

	*pResult = 0;
}

void CLogDlg::OnEnChangeSearchedit()
{
	UpdateData();
	if (m_sFilterText.IsEmpty())
	{
		CStoreSelection storeselection(this);
		// clear the filter, i.e. make all entries appear
		theApp.DoWaitCursor(1);
		KillTimer(LOGFILTER_TIMER);
		FillLogMessageCtrl(false);
		InterlockedExchange(&m_bNoDispUpdates, TRUE);
		m_arShownList.RemoveAll();
		for (DWORD i=0; i<m_logEntries.size(); ++i)
		{
			if (IsEntryInDateRange(i))
				m_arShownList.Add(m_logEntries[i]);
		}
		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		m_LogList.DeleteAllItems();
		m_LogList.SetItemCountEx(ShownCountWithStopped());
		m_LogList.RedrawItems(0, ShownCountWithStopped());
		m_LogList.SetRedraw(false);
		ResizeAllListCtrlCols();
		m_LogList.SetRedraw(true);
		theApp.DoWaitCursor(-1);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
		DialogEnableWindow(IDC_STATBUTTON, !(((m_bThreadRunning)||(m_arShownList.IsEmpty()))));
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

void CLogDlg::RecalculateShownList(CPtrArray * pShownlist)
{
	pShownlist->RemoveAll();
	tr1::wregex pat;//(_T("Remove"), tr1::regex_constants::icase);
	bool bRegex = false;
	if (m_bFilterWithRegex)
		bRegex = ValidateRegexp(m_sFilterText, pat, false);

	tr1::regex_constants::match_flag_type flags = tr1::regex_constants::match_any;
	CString sRev;
	for (DWORD i=0; i<m_logEntries.size(); ++i)
	{
		if ((bRegex)&&(m_bFilterWithRegex))
		{
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_BUGID))
			{
				ATLTRACE(_T("bugID = \"%s\"\n"), (LPCTSTR)m_logEntries[i]->sBugIDs);
				if (regex_search(wstring((LPCTSTR)m_logEntries[i]->sBugIDs), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_MESSAGES))
			{
				ATLTRACE(_T("messge = \"%s\"\n"), (LPCTSTR)m_logEntries[i]->sMessage);
				if (regex_search(wstring((LPCTSTR)m_logEntries[i]->sMessage), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_PATHS))
			{
				LogChangedPathArray * cpatharray = m_logEntries[i]->pArChangedPaths;

				bool bGoing = true;
				for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount() && bGoing; ++cpPathIndex)
				{
					LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
					if (regex_search(wstring((LPCTSTR)cpath->sCopyFromPath), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					if (regex_search(wstring((LPCTSTR)cpath->sPath), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					if (regex_search(wstring((LPCTSTR)cpath->GetAction()), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
				}
				if (!bGoing)
					continue;
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_AUTHORS))
			{
				if (regex_search(wstring((LPCTSTR)m_logEntries[i]->sAuthor), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_REVS))
			{
				sRev.Format(_T("%ld"), m_logEntries[i]->Rev);
				if (regex_search(wstring((LPCTSTR)sRev), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
		} // if (bRegex)
		else
		{
			CString find = m_sFilterText;
			find.MakeLower();
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_BUGID))
			{
				CString sBugIDs = m_logEntries[i]->sBugIDs;

				sBugIDs = sBugIDs.MakeLower();
				if ((sBugIDs.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_MESSAGES))
			{
				CString msg = m_logEntries[i]->sMessage;

				msg = msg.MakeLower();
				if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_PATHS))
			{
				LogChangedPathArray * cpatharray = m_logEntries[i]->pArChangedPaths;

				bool bGoing = true;
				for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount() && bGoing; ++cpPathIndex)
				{
					LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
					CString path = cpath->sCopyFromPath;
					path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					path = cpath->sPath;
					path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					path = cpath->GetAction();
					path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_AUTHORS))
			{
				CString msg = m_logEntries[i]->sAuthor;
				msg = msg.MakeLower();
				if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_REVS))
			{
				sRev.Format(_T("%ld"), m_logEntries[i]->Rev);
				if ((sRev.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
		} // else (from if (bRegex))	
	} // for (DWORD i=0; i<m_logEntries.size(); ++i) 

}

void CLogDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == LOGFILTER_TIMER)
	{
		if (m_bThreadRunning)
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
			DialogEnableWindow(IDC_STATBUTTON, !(((m_bThreadRunning)||(m_arShownList.IsEmpty()))));
			// do not return here!
			// we also need to run the filter if the filter text is empty:
			// 1. to clear an existing filter
			// 2. to rebuild the m_arShownList after sorting
		}
		theApp.DoWaitCursor(1);
		CStoreSelection storeselection(this);
		KillTimer(LOGFILTER_TIMER);
		FillLogMessageCtrl(false);

		// now start filter the log list
		InterlockedExchange(&m_bNoDispUpdates, TRUE);
		RecalculateShownList(&m_arShownList);
		InterlockedExchange(&m_bNoDispUpdates, FALSE);


		m_LogList.DeleteAllItems();
		m_LogList.SetItemCountEx(ShownCountWithStopped());
		m_LogList.RedrawItems(0, ShownCountWithStopped());
		m_LogList.SetRedraw(false);
		ResizeAllListCtrlCols();
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
		UpdateLogInfoLabel();
	} // if (nIDEvent == LOGFILTER_TIMER)
	DialogEnableWindow(IDC_STATBUTTON, !(((m_bThreadRunning)||(m_arShownList.IsEmpty()))));
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
	__time64_t time = m_logEntries[i]->tmDate;
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
	
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		while (pos)
		{
			int nextpos = m_LogList.GetNextSelectedItem(pos);
			if (nextpos >= m_arShownList.GetCount())
				continue;
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(nextpos));
			LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
			for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
			{
				LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
				if (cpath == NULL)
					continue;
				CTSVNPath path;
				if (!bRelativePaths)
					path.SetFromSVN(m_sRepositoryRoot);
				path.AppendPathString(cpath->sPath);
				if ((!bUseFilter)||
					((m_cHidePaths.GetState() & 0x0003)!=BST_CHECKED)||
					(cpath->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0))
					pathList.AddPath(path);
				
			}
		}
	}
	pathList.RemoveDuplicates();
	return pathList;
}

void CLogDlg::SortByColumn(int nSortColumn, bool bAscending)
{
	switch(nSortColumn)
	{
	case 0: // Revision
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscRevSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescRevSort());
		}
		break;
	case 1: // action
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscActionSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescActionSort());
		}
		break;
	case 2: // Author
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscAuthorSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescAuthorSort());
		}
		break;
	case 3: // Date
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscDateSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescDateSort());
		}
		break;
	case 4: // Message or bug id
		if (m_bShowBugtraqColumn)
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscBugIDSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescBugIDSort());
			break;
		}
		// fall through here
	case 5: // Message
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscMessageSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescMessageSort());
		}
		break;
	default:
		ATLASSERT(0);
		break;
	}
}

void CLogDlg::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if ((m_bThreadRunning)||(m_LogList.HasText()))
		return;		//no sorting while the arrays are filled
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int nColumn = pNMLV->iSubItem;
	m_bAscending = nColumn == m_nSortColumn ? !m_bAscending : TRUE;
	m_nSortColumn = nColumn;
	SortByColumn(m_nSortColumn, m_bAscending);
	SetSortArrow(&m_LogList, m_nSortColumn, !!m_bAscending);
	SortShownListArray();
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

void CLogDlg::SortShownListArray()
{
	// make sure the shown list still matches the filter after sorting.
    OnTimer(LOGFILTER_TIMER);
    // clear the selection states
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	while (pos)
	{
		m_LogList.SetItemState(m_LogList.GetNextSelectedItem(pos), 0, LVIS_SELECTED);
	}    
	m_LogList.SetSelectionMark(-1);
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
	if ((m_bThreadRunning)||(m_LogList.HasText()))
		return;		//no sorting while the arrays are filled
	if (m_currentChangedArray == NULL)
		return;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int nColumn = pNMLV->iSubItem;
	m_bAscendingPathList = nColumn == m_nSortColumnPathList ? !m_bAscendingPathList : TRUE;
	m_nSortColumnPathList = nColumn;
	qsort(m_currentChangedArray->GetData(), m_currentChangedArray->GetSize(), sizeof(LogChangedPath*), (GENERICCOMPAREFN)SortCompare);

	SetSortArrow(&m_ChangedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
	m_ChangedFileListCtrl.Invalidate();
	*pResult = 0;
}

int CLogDlg::m_nSortColumnPathList = 0;
bool CLogDlg::m_bAscendingPathList = false;

int CLogDlg::SortCompare(const void * pElem1, const void * pElem2)
{
	LogChangedPath * cpath1 = *((LogChangedPath**)pElem1);
	LogChangedPath * cpath2 = *((LogChangedPath**)pElem2);

	if (m_bAscendingPathList)
		std::swap (cpath1, cpath2);

	int cmp = 0;
	switch (m_nSortColumnPathList)
	{
	case 0:	// action
			cmp = cpath2->GetAction().Compare(cpath1->GetAction());
			if (cmp)
				return cmp;
			// fall through
	case 1:	// path
			cmp = cpath2->sPath.CompareNoCase(cpath1->sPath);
			if (cmp)
				return cmp;
			// fall through
	case 2:	// copy from path
			cmp = cpath2->sCopyFromPath.Compare(cpath1->sCopyFromPath);
			if (cmp)
				return cmp;
			// fall through
	case 3:	// copy from revision
			return cpath2->lCopyFromRev > cpath1->lCopyFromRev;
	}
	return 0;
}

void CLogDlg::ResizeAllListCtrlCols()
{
	const int nMinimumWidth = ICONITEMBORDER+16*4;
	int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int nItemCount = m_LogList.GetItemCount();
	TCHAR textbuf[MAX_PATH];
	CHeaderCtrl * pHdrCtrl = (CHeaderCtrl*)(m_LogList.GetDlgItem(0));
	if (pHdrCtrl)
	{
		for (int col = 0; col <= maxcol; col++)
		{
			HDITEM hdi = {0};
			hdi.mask = HDI_TEXT;
			hdi.pszText = textbuf;
			hdi.cchTextMax = sizeof(textbuf);
			pHdrCtrl->GetItem(col, &hdi);
			int cx = m_LogList.GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin
			for (int index = 0; index<nItemCount; ++index)
			{
				// get the width of the string and add 14 pixels for the column separator and margins
				int linewidth = m_LogList.GetStringWidth(m_LogList.GetItemText(index, col)) + 14;
				if (index < m_arShownList.GetCount())
				{
					PLOGENTRYDATA pCurLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(index));
					if ((pCurLogEntry)&&(pCurLogEntry->Rev == m_wcRev))
					{
						// set the bold font and ask for the string width again
						m_LogList.SendMessage(WM_SETFONT, (WPARAM)m_boldFont, NULL);
						linewidth = m_LogList.GetStringWidth(m_LogList.GetItemText(index, col)) + 14;
						// restore the system font
						m_LogList.SendMessage(WM_SETFONT, NULL, NULL);
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
	if ((pFindInfo->iStart < 0)||(pFindInfo->iStart >= m_arShownList.GetCount()))
		return;
	if (pFindInfo->lvfi.psz == 0)
		return;
		
	CString sCmp = pFindInfo->lvfi.psz;
	CString sRev;	
	for (int i=pFindInfo->iStart; i<m_arShownList.GetCount(); ++i)
	{
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
		sRev.Format(_T("%ld"), pLogEntry->Rev);
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
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
			sRev.Format(_T("%ld"), pLogEntry->Rev);
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
	if (m_arShownList.GetCount())
	{
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(0));
		rev1 = pLogEntry->Rev;
		pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_arShownList.GetCount()-1));
		rev2 = pLogEntry->Rev;
		selectedrevs = m_LogList.GetSelectedCount();
	}
	CString sTemp;
	sTemp.Format(IDS_LOG_LOGINFOSTRING, m_arShownList.GetCount(), rev2, rev1, selectedrevs);
	m_sLogInfo = sTemp;
	UpdateData(FALSE);
}

void CLogDlg::ShowContextMenuForRevisions(CWnd* /*pWnd*/, CPoint point)
{
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex < 0)
		return;	// nothing selected, nothing to do with a context menu

	// if the user selected the info text telling about not all revisions shown due to
	// the "stop on copy/rename" option, we also don't show the context menu
	if ((m_bStrictStopped)&&(selIndex == m_arShownList.GetCount()))
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
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	int indexNext = m_LogList.GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;
	PLOGENTRYDATA pSelLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(indexNext));
	SVNRev revSelected = pSelLogEntry->Rev;
	SVNRev revPrevious = svn_revnum_t(revSelected)-1;
	if ((pSelLogEntry->pArChangedPaths)&&(pSelLogEntry->pArChangedPaths->GetCount() <= 2))
	{
		for (int i=0; i<pSelLogEntry->pArChangedPaths->GetCount(); ++i)
		{
			LogChangedPath * changedpath = (LogChangedPath *)pSelLogEntry->pArChangedPaths->GetAt(i);
			if (changedpath->lCopyFromRev)
				revPrevious = changedpath->lCopyFromRev;
		}
	}
	SVNRev revSelected2;
	if (pos)
	{
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
		revSelected2 = pLogEntry->Rev;
	}
	bool bAllFromTheSameAuthor = true;
	CString firstAuthor;
	CLogDataVector selEntries;
	SVNRev revLowest, revHighest;
	SVNRevRangeArray revisionRanges;
	{
		POSITION pos2 = m_LogList.GetFirstSelectedItemPosition();
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos2)));
		revisionRanges.AddRevision(pLogEntry->Rev);
		selEntries.push_back(pLogEntry);
		firstAuthor = pLogEntry->sAuthor;
		revLowest = pLogEntry->Rev;
		revHighest = pLogEntry->Rev;
		while (pos2)
		{
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos2)));
			revisionRanges.AddRevision(pLogEntry->Rev);
			selEntries.push_back(pLogEntry);
			if (firstAuthor.Compare(pLogEntry->sAuthor))
				bAllFromTheSameAuthor = false;
			revLowest = (svn_revnum_t(pLogEntry->Rev) > svn_revnum_t(revLowest) ? revLowest : pLogEntry->Rev);
			revHighest = (svn_revnum_t(pLogEntry->Rev) < svn_revnum_t(revHighest) ? revHighest : pLogEntry->Rev);
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
			popup.AppendMenuIcon(ID_COPY, IDS_LOG_POPUP_COPY);
			if (m_hasWC)
				popup.AppendMenuIcon(ID_UPDATE, IDS_LOG_POPUP_UPDATE, IDI_UPDATE);
			if (m_hasWC)
				popup.AppendMenuIcon(ID_REVERTTOREV, IDS_LOG_POPUP_REVERTTOREV, IDI_REVERT);
			if (m_hasWC)
				popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);
			if (m_hasWC)
				popup.AppendMenuIcon(ID_MERGEREV, IDS_LOG_POPUP_MERGEREV, IDI_MERGE);
			popup.AppendMenuIcon(ID_CHECKOUT, IDS_MENUCHECKOUT, IDI_CHECKOUT);
			popup.AppendMenuIcon(ID_EXPORT, IDS_MENUEXPORT, IDI_EXPORT);
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
			popup.AppendMenuIcon(ID_COPYCLIPBOARD, IDS_LOG_POPUP_COPYTOCLIPBOARD);
		}
		popup.AppendMenuIcon(ID_FINDENTRY, IDS_LOG_POPUP_FIND);

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
				//user clicked on the menu item "compare revisions"
				if (PromptShown())
				{
					SVNDiff diff(this, m_hWnd, true);
					diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTSVNPath(pathURL), r2, CTSVNPath(pathURL), r1);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), r2, CTSVNPath(pathURL), r1, SVNRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			}
			break;
		case ID_COMPAREWITHPREVIOUS:
			{
				if (PromptShown())
				{
					SVNDiff diff(this, m_hWnd, true);
					diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, SVNRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
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
				if (PromptShown())
				{
					SVNDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTSVNPath(pathURL), revSelected2, CTSVNPath(pathURL), revSelected, SVNRev(), false, true);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), revSelected2, CTSVNPath(pathURL), revSelected, SVNRev(), m_LogRevision, false, false, true);
			}
			break;
		case ID_BLAMEWITHPREVIOUS:
			{
				//user clicked on the menu item "Compare and Blame with previous revision"
				if (PromptShown())
				{
					SVNDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, SVNRev(), false, true);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, SVNRev(), m_LogRevision, false, false, true);
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
					sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LPCTSTR)revSelected.ToString());
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
				sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LPCTSTR)revSelected.ToString());
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
							if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(m_path.GetFileOrDirectoryName()),sParams))
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
				CString url = _T("tsvn:")+pathURL;
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
				url = GetAbsoluteUrlFromRelativeUrl(url);
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
				url = GetAbsoluteUrlFromRelativeUrl(url);
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
	int selIndex = m_ChangedFileListCtrl.GetSelectionMark();
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		m_ChangedFileListCtrl.GetItemRect(selIndex, &rect, LVIR_LABEL);
		m_ChangedFileListCtrl.ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	if (selIndex < 0)
		return;
	int s = m_LogList.GetSelectionMark();
	if (s < 0)
		return;
	std::vector<CString> changedpaths;
	std::vector<LogChangedPath*> changedlogpaths;
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos == NULL)
		return;	// nothing is selected, get out of here

	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
	svn_revnum_t rev1 = pLogEntry->Rev;
	svn_revnum_t rev2 = rev1;
	bool bOneRev = true;
	if (pos)
	{
		while (pos)
		{
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			if (pLogEntry)
			{
				rev1 = max(rev1,(svn_revnum_t)pLogEntry->Rev);
				rev2 = min(rev2,(svn_revnum_t)pLogEntry->Rev);
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
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
			LogChangedPath * changedlogpath = pLogEntry->pArChangedPaths->GetAt(nItem);

			if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
			{
				if ((changedlogpath)&&(!changedlogpath->sCopyFromPath.IsEmpty()))
					rev2 = changedlogpath->lCopyFromRev;
				else
				{
					// if the path was modified but the parent path was 'added with history'
					// then we have to use the copy from revision of the parent path
					CTSVNPath cpath = CTSVNPath(changedlogpath->sPath);
					for (int flist = 0; flist < pLogEntry->pArChangedPaths->GetCount(); ++flist)
					{
						CTSVNPath p = CTSVNPath(pLogEntry->pArChangedPaths->GetAt(flist)->sPath);
						if (p.IsAncestorOf(cpath))
						{
							if (!pLogEntry->pArChangedPaths->GetAt(flist)->sCopyFromPath.IsEmpty())
								rev2 = pLogEntry->pArChangedPaths->GetAt(flist)->lCopyFromRev;
						}
					}
				}
			}
			if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
			{
				// some items are hidden! So find out which item the user really clicked on
				INT_PTR selRealIndex = -1;
				for (INT_PTR hiddenindex=0; hiddenindex<pLogEntry->pArChangedPaths->GetCount(); ++hiddenindex)
				{
					if (pLogEntry->pArChangedPaths->GetAt(hiddenindex)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
						selRealIndex++;
					if (selRealIndex == nItem)
					{
						selIndex = hiddenindex;
						changedlogpath = pLogEntry->pArChangedPaths->GetAt(selIndex);
						break;
					}
				}
			}
			if (changedlogpath)
			{
				changedpaths.push_back(changedlogpath->sPath);
				changedlogpaths.push_back(changedlogpath);
			}
		}
	}

	//entry is selected, now show the popup menu
	CIconMenu popup;
	if (popup.CreatePopupMenu())
	{
		bool bEntryAdded = false;
		if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
		{
			if ((!bOneRev)||(IsDiffPossible(changedlogpaths[0], rev1)))
			{
				popup.AppendMenuIcon(ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
				popup.AppendMenuIcon(ID_BLAMEDIFF, IDS_LOG_POPUP_BLAMEDIFF, IDI_BLAME);
				popup.SetDefaultItem(ID_DIFF, FALSE);
				popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
				bEntryAdded = true;
			}
			if (rev2 == rev1-1)
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
		else if (changedlogpaths.size())
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
				DoDiffFromLog(selIndex, rev1, rev2, false, false);
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
				if (changedlogpaths[0]->action == LOGACTIONS_DELETED)
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
					for (std::vector<LogChangedPath*>::iterator it = changedlogpaths.begin(); it!= changedlogpaths.end(); ++it)
					{
						SVNRev getrev = ((*it)->action == LOGACTIONS_DELETED) ? rev2 : rev1;

						CString sInfoLine;
						sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)filepath, (LPCTSTR)getrev.ToString());
						progDlg.SetLine(1, sInfoLine, true);
						SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);

						CTSVNPath tempfile = TargetPath;
						if (changedpaths.size() > 1)
						{
							// if multiple items are selected, then the TargetPath
							// points to a folder and we have to append the filename
							// to save to to that folder.
							CString sName = (*it)->sPath;
							int slashpos = sName.ReverseFind('/');
							if (slashpos >= 0)
								sName = sName.Mid(slashpos);
							tempfile.AppendPathString(sName);
							// one problem here:
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
						filepath = sRoot + (*it)->sPath;
						if (!Cat(CTSVNPath(filepath), getrev, getrev, tempfile))
						{
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
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
				SVNRev getrev = pLogEntry->pArChangedPaths->GetAt(selIndex)->action == LOGACTIONS_DELETED ? rev2 : rev1;
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
							if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(filepath),sParams))
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
				if (changedlogpaths[0]->action == LOGACTIONS_DELETED)
				{
					// if the item got deleted in this revision,
					// fetch the log from the previous revision where it
					// still existed.
					logrev--;
				}
				CString sCmd;
				sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /startrev:%ld"), (LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)filepath, logrev);
				if (bMergeLog)
					sCmd += _T(" /merge");
				CAppUtils::LaunchApplication(sCmd, NULL, false);
				EnableOKButton();
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_VIEWPATHREV:
			{
				PLOGENTRYDATA pLogEntry2 = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
				SVNRev rev = pLogEntry2->Rev;
				CString relurl = changedpaths[0];
				CString url = m_ProjectProperties.sWebViewerPathRev;
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

CString CLogDlg::GetAbsoluteUrlFromRelativeUrl(const CString& url)
{
	// is the URL a relative one?
	if (url.Left(2).Compare(_T("^/")) == 0)
	{
		// URL is relative to the repository root
		CString url1 = m_sRepositoryRoot + url.Mid(1);
		TCHAR buf[INTERNET_MAX_URL_LENGTH];
		DWORD len = url.GetLength();
		if (UrlCanonicalize((LPCTSTR)url1, buf, &len, 0) == S_OK)
			return CString(buf, len);
		return url1;
	}
	else if (url[0] == '/')
	{
		// URL is relative to the server's hostname
		CString sHost;
		// find the server's hostname
		int schemepos = m_sRepositoryRoot.Find(_T("//"));
		if (schemepos >= 0)
		{
			sHost = m_sRepositoryRoot.Left(m_sRepositoryRoot.Find('/', schemepos+3));
			CString url1 = sHost + url;
			TCHAR buf[INTERNET_MAX_URL_LENGTH];
			DWORD len = url.GetLength();
			if (UrlCanonicalize((LPCTSTR)url, buf, &len, 0) == S_OK)
				return CString(buf, len);
			return url1;
		}
	}
	return url;
}
