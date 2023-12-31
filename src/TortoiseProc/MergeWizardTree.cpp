﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010, 2012-2014, 2020-2022 - TortoiseSVN

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
#include "MergeWizard.h"
#include "MergeWizardTree.h"

#include "AppUtils.h"
#include "PathUtils.h"
#include "LogDialog/LogDlg.h"
#include "Theme.h"

IMPLEMENT_DYNAMIC(CMergeWizardTree, CMergeWizardBasePage)

CMergeWizardTree::CMergeWizardTree()
    : CMergeWizardBasePage(CMergeWizardTree::IDD)
    , m_pLogDlg(nullptr)
    , m_pLogDlg2(nullptr)
    , m_pLogDlg3(nullptr)
    , m_startRev(0)
    , m_endRev(L"HEAD")
{
    m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    m_psp.pszHeaderTitle    = MAKEINTRESOURCE(IDS_MERGEWIZARD_TREETITLE);
    m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_TREESUBTITLE);
}

CMergeWizardTree::~CMergeWizardTree()
{
    if (m_pLogDlg)
    {
        m_pLogDlg->DestroyWindow();
        delete m_pLogDlg;
    }
    if (m_pLogDlg2)
    {
        m_pLogDlg2->DestroyWindow();
        delete m_pLogDlg2;
    }
    if (m_pLogDlg3)
    {
        m_pLogDlg3->DestroyWindow();
        delete m_pLogDlg3;
    }
}

void CMergeWizardTree::DoDataExchange(CDataExchange* pDX)
{
    CMergeWizardBasePage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLCOMBO, m_urlCombo);
    DDX_Text(pDX, IDC_REVISION_START, m_sStartRev);
    DDX_Text(pDX, IDC_REVISION_END, m_sEndRev);
    DDX_Control(pDX, IDC_URLCOMBO2, m_urlCombo2);
    DDX_Control(pDX, IDC_WCEDIT, m_wc);
}

BEGIN_MESSAGE_MAP(CMergeWizardTree, CMergeWizardBasePage)
    ON_MESSAGE(WM_TSVN_MAXREVFOUND, &CMergeWizardTree::OnWCStatus)
    ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
    ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_BROWSE2, OnBnClickedBrowse2)
    ON_BN_CLICKED(IDC_FINDBRANCHSTART, OnBnClickedFindbranchstart)
    ON_BN_CLICKED(IDC_FINDBRANCHEND, OnBnClickedFindbranchend)
    ON_EN_CHANGE(IDC_REVISION_END, &CMergeWizardTree::OnEnChangeRevisionEnd)
    ON_EN_CHANGE(IDC_REVISION_START, &CMergeWizardTree::OnEnChangeRevisionStart)
    ON_BN_CLICKED(IDC_SHOWLOGWC, &CMergeWizardTree::OnBnClickedShowlogwc)
    ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CMergeWizardTree::OnCbnEditchangeUrlcombo)
    ON_CBN_EDITCHANGE(IDC_URLCOMBO2, &CMergeWizardTree::OnCbnEditchangeUrlcombo2)
END_MESSAGE_MAP()

