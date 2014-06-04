// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2013-2014 - TortoiseSVN

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
#include "SetOverlayPage.h"
#include "SetOverlayIcons.h"
#include "Globals.h"
#include "ShellUpdater.h"
#include "../TSVNCache/CacheInterface.h"
#include "SetOverlayPage.h"


IMPLEMENT_DYNAMIC(CSetOverlayPage, ISettingsPropPage)
CSetOverlayPage::CSetOverlayPage()
    : ISettingsPropPage(CSetOverlayPage::IDD)
    , m_bRemovable(FALSE)
    , m_bNetwork(FALSE)
    , m_bFixed(FALSE)
    , m_bCDROM(FALSE)
    , m_bRAM(FALSE)
    , m_bUnknown(FALSE)
    , m_bOnlyExplorer(FALSE)
    , m_sExcludePaths(L"")
    , m_sIncludePaths(L"")
    , m_bUnversionedAsModified(FALSE)
    , m_bIgnoreOnCommitIgnored(TRUE)
    , m_bFloppy(FALSE)
    , m_bShowExcludedAsNormal(TRUE)
{
    m_regOnlyExplorer = CRegDWORD(L"Software\\TortoiseSVN\\LoadDllOnlyInExplorer", FALSE);
    m_regDriveMaskRemovable = CRegDWORD(L"Software\\TortoiseSVN\\DriveMaskRemovable");
    m_regDriveMaskFloppy = CRegDWORD(L"Software\\TortoiseSVN\\DriveMaskFloppy");
    m_regDriveMaskRemote = CRegDWORD(L"Software\\TortoiseSVN\\DriveMaskRemote");
    m_regDriveMaskFixed = CRegDWORD(L"Software\\TortoiseSVN\\DriveMaskFixed", TRUE);
    m_regDriveMaskCDROM = CRegDWORD(L"Software\\TortoiseSVN\\DriveMaskCDROM");
    m_regDriveMaskRAM = CRegDWORD(L"Software\\TortoiseSVN\\DriveMaskRAM");
    m_regDriveMaskUnknown = CRegDWORD(L"Software\\TortoiseSVN\\DriveMaskUnknown");
    m_regExcludePaths = CRegString(L"Software\\TortoiseSVN\\OverlayExcludeList");
    m_regIncludePaths = CRegString(L"Software\\TortoiseSVN\\OverlayIncludeList");
    m_regCacheType = CRegDWORD(L"Software\\TortoiseSVN\\CacheType", GetSystemMetrics(SM_REMOTESESSION) ? 2 : 1);
    m_regUnversionedAsModified = CRegDWORD(L"Software\\TortoiseSVN\\UnversionedAsModified", FALSE);
    m_regIgnoreOnCommitIgnored = CRegDWORD(L"Software\\TortoiseSVN\\IgnoreOnCommitIgnored", TRUE);
    m_regShowExcludedAsNormal = CRegDWORD(L"Software\\TortoiseSVN\\ShowExcludedFoldersAsNormal", FALSE);

    m_bOnlyExplorer = m_regOnlyExplorer;
    m_bRemovable = m_regDriveMaskRemovable;
    m_bFloppy = m_regDriveMaskFloppy;
    m_bNetwork = m_regDriveMaskRemote;
    m_bFixed = m_regDriveMaskFixed;
    m_bCDROM = m_regDriveMaskCDROM;
    m_bRAM = m_regDriveMaskRAM;
    m_bUnknown = m_regDriveMaskUnknown;
    m_bUnversionedAsModified = m_regUnversionedAsModified;
    m_bIgnoreOnCommitIgnored = m_regIgnoreOnCommitIgnored;
    m_bShowExcludedAsNormal = m_regShowExcludedAsNormal;
    m_sExcludePaths = m_regExcludePaths;
    m_sExcludePaths.Replace(L"\n", L"\r\n");
    m_sIncludePaths = m_regIncludePaths;
    m_sIncludePaths.Replace(L"\n", L"\r\n");
    m_dwCacheType = m_regCacheType;
}

CSetOverlayPage::~CSetOverlayPage()
{
}

