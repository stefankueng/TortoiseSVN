// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014, 2021 - TortoiseSVN
// Copyright (C) 2019 - TortoiseGit

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
#include "AddDlg.h"
#include "SVNConfig.h"
#include "registry.h"
#include "AppUtils.h"

#define REFRESHTIMER 100

IMPLEMENT_DYNAMIC(CAddDlg, CResizableStandAloneDialog)
CAddDlg::CAddDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CAddDlg::IDD, pParent)
    , m_useAutoprops(true)
    , m_bThreadRunning(FALSE)
    , m_bCancelled(false)
{
}

CAddDlg::~CAddDlg()
{
}

void CAddDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ADDLIST, m_addListCtrl);
    DDX_Control(pDX, IDC_SELECTALL, m_selectAll);
    DDX_Check(pDX, IDC_USEAUTOPROPS, m_useAutoprops);
}

BEGIN_MESSAGE_MAP(CAddDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
    ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CAddDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_ADDLIST, IDC_ADDLIST, IDC_ADDLIST, IDC_ADDLIST);
    m_aeroControls.SubclassControl(this, IDC_SELECTALL);
    m_aeroControls.SubclassControl(this, IDC_USEAUTOPROPS);
    m_aeroControls.SubclassOkCancelHelp(this);

    // initialize the svn status list control
    m_addListCtrl.Init(SVNSLC_COLEXT, L"AddDlg", SVNSLC_POPALL ^ (SVNSLC_POPADD | SVNSLC_POPCOMMIT | SVNSLC_POPCHANGELISTS | SVNSLC_POPCREATEPATCH | SVNSLC_POPRESTORE)); // adding and committing is useless in the add dialog
    m_addListCtrl.SetIgnoreRemoveOnly();                                                                                                                                  // when ignoring, don't add the parent folder since we're in the add dialog
    m_addListCtrl.SetUnversionedRecurse(true);                                                                                                                            // recurse into unversioned folders - user might want to add those too
    m_addListCtrl.SetSelectButton(&m_selectAll);
    m_addListCtrl.SetConfirmButton(static_cast<CButton*>(GetDlgItem(IDOK)));
    m_addListCtrl.SetEmptyString(IDS_ERR_NOTHINGTOADD);
    m_addListCtrl.SetCancelBool(&m_bCancelled);
    m_addListCtrl.SetBackgroundImage(IDI_ADD_BKG);
    m_addListCtrl.EnableFileDrop();

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    AdjustControlSize(IDC_SELECTALL);

    AddAnchor(IDC_ADDLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
    AddAnchor(IDC_USEAUTOPROPS, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"AddDlg");

    //first start a thread to obtain the file list with the status without
    //blocking the dialog
    InterlockedExchange(&m_bThreadRunning, TRUE);
    if (AfxBeginThread(AddThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }

    return TRUE;
}

void CAddDlg::OnOK()
{
    if (m_bThreadRunning)
        return;

    // save only the files the user has selected into the path list
    m_addListCtrl.WriteCheckedNamesToPathList(m_pathList);

    CResizableStandAloneDialog::OnOK();
}

void CAddDlg::OnCancel()
{
    m_bCancelled = true;
    if (m_bThreadRunning)
        return;

    CResizableStandAloneDialog::OnCancel();
}

void CAddDlg::OnBnClickedSelectall()
{
    UINT state = (m_selectAll.GetState() & 0x0003);
    if (state == BST_INDETERMINATE)
    {
        // It is not at all useful to manually place the checkbox into the indeterminate state...
        // We will force this on to the unchecked state
        state = BST_UNCHECKED;
        m_selectAll.SetCheck(state);
    }
    theApp.DoWaitCursor(1);
    m_addListCtrl.SelectAll(state == BST_CHECKED);
    theApp.DoWaitCursor(-1);
}

UINT CAddDlg::AddThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return static_cast<CAddDlg*>(pVoid)->AddThread();
}

UINT CAddDlg::AddThread()
{
    // get the status of all selected file/folders recursively
    // and show the ones which the user can add (i.e. the unversioned ones)
    DialogEnableWindow(IDOK, false);
    m_bCancelled = false;
    if (!m_addListCtrl.GetStatus(m_pathList))
    {
        m_addListCtrl.SetEmptyString(m_addListCtrl.GetLastErrorMessage());
    }
    bool bSelectFilesForCommit = !!static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\SelectFilesForCommit", TRUE));
    m_addListCtrl.Show(SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWREMOVEDANDPRESENT, CTSVNPathList(),
                       bSelectFilesForCommit ? SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWREMOVEDANDPRESENT : 0, true, true);

    InterlockedExchange(&m_bThreadRunning, FALSE);
    return 0;
}

void CAddDlg::OnBnClickedHelp()
{
    OnHelp();
}

BOOL CAddDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
            case VK_RETURN:
                if (OnEnterPressed())
                    return TRUE;
                break;
            case VK_F5:
            {
                if (!InterlockedExchange(&m_bThreadRunning, TRUE))
                {
                    if (AfxBeginThread(AddThreadEntry, this) == nullptr)
                    {
                        InterlockedExchange(&m_bThreadRunning, FALSE);
                        OnCantStartThread();
                    }
                }
            }
            break;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CAddDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    if (InterlockedExchange(&m_bThreadRunning, TRUE))
        return 0;
    if (AfxBeginThread(AddThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }
    return 0;
}

LRESULT CAddDlg::OnFileDropped(WPARAM, LPARAM lParam)
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
    path.SetFromWin(reinterpret_cast<LPCWSTR>(lParam));

    if (!m_addListCtrl.HasPath(path))
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
            for (int i = 0; i < m_pathList.GetCount(); ++i)
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
    SetTimer(REFRESHTIMER, 200, nullptr);
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
    return 0;
}

void CAddDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
        case REFRESHTIMER:
            if (m_bThreadRunning)
            {
                SetTimer(REFRESHTIMER, 200, nullptr);
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