BOOL CMergeWizardTree::OnInitDialog()
{
    CMergeWizardBasePage::OnInitDialog();

    CMergeWizard* pWizard = static_cast<CMergeWizard*>(GetParent());
    CString       sUuid   = pWizard->m_sUuid;
    m_urlCombo.SetURLHistory(true, false);
    m_urlCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoURLS\\" + sUuid, L"url");
    m_urlCombo2.SetURLHistory(true, false);
    m_urlCombo2.LoadHistory(L"Software\\TortoiseSVN\\History\\repoURLS\\" + sUuid, L"url");
    m_urlCombo2.SetCurSel(0);

    CString sRegKeyFrom        = L"Software\\TortoiseSVN\\History\\repoURLS\\MergeURLForFrom" + static_cast<CMergeWizard*>(GetParent())->m_wcPath.GetSVNPathString();
    CString sMergeUrlForWCFrom = CRegString(sRegKeyFrom);
    CString sRegKeyTo          = L"Software\\TortoiseSVN\\History\\repoURLS\\MergeURLForTo" + static_cast<CMergeWizard*>(GetParent())->m_wcPath.GetSVNPathString();
    CString sMergeUrlForWCTo   = CRegString(sRegKeyTo);

    if (!static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\MergeWCURL", FALSE)))
        m_urlCombo.SetCurSel(0);
    else
    {
        if (!sMergeUrlForWCFrom.IsEmpty())
            m_urlCombo.SetWindowText(CPathUtils::PathUnescape(sMergeUrlForWCFrom));
        if (!sMergeUrlForWCTo.IsEmpty())
            m_urlCombo2.SetWindowText(CPathUtils::PathUnescape(sMergeUrlForWCTo));
    }
    // Only set the "From" Url if there is no url history available
    if (m_urlCombo.GetString().IsEmpty())
        m_urlCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->m_url));
    GetDlgItem(IDC_BROWSE)->EnableWindow(!m_urlCombo.GetString().IsEmpty());
    if (m_urlCombo2.GetString().IsEmpty())
        m_urlCombo2.SetWindowText(CPathUtils::PathUnescape(pWizard->m_url));
    if (!pWizard->m_url1.IsEmpty())
        m_urlCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->m_url1));
    if (!pWizard->m_url2.IsEmpty())
        m_urlCombo2.SetWindowText(CPathUtils::PathUnescape(pWizard->m_url2));
    GetDlgItem(IDC_BROWSE2)->EnableWindow(!m_urlCombo2.GetString().IsEmpty());

    SetDlgItemText(IDC_WCEDIT, static_cast<CMergeWizard*>(GetParent())->m_wcPath.GetWinPath());

    // set head revision as default revision
    if (pWizard->m_startRev.IsHead() || !pWizard->m_startRev.IsValid())
        CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_HEAD1);
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
        m_sStartRev = pWizard->m_startRev.ToString();
        SetDlgItemText(IDC_REVISION_START, m_sStartRev);
    }
    if (pWizard->m_endRev.IsHead() || !pWizard->m_endRev.IsValid())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
        m_sEndRev = pWizard->m_endRev.ToString();
        SetDlgItemText(IDC_REVISION_END, m_sEndRev);
    }

    AdjustControlSize(IDC_REVISION_HEAD1);
    AdjustControlSize(IDC_REVISION_N1);
    AdjustControlSize(IDC_REVISION_HEAD);
    AdjustControlSize(IDC_REVISION_N);

    AddAnchor(IDC_MERGETREEFROMGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_REVISION_HEAD1, TOP_LEFT);
    AddAnchor(IDC_REVISION_N1, TOP_LEFT);
    AddAnchor(IDC_REVISION_START, TOP_LEFT);
    AddAnchor(IDC_FINDBRANCHSTART, TOP_LEFT);
    AddAnchor(IDC_MERGETREETOGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLCOMBO2, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE2, TOP_RIGHT);
    AddAnchor(IDC_REVISION_HEAD, TOP_LEFT);
    AddAnchor(IDC_REVISION_N, TOP_LEFT);
    AddAnchor(IDC_REVISION_END, TOP_LEFT);
    AddAnchor(IDC_FINDBRANCHEND, TOP_LEFT);
    AddAnchor(IDC_MERGETREEWCGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_WCEDIT, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SHOWLOGWC, TOP_RIGHT);

    CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), CTheme::Instance().IsDarkTheme());

    StartWCCheckThread(static_cast<CMergeWizard*>(GetParent())->m_wcPath);

    return TRUE;
}

