﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2012, 2015, 2021 - TortoiseSVN

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
#include "Hooks.h"
#include "StandAloneDlg.h"

/**
 * \ingroup TortoiseProc
 * helper dialog to configure client side hook scripts.
 */
class CSetHooksAdv : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CSetHooksAdv)

public:
    CSetHooksAdv(CWnd* pParent = nullptr); // standard constructor
    ~CSetHooksAdv() override;

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSHOOKCONFIG
    };

    HookKey key;
    HookCmd cmd;

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    void         OnOK() override;
    afx_msg void OnBnClickedHookbrowse();
    afx_msg void OnBnClickedHookcommandbrowse();
    afx_msg void OnBnClickedHelp();

    DECLARE_MESSAGE_MAP()

protected:
    CString   m_sPath;
    CString   m_sCommandLine;
    BOOL      m_bWait;
    BOOL      m_bHide;
    BOOL      m_bEnforce;
    CComboBox m_cHookTypeCombo;
};