void CSetOverlayPage::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_REMOVABLE, m_bRemovable);
    DDX_Check(pDX, IDC_NETWORK, m_bNetwork);
    DDX_Check(pDX, IDC_FIXED, m_bFixed);
    DDX_Check(pDX, IDC_CDROM, m_bCDROM);
    DDX_Check(pDX, IDC_RAM, m_bRAM);
    DDX_Check(pDX, IDC_UNKNOWN, m_bUnknown);
    DDX_Check(pDX, IDC_ONLYEXPLORER, m_bOnlyExplorer);
    DDX_Text(pDX, IDC_EXCLUDEPATHS, m_sExcludePaths);
    DDX_Text(pDX, IDC_INCLUDEPATHS, m_sIncludePaths);
    DDX_Check(pDX, IDC_UNVERSIONEDASMODIFIED, m_bUnversionedAsModified);
    DDX_Check(pDX, IDC_IGNOREONCOMMITIGNORE, m_bIgnoreOnCommitIgnored);
    DDX_Check(pDX, IDC_FLOPPY, m_bFloppy);
    DDX_Check(pDX, IDC_SHOWEXCLUDEDASNORMAL, m_bShowExcludedAsNormal);
}

BEGIN_MESSAGE_MAP(CSetOverlayPage, ISettingsPropPage)
    ON_BN_CLICKED(IDC_REMOVABLE, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_FLOPPY, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_NETWORK, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_FIXED, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_CDROM, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_UNKNOWN, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_RAM, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_ONLYEXPLORER, &CSetOverlayPage::OnChange)
    ON_EN_CHANGE(IDC_EXCLUDEPATHS, &CSetOverlayPage::OnChange)
    ON_EN_CHANGE(IDC_INCLUDEPATHS, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_CACHEDEFAULT, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_CACHESHELL, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_CACHENONE, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_UNVERSIONEDASMODIFIED, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_IGNOREONCOMMITIGNORE, &CSetOverlayPage::OnChange)
    ON_BN_CLICKED(IDC_SHOWEXCLUDEDASNORMAL, &CSetOverlayPage::OnChange)
END_MESSAGE_MAP()

BOOL CSetOverlayPage::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    switch (m_dwCacheType)
    {
    case 0:
        CheckRadioButton(IDC_CACHEDEFAULT, IDC_CACHENONE, IDC_CACHENONE);
        break;
    default:
    case 1:
        CheckRadioButton(IDC_CACHEDEFAULT, IDC_CACHENONE, IDC_CACHEDEFAULT);
        break;
    case 2:
        CheckRadioButton(IDC_CACHEDEFAULT, IDC_CACHENONE, IDC_CACHESHELL);
        break;
    }
    DialogEnableWindow(IDC_UNVERSIONEDASMODIFIED, m_dwCacheType == 1);

    m_tooltips.Create(this);
    m_tooltips.AddTool(IDC_ONLYEXPLORER, IDS_SETTINGS_ONLYEXPLORER_TT);
    m_tooltips.AddTool(IDC_EXCLUDEPATHSLABEL, IDS_SETTINGS_EXCLUDELIST_TT);
    m_tooltips.AddTool(IDC_EXCLUDEPATHS, IDS_SETTINGS_EXCLUDELIST_TT);
    m_tooltips.AddTool(IDC_INCLUDEPATHSLABEL, IDS_SETTINGS_INCLUDELIST_TT);
    m_tooltips.AddTool(IDC_INCLUDEPATHS, IDS_SETTINGS_INCLUDELIST_TT);
    m_tooltips.AddTool(IDC_CACHEDEFAULT, IDS_SETTINGS_CACHEDEFAULT_TT);
    m_tooltips.AddTool(IDC_CACHESHELL, IDS_SETTINGS_CACHESHELL_TT);
    m_tooltips.AddTool(IDC_CACHENONE, IDS_SETTINGS_CACHENONE_TT);
    m_tooltips.AddTool(IDC_UNVERSIONEDASMODIFIED, IDS_SETTINGS_UNVERSIONEDASMODIFIED_TT);
    m_tooltips.AddTool(IDC_IGNOREONCOMMITIGNORE, IDS_SETTINGS_IGNOREONCOMMITIGNORE_TT);
    m_tooltips.AddTool(IDC_SHOWEXCLUDEDASNORMAL, IDS_SETTINGS_SHOWEXCLUDEDASNORMAL_TT);

    UpdateData(FALSE);

    return TRUE;
}