BOOL CMergeWizardTree::CheckData(bool bShowErrors /* = true */)
{
    if (!UpdateData(TRUE))
        return FALSE;

    m_startRev = SVNRev(m_sStartRev);
    m_endRev   = SVNRev(m_sEndRev);
    if (GetCheckedRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1) == IDC_REVISION_HEAD1)
    {
        m_startRev = SVNRev(L"HEAD");
    }
    if (!m_startRev.IsValid())
    {
        if (bShowErrors)
            ShowEditBalloon(IDC_REVISION_START, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        return FALSE;
    }

    // if head revision, set revision as -1
    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
    {
        m_endRev = SVNRev(L"HEAD");
    }
    if (!m_endRev.IsValid())
    {
        if (bShowErrors)
            ShowEditBalloon(IDC_REVISION_END, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        return FALSE;
    }

    CString sUrl;

    m_urlCombo.GetWindowText(sUrl);
    auto newlinePos = sUrl.FindOneOf(L"\r\n");
    if (newlinePos >= 0)
        sUrl = sUrl.Left(newlinePos);
    CTSVNPath url(sUrl);
    if (!url.IsUrl())
    {
        ShowComboBalloon(&m_urlCombo, IDS_ERR_MUSTBEURL, IDS_ERR_ERROR, TTI_ERROR);
        return FALSE;
    }

    m_urlCombo.SaveHistory();
    m_urlFrom = sUrl;

    m_urlCombo2.GetWindowText(sUrl);
    newlinePos = sUrl.FindOneOf(L"\r\n");
    if (newlinePos >= 0)
        sUrl = sUrl.Left(newlinePos);
    CTSVNPath url2(sUrl);
    if (!url2.IsUrl())
    {
        ShowComboBalloon(&m_urlCombo2, IDS_ERR_MUSTBEURL, IDS_ERR_ERROR, TTI_ERROR);
        return FALSE;
    }

    m_urlCombo2.SaveHistory();
    m_urlTo = sUrl;

    CString    sRegKeyFrom = L"Software\\TortoiseSVN\\History\\repoURLS\\MergeURLForFrom" + static_cast<CMergeWizard*>(GetParent())->m_wcPath.GetSVNPathString();
    CRegString regMergeUrlForWCFrom(sRegKeyFrom);
    regMergeUrlForWCFrom = m_urlFrom;
    CString    sRegKeyTo = L"Software\\TortoiseSVN\\History\\repoURLS\\MergeURLForTo" + static_cast<CMergeWizard*>(GetParent())->m_wcPath.GetSVNPathString();
    CRegString regMergeUrlForWCTo(sRegKeyTo);
    regMergeUrlForWCTo = m_urlTo;

    static_cast<CMergeWizard*>(GetParent())->m_url1     = m_urlFrom;
    static_cast<CMergeWizard*>(GetParent())->m_url2     = m_urlTo;
    static_cast<CMergeWizard*>(GetParent())->m_startRev = m_startRev;
    static_cast<CMergeWizard*>(GetParent())->m_endRev   = m_endRev;

    UpdateData(FALSE);
    return TRUE;
}

void CMergeWizardTree::OnEnChangeRevisionEnd()
{
    UpdateData();
    if (m_sEndRev.IsEmpty())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CMergeWizardTree::OnEnChangeRevisionStart()
{
    UpdateData();
    if (m_sStartRev.IsEmpty())
        CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_HEAD1);
    else
        CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
}

void CMergeWizardTree::SetStartRevision(const SVNRev& rev)
{
    if (rev.IsHead())
        CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_HEAD1);
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
        m_sStartRev = rev.ToString();
        UpdateData(FALSE);
    }
}

void CMergeWizardTree::SetEndRevision(const SVNRev& rev)
{
    if (rev.IsHead())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
        m_sEndRev = rev.ToString();
        UpdateData(FALSE);
    }
}

