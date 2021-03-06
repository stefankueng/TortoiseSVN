// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009-2010, 2021 - TortoiseSVN

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
#include "SciEdit.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog to ask for various text inputs.
 */
class CInputDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CInputDlg)

public:
    CInputDlg(CWnd* pParent = nullptr);
    ~CInputDlg() override;

    enum
    {
        IDD = IDD_INPUTDLG
    };

protected:
    CFont m_logFont;

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    BOOL PreTranslateMessage(MSG* pMsg) override;
    void OnOK() override;

    DECLARE_MESSAGE_MAP()
public:
    CString  m_sInputText;
    CString  m_sHintText;
    CString  m_sTitle;
    CString  m_sCheckText;
    int      m_iCheck;
    CSciEdit m_cInput;
};
