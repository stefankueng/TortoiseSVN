// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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

#include "Tooltip.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog to configure diff/merge tools according to file type.
 */
class CToolAssocDlg : public CDialog
{
    DECLARE_DYNAMIC(CToolAssocDlg)

public:
    CToolAssocDlg(const CString& type, bool add, CWnd* pParent = NULL);
    virtual ~CToolAssocDlg();

    enum { IDD = IDD_TOOLASSOC };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnBnClickedToolbrowse();

    DECLARE_MESSAGE_MAP()

public:
    CString     m_sType;
    bool        m_bAdd;
    CString     m_sExtension;
    CString     m_sTool;
    CToolTips   m_tooltips;
};