void CMergeWizardTree::OnBnClickedBrowse()
{
    CheckData(false);
    if ((!m_startRev.IsValid()) || (m_startRev == 0))
        m_startRev = SVNRev::REV_HEAD;
    if (CAppUtils::BrowseRepository(m_urlCombo, this, m_startRev))
    {
        SetStartRevision(m_startRev);
    }
}

void CMergeWizardTree::OnBnClickedBrowse2()
{
    CheckData(false);

    if ((!m_endRev.IsValid()) || (m_endRev == 0))
        m_endRev = SVNRev::REV_HEAD;

    CAppUtils::BrowseRepository(m_urlCombo2, this, m_endRev);
    SetEndRevision(m_endRev);
}

void CMergeWizardTree::OnBnClickedFindbranchstart()
{
    CheckData(false);
    if ((!m_startRev.IsValid()) || (m_startRev == 0))
        m_startRev = SVNRev::REV_HEAD;
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
        return;
    CString sUrl;
    m_urlCombo.GetWindowText(sUrl);
    auto newlinePos = sUrl.FindOneOf(L"\r\n");
    if (newlinePos >= 0)
        sUrl = sUrl.Left(newlinePos);
    // now show the log dialog for the main trunk
    CTSVNPath url(sUrl);
    if (!url.IsEmpty() && url.IsUrl())
    {
        StopWCCheckThread();
        CTSVNPath wcPath = static_cast<CMergeWizard*>(GetParent())->m_wcPath;
        if (m_pLogDlg)
        {
            m_pLogDlg->DestroyWindow();
            delete m_pLogDlg;
        }
        m_pLogDlg           = new CLogDlg();
        m_pLogDlg->m_wParam = MERGE_REVSELECTSTART;
        m_pLogDlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTSTARTREVISION)));
        m_pLogDlg->SetSelect(true);
        m_pLogDlg->m_pNotifyWindow = this;
        m_pLogDlg->SetParams(url, m_startRev, m_startRev, 1, TRUE, FALSE);
        m_pLogDlg->SetProjectPropertiesPath(wcPath);
        m_pLogDlg->ContinuousSelection(true);
        m_pLogDlg->SetMergePath(wcPath);
        m_pLogDlg->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg->ShowWindow(SW_SHOW);
    }
}

void CMergeWizardTree::OnBnClickedFindbranchend()
{
    CheckData(false);

    if ((!m_endRev.IsValid()) || (m_endRev == 0))
        m_endRev = SVNRev::REV_HEAD;
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd()) && (m_pLogDlg2->IsWindowVisible()))
        return;
    CString sUrl;

    m_urlCombo2.GetWindowText(sUrl);
    //now show the log dialog for the main trunk
    CTSVNPath url(sUrl);
    if (!url.IsEmpty() && url.IsUrl())
    {
        StopWCCheckThread();
        CTSVNPath wcPath = static_cast<CMergeWizard*>(GetParent())->m_wcPath;
        if (m_pLogDlg2)
        {
            m_pLogDlg2->DestroyWindow();
            delete m_pLogDlg2;
        }
        m_pLogDlg2           = new CLogDlg();
        m_pLogDlg2->m_wParam = MERGE_REVSELECTEND;
        m_pLogDlg2->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTENDREVISION)));
        m_pLogDlg2->SetSelect(true);
        m_pLogDlg2->m_pNotifyWindow = this;
        m_pLogDlg2->SetProjectPropertiesPath(wcPath);
        m_pLogDlg2->SetParams(url, m_endRev, m_endRev, 1, TRUE, FALSE);
        m_pLogDlg2->ContinuousSelection(true);
        m_pLogDlg2->SetMergePath(wcPath);
        m_pLogDlg2->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg2->ShowWindow(SW_SHOW);
    }
}

