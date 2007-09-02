// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "MergeDlg.h"
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
#include "RevisionRangeDlg.h"
#include "BrowseFolder.h"
#include "BlameDlg.h"
#include "Blame.h"
#include "SVNHelpers.h"
#include "LogDlgHelper.h"

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
	ID_GETMERGELOGS
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
	, m_bIncludeMerges(FALSE)
	, m_hAccel(NULL)
{
	m_bFilterWithRegex = !!CRegDWORD(_T("Software\\TortoiseSVN\\UseRegexFilter"), TRUE);
}

CLogDlg::~CLogDlg()
{
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
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
	ON_NOTIFY(LVN_KEYDOWN, IDC_LOGLIST, OnLvnKeydownLoglist)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage) 
	ON_BN_CLICKED(IDC_GETALL, OnBnClickedGetall)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGMSG, OnNMDblclkChangedFileList)
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
	if ((!m_ProjectProperties.sUrl.IsEmpty())||(!m_ProjectProperties.sBugIDRe.IsEmpty()))
		m_bShowBugtraqColumn = true;

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
		temp.LoadString(IDS_LOG_BUGIDS);
		m_LogList.InsertColumn(4, temp);
	}
	temp.LoadString(IDS_LOG_MESSAGE);
	m_LogList.InsertColumn(m_bShowBugtraqColumn ? 5 : 4, temp);
	m_LogList.SetRedraw(false);
	ResizeAllListCtrlCols(m_LogList);
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
	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);
	if(m_path.IsDirectory())
		SetWindowText(m_sTitle + _T(" - ") + m_path.GetWinPathString());
	else
		SetWindowText(m_sTitle + _T(" - ") + m_path.GetFilename());

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_SEARCHEDIT, IDS_LOG_FILTER_REGEX_TT);

	SetSplitterRange();
	
	// the filter control has a 'cancel' button (the red 'X'), we need to load its bitmap
	m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED);
	m_cFilter.SetInfoIcon(IDI_LOGFILTER);
	m_cFilter.SetValidator(this);
	
	AdjustControlSize(IDC_HIDEPATHS);
	AdjustControlSize(IDC_CHECK_STOPONCOPY);
	AdjustControlSize(IDC_INCLUDEMERGE);

	// resizable stuff
	AddAnchor(IDC_FROMLABEL, TOP_LEFT);
	AddAnchor(IDC_DATEFROM, TOP_LEFT);
	AddAnchor(IDC_TOLABEL, TOP_LEFT);
	AddAnchor(IDC_DATETO, TOP_LEFT);

	SetFilterCueText();
	AddAnchor(IDC_SEARCHEDIT, TOP_LEFT, TOP_RIGHT);
	
	AddAnchor(IDC_LOGLIST, TOP_LEFT, ANCHOR(100, 40));
	AddAnchor(IDC_SPLITTERTOP, ANCHOR(0, 40), ANCHOR(100, 40));
	AddAnchor(IDC_MSGVIEW, ANCHOR(0, 40), ANCHOR(100, 90));
	AddAnchor(IDC_SPLITTERBOTTOM, ANCHOR(0, 90), ANCHOR(100, 90));
	AddAnchor(IDC_LOGMSG, ANCHOR(0, 90), BOTTOM_RIGHT);

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
	if (yPos1)
	{
		RECT rectSplitter;
		m_wndSplitter1.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos1 - rectSplitter.top;
		m_wndSplitter1.SetWindowPos(NULL, 0, yPos1, 0, 0, SWP_NOSIZE);
		DoSizeV1(delta);
	}
	if (yPos2)
	{
		RECT rectSplitter;
		m_wndSplitter2.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos2 - rectSplitter.top;
		m_wndSplitter2.SetWindowPos(NULL, 0, yPos2, 0, 0, SWP_NOSIZE);
		DoSizeV2(delta);
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
		GetDlgItem(IDOK)->GetWindowText(temp);
		SetDlgItemText(IDCANCEL, temp);
		GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
	}
	
	GetDlgItem(IDC_LOGLIST)->SetFocus();

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
	return FALSE;
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
		// with the corresponding log message, and also fill the change files
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
		m_endrev = 1;
		m_startrev = m_LogRevision;
		if (m_bStrict)
			m_bShowedAll = true;
		break;
	case 1: // show range
		{
			// ask for a revision range
			CRevisionRangeDlg dlg;
			dlg.SetStartRevision(m_startrev);
			dlg.SetEndRevision(m_endrev);
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
	Refresh();
}

