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
#include "ChangedDlg.h"
#include "AppUtils.h"


IMPLEMENT_DYNAMIC(CChangedDlg, CResizableStandAloneDialog)
CChangedDlg::CChangedDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CChangedDlg::IDD, pParent)
    , m_bShowUnversioned(FALSE)
    , m_iShowUnmodified(0)
    , m_bBlock(FALSE)
    , m_bCanceled(false)
    , m_bShowIgnored(FALSE)
    , m_bShowExternals(TRUE)
    , m_bShowUserProps(FALSE)
    , m_bShowDirs(TRUE)
    , m_bShowFiles(TRUE)
    , m_bDepthInfinity(false)
    , m_bRemote(false)
    , m_bContactRepository(false)
    , m_bShowPropertiesClicked(false)
{
}

CChangedDlg::~CChangedDlg()
{
}

void CChangedDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CHANGEDLIST, m_FileListCtrl);
    DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
    DDX_Check(pDX, IDC_SHOWUNMODIFIED, m_iShowUnmodified);
    DDX_Check(pDX, IDC_SHOWIGNORED, m_bShowIgnored);
    DDX_Check(pDX, IDC_SHOWEXTERNALS, m_bShowExternals);
    DDX_Check(pDX, IDC_SHOWUSERPROPS, m_bShowUserProps);
    DDX_Check(pDX, IDC_SHOWFILES, m_bShowFiles);
    DDX_Check(pDX, IDC_SHOWFOLDERS, m_bShowDirs);
}


BEGIN_MESSAGE_MAP(CChangedDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_CHECKREPO, OnBnClickedCheckrepo)
    ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
    ON_BN_CLICKED(IDC_SHOWUNMODIFIED, OnBnClickedShowUnmodified)
    ON_BN_CLICKED(IDC_SHOWUSERPROPS, OnBnClickedShowUserProps)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED, OnSVNStatusListCtrlItemCountChanged)
    ON_BN_CLICKED(IDC_SHOWIGNORED, &CChangedDlg::OnBnClickedShowignored)
    ON_BN_CLICKED(IDC_REFRESH, &CChangedDlg::OnBnClickedRefresh)
    ON_BN_CLICKED(IDC_SHOWEXTERNALS, &CChangedDlg::OnBnClickedShowexternals)
    ON_BN_CLICKED(IDC_SHOWFOLDERS, &CChangedDlg::OnBnClickedShowfolders)
    ON_BN_CLICKED(IDC_SHOWFILES, &CChangedDlg::OnBnClickedShowfiles)
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