LPARAM CMergeWizardTree::OnRevSelected(WPARAM wParam, LPARAM lParam)
{
    CString temp;

    if (wParam & MERGE_REVSELECTSTART)
    {
        if (wParam & MERGE_REVSELECTMINUSONE)
            lParam--;
        temp.Format(L"%Id", lParam);
        SetDlgItemText(IDC_REVISION_START, temp);
        CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
    }
    if (wParam & MERGE_REVSELECTEND)
    {
        temp.Format(L"%Id", lParam);
        SetDlgItemText(IDC_REVISION_END, temp);
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    }
    return 0;
}

LRESULT CMergeWizardTree::OnWizardNext()
{
    StopWCCheckThread();

    if (!CheckData(true))
        return -1;

    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return -1;
    }
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd()) && (m_pLogDlg2->IsWindowVisible()))
    {
        m_pLogDlg2->SendMessage(WM_CLOSE);
        return -1;
    }
    if (::IsWindow(m_pLogDlg3->GetSafeHwnd()) && (m_pLogDlg3->IsWindowVisible()))
    {
        m_pLogDlg3->SendMessage(WM_CLOSE);
        return -1;
    }

    return IDD_MERGEWIZARD_OPTIONS;
}

LRESULT CMergeWizardTree::OnWizardBack()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return -1;
    }
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd()) && (m_pLogDlg2->IsWindowVisible()))
    {
        m_pLogDlg2->SendMessage(WM_CLOSE);
        return -1;
    }
    if (::IsWindow(m_pLogDlg3->GetSafeHwnd()) && (m_pLogDlg3->IsWindowVisible()))
    {
        m_pLogDlg3->SendMessage(WM_CLOSE);
        return -1;
    }

    return IDD_MERGEWIZARD_START;
}

BOOL CMergeWizardTree::OnSetActive()
{
    CPropertySheet* pSheet = static_cast<CPropertySheet*>(GetParent());
    pSheet->SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
    SetButtonTexts();

    return CMergeWizardBasePage::OnSetActive();
}

void CMergeWizardTree::OnBnClickedShowlogwc()
{
    StopWCCheckThread();
    CTSVNPath wcPath = static_cast<CMergeWizard*>(GetParent())->m_wcPath;
    if (m_pLogDlg3)
        m_pLogDlg3->DestroyWindow();
    delete m_pLogDlg3;
    m_pLogDlg3 = new CLogDlg();
    m_pLogDlg3->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));

    m_pLogDlg3->m_pNotifyWindow = nullptr;
    m_pLogDlg3->SetParams(wcPath, SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, TRUE, FALSE);
    m_pLogDlg3->SetProjectPropertiesPath(wcPath);
    m_pLogDlg3->SetMergePath(wcPath);
    m_pLogDlg3->Create(IDD_LOGMESSAGE, this);
    m_pLogDlg3->ShowWindow(SW_SHOW);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMergeWizardTree::OnCbnEditchangeUrlcombo()
{
    GetDlgItem(IDC_BROWSE)->EnableWindow(!m_urlCombo.GetString().IsEmpty());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMergeWizardTree::OnCbnEditchangeUrlcombo2()
{
    GetDlgItem(IDC_BROWSE2)->EnableWindow(!m_urlCombo2.GetString().IsEmpty());
}

LPARAM CMergeWizardTree::OnWCStatus(WPARAM wParam, LPARAM /*lParam*/)
{
    if (wParam)
    {
        ShowEditBalloon(IDC_WCEDIT, IDS_MERGE_WCDIRTY, IDS_WARN_WARNING, TTI_WARNING);
    }
    return 0;
}

bool CMergeWizardTree::OkToCancel()
{
    StopWCCheckThread();
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return false;
    }
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd()) && (m_pLogDlg2->IsWindowVisible()))
    {
        m_pLogDlg2->SendMessage(WM_CLOSE);
        return false;
    }
    if (::IsWindow(m_pLogDlg3->GetSafeHwnd()) && (m_pLogDlg3->IsWindowVisible()))
    {
        m_pLogDlg3->SendMessage(WM_CLOSE);
        return false;
    }
    return true;
}
