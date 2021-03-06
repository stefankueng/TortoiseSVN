// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2013, 2020-2021 - TortoiseSVN

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
#include "MergeWizardStart.h"
#include "AppUtils.h"
#include "Theme.h"

IMPLEMENT_DYNAMIC(CMergeWizardStart, CMergeWizardBasePage)

CMergeWizardStart::CMergeWizardStart()
    : CMergeWizardBasePage(CMergeWizardStart::IDD)
{
    m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    m_psp.pszHeaderTitle    = MAKEINTRESOURCE(IDS_MERGEWIZARD_STARTTITLE);
    m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_STARTSUBTITLE);
}

CMergeWizardStart::~CMergeWizardStart()
{
}

void CMergeWizardStart::DoDataExchange(CDataExchange* pDX)
{
    CMergeWizardBasePage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMergeWizardStart, CMergeWizardBasePage)
END_MESSAGE_MAP()

LRESULT CMergeWizardStart::OnWizardNext()
{
    int nButton = GetCheckedRadioButton(IDC_MERGE_REVRANGE, IDC_MERGE_TREE);

    CMergeWizard* wiz = static_cast<CMergeWizard*>(GetParent());
    switch (nButton)
    {
        case IDC_MERGE_REVRANGE:
            wiz->m_nRevRangeMerge = MERGEWIZARD_REVRANGE;
            break;
        case IDC_MERGE_TREE:
            wiz->m_nRevRangeMerge = MERGEWIZARD_TREE;
            break;
    }

    wiz->SaveMode();

    return wiz->GetSecondPage();
}

BOOL CMergeWizardStart::OnInitDialog()
{
    CMergeWizardBasePage::OnInitDialog();

    CString sLabel;
    sLabel.LoadString(IDS_MERGEWIZARD_REVRANGELABEL);
    SetDlgItemText(IDC_MERGERANGELABEL, sLabel);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_MERGE_REVRANGE)->GetSafeHwnd(), PROPID_ACC_DESCRIPTION, sLabel);

    sLabel.LoadString(IDS_MERGEWIZARD_TREELABEL);
    SetDlgItemText(IDC_TREELABEL, sLabel);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_MERGE_TREE)->GetSafeHwnd(), PROPID_ACC_DESCRIPTION, sLabel);

    AdjustControlSize(IDC_MERGE_REVRANGE);
    AdjustControlSize(IDC_MERGE_TREE);

    AddAnchor(IDC_MERGETYPEGROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_MERGE_REVRANGE, TOP_LEFT);
    AddAnchor(IDC_MERGERANGELABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MERGE_TREE, TOP_LEFT);
    AddAnchor(IDC_TREELABEL, TOP_LEFT, TOP_RIGHT);

    CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), CTheme::Instance().IsDarkTheme());

    return TRUE;
}

BOOL CMergeWizardStart::OnSetActive()
{
    CMergeWizard* wiz = static_cast<CMergeWizard*>(GetParent());

    wiz->SetWizardButtons(PSWIZB_NEXT);
    SetButtonTexts();

    int nButton = IDC_MERGE_REVRANGE;
    switch (wiz->m_nRevRangeMerge)
    {
        case MERGEWIZARD_REVRANGE:
            nButton = IDC_MERGE_REVRANGE;
            break;
        case MERGEWIZARD_TREE:
            nButton = IDC_MERGE_TREE;
            break;
    }
    CheckRadioButton(
        IDC_MERGE_REVRANGE, IDC_MERGE_TREE,
        nButton);

    return CMergeWizardBasePage::OnSetActive();
}
