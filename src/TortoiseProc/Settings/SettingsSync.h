// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014-2015, 2021 - TortoiseSVN

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
#pragma once
#include "SettingsPropPage.h"
#include "registry.h"

/**
 * \ingroup TortoiseProc
 * Settings page to configure Sync
 */
class CSettingsSync : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSettingsSync)

public:
    CSettingsSync();
    ~CSettingsSync() override;

    UINT GetIconID() override { return IDI_SYNC; }

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSSYNC
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    BOOL OnApply() override;
    BOOL OnKillActive() override;

    BOOL ValidateInput();

    afx_msg void OnBnClickedSyncbrowse();
    afx_msg void OnEnChange();
    afx_msg void OnBnClickedLoad();
    afx_msg void OnBnClickedSave();
    afx_msg void OnBnClickedLoadfile();
    afx_msg void OnBnClickedSavefile();
    afx_msg void OnBnClickedSyncauth();

    DECLARE_MESSAGE_MAP()

    CString GetHWndParam() const;

private:
    CString    m_sPw1;
    CString    m_sPw2;
    CString    m_sSyncPath;
    CRegString m_regSyncPath;
    CRegString m_regSyncPw;
    CRegDWORD  m_regSyncAuth;
    BOOL       m_bSyncAuth;
};