BOOL CChangedDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_CHANGEDLIST, IDC_CHANGEDLIST, IDC_CHANGEDLIST, IDC_CHANGEDLIST);
    m_aeroControls.SubclassControl(this, IDC_SHOWGROUP);
    m_aeroControls.SubclassControl(this, IDC_SHOWFOLDERS);
    m_aeroControls.SubclassControl(this, IDC_SHOWFILES);
    m_aeroControls.SubclassControl(this, IDC_SHOWUNVERSIONED);
    m_aeroControls.SubclassControl(this, IDC_SHOWUNMODIFIED);
    m_aeroControls.SubclassControl(this, IDC_SHOWIGNORED);
    m_aeroControls.SubclassControl(this, IDC_SHOWUSERPROPS);
    m_aeroControls.SubclassControl(this, IDC_SHOWEXTERNALS);
    m_aeroControls.SubclassControl(this, IDC_INFOLABEL);
    m_aeroControls.SubclassControl(this, IDC_SUMMARYTEXT);
    m_aeroControls.SubclassControl(this, IDC_REFRESH);
    m_aeroControls.SubclassControl(this, IDC_CHECKREPO);
    m_aeroControls.SubclassControl(this, IDOK);

    GetWindowText(m_sTitle);

    m_tooltips.Create(this);
    m_tooltips.AddTool(IDC_CHECKREPO, IDS_REPOSTATUS_TT_REPOCHECK);

    m_regAddBeforeCommit = CRegDWORD(L"Software\\TortoiseSVN\\AddBeforeCommit", TRUE);
    m_bShowUnversioned = m_regAddBeforeCommit;
    m_regShowUserProps = CRegDWORD(L"Software\\TortoiseSVN\\ShowUserProps", TRUE);
    m_bShowUserProps = m_regShowUserProps;
    UpdateData(FALSE);

    m_FileListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS | SVNSLC_COLPROPSTATUS |
                        SVNSLC_COLREMOTETEXT | SVNSLC_COLREMOTEPROP |
                        SVNSLC_COLLOCK | SVNSLC_COLLOCKCOMMENT |
                        SVNSLC_COLAUTHOR |
                        SVNSLC_COLREVISION | SVNSLC_COLDATE, L"ChangedDlg",
                        SVNSLC_POPALL ^ SVNSLC_POPRESTORE, false);
    m_FileListCtrl.SetCancelBool(&m_bCanceled);
    m_FileListCtrl.SetBackgroundImage(IDI_CFM_BKG);
    m_FileListCtrl.SetEmptyString(IDS_REPOSTATUS_EMPTYFILELIST);

    AdjustControlSize(IDC_SHOWUNVERSIONED);
    AdjustControlSize(IDC_SHOWUNMODIFIED);
    AdjustControlSize(IDC_SHOWIGNORED);
    AdjustControlSize(IDC_SHOWEXTERNALS);
    AdjustControlSize(IDC_SHOWUSERPROPS);
    AdjustControlSize(IDC_SHOWFILES);
    AdjustControlSize(IDC_SHOWFOLDERS);

    AddAnchor(IDC_CHANGEDLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SUMMARYTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_INFOLABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SHOWGROUP, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWUNMODIFIED, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWIGNORED, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWEXTERNALS, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWUSERPROPS, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWFILES, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWFOLDERS, BOTTOM_LEFT);
    AddAnchor(IDC_REFRESH, BOTTOM_RIGHT);
    AddAnchor(IDC_CHECKREPO, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    SetPromptParentWindow(m_hWnd);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"ChangedDlg");

    m_bRemote = !!(DWORD)CRegDWORD(L"Software\\TortoiseSVN\\CheckRepo", FALSE);
    if(m_bContactRepository){m_bRemote = true;}
    // first start a thread to obtain the status without
    // blocking the dialog
    OnBnClickedRefresh();

    return TRUE;
}

UINT CChangedDlg::ChangedStatusThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return ((CChangedDlg*)pVoid)->ChangedStatusThread();
}

