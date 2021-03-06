// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2012, 2015, 2021 - TortoiseSVN

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
#include "RegexEdit.h"

class CEditPropBugtraq : public CResizableStandAloneDialog
    , public EditPropBase
{
    DECLARE_DYNAMIC(CEditPropBugtraq)

public:
    CEditPropBugtraq(CWnd* pParent = nullptr);
    ~CEditPropBugtraq() override;

    enum
    {
        IDD = IDD_EDITPROPBUGTRAQ
    };

    bool HasMultipleProperties() override { return true; }

protected:
    void         DoDataExchange(CDataExchange* pDX) override;
    void         OnOK() override;
    BOOL         OnInitDialog() override;
    afx_msg void OnBnClickedHelp();
    afx_msg void OnBnClickedTestregex();

    DECLARE_MESSAGE_MAP()

    INT_PTR DoModal() override { return CResizableStandAloneDialog::DoModal(); }

private:
    BOOL    m_bWarnIfNoIssue;
    CString m_sBugtraqUrl;
    CString m_sBugtraqMessage;
    CString m_sBugtraqLabel;
    CString m_sBugtraqRegex1;
    CString m_sBugtraqRegex2;
    CString m_sProviderUuid;
    CString m_sProviderUuid64;
    CString m_sProviderParams;

    CRegexEdit m_bugtraqRegex1;
    CRegexEdit m_bugtraqRegex2;
};
