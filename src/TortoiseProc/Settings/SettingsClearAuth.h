// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013, 2015, 2021 - TortoiseSVN

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
#include <afxcmn.h>
#include "StandAloneDlg.h"

// CSettingsClearAuth dialog

class CSettingsClearAuth : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CSettingsClearAuth)

public:
    CSettingsClearAuth(CWnd* pParent = nullptr); // standard constructor
    ~CSettingsClearAuth() override;

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGS_CLEARAUTH
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnOK() override;

    DECLARE_MESSAGE_MAP()
private:
    void FillAuthListControl();

private:
    CListCtrl m_cAuthList;
    bool      m_bShowPasswords;

public:
    afx_msg void OnNMDblclkAuthdatalist(NMHDR* pNMHDR, LRESULT* pResult);
};
