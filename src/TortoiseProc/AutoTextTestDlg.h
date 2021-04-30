﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009, 2013, 2020-2021 - TortoiseSVN

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
#include "SciEdit.h"
#include "StandAloneDlg.h"
#include <afxcmn.h>

// CAutoTextTestDlg dialog

class CAutoTextTestDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CAutoTextTestDlg)

public:
    CAutoTextTestDlg(CWnd* pParent = nullptr); // standard constructor
    ~CAutoTextTestDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_AUTOTEXTTESTDLG
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    afx_msg void OnBnClickedAutotextscan();

    DECLARE_MESSAGE_MAP()

private:
    CString       m_sContent;
    CString       m_sRegex;
    CString       m_sResult;
    CString       m_sTimingLabel;
    CRichEditCtrl m_cContent;
};