UINT CChangedDlg::ChangedStatusThread()
{
    InterlockedExchange(&m_bBlock, TRUE);
    RefreshCursor();
    m_bCanceled = false;
    SetDlgItemText(IDOK, CString(MAKEINTRESOURCE(IDS_MSGBOX_CANCEL)));
    DialogEnableWindow(IDC_REFRESH, FALSE);
    DialogEnableWindow(IDC_CHECKREPO, FALSE);
    DialogEnableWindow(IDC_SHOWUNVERSIONED, FALSE);
    DialogEnableWindow(IDC_SHOWUNMODIFIED, FALSE);
    DialogEnableWindow(IDC_SHOWIGNORED, FALSE);
    DialogEnableWindow(IDC_SHOWUSERPROPS, FALSE);
    DialogEnableWindow(IDC_SHOWEXTERNALS, FALSE);
    DialogEnableWindow(IDC_SHOWFILES, FALSE);
    DialogEnableWindow(IDC_SHOWFOLDERS, FALSE);
    CString temp;
    m_FileListCtrl.SetDepthInfinity(m_bDepthInfinity);

    if (m_bShowPropertiesClicked)
    {
        m_bShowPropertiesClicked = false;
        m_FileListCtrl.GetUserProps(m_bShowUserProps != FALSE);
    }
    else
        if (!m_FileListCtrl.GetStatus(m_pathList, m_bRemote, m_bShowIgnored != FALSE, m_bShowUserProps != FALSE))
            m_FileListCtrl.SetEmptyString(m_FileListCtrl.GetLastErrorMessage());

    DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | SVNSLC_SHOWLOCKS | SVNSLC_SHOWSWITCHED | SVNSLC_SHOWINCHANGELIST | SVNSLC_SHOWNESTED;
    dwShow |= m_bShowUnversioned ? SVNSLC_SHOWUNVERSIONED : 0;
    dwShow |= m_iShowUnmodified ? SVNSLC_SHOWNORMAL : 0;
    dwShow |= m_bShowIgnored ? SVNSLC_SHOWIGNORED : 0;
    dwShow |= m_bShowExternals ? SVNSLC_SHOWEXTERNAL | SVNSLC_SHOWINEXTERNALS | SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO : 0;
    m_FileListCtrl.Show(dwShow, CTSVNPathList(), 0, !!m_bShowDirs, !!m_bShowFiles);
    UpdateStatistics();

    CTSVNPath commonDir = m_FileListCtrl.GetCommonDirectory(false);
    bool bSingleFile = ((m_pathList.GetCount()==1)&&(!m_pathList[0].IsDirectory()));
    if (bSingleFile)
        CAppUtils::SetWindowTitle(m_hWnd, m_pathList[0].GetWinPathString(), m_sTitle);
    else
        CAppUtils::SetWindowTitle(m_hWnd, commonDir.GetWinPathString(), m_sTitle);
    SetDlgItemText(IDOK, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)));
    DialogEnableWindow(IDC_REFRESH, TRUE);
    DialogEnableWindow(IDC_CHECKREPO, TRUE);
    DialogEnableWindow(IDC_SHOWUNVERSIONED, !bSingleFile);
    DialogEnableWindow(IDC_SHOWUNMODIFIED, !bSingleFile);
    DialogEnableWindow(IDC_SHOWIGNORED, !bSingleFile);
    DialogEnableWindow(IDC_SHOWUSERPROPS, TRUE);
    DialogEnableWindow(IDC_SHOWEXTERNALS, !bSingleFile);
    DialogEnableWindow(IDC_SHOWFILES, TRUE);
    DialogEnableWindow(IDC_SHOWFOLDERS, TRUE);
    InterlockedExchange(&m_bBlock, FALSE);
    // revert the remote flag back to the default
    m_bRemote = !!(DWORD)CRegDWORD(L"Software\\TortoiseSVN\\CheckRepo", FALSE);
    RefreshCursor();
    return 0;
}

void CChangedDlg::OnOK()
{
    if (m_bBlock)
    {
        m_bCanceled = true;
        return;
    }
    __super::OnOK();
}

void CChangedDlg::OnCancel()
{
    if (m_bBlock)
    {
        m_bCanceled = true;
        return;
    }
    __super::OnCancel();
}

void CChangedDlg::OnBnClickedCheckrepo()
{
    m_bRemote = TRUE;
    m_bDepthInfinity = (GetKeyState(VK_SHIFT)&0x8000) != 0;
    OnBnClickedRefresh();
}

DWORD CChangedDlg::UpdateShowFlags()
{
    DWORD dwShow = m_FileListCtrl.GetShowFlags();
    if (m_bShowUnversioned)
        dwShow |= SVNSLC_SHOWUNVERSIONED;
    else
        dwShow &= ~SVNSLC_SHOWUNVERSIONED;
    if (m_iShowUnmodified)
        dwShow |= SVNSLC_SHOWNORMAL;
    else
        dwShow &= ~SVNSLC_SHOWNORMAL;
    if (m_bShowIgnored)
        dwShow |= SVNSLC_SHOWIGNORED;
    else
        dwShow &= ~SVNSLC_SHOWIGNORED;
    if (m_bShowExternals)
        dwShow |= SVNSLC_SHOWEXTERNAL | SVNSLC_SHOWINEXTERNALS | SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO;
    else
        dwShow &= ~(SVNSLC_SHOWEXTERNAL | SVNSLC_SHOWINEXTERNALS | SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO);

    return dwShow;
}

