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
#include "ResolveDlg.h"
#include "AppUtils.h"

#define REFRESHTIMER   100

IMPLEMENT_DYNAMIC(CResolveDlg, CResizableStandAloneDialog)
CResolveDlg::CResolveDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CResolveDlg::IDD, pParent)
    , m_bThreadRunning(FALSE)
    , m_bCancelled(false)
{
}

CResolveDlg::~CResolveDlg()
{
}

void CResolveDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RESOLVELIST, m_resolveListCtrl);
    DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CResolveDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
    ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CResolveDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_RESOLVELIST, IDC_RESOLVELIST, IDC_RESOLVELIST, IDC_RESOLVELIST);
    m_aeroControls.SubclassControl(this, IDC_SELECTALL);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_resolveListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS | SVNSLC_COLPROPSTATUS, L"ResolveDlg", SVNSLC_POPALL ^ (SVNSLC_POPIGNORE|SVNSLC_POPADD|SVNSLC_POPCOMMIT|SVNSLC_POPCREATEPATCH|SVNSLC_POPRESTORE));
    m_resolveListCtrl.SetConfirmButton((CButton*)GetDlgItem(IDOK));
    m_resolveListCtrl.SetSelectButton(&m_SelectAll);
    m_resolveListCtrl.SetCancelBool(&m_bCancelled);
    m_resolveListCtrl.SetBackgroundImage(IDI_RESOLVE_BKG);
    m_resolveListCtrl.EnableFileDrop();

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    AdjustControlSize(IDC_SELECTALL);

    AddAnchor(IDC_RESOLVELIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"ResolveDlg");

    // first start a thread to obtain the file list with the status without
    // blocking the dialog
    if(AfxBeginThread(ResolveThreadEntry, this) == NULL)
    {
        OnCantStartThread();
    }
    InterlockedExchange(&m_bThreadRunning, TRUE);

    return TRUE;
}

void CResolveDlg::OnOK()
{
    if (m_bThreadRunning)
        return;

    //save only the files the user has selected into the path list
    m_resolveListCtrl.WriteCheckedNamesToPathList(m_pathList);

    CResizableStandAloneDialog::OnOK();
}

void CResolveDlg::OnCancel()
{
    m_bCancelled = true;
    if (m_bThreadRunning)
        return;

    CResizableStandAloneDialog::OnCancel();
}

void CResolveDlg::OnBnClickedSelectall()
{
    UINT state = (m_SelectAll.GetState() & 0x0003);
    if (state == BST_INDETERMINATE)
    {
        // It is not at all useful to manually place the checkbox into the indeterminate state...
        // We will force this on to the unchecked state
        state = BST_UNCHECKED;
        m_SelectAll.SetCheck(state);
    }
    theApp.DoWaitCursor(1);
    m_resolveListCtrl.SelectAll(state == BST_CHECKED);
    theApp.DoWaitCursor(-1);
}

UINT CResolveDlg::ResolveThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return ((CResolveDlg*)pVoid)->ResolveThread();
}
UINT CResolveDlg::ResolveThread()
{
    InterlockedExchange(&m_bThreadRunning, TRUE);
    // get the status of all selected file/folders recursively
    // and show the ones which are in conflict
    DialogEnableWindow(IDOK, false);

    m_bCancelled = false;

    if (!m_resolveListCtrl.GetStatus(m_pathList))
    {
        m_resolveListCtrl.SetEmptyString(m_resolveListCtrl.GetLastErrorMessage());
    }
    m_resolveListCtrl.Show(SVNSLC_SHOWCONFLICTED|SVNSLC_SHOWINEXTERNALS, CTSVNPathList(), SVNSLC_SHOWCONFLICTED, true, true);

    InterlockedExchange(&m_bThreadRunning, FALSE);
    return 0;
}

void CResolveDlg::OnBnClickedHelp()
{
    OnHelp();
}

BOOL CResolveDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_RETURN:
            if(OnEnterPressed())
                return TRUE;
            break;
        case VK_F5:
            {
                if (!m_bThreadRunning)
                {
                    if(AfxBeginThread(ResolveThreadEntry, this) == NULL)
                    {
                        OnCantStartThread();
                    }
                    else
                        InterlockedExchange(&m_bThreadRunning, TRUE);
                }
            }
            break;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CResolveDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    if(AfxBeginThread(ResolveThreadEntry, this) == NULL)
    {
        OnCantStartThread();
    }
    return 0;
}

LRESULT CResolveDlg::OnFileDropped(WPARAM, LPARAM lParam)
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

    if (!m_resolveListCtrl.HasPath(path))
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

void CResolveDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
    case REFRESHTIMER:
        if (m_bThreadRunning)
        {
            SetTimer(REFRESHTIMER, 200, NULL);
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Wait some more before refreshing\n");
        }
        else
        {
            KillTimer(REFRESHTIMER);
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Refreshing after items dropped\n");
            OnSVNStatusListCtrlNeedsRefresh(0, 0);
        }
        break;
    }
    __super::OnTimer(nIDEvent);
}