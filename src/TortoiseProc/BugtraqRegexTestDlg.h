﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2021 - TortoiseSVN

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
#include "StandAloneDlg.h"
#include "RegexEdit.h"
#include "SciEdit.h"

// CBugtraqRegexTestDlg dialog

class CBugtraqRegexTestDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CBugtraqRegexTestDlg)

public:
    CBugtraqRegexTestDlg(CWnd* pParent = nullptr); // standard constructor
    ~CBugtraqRegexTestDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_BUGTRAQREGEXTESTER
    };

    CString m_sBugtraqRegex1;
    CString m_sBugtraqRegex2;

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;

    DECLARE_MESSAGE_MAP()

    afx_msg void OnEnChangeBugtraqlogregex1();
    afx_msg void OnEnChangeBugtraqlogregex2();

private:
    void UpdateLogControl();

private:
    CRegexEdit m_bugtraqRegex1;
    CRegexEdit m_bugtraqRegex2;
    CSciEdit   m_cLogMessage;
};
