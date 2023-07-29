﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2012, 2021 - TortoiseSVN

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

#include "registry.h"
#include "StandAloneDlg.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog to configure the external tools used e.g. for diffing/merging/...
 */
class CSetProgsAdvDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CSetProgsAdvDlg)

public:
    CSetProgsAdvDlg(const CString& type, CWnd* pParent = nullptr);
    ~CSetProgsAdvDlg() override;
    /**
     * Loads the tools from the registry.
     */
    void LoadData();
    /**
     * Saves the changed tools to the registry.
     * returns 0 if no restart is needed for the changes to take effect
     * \remark If the dialog is closed/dismissed without calling
     * this method first then all settings the user made must be
     * discarded!
     */
    int SaveData();

    int  AddExtension(const CString& ext, const CString& tool);
    int  FindExtension(const CString& ext) const;
    void EnableBtns() const;

    enum
    {
        IDD = IDD_SETTINGSPROGSADV
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override;
    BOOL         OnInitDialog() override;
    afx_msg void OnBnClickedAddtool();
    afx_msg void OnBnClickedEdittool();
    afx_msg void OnBnClickedRemovetool();
    afx_msg void OnNMDblclkToollistctrl(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLvnItemchangedToollistctrl(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedRestoredefaults();

    DECLARE_MESSAGE_MAP()

private:
    CString      m_sType;        ///< tool type ("Diff" or "Merge")
    CRegistryKey m_regToolKey;   ///< registry key where the tools are stored
    CListCtrl    m_toolListCtrl; ///< list control used for viewing and editing

    std::map<CString, CString> m_tools;      ///< internal storage of all tools
    bool                       m_toolsValid; ///< true if m_Tools was ever read
};
