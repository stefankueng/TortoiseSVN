﻿// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2013-2014, 2020-2021 - TortoiseSVN

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
#include "resource.h"

// CRegexFilterDlg dialog

class CRegexFilterDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CRegexFilterDlg)

public:
    CRegexFilterDlg(CWnd* pParent = nullptr); // standard constructor
    ~CRegexFilterDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_REGEXFILTER
    };

    CString m_sName;
    CString m_sRegex;
    CString m_sReplace;

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnOK() override;
    void ShowEditBalloon(UINT nIdControl, UINT nIdText, UINT nIdTitle, int nIcon = TTI_WARNING) override;

    DECLARE_MESSAGE_MAP()
};
