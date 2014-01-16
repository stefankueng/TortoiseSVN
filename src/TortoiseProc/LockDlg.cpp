// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "LockDlg.h"
#include "UnicodeUtils.h"
#include "SVNProperties.h"
#include "SVN.h"
#include "HistoryDlg.h"
#include "AppUtils.h"

#define REFRESHTIMER   100

IMPLEMENT_DYNAMIC(CLockDlg, CResizableStandAloneDialog)
CLockDlg::CLockDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CLockDlg::IDD, pParent)
    , m_bBlock(FALSE)
    , m_bStealLocks(FALSE)
    , m_pThread(NULL)
    , m_bCancelled(false)
    , m_ProjectProperties(NULL)
{
}

CLockDlg::~CLockDlg()
{
    delete m_pThread;
}

void CLockDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_STEALLOCKS, m_bStealLocks);
    DDX_Control(pDX, IDC_FILELIST, m_cFileList);
    DDX_Control(pDX, IDC_LOCKMESSAGE, m_cEdit);
    DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CLockDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_EN_CHANGE(IDC_LOCKMESSAGE, OnEnChangeLockmessage)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_BN_CLICKED(IDC_SELECTALL, &CLockDlg::OnBnClickedSelectall)
    ON_BN_CLICKED(IDC_HISTORY, &CLockDlg::OnBnClickedHistory)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
    ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CLockDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_FILELIST);
    m_aeroControls.SubclassControl(this, IDC_SELECTALL);
    m_aeroControls.SubclassControl(this, IDC_STEALLOCKS);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_History.SetMaxHistoryItems((LONG)CRegDWORD(L"Software\\TortoiseSVN\\MaxHistoryItems", 25));
    m_History.Load(L"Software\\TortoiseSVN\\History\\commit", L"logmsgs");

    m_cFileList.Init(SVNSLC_COLEXT | SVNSLC_COLLOCK, L"LockDlg");
    m_cFileList.SetSelectButton(&m_SelectAll);
    m_cFileList.SetConfirmButton((CButton*)GetDlgItem(IDOK));
    m_cFileList.SetCancelBool(&m_bCancelled);
    m_cFileList.EnableFileDrop();
    m_cFileList.SetBackgroundImage(IDI_LOCK_BKG);
    if (m_ProjectProperties)
        m_cEdit.Init(*m_ProjectProperties);
    else
        m_cEdit.Init();
    m_cEdit.SetFont((CString)CRegString(L"Software\\TortoiseSVN\\LogFontName", L"Courier New"), (DWORD)CRegDWORD(L"Software\\TortoiseSVN\\LogFontSize", 8));

    if (!m_sLockMessage.IsEmpty())
        m_cEdit.SetText(m_sLockMessage);
    else if (m_ProjectProperties)
        m_cEdit.SetText(m_ProjectProperties->GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATELOCK));

    CAppUtils::SetAccProperty(m_cEdit.GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_TEXT);
    CAppUtils::SetAccProperty(m_cEdit.GetSafeHwnd(), PROPID_ACC_HELP, CString(MAKEINTRESOURCE(IDS_INPUT_ENTERLOG)));

    m_tooltips.Create(this);

    m_SelectAll.SetCheck(BST_INDETERMINATE);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    OnEnChangeLockmessage();

    AdjustControlSize(IDC_STEALLOCKS);
    AdjustControlSize(IDC_SELECTALL);

    AddAnchor(IDC_HISTORY, TOP_LEFT);
    AddAnchor(IDC_LOCKTITLELABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_LOCKMESSAGE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_STEALLOCKS, BOTTOM_LEFT);
    AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"LockDlg");

    // start a thread to obtain the file list with the status without
    // blocking the dialog
    m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
    if (m_pThread==NULL)
    {
        OnCantStartThread();
    }
    else
    {
        m_pThread->m_bAutoDelete = FALSE;
        m_pThread->ResumeThread();
    }
    m_bBlock = TRUE;

    GetDlgItem(IDC_LOCKMESSAGE)->SetFocus();
    return FALSE;
}

void CLockDlg::OnOK()
{
    if (m_bBlock)
        return;
    m_cFileList.WriteCheckedNamesToPathList(m_pathList);
    UpdateData();
    m_sLockMessage = m_cEdit.GetText();
    if (!m_sLockMessage.IsEmpty())
    {
        m_History.AddEntry(m_sLockMessage);
        m_History.Save();
    }

    CResizableStandAloneDialog::OnOK();
}

void CLockDlg::OnCancel()
{
    m_bCancelled = true;
    if (m_bBlock)
        return;
    UpdateData();
    if ((m_ProjectProperties == 0)||(m_ProjectProperties->GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATELOCK).Compare(m_cEdit.GetText()) != 0))
        m_sLockMessage = m_cEdit.GetText();
    if (!m_sLockMessage.IsEmpty())
    {
        m_History.AddEntry(m_sLockMessage);
        m_History.Save();
    }
    CResizableStandAloneDialog::OnCancel();
}

UINT CLockDlg::StatusThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return ((CLockDlg*)pVoid)->StatusThread();
}

