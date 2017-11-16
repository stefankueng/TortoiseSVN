// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2017 - TortoiseSVN

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
#include "SVNStatusListCtrl.h"

/**
 * \ingroup TortoiseProc
 * Shows the patch dialog where the user can select the files/folders to be
 * included in the resulting patch (unified diff) file.
 */
class CUnshelve : public CResizableStandAloneDialog //CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CUnshelve)

public:
    CUnshelve(CWnd* pParent = NULL);   // standard constructor
    virtual ~CUnshelve();

    enum { IDD = IDD_UNSHELVE };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnCancel();
    virtual void OnOK();
    afx_msg void OnBnClickedHelp();

    DECLARE_MESSAGE_MAP()

    CComboBox           m_ShelveNameCombo;

public:
    std::vector<CString> m_Names;
    CString             m_sShelveName;
    CTSVNPathList       m_pathList;
};
