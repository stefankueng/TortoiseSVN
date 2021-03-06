// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2020-2021 - TortoiseSVN

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
#include "SetMainPage.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "DirFileEnum.h"
#include "SVNProgressDlg.h"
#include "../version.h"
#include "SVN.h"
#include "Libraries.h"
#include "CreateProcessHelper.h"

IMPLEMENT_DYNAMIC(CSetMainPage, ISettingsPropPage)
CSetMainPage::CSetMainPage()
    : ISettingsPropPage(CSetMainPage::IDD)
    , m_dwLanguage(0)
    , m_bLastCommitTime(FALSE)
    , m_bUseAero(TRUE)
{
    m_regLanguage = CRegDWORD(L"Software\\TortoiseSVN\\LanguageID", 1033);
    CString temp(SVN_CONFIG_DEFAULT_GLOBAL_IGNORES);
    m_regExtensions     = CRegString(L"Software\\Tigris.org\\Subversion\\Config\\miscellany\\global-ignores", temp);
    m_regLastCommitTime = CRegString(L"Software\\Tigris.org\\Subversion\\Config\\miscellany\\use-commit-times", L"");
    m_regUseAero        = CRegDWORD(L"Software\\TortoiseSVN\\EnableDWMFrame", TRUE);
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LANGUAGECOMBO, m_languageCombo);
    m_dwLanguage = static_cast<DWORD>(m_languageCombo.GetItemData(m_languageCombo.GetCurSel()));
    DDX_Text(pDX, IDC_TEMPEXTENSIONS, m_sTempExtensions);
    DDX_Check(pDX, IDC_COMMITFILETIMES, m_bLastCommitTime);
    DDX_Check(pDX, IDC_AERODWM, m_bUseAero);
}

BEGIN_MESSAGE_MAP(CSetMainPage, ISettingsPropPage)
    ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnModified)
    ON_EN_CHANGE(IDC_TEMPEXTENSIONS, OnModified)
    ON_BN_CLICKED(IDC_EDITCONFIG, OnBnClickedEditconfig)
    ON_BN_CLICKED(IDC_CHECKNEWERVERSION, OnModified)
    ON_BN_CLICKED(IDC_CHECKNEWERBUTTON, OnBnClickedChecknewerbutton)
    ON_BN_CLICKED(IDC_COMMITFILETIMES, OnModified)
    ON_BN_CLICKED(IDC_SOUNDS, OnBnClickedSounds)
    ON_BN_CLICKED(IDC_AERODWM, OnModified)
    ON_BN_CLICKED(IDC_CREATELIB, &CSetMainPage::OnBnClickedCreatelib)
END_MESSAGE_MAP()

BOOL CSetMainPage::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    EnableToolTips();

    m_sTempExtensions = m_regExtensions;
    m_dwLanguage      = m_regLanguage;
    m_bUseAero        = m_regUseAero;
    HIGHCONTRAST hc   = {sizeof(HIGHCONTRAST)};
    SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, FALSE);
    BOOL bEnabled = FALSE;
    DialogEnableWindow(IDC_AERODWM, ((hc.dwFlags & HCF_HIGHCONTRASTON) == 0) && SUCCEEDED(DwmIsCompositionEnabled(&bEnabled)) && bEnabled);
    if (IsWindows10OrGreater())
        GetDlgItem(IDC_AERODWM)->ShowWindow(SW_HIDE);
    CString temp      = m_regLastCommitTime;
    m_bLastCommitTime = (temp.CompareNoCase(L"yes") == 0);

    m_tooltips.AddTool(IDC_TEMPEXTENSIONSLABEL, IDS_SETTINGS_TEMPEXTENSIONS_TT);
    m_tooltips.AddTool(IDC_TEMPEXTENSIONS, IDS_SETTINGS_TEMPEXTENSIONS_TT);
    m_tooltips.AddTool(IDC_COMMITFILETIMES, IDS_SETTINGS_COMMITFILETIMES_TT);
    m_tooltips.AddTool(IDC_CREATELIB, IDS_SETTINGS_CREATELIB_TT);

    DialogEnableWindow(IDC_CREATELIB, TRUE);

    // set up the language selecting combobox
    wchar_t buf[MAX_PATH] = {0};
    GetLocaleInfo(1033, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
    m_languageCombo.AddString(buf);
    m_languageCombo.SetItemData(0, 1033);
    CString path = CPathUtils::GetAppParentDirectory();
    path         = path + L"Languages\\";
    CSimpleFileFind finder(path, L"*.dll");
    int             langCount = 1;
    while (finder.FindNextFileNoDirectories())
    {
        CString file     = finder.GetFilePath();
        CString filename = finder.GetFileName();
        if (filename.Left(12).CompareNoCase(L"TortoiseProc") == 0)
        {
            CString sVer     = _T(STRPRODUCTVER);
            sVer             = sVer.Left(sVer.ReverseFind('.'));
            CString sFileVer = CPathUtils::GetVersionFromFile(file).c_str();
            sFileVer         = sFileVer.Left(sFileVer.ReverseFind('.'));
            if (sFileVer.Compare(sVer) != 0)
                continue;
            CString sLoc = filename.Mid(12);
            sLoc         = sLoc.Left(sLoc.GetLength() - 4); // cut off ".dll"
            if ((sLoc.Left(2) == L"32") && (sLoc.GetLength() > 5))
                continue;
            DWORD loc = _tstoi(filename.Mid(12));
            GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
            CString sLang = buf;
            GetLocaleInfo(loc, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
            if (buf[0])
            {
                sLang += L" (";
                sLang += buf;
                sLang += L")";
            }
            m_languageCombo.AddString(sLang);
            m_languageCombo.SetItemData(langCount++, loc);
        }
    }

    for (int i = 0; i < m_languageCombo.GetCount(); i++)
    {
        if (m_languageCombo.GetItemData(i) == m_dwLanguage)
            m_languageCombo.SetCurSel(i);
    }

    UpdateData(FALSE);
    return TRUE;
}

void CSetMainPage::OnModified()
{
    SetModified();
}

BOOL CSetMainPage::OnApply()
{
    UpdateData();
    Store(m_dwLanguage, m_regLanguage);
    if (m_sTempExtensions.Compare(CString(m_regExtensions)))
    {
        Store(m_sTempExtensions, m_regExtensions);
        m_restart = Restart_Cache;
    }
    Store((m_bLastCommitTime ? L"yes" : L"no"), m_regLastCommitTime);
    Store(m_bUseAero, m_regUseAero);

    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CSetMainPage::OnBnClickedEditconfig()
{
    SVN::EnsureConfigFile();

    PWSTR pszPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &pszPath) == S_OK)
    {
        CString path = pszPath;
        CoTaskMemFree(pszPath);

        path += L"\\Subversion\\config";
        CAppUtils::StartTextViewer(path);
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CSetMainPage::OnBnClickedChecknewerbutton()
{
    wchar_t com[MAX_PATH + 100] = {0};
    GetModuleFileName(nullptr, com, MAX_PATH);

    CCreateProcessHelper::CreateProcessDetached(com, L" /command:updatecheck /visible");
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CSetMainPage::OnBnClickedSounds()
{
    CAppUtils::LaunchApplication(L"RUNDLL32 Shell32,Control_RunDLL mmsys.cpl,,2", NULL, false);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CSetMainPage::OnBnClickedCreatelib()
{
    CoInitialize(nullptr);
    EnsureSVNLibrary();
    CoUninitialize();
}