void CLogDlg::Refresh()
{
	// refreshing means re-downloading the already shown log messages
	UpdateData();

	if ((m_limit == 0)||(m_bStrict)||(int(m_logEntries.size()-1) > m_limit))
	{
		m_limit = 0;
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
	// we have to fetch the next hundred log messages.
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
	m_endrev = 1;
	m_bCancelled = FALSE;
	m_limit = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
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

void CLogDlg::OnCancel()
{
	// canceling means stopping the working thread if it's still running.
	// we do this by using the Subversion cancel callback.
	// But canceling can also mean just to close the dialog, depending on the
	// text shown on the cancel button (it could simply read "OK").
	CString temp, temp2;
	GetDlgItem(IDOK)->GetWindowText(temp);
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

BOOL CLogDlg::Log(svn_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies, DWORD actions)
{
	return Log(rev, author, date, message, cpaths, time, filechanges, copies, actions, 0);
}
BOOL CLogDlg::Log(svn_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies, DWORD actions, DWORD children)
{
	// this is the callback function which receives the data for every revision we ask the log for
	// we store this information here one by one.
	int found = 0;
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
	// so it has a fixed width. If the log message is longer than
	// this predefined fixed with, add "..." as an indication.
	CString sShortMessage = message;
	// Remove newlines and tabs 'cause those are not shown nicely in the listcontrol
	sShortMessage.Replace(_T("\r"), _T(""));
	sShortMessage.Replace(_T("\t"), _T(" "));
	
	found = sShortMessage.Find(_T("\n\n"));
	// to avoid too short 'short' messages 
	// (e.g. if the message looks something like "Bugfix:\n\n*done this\n*done that")
	// only use the emtpy newline as a separator if it comes
	// after at least 15 chars.
	if (found >= 15)
	{
		sShortMessage = sShortMessage.Left(found);
	}
	sShortMessage.Replace('\n', ' ');
	
	
	PLOGENTRYDATA pLogItem = new LOGENTRYDATA;
	pLogItem->bCopies = !!copies;
	pLogItem->tmDate = ttime;
	pLogItem->sAuthor = author;
	pLogItem->sDate = date;
	pLogItem->sShortMessage = sShortMessage;
	pLogItem->dwFileChanges = filechanges;
	pLogItem->actions = actions;
	pLogItem->children = children;
	pLogItem->isChild = (m_childCounter > 0);
	pLogItem->sBugIDs = m_ProjectProperties.FindBugID(message).Trim();

	if (m_childCounter > 0)
		m_childCounter--;
	m_childCounter += children;
	
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
        pLogItem->pArChangedPaths = cpaths;
        pLogItem->Rev = rev;
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

	//disable the "Get All" button while we're receiving
	//log messages.
	DialogEnableWindow(IDC_GETALL, FALSE);
	DialogEnableWindow(IDC_NEXTHUNDRED, FALSE);
	DialogEnableWindow(IDC_CHECK_STOPONCOPY, FALSE);
	DialogEnableWindow(IDC_INCLUDEMERGE, FALSE);
	DialogEnableWindow(IDC_STATBUTTON, FALSE);
	DialogEnableWindow(IDC_REFRESH, FALSE);
	
	// change the text of the close button to "Cancel" since now the thread
	// is running, and simply closing the dialog doesn't work.
	CString temp;
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
	GetRootAndHead(m_path, rootpath, r);	
	m_sRepositoryRoot = rootpath.GetSVNPathString();
	CString sUrl = m_path.GetSVNPathString();
	// if the log dialog is started from a working copy, we need to turn that
	// local path into an url here
	if (!m_path.IsUrl())
	{
		sUrl = GetURLFromPath(m_path);

		// The URL is escaped because SVN::logReceiver
		// returns the path in a native format
		sUrl = CPathUtils::PathUnescape(sUrl);
	}
	m_sRelativeRoot = sUrl.Mid(CPathUtils::PathUnescape(m_sRepositoryRoot).GetLength());
	
	if (!m_mergePath.IsEmpty() && m_mergedRevs.empty())
	{
		// in case we got a merge path set, retrieve the merge info
		// of that path and check whether one of the merge URLs
		// match the URL we show the log for.
		SVNPool localpool(pool);
		svn_error_clear(Err);
		apr_hash_t * mergeinfo = NULL;
		if (svn_client_get_mergeinfo(&mergeinfo, m_mergePath.GetSVNApiPath(localpool), SVNRev(SVNRev::REV_WC), m_pctx, localpool) == NULL)
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
					if (m_sRelativeRoot.Compare(CUnicodeUtils::GetUnicode((char*)key)) == 0)
					{
						apr_array_header_t * arr = (apr_array_header_t*)val;
						if (val)
						{
							for (long i=0; i<arr->nelts; ++i)
							{
								svn_merge_range_t * pRange = APR_ARRAY_IDX(arr, i, svn_merge_range_t*);
								if (pRange)
								{
									for (svn_revnum_t r=pRange->start; r<=pRange->end; ++r)
									{
										m_mergedRevs.insert(r);
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
	
	// TODO:
	// Subversion currently doesn't like WC or BASE as a peg revision for logs,
	// so we just work around that problem by setting the peg revision to invalid
	// here.
	// Once Subversion can work with WC and BASE peg revisions, we have to
	// remove this workaround again.
	if (m_pegrev.IsBase() || m_pegrev.IsWorking())
		m_pegrev = SVNRev();
	if (!m_pegrev.IsValid())
		m_pegrev = m_startrev;
	size_t startcount = m_logEntries.size();
	m_lowestRev = -1;
	m_bStrictStopped = false;
	if (m_bIncludeMerges)
	{
		BOOL bLogRes = GetLogWithMergeInfo(CTSVNPathList(m_path), m_pegrev, m_startrev, m_endrev, m_limit, m_bStrict);
		if ((!bLogRes)&&(!m_path.IsUrl()))
		{
			// try again with REV_WC as the start revision, just in case the path doesn't
			// exist anymore in HEAD
			bLogRes = GetLogWithMergeInfo(CTSVNPathList(m_path), SVNRev(), SVNRev::REV_WC, m_endrev, m_limit, m_bStrict);
		}
		if (!bLogRes)
			CMessageBox::Show(m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
	}
	else
	{
		BOOL bLogRes = ReceiveLog(CTSVNPathList(m_path), m_pegrev, m_startrev, m_endrev, m_limit, m_bStrict);
		if ((!bLogRes)&&(!m_path.IsUrl()))
		{
			// try again with REV_WC as the start revision, just in case the path doesn't
			// exist anymore in HEAD
			bLogRes = ReceiveLog(CTSVNPathList(m_path), SVNRev(), SVNRev::REV_WC, m_endrev, m_limit, m_bStrict);
		}
		if (!bLogRes)
			CMessageBox::Show(m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
	}
	if (m_bStrict && (m_lowestRev>1) && ((m_limit>0) ? ((startcount + m_limit)>m_logEntries.size()) : (m_endrev<m_lowestRev)))
		m_bStrictStopped = true;
	m_LogList.SetItemCountEx(m_bStrictStopped ? m_arShownList.GetCount()+1 : m_arShownList.GetCount());

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

	GetDlgItem(IDC_PROGRESS)->ShowWindow(FALSE);
	m_bCancelled = true;
	InterlockedExchange(&m_bThreadRunning, FALSE);
	m_LogList.RedrawItems(0, m_arShownList.GetCount());
	m_LogList.SetRedraw(false);
	ResizeAllListCtrlCols(m_LogList);
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
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	// make sure the filter is applied (if any) now, after we refreshed/fetched
	// the log messages
	PostMessage(WM_TIMER, LOGFILTER_TIMER);
	return 0;
}

void CLogDlg::OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	if (pLVKeyDow->wVKey == 'C')
	{
		if (GetKeyState(VK_CONTROL)&0x8000)
		{
			//Ctrl-C -> copy to clipboard
			CopySelectionToClipBoard();
		}
	}
	*pResult = 0;
}

void CLogDlg::CopySelectionToClipBoard()
{
	CStringA sClipdata;
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
					sCopyFrom.Format(_T("(%s: %s, %s, %ld\r\n"), CString(MAKEINTRESOURCE(IDS_LOG_COPYFROM)), 
						cpath->sCopyFromPath, 
						CString(MAKEINTRESOURCE(IDS_LOG_REVISION)), 
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
			sClipdata +=  CStringA(sLogCopyText);
		}
		CStringUtils::WriteAsciiStringToClipboard(sClipdata);
	}
}

BOOL CLogDlg::DiffPossible(LogChangedPath * changedpath, svn_revnum_t rev)
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
	if (pWnd == &m_LogList)
	{
		ShowContextMenuForRevisions(pWnd, point);
	}
	if (pWnd == &m_ChangedFileListCtrl)
	{
		ShowContextMenuForChangedpaths(pWnd, point);
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
		bool bRegex = false;
		rpattern pat;
		try
		{
			pat.init( (LPCTSTR)FindText, MULTILINE | (bMatchCase ? NOFLAGS : NOCASE) );
			bRegex = true;
		}
		catch (bad_alloc) {}
		catch (bad_regexpr) {}

		int i;
		for (i = this->m_nSearchIndex; i<m_arShownList.GetCount()&&!bFound; i++)
		{
			if (bRegex)
			{
				match_results results;
				match_results::backref_type br;
				PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));

				br = pat.match( restring((LPCTSTR)pLogEntry->sMessage), results );
				if (br.matched)
				{
					bFound = true;
					break;
				}
				LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
				for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
				{
					LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
					br = pat.match( restring ((LPCTSTR)cpath->sCopyFromPath), results);
					if (br.matched)
					{
						bFound = true;
						--i;
						break;
					}
					br = pat.match( restring ((LPCTSTR)cpath->sPath), results);
					if (br.matched)
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

	CString temp;
	CString buttontext;
	GetDlgItem(IDOK)->GetWindowText(buttontext);
	temp.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(buttontext) != 0)
		__super::OnOK();	// only exit if the button text matches, and that will match only if the thread isn't running anymore
	m_bCancelled = TRUE;
	if (m_pNotifyWindow)
	{
		int selIndex = m_LogList.GetSelectionMark();
		if (selIndex >= 0)
		{	
		    PLOGENTRYDATA pLogEntry = NULL;
			POSITION pos = m_LogList.GetFirstSelectedItemPosition();
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			svn_revnum_t lowerRev = pLogEntry->Rev;
			svn_revnum_t higherRev = lowerRev;
			while (pos)
			{
			    pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
				svn_revnum_t rev = pLogEntry->Rev;
				if (lowerRev > rev)
					lowerRev = rev;
				if (higherRev < rev)
					higherRev = rev;
			}
			BOOL bSentMessage = FALSE;
			if (m_LogList.GetSelectedCount() == 1)
			{
				// if only one revision is selected, check if the path/url with which the dialog was started
				// was directly affected in that revision. If it was, then check if our path was copied from somewhere.
				// if it was copied, use the copyfrom revision as lowerRev
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
	// a doubleclick on an entry in the changed-files list has happened
	*pResult = 0;
	if (m_bThreadRunning)
		return;
	UpdateLogInfoLabel();
	int selIndex = m_ChangedFileListCtrl.GetSelectionMark();
	if (selIndex < 0)
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
		// now we have both revisions selected in the log list, so we can do a diff of the doubleclicked
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
			int selRealIndex = -1;
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

		if (DiffPossible(changedpath, rev1))
		{
			// diffs with renamed files are possible
			if ((changedpath)&&(!changedpath->sCopyFromPath.IsEmpty()))
				rev2 = changedpath->lCopyFromRev;
			else
			{
				// if the path was modified but the parent path was 'added with history'
				// then we have to use the 'copyfrom' revision of the parent path
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
		else if (changedpath->action == LOGACTIONS_DELETED)
			// deleted files must be opened from the revision before the deletion
			Open(false,changedpath->sPath,rev1-1);
		else
			// added files must be opened with the actual revision
			Open(false,changedpath->sPath,rev1);
	}
}

void CLogDlg::DoDiffFromLog(int selIndex, svn_revnum_t rev1, svn_revnum_t rev2, bool blame, bool unified)
{
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	//get the filename
	CString filepath;
	if (SVN::PathIsURL(m_path.GetSVNPathString()))
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
			temp.Format(IDS_ERR_NOURLOFFILE, filepath);
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
	diff.SetHEADPeg(m_LogRevision);
	if (unified)
		diff.ShowUnifiedDiff(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1);
	else
	{
		if (diff.ShowCompare(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1, SVNRev(), false, blame))
		{
			if (firstfile.Compare(secondfile)==0)
				diff.DiffProps(CTSVNPath(firstfile), rev2, rev1);
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
	if (SVN::PathIsURL(m_path.GetSVNPathString()))
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
			temp.Format(IDS_ERR_NOURLOFFILE, filepath);
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
	sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, filepath, SVNRev(rev).ToString());
	progDlg.SetLine(1, sInfoLine);
	SetAndClearProgressInfo(&progDlg);
	progDlg.ShowModeless(m_hWnd);

	CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, CTSVNPath(filepath), rev);
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
		cmd += tempfile.GetWinPathString();
		CAppUtils::LaunchApplication(cmd, NULL, false);
	}
	EnableOKButton();
	theApp.DoWaitCursor(-1);
	return TRUE;
}

void CLogDlg::EditAuthor(int index)
{
	CString url;
	CString name;
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	if (SVN::PathIsURL(m_path.GetSVNPathString()))
		url = m_path.GetSVNPathString();
	else
	{
		url = GetURLFromPath(m_path);
	}
	name = SVN_PROP_REVISION_AUTHOR;

	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(index));
	CString value = RevPropertyGet(name, url, pLogEntry->Rev);
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
		if (!RevPropertySet(name, dlg.m_sInputText, url, pLogEntry->Rev))
		{
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		else
		{
			pLogEntry->sAuthor = dlg.m_sInputText;
			m_LogList.Invalidate();
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
	if (SVN::PathIsURL(m_path.GetSVNPathString()))
		url = m_path.GetSVNPathString();
	else
	{
		url = GetURLFromPath(m_path);
	}
	name = SVN_PROP_REVISION_LOG;

	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(index));
	CString value = RevPropertyGet(name, url, pLogEntry->Rev);
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
		if (!RevPropertySet(name, dlg.m_sInputText, url, pLogEntry->Rev))
		{
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		else
		{
			// Add as many characters from the log message to the list control
			// so it has a fixed width. If the log message is longer than
			// this predefined fixed with, add "..." as an indication.
			CString sShortMessage = dlg.m_sInputText;
			// Remove newlines 'cause those are not shown nicely in the listcontrol
			sShortMessage.Replace(_T("\r"), _T(""));
			sShortMessage.Replace('\n', ' ');

			int found = sShortMessage.Find(_T("\n\n"));
			if (found >=0)
			{
				sShortMessage = sShortMessage.Left(found);
			}
			sShortMessage.Replace('\n', ' ');

			pLogEntry->sShortMessage = sShortMessage;
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
			CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
			pMsgView->SetWindowText(_T(" "));
			pMsgView->SetWindowText(dlg.m_sInputText);
			m_ProjectProperties.FindBugID(dlg.m_sInputText, pMsgView);
			m_LogList.Invalidate();
		}
	}
	theApp.DoWaitCursor(-1);
	EnableOKButton();
}

BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
	if (m_hAccel)
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
	if (m_bThreadRunning)
		return;
	if (pNMLV->iItem >= 0)
	{
		m_nSearchIndex = pNMLV->iItem;
		if (pNMLV->iSubItem != 0)
			return;
		if ((pNMLV->iItem == m_arShownList.GetCount())&&(m_bStrict)&&(m_bStrictStopped))
			return;
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
		GetDlgItem(IDC_MSGVIEW)->GetWindowText(msg);
		msg.Replace(_T("\r\n"), _T("\n"));
		url = msg.Mid(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax-pEnLink->chrg.cpMin);
		if (!::PathIsURL(url))
		{
			url = m_ProjectProperties.GetBugIDUrl(url);
		}
		if (!url.IsEmpty())
			ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
	}
	*pResult = 0;
}

void CLogDlg::OnBnClickedStatbutton()
{
	if (m_bThreadRunning)
		return;
	if (m_arShownList.IsEmpty())
		return;		// nothing is shown, so no statistics.
	// the statistics dialog expects the log entries to be sorted by revision
	SortByColumn(0, false);
	CPtrArray shownlist;
	RecalculateShownList(&shownlist);
	// create arrays which are aware of the current filter
	CStringArray m_arAuthorsFiltered;
	CDWordArray m_arDatesFiltered;
	CDWordArray m_arFileChangesFiltered;
	for (INT_PTR i=0; i<shownlist.GetCount(); ++i)
	{
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(shownlist.GetAt(i));
		m_arAuthorsFiltered.Add(pLogEntry->sAuthor);
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
					if (data->bCopies)
						crText = m_Colors.GetColor(CColors::Modified);
					if ((data->isChild)||(m_mergedRevs.find(data->Rev) != m_mergedRevs.end()))
						crText = GetSysColor(COLOR_GRAYTEXT);
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
				ZeroMemory(&rItem, sizeof(LVITEM));
				rItem.mask  = LVIF_STATE;
				rItem.iItem = pLVCD->nmcd.dwItemSpec;
				rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
				m_LogList.GetItem(&rItem);

				// Fill the background
				HBRUSH brush;
				if (rItem.state & LVIS_SELECTED)
				{
					if (::GetFocus() == m_LogList.m_hWnd)
						brush = ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
					else
						brush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
				}
				else
					brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
				if (brush == NULL)
					return;

				CRect rect;
				m_LogList.GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);
				::FillRect(pLVCD->nmcd.hdc, &rect, brush);
				::DeleteObject(brush);
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
	AddAnchor(IDC_LOGLIST, TOP_LEFT, ANCHOR(100, 40));
	AddAnchor(IDC_SPLITTERTOP, ANCHOR(0, 40), ANCHOR(100, 40));
	AddAnchor(IDC_MSGVIEW, ANCHOR(0, 40), ANCHOR(100, 90));
	AddAnchor(IDC_SPLITTERBOTTOM, ANCHOR(0, 90), ANCHOR(100, 90));
	AddAnchor(IDC_LOGMSG, ANCHOR(0, 90), BOTTOM_RIGHT);
	ArrangeLayout();
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
	AddAnchor(IDC_LOGLIST, TOP_LEFT, ANCHOR(100, 40));
	AddAnchor(IDC_SPLITTERTOP, ANCHOR(0, 40), ANCHOR(100, 40));
	AddAnchor(IDC_MSGVIEW, ANCHOR(0, 40), ANCHOR(100, 90));
	AddAnchor(IDC_SPLITTERBOTTOM, ANCHOR(0, 90), ANCHOR(100, 90));
	AddAnchor(IDC_LOGMSG, ANCHOR(0, 90), BOTTOM_RIGHT);
	ArrangeLayout();
	SetSplitterRange();
	GetDlgItem(IDC_MSGVIEW)->Invalidate();
	m_ChangedFileListCtrl.Invalidate();
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
		m_wndSplitter1.SetRange(rcTop.top+20, rcMiddle.bottom-20);
		CRect rcBottom;
		m_ChangedFileListCtrl.GetWindowRect(rcBottom);
		ScreenToClient(rcBottom);
		m_wndSplitter2.SetRange(rcMiddle.top+20, rcBottom.bottom-20);
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
		
		popup.AppendMenu(MF_SEPARATOR, NULL);

		temp.LoadString(IDS_LOG_FILTER_REGEX);
		popup.AppendMenu(MF_STRING | MF_ENABLED | (m_bFilterWithRegex ? MF_CHECKED : MF_UNCHECKED), LOGFILTER_REGEX, temp);

		int oldfilter = m_nSelectedFilter;
		m_nSelectedFilter = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		if (m_nSelectedFilter == LOGFILTER_REGEX)
		{
			m_bFilterWithRegex = !m_bFilterWithRegex;
			CRegDWORD b = CRegDWORD(_T("Software\\TortoiseSVN\\UseRegexFilter"), TRUE);
			b = m_bFilterWithRegex;
			m_nSelectedFilter = oldfilter;
		}
		else if (m_nSelectedFilter == 0)
			m_nSelectedFilter = oldfilter;
		SetFilterCueText();
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
	m_LogList.SetItemCountEx(m_bStrictStopped ? m_arShownList.GetCount()+1 : m_arShownList.GetCount());
	m_LogList.RedrawItems(0, m_bStrictStopped ? m_arShownList.GetCount()+1 : m_arShownList.GetCount());
	m_LogList.SetRedraw(false);
	ResizeAllListCtrlCols(m_LogList);
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
	}
	// to make the cue banner text appear more to the right of the edit control
	temp = _T("   ")+temp;
	m_cFilter.SetCueBanner(temp);
	//::SendMessage(GetDlgItem(IDC_SEARCHEDIT)->GetSafeHwnd(), EM_SETCUEBANNER, 0, (LPARAM)(LPCTSTR)temp);
}

void CLogDlg::OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	//Create a pointer to the item
	LV_ITEM* pItem= &(pDispInfo)->item;

	bool bOutOfRange = m_bStrictStopped ? pItem->iItem > m_arShownList.GetCount() : pItem->iItem >= m_arShownList.GetCount();
	if ((m_bNoDispUpdates)||(m_bThreadRunning)||(bOutOfRange))
	{
		if (pItem->mask & LVIF_TEXT)
			lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		*pResult = 0;
		return;
	}

	//Which item number?
	int itemid = pItem->iItem;
	PLOGENTRYDATA pLogEntry = NULL;
	if (itemid < m_arShownList.GetCount())
		pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(pItem->iItem));
    
	//Do the list need text information?
	if (pItem->mask & LVIF_TEXT)
	{
		//Which column?
		switch (pItem->iSubItem)
		{
		case 0:	//revision
			if (itemid < m_arShownList.GetCount())
			{
				if (pLogEntry->isChild)
					_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld"), pLogEntry->Rev);
				else
					_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld  "), pLogEntry->Rev);
			}
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 1: //action
			lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 2: //author
			if (itemid < m_arShownList.GetCount())
				lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->sAuthor, pItem->cchTextMax);
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 3: //date
			if (itemid < m_arShownList.GetCount())
				lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->sDate, pItem->cchTextMax);
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 4: //message or bug id
			if (m_bShowBugtraqColumn)
			{
				if (itemid < m_arShownList.GetCount())
				{
					lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->sBugIDs, pItem->cchTextMax);
				}
				else
					lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
				break;
			}
			// fall through here!
		case 5:
			if (itemid < m_arShownList.GetCount())
			{
				lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->sShortMessage, pItem->cchTextMax);
			}
			else if ((itemid == m_arShownList.GetCount())&&(m_bStrict)&&(m_bStrictStopped))
			{
				CString sTemp;
				sTemp.LoadString(IDS_LOG_STOPONCOPY_HINT);
				lstrcpyn(pItem->pszText, sTemp, pItem->cchTextMax);
			}
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		}
	}
	*pResult = 0;
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
		m_LogList.SetItemCountEx(m_bStrictStopped ? m_arShownList.GetCount()+1 : m_arShownList.GetCount());
		m_LogList.RedrawItems(0, m_bStrictStopped ? m_arShownList.GetCount()+1 : m_arShownList.GetCount());
		m_LogList.SetRedraw(false);
		ResizeAllListCtrlCols(m_LogList);
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

bool CLogDlg::Validate(LPCTSTR string)
{
	if (!m_bFilterWithRegex)
		return true;
	bool bRegex = false;
	rpattern pat;
	try
	{
		pat.init(string, MULTILINE | NOCASE);
		bRegex = true;
	}
	catch (bad_alloc) {}
	catch (bad_regexpr) {}
	return bRegex;
}

void CLogDlg::RecalculateShownList(CPtrArray * pShownlist)
{
	pShownlist->RemoveAll();
	bool bRegex = false;
	rpattern pat;
	try
	{
		pat.init( (LPCTSTR)m_sFilterText, MULTILINE | NOCASE );
		bRegex = true;
	}
	catch (bad_alloc) {}
	catch (bad_regexpr) {}

	CString sRev;
	for (DWORD i=0; i<m_logEntries.size(); ++i)
	{
		if ((bRegex)&&(m_bFilterWithRegex))
		{
			match_results results;
			match_results::backref_type br;
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_MESSAGES))
			{
				br = pat.match( restring ((LPCTSTR)m_logEntries[i]->sMessage), results );
				if ((br.matched)&&(IsEntryInDateRange(i)))
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
					br = pat.match( restring ((LPCTSTR)cpath->sCopyFromPath), results);
					if ((br.matched)&&(IsEntryInDateRange(i)))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					br = pat.match( restring ((LPCTSTR)cpath->sPath), results);
					if ((br.matched)&&(IsEntryInDateRange(i)))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					br = pat.match( restring ((LPCTSTR)cpath->GetAction()), results);
					if ((br.matched)&&(IsEntryInDateRange(i)))
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
				br = pat.match( restring ((LPCTSTR)m_logEntries[i]->sAuthor), results );
				if ((br.matched)&&(IsEntryInDateRange(i)))
				{
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_REVS))
			{
				sRev.Format(_T("%ld"), m_logEntries[i]->Rev);
				br = pat.match( restring ((LPCTSTR)sRev), results );
				if ((br.matched)&&(IsEntryInDateRange(i)))
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
		bool bSetFocusToFilterControl = ((focusWnd != GetDlgItem(IDC_DATEFROM))&&(focusWnd != GetDlgItem(IDC_DATETO)));
		if (m_sFilterText.IsEmpty())
		{
			DialogEnableWindow(IDC_STATBUTTON, !(((m_bThreadRunning)||(m_arShownList.IsEmpty()))));
			// do not return here!
			// we also need to run the filter if the filtertext is empty:
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
		m_LogList.SetItemCountEx(m_bStrictStopped ? m_arShownList.GetCount()+1 : m_arShownList.GetCount());
		m_LogList.RedrawItems(0, m_bStrictStopped ? m_arShownList.GetCount()+1 : m_arShownList.GetCount());
		m_LogList.SetRedraw(false);
		ResizeAllListCtrlCols(m_LogList);
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
	CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 23, 59, 59);
	
	if (time.GetTime() != m_tTo)
	{
		m_tTo = (DWORD)time.GetTime();
		SetTimer(LOGFILTER_TIMER, 10, NULL);
	}
	
	*pResult = 0;
}

void CLogDlg::OnDtnDatetimechangeDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	CTime _time;
	m_DateFrom.GetTime(_time);
	CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 0, 0, 0);
	if (time.GetTime() != m_tFrom)
	{
		m_tFrom = (DWORD)time.GetTime();
		SetTimer(LOGFILTER_TIMER, 10, NULL);
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
	if (m_bThreadRunning)
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
	if (m_bThreadRunning)
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
	case 2:	// copyfrom path
			cmp = cpath2->sCopyFromPath.Compare(cpath1->sCopyFromPath);
			if (cmp)
				return cmp;
			// fall through
	case 3:	// copyfrom revision
			return cpath2->lCopyFromRev > cpath1->lCopyFromRev;
	}
	return 0;
}

void CLogDlg::ResizeAllListCtrlCols(CListCtrl& list)
{
	CAppUtils::ResizeAllListCtrlCols(&list);

	// Adjust columns "Actions" containing icons
	int nWidth = list.GetColumnWidth(1);

	const int nMinimumWidth = ICONITEMBORDER+16*4;
	if ( nWidth<nMinimumWidth )
	{
		list.SetColumnWidth(1,nMinimumWidth);
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
	//  we see immediately after switching to
	//  copy-following)

	m_endrev = 1;

	// now, restart the query

	Refresh();
}

void CLogDlg::OnBnClickedIncludemerge()
{
	m_endrev = 1;

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
	PLOGENTRYDATA pSelLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
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
	SVNRev revLowest, revHighest;
	{
		POSITION pos = m_LogList.GetFirstSelectedItemPosition();
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
		revLowest = pLogEntry->Rev;
		revHighest = pLogEntry->Rev;
		while (pos)
		{
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			revLowest = (svn_revnum_t(pLogEntry->Rev) > svn_revnum_t(revLowest) ? revLowest : pLogEntry->Rev);
			revHighest = (svn_revnum_t(pLogEntry->Rev) < svn_revnum_t(revHighest) ? revHighest : pLogEntry->Rev);
		}
	}



	//entry is selected, now show the popup menu
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		CString temp;
		if (m_LogList.GetSelectedCount() == 1)
		{
			if (!m_path.IsDirectory())
			{
				if (m_hasWC)
				{
					temp.LoadString(IDS_LOG_POPUP_COMPARE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
					temp.LoadString(IDS_LOG_POPUP_BLAMECOMPARE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAMECOMPARE, temp);
				}
				temp.LoadString(IDS_LOG_POPUP_GNUDIFF_CH);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
				temp.LoadString(IDS_LOG_POPUP_COMPAREWITHPREVIOUS);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPAREWITHPREVIOUS, temp);
				popup.AppendMenu(MF_SEPARATOR, NULL);
				temp.LoadString(IDS_LOG_POPUP_SAVE);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
				temp.LoadString(IDS_LOG_POPUP_OPEN);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPEN, temp);
				temp.LoadString(IDS_LOG_POPUP_OPENWITH);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPENWITH, temp);
				temp.LoadString(IDS_LOG_POPUP_BLAME);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAME, temp);
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
			else
			{
				if (m_hasWC)
				{
					temp.LoadString(IDS_LOG_POPUP_COMPARE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
					// TODO:
					// TortoiseMerge could be improved to take a /blame switch
					// and then not 'cat' the files from a unified diff but
					// blame then.
					// But until that's implemented, the context menu entry for
					// this feature is commented out.
					//temp.LoadString(IDS_LOG_POPUP_BLAMECOMPARE);
					//popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAMECOMPARE, temp);
				}
				temp.LoadString(IDS_LOG_POPUP_GNUDIFF_CH);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
				temp.LoadString(IDS_LOG_POPUP_COMPAREWITHPREVIOUS);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPAREWITHPREVIOUS, temp);
				temp.LoadString(IDS_LOG_POPUP_BLAMEWITHPREVIOUS);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAMEWITHPREVIOUS, temp);
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
			if (!m_ProjectProperties.sWebViewerRev.IsEmpty())
			{
				temp.LoadString(IDS_LOG_POPUP_VIEWREV);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_VIEWREV, temp);
			}
			if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
			{
				temp.LoadString(IDS_LOG_POPUP_VIEWPATHREV);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_VIEWPATHREV, temp);
			}
			if ((!m_ProjectProperties.sWebViewerPathRev.IsEmpty())||
				(!m_ProjectProperties.sWebViewerRev.IsEmpty()))
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}

			temp.LoadString(IDS_LOG_BROWSEREPO);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REPOBROWSE, temp);
			temp.LoadString(IDS_LOG_POPUP_COPY);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COPY, temp);
			temp.LoadString(IDS_LOG_POPUP_UPDATE);
			if (m_hasWC)
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UPDATE, temp);
			temp.LoadString(IDS_LOG_POPUP_REVERTTOREV);
			if (m_hasWC)
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTTOREV, temp);					
			temp.LoadString(IDS_LOG_POPUP_REVERTREV);
			if (m_hasWC)
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);					
			temp.LoadString(IDS_MENUCHECKOUT);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_CHECKOUT, temp);
			temp.LoadString(IDS_MENUEXPORT);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EXPORT, temp);
			popup.AppendMenu(MF_SEPARATOR, NULL);
			temp.LoadString(IDS_LOG_POPUP_GETMERGELOGS);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GETMERGELOGS, temp);
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}
		else if (m_LogList.GetSelectedCount() >= 2)
		{
			bool bAddSeparator = false;
			if (m_LogList.GetSelectedCount() == 2)
			{
				temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARETWO, temp);
				temp.LoadString(IDS_LOG_POPUP_BLAMEREVS);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAMETWO, temp);
				temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF2, temp);
				bAddSeparator = true;
			}
			// reverting revisions only works (in one merge!) when the selected
			// revisions are continuous. So check first if that's the case before
			// we show the context menu.
			bool bContinuous = IsSelectionContinuous();
			temp.LoadString(IDS_LOG_POPUP_REVERTREVS);
			if ((m_hasWC)&&(bContinuous))
			{
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);
				bAddSeparator = true;
			}
			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if (m_LogList.GetSelectedCount() == 1)
		{
			temp.LoadString(IDS_LOG_POPUP_EDITAUTHOR);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITAUTHOR, temp);
			temp.LoadString(IDS_LOG_POPUP_EDITLOG);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITLOG, temp);
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}
		if (m_LogList.GetSelectedCount() != 0)
		{
			temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COPYCLIPBOARD, temp);
		}
		temp.LoadString(IDS_LOG_POPUP_FIND);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_FINDENTRY, temp);

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		DialogEnableWindow(IDOK, FALSE);
		SetPromptApp(&theApp);
		theApp.DoWaitCursor(1);
		bool bOpenWith = false;
		switch (cmd)
		{
		case ID_GNUDIFF1:
			{
				SVNDiff diff(this, this->m_hWnd, true);
				diff.SetHEADPeg(m_LogRevision);
				diff.ShowUnifiedDiff(m_path, revPrevious, m_path, revSelected);
			}
			break;
		case ID_GNUDIFF2:
			{
				SVNDiff diff(this, this->m_hWnd, true);
				diff.SetHEADPeg(m_LogRevision);
				diff.ShowUnifiedDiff(m_path, revSelected2, m_path, revSelected);
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
				msg.Format(IDS_LOG_REVERT_CONFIRM, m_path.GetWinPathString());
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					CSVNProgressDlg dlg;
					dlg.SetParams(CSVNProgressDlg::SVNProgress_Merge, 0, CTSVNPathList(m_path), pathURL, pathURL, revHighest);		//use the message as the second url
					dlg.m_RevisionEnd = svn_revnum_t(revLowest)-1;
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
				msg.Format(IDS_LOG_REVERTTOREV_CONFIRM, m_path.GetWinPathString());
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					CSVNProgressDlg dlg;
					dlg.SetParams(CSVNProgressDlg::SVNProgress_Merge, 0, CTSVNPathList(m_path), pathURL, pathURL, SVNRev::REV_HEAD);		//use the message as the second url
					dlg.m_RevisionEnd = revSelected;
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
					SVN svn;
					if (!svn.Copy(CTSVNPathList(CTSVNPath(pathURL)), CTSVNPath(dlg.m_URL), dlg.m_CopyRev, dlg.m_CopyRev, dlg.m_sLogMessage))
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					else
						CMessageBox::Show(this->m_hWnd, IDS_LOG_COPY_SUCCESS, IDS_APPNAME, MB_ICONINFORMATION);
				}
			} 
			break;
		case ID_COMPARE:
			{
				//user clicked on the menu item "compare with working copy"
				SVNDiff diff(this, m_hWnd, true);
				diff.SetHEADPeg(m_LogRevision);
				diff.ShowCompare(m_path, SVNRev::REV_WC, m_path, revSelected);
			}
			break;
		case ID_COMPARETWO:
			{
				//user clicked on the menu item "compare revisions"
				SVNDiff diff(this, m_hWnd, true);
				diff.SetHEADPeg(m_LogRevision);
				diff.ShowCompare(CTSVNPath(pathURL), revSelected2, CTSVNPath(pathURL), revSelected);
			}
			break;
		case ID_COMPAREWITHPREVIOUS:
			{
				SVNDiff diff(this, m_hWnd, true);
				diff.SetHEADPeg(m_LogRevision);
				diff.ShowCompare(CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected);
			}
			break;
		case ID_BLAMECOMPARE:
			{
				//user clicked on the menu item "compare with working copy"
				//now first get the revision which is selected
				SVNDiff diff(this, this->m_hWnd, true);
				diff.SetHEADPeg(m_LogRevision);
				diff.ShowCompare(m_path, SVNRev::REV_BASE, m_path, revSelected, SVNRev(), false, true);
			}
			break;
		case ID_BLAMETWO:
			{
				//user clicked on the menu item "compare and blame revisions"
				SVNDiff diff(this, this->m_hWnd, true);
				diff.SetHEADPeg(m_LogRevision);
				diff.ShowCompare(CTSVNPath(pathURL), revSelected2, CTSVNPath(pathURL), revSelected, SVNRev(), false, true);
			}
			break;
		case ID_BLAMEWITHPREVIOUS:
			{
				//user clicked on the menu item "Compare and Blame with previous revision"
				SVNDiff diff(this, this->m_hWnd, true);
				diff.SetHEADPeg(m_LogRevision);
				diff.ShowCompare(CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, SVNRev(), false, true);
			}
			break;
		case ID_SAVEAS:
			{
				//now first get the revision which is selected
				OPENFILENAME ofn = {0};				// common dialog box structure
				TCHAR szFile[MAX_PATH] = {0};		// buffer for file name
				if (m_hasWC)
				{
					CString revFilename;
					CString strWinPath = m_path.GetWinPathString();
					int rfind = strWinPath.ReverseFind('.');
					if (rfind > 0)
						revFilename.Format(_T("%s-%ld%s"), (LPCTSTR)strWinPath.Left(rfind), (LONG)revSelected, (LPCTSTR)strWinPath.Mid(rfind));
					else
						revFilename.Format(_T("%s-%ld"), (LPCTSTR)strWinPath, revSelected);
					_tcscpy_s(szFile, MAX_PATH, revFilename);
				}
				// Initialize OPENFILENAME
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = this->m_hWnd;
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
				CString temp;
				temp.LoadString(IDS_LOG_POPUP_SAVE);
				//ofn.lpstrTitle = "Save revision to...\0";
				CStringUtils::RemoveAccelerators(temp);
				if (temp.IsEmpty())
					ofn.lpstrTitle = NULL;
				else
					ofn.lpstrTitle = temp;
				ofn.Flags = OFN_OVERWRITEPROMPT;

				CString sFilter;
				sFilter.LoadString(IDS_COMMONFILEFILTER);
				TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
				_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
				// Replace '|' delimiters with '\0's
				TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
				while (ptr != pszFilters)
				{
					if (*ptr == '|')
						*ptr = '\0';
					ptr--;
				}
				ofn.lpstrFilter = pszFilters;
				ofn.nFilterIndex = 1;
				// Display the Open dialog box. 
				CTSVNPath tempfile;
				if (GetSaveFileName(&ofn)==TRUE)
				{
					tempfile.SetFromWin(ofn.lpstrFile);
					SVN svn;
					CProgressDlg progDlg;
					progDlg.SetTitle(IDS_APPNAME);
					progDlg.SetAnimation(IDR_DOWNLOAD);
					CString sInfoLine;
					sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), revSelected.ToString());
					progDlg.SetLine(1, sInfoLine);
					svn.SetAndClearProgressInfo(&progDlg);
					progDlg.ShowModeless(m_hWnd);
					if (!svn.Cat(m_path, SVNRev(SVNRev::REV_HEAD), revSelected, tempfile))
					{
						// try again with another peg revision
						if (!svn.Cat(m_path, revSelected, revSelected, tempfile))
						{
							progDlg.Stop();
							svn.SetAndClearProgressInfo((HWND)NULL);
							delete [] pszFilters;
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							EnableOKButton();
							break;
						}
					}
					progDlg.Stop();
					svn.SetAndClearProgressInfo((HWND)NULL);
				}
				delete [] pszFilters;
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
				sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), revSelected.ToString());
				progDlg.SetLine(1, sInfoLine);
				SetAndClearProgressInfo(&progDlg);
				progDlg.ShowModeless(m_hWnd);
				CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, m_path, revSelected);
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
						CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
						cmd += tempfile.GetWinPathString();
						CAppUtils::LaunchApplication(cmd, NULL, false);
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
					tempfile = blame.BlameToTempFile(m_path, dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, _T(""), TRUE);
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
				//now first get the revision which is selected
				SVN svn;
				CProgressDlg progDlg;
				progDlg.SetTitle(IDS_APPNAME);
				progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
				progDlg.SetTime(false);
				svn.SetAndClearProgressInfo(&progDlg);
				progDlg.ShowModeless(m_hWnd);
				if (!svn.Update(CTSVNPathList(m_path), revSelected, svn_depth_unknown, FALSE))
				{
					progDlg.Stop();
					svn.SetAndClearProgressInfo((HWND)NULL);
					CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					EnableOKButton();
					break;
				}
				progDlg.Stop();
				svn.SetAndClearProgressInfo((HWND)NULL);
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
					CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"),
					pathURL, revSelected.ToString());

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
				EditAuthor(selIndex);
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
				CString url = _T("tsvn:")+pathURL;
				sCmd.Format(_T("%s /command:export /url:\"%s\" /revision:%ld"),
					CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"),
					url, (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_CHECKOUT:
			{
				CString sCmd;
				CString url = _T("tsvn:")+pathURL;
				sCmd.Format(_T("%s /command:checkout /url:\"%s\" /revision:%ld"),
					CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"),
					url, (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_GETMERGELOGS:
			{
				CString sCmd;
				CString url = _T("tsvn:")+pathURL;
				sCmd.Format(_T("%s /command:log /path:\"%s\" /revstart:%ld /pegrev:%ld /merge"),
					CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"),
					pathURL, (LONG)revSelected, (LONG)m_startrev);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_VIEWREV:
			{
				CString url = m_ProjectProperties.sWebViewerRev;
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
		POSITION pos = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos);
			changedpaths.push_back(m_currentChangedPathList[nItem].GetSVNPathString());
		}
	}
	else
	{
		// only one revision is selected in the log dialog top pane
		// but multiple items could be selected  in the changed items list
		rev2 = rev1-1;

		POSITION pos = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos);
			LogChangedPath * changedlogpath = pLogEntry->pArChangedPaths->GetAt(nItem);

			if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
			{
				if ((changedlogpath)&&(!changedlogpath->sCopyFromPath.IsEmpty()))
					rev2 = changedlogpath->lCopyFromRev;
				else
				{
					// if the path was modified but the parent path was 'added with history'
					// then we have to use the 'copyfrom' revision of the parent path
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
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		CString temp;
		bool bEntryAdded = false;
		if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
		{
			if ((!bOneRev)||(DiffPossible(changedlogpaths[0], rev1)))
			{
				temp.LoadString(IDS_LOG_POPUP_DIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_DIFF, temp);
				temp.LoadString(IDS_LOG_POPUP_BLAMEDIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAMEDIFF, temp);
				popup.SetDefaultItem(ID_DIFF, FALSE);
				temp.LoadString(IDS_LOG_POPUP_GNUDIFF_CH);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
				bEntryAdded = true;
			}
			if (rev2 == rev1-1)
			{
				if (bEntryAdded)
					popup.AppendMenu(MF_SEPARATOR, NULL);
				temp.LoadString(IDS_LOG_POPUP_OPEN);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPEN, temp);
				temp.LoadString(IDS_LOG_POPUP_OPENWITH);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPENWITH, temp);
				temp.LoadString(IDS_LOG_POPUP_BLAME);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAME, temp);
				popup.AppendMenu(MF_SEPARATOR, NULL);
				temp.LoadString(IDS_LOG_POPUP_REVERTREV);
				if (m_hasWC)
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);
				temp.LoadString(IDS_REPOBROWSE_SHOWPROP);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPPROPS, temp);			// "Show Properties"
				temp.LoadString(IDS_MENULOG);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_LOG, temp);					// "Show Log"				
				temp.LoadString(IDS_LOG_POPUP_SAVE);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
				bEntryAdded = true;
				if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
				{
					popup.AppendMenu(MF_SEPARATOR, NULL);
					temp.LoadString(IDS_LOG_POPUP_VIEWPATHREV);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_VIEWPATHREV, temp);
				}
				if (popup.GetDefaultItem(0,FALSE)==-1)
					popup.SetDefaultItem(ID_OPEN, FALSE);
			}
		}
		else
		{
			// more than one entry is selected
			temp.LoadString(IDS_LOG_POPUP_SAVE);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
			bEntryAdded = true;
		}

		if (!bEntryAdded)
			return;
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		bool bOpenWith = false;
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
				if (SVN::PathIsURL(m_path.GetSVNPathString()))
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
						temp.Format(IDS_ERR_NOURLOFFILE, m_path);
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
					dlg.SetParams(CSVNProgressDlg::SVNProgress_Copy, 0, CTSVNPathList(CTSVNPath(fileURL)), wcPath, _T(""), rev2);
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
					dlg.SetParams(CSVNProgressDlg::SVNProgress_Merge, 0, CTSVNPathList(CTSVNPath(wcPath)), fileURL, fileURL, rev1);		//use the message as the second url
					dlg.m_RevisionEnd = rev2;
				}
				CString msg;
				msg.Format(IDS_LOG_REVERT_CONFIRM, wcPath);
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
				if (SVN::PathIsURL(m_path.GetSVNPathString()))
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
						temp.Format(IDS_ERR_NOURLOFFILE, filepath);
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
				if (SVN::PathIsURL(m_path.GetSVNPathString()))
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
						temp.Format(IDS_ERR_NOURLOFFILE, filepath);
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
					OPENFILENAME ofn = {0};				// common dialog box structure
					TCHAR szFile[MAX_PATH] = {0};		// buffer for file name
					CString revFilename;
					temp = CPathUtils::GetFileNameFromPath(changedpaths[0]);
					int rfind = temp.ReverseFind('.');
					if (rfind > 0)
						revFilename.Format(_T("%s-%ld%s"), temp.Left(rfind), rev1, temp.Mid(rfind));
					else
						revFilename.Format(_T("%s-%ld"), temp, rev1);
					_tcscpy_s(szFile, MAX_PATH, revFilename);
					// Initialize OPENFILENAME
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = this->m_hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
					temp.LoadString(IDS_LOG_POPUP_SAVE);
					CStringUtils::RemoveAccelerators(temp);
					if (temp.IsEmpty())
						ofn.lpstrTitle = NULL;
					else
						ofn.lpstrTitle = temp;
					ofn.Flags = OFN_OVERWRITEPROMPT;

					CString sFilter;
					sFilter.LoadString(IDS_COMMONFILEFILTER);
					TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
					_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
					// Replace '|' delimiters with '\0's
					TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
					while (ptr != pszFilters)
					{
						if (*ptr == '|')
							*ptr = '\0';
						ptr--;
					}
					ofn.lpstrFilter = pszFilters;
					ofn.nFilterIndex = 1;
					// Display the Open dialog box. 
					bTargetSelected = !!GetSaveFileName(&ofn);
					TargetPath.SetFromWin(ofn.lpstrFile);
					delete [] pszFilters;
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
						sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, filepath, getrev.ToString());
						progDlg.SetLine(1, sInfoLine);
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
							// items and then create subfolders to save those files
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
				Open(bOpenWith,changedpaths[0],rev1);
			}
			break;
		case ID_BLAME:
			{
				CString filepath;
				if (SVN::PathIsURL(m_path.GetSVNPathString()))
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
						temp.Format(IDS_ERR_NOURLOFFILE, filepath);
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
					tempfile = blame.BlameToTempFile(CTSVNPath(filepath), dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, _T(""), TRUE);
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
		case ID_LOG:
			{
				DialogEnableWindow(IDOK, FALSE);
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				CString filepath;
				if (SVN::PathIsURL(m_path.GetSVNPathString()))
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
						temp.Format(IDS_ERR_NOURLOFFILE, filepath);
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
				sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /startrev:%ld"), CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"), filepath, logrev);

				CAppUtils::LaunchApplication(sCmd, NULL, false);
				EnableOKButton();
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_VIEWPATHREV:
			{
				PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
				SVNRev rev = pLogEntry->Rev;
				CString relurl = changedpaths[0];
				CString url = m_ProjectProperties.sWebViewerPathRev;
				url.Replace(_T("%REVISION%"), rev.ToString());
				url.Replace(_T("%PATH%"), relurl);
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
	if (nType == SIZE_MAXIMIZED)
	{
		DoSizeV1(0);
		DoSizeV2(0);
	}
	//set range
	SetSplitterRange();
}

void CLogDlg::OnRefresh()
{
	if (GetDlgItem(IDC_GETALL)->IsWindowEnabled())
		Refresh();
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
