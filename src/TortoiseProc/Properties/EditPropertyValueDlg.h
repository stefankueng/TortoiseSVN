// TortoiseSVN - a Windows shell extension for easy version control

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
#include "EditPropBase.h"
#include "StandAloneDlg.h"
#include "ProjectProperties.h"

#define MAX_TT_LENGTH 10000

/**
 * \ingroup TortoiseProc
 * Helper dialog to edit the Subversion properties.
 */

class CEditPropertyValueDlg : public CResizableStandAloneDialog
    , public EditPropBase
{
    DECLARE_DYNAMIC(CEditPropertyValueDlg)

public:
    CEditPropertyValueDlg(CWnd* pParent = nullptr); // standard constructor
    ~CEditPropertyValueDlg() override;

    enum
    {
        IDD = IDD_EDITPROPERTYVALUE
    };

    void SetPropertyName(const std::string& sName) override;
    void SetPropertyValue(const std::string& sValue) override;

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    void         OnOK() override;
    void         OnCancel() override;
    BOOL         PreTranslateMessage(MSG* pMsg) override;
    afx_msg void OnBnClickedHelp();
    afx_msg void OnBnClickedLoadprop();
    afx_msg void OnEnChangePropvalue();

    DECLARE_MESSAGE_MAP()

    void    CheckRecursive();
    INT_PTR DoModal() override { return CResizableStandAloneDialog::DoModal(); }

protected:
    CComboBox         m_propNames;
    CFont             m_valueFont;
    CString           m_sPropValue;
    CString           m_sPropName;
    ProjectProperties m_projectProperties;
};
