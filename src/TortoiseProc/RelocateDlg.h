﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2011, 2015, 2021 - TortoiseSVN

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
#include "HistoryCombo.h"
#include "PathEdit.h"
#include "TSVNPath.h"

/**
 * \ingroup TortoiseProc
 * Dialog asking for a new URL for the working copy.
 */
class CRelocateDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CRelocateDlg)

public:
    CRelocateDlg(CWnd* pParent = nullptr); // standard constructor
    ~CRelocateDlg() override;

    enum
    {
        IDD = IDD_RELOCATE
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    void         OnOK() override;
    BOOL         OnInitDialog() override;
    afx_msg void OnBnClickedHelp();

    DECLARE_MESSAGE_MAP()

    CPathEdit m_fromUrl;

public:
    CTSVNPath     m_path;
    CHistoryCombo m_urlCombo;
    CString       m_sToUrl;
    CString       m_sFromUrl;
    BOOL          m_bIncludeExternals;
};
