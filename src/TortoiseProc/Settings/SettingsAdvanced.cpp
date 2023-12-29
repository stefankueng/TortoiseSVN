// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2015, 2018-2023 - TortoiseSVN
// Copyright (C) 2023 - TortoiseGit

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
#include "SettingsAdvanced.h"
#include "registry.h"

IMPLEMENT_DYNAMIC(CSettingsAdvanced, ISettingsPropPage)

CSettingsAdvanced::CSettingsAdvanced()
    : ISettingsPropPage(CSettingsAdvanced::IDD)
{
    AddSetting<BooleanSetting>(L"AllowAuthSave", true);
    AddSetting<BooleanSetting>(L"AllowUnversionedObstruction", true);
    AddSetting<BooleanSetting>(L"AlwaysExtendedMenu", false);
    AddSetting<DWORDSetting>  (L"AutoCompleteMinChars", 3);
    AddSetting<BooleanSetting>(L"AutocompleteRemovesExtensions", false);
    AddSetting<BooleanSetting>(L"BlockPeggedExternals", true);
    AddSetting<BooleanSetting>(L"BlockStatus", false);
    AddSetting<BooleanSetting>(L"CacheTrayIcon", false);
    AddSetting<BooleanSetting>(L"ColumnsEveryWhere", false);
    AddSetting<StringSetting> (L"ConfigDir", L"");
    AddSetting<BooleanSetting>(L"CtrlEnter", true);
    AddSetting<BooleanSetting>(L"Debug", false);
    AddSetting<BooleanSetting>(L"DebugOutputString", false);
    AddSetting<DWORDSetting>  (L"DialogTitles", 0);
    AddSetting<BooleanSetting>(L"DiffBlamesWithTortoiseMerge", false);
    AddSetting<DWORDSetting>  (L"DlgStickySize", 3);
    AddSetting<BooleanSetting>(L"FixCaseRenames", true);
    AddSetting<BooleanSetting>(L"FullRowSelect", true);
    AddSetting<DWORDSetting>  (L"GroupTaskbarIconsPerRepo", 3);
    AddSetting<BooleanSetting>(L"GroupTaskbarIconsPerRepoOverlay", true);
    AddSetting<BooleanSetting>(L"HideExternalInfo", true);
    AddSetting<BooleanSetting>(L"HookCancelError", false);
    AddSetting<BooleanSetting>(L"IncludeExternals", true);
    AddSetting<BooleanSetting>(L"LogFindCopyFrom", true);
    AddSetting<StringSetting> (L"LogMultiRevFormat", L"r%1!ld!\n%2!s!\n---------------------\n");
    AddSetting<BooleanSetting>(L"LogStatusCheck", true);
    AddSetting<DWORDSetting>  (L"MaxHistoryComboItems", 25);
    AddSetting<StringSetting> (L"MergeLogSeparator", L"........");
    AddSetting<BooleanSetting>(L"MergeAllowMixedRevisionsDefault", false);
    AddSetting<DWORDSetting>  (L"NumDiffWarning", 10);
    AddSetting<BooleanSetting>(L"OldVersionCheck", false);
    AddSetting<DWORDSetting>  (L"OutOfDateRetry", 1);
    AddSetting<BooleanSetting>(L"PlaySound", true);
    AddSetting<BooleanSetting>(L"RepoBrowserTrySVNParentPath", true);
    AddSetting<BooleanSetting>(L"ScintillaBidirectional", false);
    AddSetting<BooleanSetting>(L"ScintillaDirect2D", true);
    AddSetting<BooleanSetting>(L"ShellMenuAccelerators", true);
    AddSetting<BooleanSetting>(L"ShowContextMenuIcons", true);
    AddSetting<BooleanSetting>(L"ShowAppContextMenuIcons", true);
    AddSetting<BooleanSetting>(L"ShowNotifications", true);
    AddSetting<BooleanSetting>(L"StyleCommitMessages", true);
    AddSetting<StringSetting> (L"UpdateCheckURL", L"");
    AddSetting<DWORDSetting>  (L"UseCustomWordBreak", 2);
    AddSetting<BooleanSetting>(L"VersionCheck", true);
}

CSettingsAdvanced::~CSettingsAdvanced()
{
}

void CSettingsAdvanced::DoDataExchange(CDataExchange *pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CONFIG, m_listCtrl);
}

BEGIN_MESSAGE_MAP(CSettingsAdvanced, ISettingsPropPage)
    ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_CONFIG, &CSettingsAdvanced::OnLvnBeginlabeledit)
    ON_NOTIFY(LVN_ENDLABELEDIT, IDC_CONFIG, &CSettingsAdvanced::OnLvnEndlabeledit)
    ON_NOTIFY(NM_DBLCLK, IDC_CONFIG, &CSettingsAdvanced::OnNMDblclkConfig)
END_MESSAGE_MAP()

BOOL CSettingsAdvanced::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_listCtrl.DeleteAllItems();
    int c = m_listCtrl.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_listCtrl.DeleteColumn(c--);

    SetWindowTheme(m_listCtrl.GetSafeHwnd(), L"Explorer", nullptr);

    CString temp;
    temp.LoadString(IDS_SETTINGS_CONF_VALUECOL);
    m_listCtrl.InsertColumn(0, temp);
    temp.LoadString(IDS_SETTINGS_CONF_NAMECOL);
    m_listCtrl.InsertColumn(1, temp);

    m_listCtrl.SetRedraw(FALSE);

    for (int i = 0; i < static_cast<int>(settings.size()); ++i)
    {
        m_listCtrl.InsertItem(i, settings.at(i)->GetName());
        m_listCtrl.SetItemText(i, 1, settings.at(i)->GetName());
        m_listCtrl.SetItemText(i, 0, settings.at(i)->GetValue());
    }

    int minCol = 0;
    int maxCol = m_listCtrl.GetHeaderCtrl()->GetItemCount() - 1;
    for (int col = minCol; col <= maxCol; col++)
    {
        m_listCtrl.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
    }
    int arr[2] = {1, 0};
    m_listCtrl.SetColumnOrderArray(2, arr);
    m_listCtrl.SetRedraw(TRUE);

    return TRUE;
}

BOOL CSettingsAdvanced::OnApply()
{
    for (int i = 0; i < static_cast<int>(settings.size()); ++i)
    {
        settings.at(i)->StoreValue(m_listCtrl.GetItemText(i, 0));
    }

    return ISettingsPropPage::OnApply();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CSettingsAdvanced::OnLvnBeginlabeledit(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    *pResult = FALSE;
}

void CSettingsAdvanced::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO *>(pNMHDR);
    *pResult                = 0;
    if (pDispInfo->item.pszText == nullptr)
        return;

    bool allowEdit = settings.at(pDispInfo->item.iItem)->IsValid(pDispInfo->item.pszText);

    if (allowEdit)
        SetModified();

    *pResult = allowEdit ? TRUE : FALSE;
}

BOOL CSettingsAdvanced::PreTranslateMessage(MSG *pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
            case VK_F2:
            {
                m_listCtrl.EditLabel(m_listCtrl.GetSelectionMark());
            }
            break;
        }
    }
    return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSettingsAdvanced::OnNMDblclkConfig(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    m_listCtrl.EditLabel(pNMItemActivate->iItem);
    *pResult = 0;
}