BOOL CSetOverlayPage::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSetOverlayPage::OnChange()
{
    UpdateData();
    int id = GetCheckedRadioButton(IDC_CACHEDEFAULT, IDC_CACHENONE);
    switch (id)
    {
    default:
    case IDC_CACHEDEFAULT:
        m_dwCacheType = 1;
        break;
    case IDC_CACHESHELL:
        m_dwCacheType = 2;
        break;
    case IDC_CACHENONE:
        m_dwCacheType = 0;
        break;
    }
    DialogEnableWindow(IDC_UNVERSIONEDASMODIFIED, m_dwCacheType == 1);
    SetModified();
}

BOOL CSetOverlayPage::OnApply()
{
    UpdateData();
    Store (m_bOnlyExplorer, m_regOnlyExplorer);
    if (DWORD(m_regDriveMaskRemovable) != DWORD(m_bRemovable))
        m_restart = Restart_Cache;
    Store (m_bRemovable, m_regDriveMaskRemovable);

    if (DWORD(m_regDriveMaskFloppy) != DWORD(m_bFloppy))
        m_restart = Restart_Cache;
    Store (m_bFloppy, m_regDriveMaskFloppy);

    if (DWORD(m_regDriveMaskRemote) != DWORD(m_bNetwork))
        m_restart = Restart_Cache;
    Store (m_bNetwork, m_regDriveMaskRemote);

    if (DWORD(m_regDriveMaskFixed) != DWORD(m_bFixed))
        m_restart = Restart_Cache;
    Store (m_bFixed, m_regDriveMaskFixed);

    if (DWORD(m_regDriveMaskCDROM) != DWORD(m_bCDROM))
        m_restart = Restart_Cache;
    Store (m_bCDROM, m_regDriveMaskCDROM);

    if (DWORD(m_regDriveMaskRAM) != DWORD(m_bRAM))
        m_restart = Restart_Cache;
    Store (m_bRAM, m_regDriveMaskRAM);

    if (DWORD(m_regDriveMaskUnknown) != DWORD(m_bUnknown))
        m_restart = Restart_Cache;
    Store (m_bUnknown, m_regDriveMaskUnknown);

    if (m_sExcludePaths.Compare(CString(m_regExcludePaths)))
        m_restart = Restart_Cache;
    m_sExcludePaths.Remove('\r');
    if (m_sExcludePaths.Right(1).Compare(L"\n")!=0)
        m_sExcludePaths += L"\n";
    Store (m_sExcludePaths, m_regExcludePaths);
    m_sExcludePaths.Replace(L"\n", L"\r\n");
    m_sIncludePaths.Remove('\r');
    if (m_sIncludePaths.Right(1).Compare(L"\n")!=0)
        m_sIncludePaths += L"\n";
    if (m_sIncludePaths.Compare(CString(m_regIncludePaths)))
        m_restart = Restart_Cache;
    Store (m_sIncludePaths, m_regIncludePaths);
    m_sIncludePaths.Replace(L"\n", L"\r\n");

    if (DWORD(m_regUnversionedAsModified) != DWORD(m_bUnversionedAsModified))
        m_restart = Restart_Cache;
    Store (m_bUnversionedAsModified, m_regUnversionedAsModified);
    if (DWORD(m_regIgnoreOnCommitIgnored) != DWORD(m_bIgnoreOnCommitIgnored))
        m_restart = Restart_Cache;
    Store (m_bIgnoreOnCommitIgnored, m_regIgnoreOnCommitIgnored);
    if (DWORD(m_regShowExcludedAsNormal) != DWORD(m_bShowExcludedAsNormal))
        m_restart = Restart_Cache;
    Store (m_bShowExcludedAsNormal, m_regShowExcludedAsNormal);

    Store (m_dwCacheType, m_regCacheType);
    if (m_dwCacheType != 1)
    {
        // close the possible running cache process
        HWND hWnd = ::FindWindow(TSVN_CACHE_WINDOW_NAME, TSVN_CACHE_WINDOW_NAME);
        if (hWnd)
        {
            ::PostMessage(hWnd, WM_CLOSE, NULL, NULL);
        }
        m_restart = Restart_None;
    }
    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}