void CChangedDlg::OnBnClickedShowunversioned()
{
    UpdateData();
    CWaitCursor wait;
    m_FileListCtrl.Show(UpdateShowFlags(), CTSVNPathList(), 0, !!m_bShowDirs, !!m_bShowFiles);
    m_regAddBeforeCommit = m_bShowUnversioned;
    UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowUnmodified()
{
    UpdateData();
    CWaitCursor wait;
    m_FileListCtrl.Show(UpdateShowFlags(), CTSVNPathList(), 0, !!m_bShowDirs, !!m_bShowFiles);
    m_regAddBeforeCommit = m_bShowUnversioned;
    UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowignored()
{
    UpdateData();
    OnBnClickedRefresh();
}

void CChangedDlg::OnBnClickedShowexternals()
{
    UpdateData();
    m_FileListCtrl.Show(UpdateShowFlags(), CTSVNPathList(), 0, !!m_bShowDirs, !!m_bShowFiles);
    UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowUserProps()
{
    UpdateData();
    m_bShowPropertiesClicked = true;
    m_regShowUserProps = m_bShowUserProps;
    OnBnClickedRefresh();
}

void CChangedDlg::OnBnClickedShowfolders()
{
    UpdateData();
    CWaitCursor wait;
    m_FileListCtrl.Show(UpdateShowFlags(), CTSVNPathList(), 0, !!m_bShowDirs, !!m_bShowFiles);
    UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowfiles()
{
    UpdateData();
    CWaitCursor wait;
    m_FileListCtrl.Show(UpdateShowFlags(), CTSVNPathList(), 0, !!m_bShowDirs, !!m_bShowFiles);
    UpdateStatistics();
}

LRESULT CChangedDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    OnBnClickedRefresh();
    return 0;
}

LRESULT CChangedDlg::OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
    UpdateStatistics();
    return 0;
}

BOOL CChangedDlg::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_F5:
            {
                if (m_bBlock)
                    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
                OnBnClickedRefresh();
            }
            break;
        case VK_RETURN:
            if(OnEnterPressed())
                return TRUE;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CChangedDlg::OnBnClickedRefresh()
{
    if (!m_bBlock)
    {
        if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
        {
            OnCantStartThread();
        }
    }
}

void CChangedDlg::UpdateStatistics()
{
    LONG lMin, lMax;
    CString temp;
    m_FileListCtrl.GetMinMaxRevisions(lMin, lMax, true, false);
    if (LONG(m_FileListCtrl.m_HeadRev) >= 0)
    {
        temp.FormatMessage(IDS_REPOSTATUS_HEADREV, lMin, lMax, LONG(m_FileListCtrl.m_HeadRev));
        SetDlgItemText(IDC_SUMMARYTEXT, temp);
    }
    else
    {
        temp.FormatMessage(IDS_REPOSTATUS_WCINFO, lMin, lMax);
        SetDlgItemText(IDC_SUMMARYTEXT, temp);
    }
    GetDlgItem(IDC_SUMMARYTEXT)->Invalidate();
    temp = m_FileListCtrl.GetStatisticsString();
    temp.Replace(L" = ", L"=");
    temp.Replace(L"\n", L", ");
    SetDlgItemText(IDC_INFOLABEL, temp);
    GetDlgItem(IDC_INFOLABEL)->Invalidate();
}


BOOL CChangedDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (m_bBlock)
    {
        HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
        SetCursor(hCur);
        return TRUE;
    }
    HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    SetCursor(hCur);
    return __super::OnSetCursor(pWnd, nHitTest, message);
}