UINT CLockDlg::StatusThread()
{
    // get the status of all selected file/folders recursively
    // and show the ones which can be locked to the user
    // in a list control.
    m_bBlock = TRUE;
    DialogEnableWindow(IDOK, false);
    m_bCancelled = false;
    // Initialise the list control with the status of the files/folders below us
    if (!m_cFileList.GetStatus(m_pathList))
    {
        m_cFileList.SetEmptyString(m_cFileList.GetLastErrorMessage());
    }

    DWORD dwShow = SVNSLC_SHOWNORMAL | SVNSLC_SHOWMODIFIED | SVNSLC_SHOWMERGED | SVNSLC_SHOWLOCKS;
    m_cFileList.Show(dwShow, CTSVNPathList(), dwShow, false, true);

    RefreshCursor();
    CString logmsg;
    GetDlgItemText(IDC_LOCKMESSAGE, logmsg);
    DialogEnableWindow(IDOK, m_ProjectProperties ? m_ProjectProperties->nMinLockMsgSize <= logmsg.GetLength() : TRUE);
    m_bBlock = FALSE;
    return 0;
}

BOOL CLockDlg::PreTranslateMessage(MSG* pMsg)
{
    if (!m_bBlock)
        m_tooltips.RelayEvent(pMsg);

    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_F5:
            {
                if (m_bBlock)
                    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
                Refresh();
            }
            break;
        case VK_RETURN:
            if(OnEnterPressed())
                return TRUE;
            break;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CLockDlg::Refresh()
{
    m_bBlock = TRUE;
    if (AfxBeginThread(StatusThreadEntry, this)==NULL)
    {
        OnCantStartThread();
    }
}

void CLockDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CLockDlg::OnEnChangeLockmessage()
{
    CString sTemp;
    GetDlgItemText(IDC_LOCKMESSAGE, sTemp);
    if ((m_ProjectProperties == NULL)||((m_ProjectProperties)&&(sTemp.GetLength() >= m_ProjectProperties->nMinLockMsgSize)))
    {
        if (!m_bBlock)
            DialogEnableWindow(IDOK, TRUE);
    }
    else
    {
        DialogEnableWindow(IDOK, FALSE);
    }
}

LRESULT CLockDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    Refresh();
    return 0;
}

void CLockDlg::OnBnClickedSelectall()
{
    m_tooltips.Pop();   // hide the tooltips
    UINT state = (m_SelectAll.GetState() & 0x0003);
    if (state == BST_INDETERMINATE)
    {
        // It is not at all useful to manually place the checkbox into the indeterminate state...
        // We will force this on to the unchecked state
        state = BST_UNCHECKED;
        m_SelectAll.SetCheck(state);
    }
    m_cFileList.SelectAll(state == BST_CHECKED);
    OnEnChangeLockmessage();
}

void CLockDlg::OnBnClickedHistory()
{
    m_tooltips.Pop();   // hide the tooltips
    if (m_pathList.GetCount() == 0)
        return;
    SVN svn;
    CHistoryDlg historyDlg;
    historyDlg.SetHistory(m_History);
    if (historyDlg.DoModal()==IDOK)
    {
        if (historyDlg.GetSelectedText().Compare(m_cEdit.GetText().Left(historyDlg.GetSelectedText().GetLength()))!=0)
        {
            if ((m_ProjectProperties)&&(m_ProjectProperties->GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATELOCK).Compare(m_cEdit.GetText())!=0))
                m_cEdit.InsertText(historyDlg.GetSelectedText(), !m_cEdit.GetText().IsEmpty());
            else
                m_cEdit.SetText(historyDlg.GetSelectedText());
        }

        OnEnChangeLockmessage();
        GetDlgItem(IDC_LOCKMESSAGE)->SetFocus();
    }
}

LRESULT CLockDlg::OnFileDropped(WPARAM, LPARAM lParam)
{
    BringWindowToTop();
    SetForegroundWindow();
    SetActiveWindow();
    // if multiple files/folders are dropped
    // this handler is called for every single item
    // separately.
    // To avoid creating multiple refresh threads and
    // causing crashes, we only add the items to the
    // list control and start a timer.
    // When the timer expires, we start the refresh thread,
    // but only if it isn't already running - otherwise we
    // restart the timer.
    CTSVNPath path;
    path.SetFromWin((LPCTSTR)lParam);

    if (!m_cFileList.HasPath(path))
    {
        if (m_pathList.AreAllPathsFiles())
        {
            m_pathList.AddPath(path);
            m_pathList.RemoveDuplicates();
        }
        else
        {
            // if the path list contains folders, we have to check whether
            // our just (maybe) added path is a child of one of those. If it is
            // a child of a folder already in the list, we must not add it. Otherwise
            // that path could show up twice in the list.
            bool bHasParentInList = false;
            for (int i=0; i<m_pathList.GetCount(); ++i)
            {
                if (m_pathList[i].IsAncestorOf(path))
                {
                    bHasParentInList = true;
                    break;
                }
            }
            if (!bHasParentInList)
            {
                m_pathList.AddPath(path);
                m_pathList.RemoveDuplicates();
            }
        }
    }

    // Always start the timer, since the status of an existing item might have changed
    SetTimer(REFRESHTIMER, 200, NULL);
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
    return 0;
}

void CLockDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
    case REFRESHTIMER:
        if (m_bBlock)
        {
            SetTimer(REFRESHTIMER, 200, NULL);
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Wait some more before refreshing\n");
        }
        else
        {
            KillTimer(REFRESHTIMER);
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Refreshing after items dropped\n");
            Refresh();
        }
        break;
    }
    __super::OnTimer(nIDEvent);
}