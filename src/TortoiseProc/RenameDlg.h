﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009-2010, 2013-2015, 2017-2018, 2021 - TortoiseSVN

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
#include "IInputValidator.h"
#include "AutoComplete.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog, asking for a new name.
 */
class CRenameDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CRenameDlg)

public:
    CRenameDlg(CWnd* pParent = nullptr);
    ~CRenameDlg() override;

    void SetInputValidator(IInputValidator* validator) { m_pInputValidator = validator; }
    void SetRenameRequired(bool renameRequired) { m_renameRequired = renameRequired; }
    void SetFileSystemAutoComplete() { m_bFsAutoComplete = true; }
    void SetAutoComplete(bool bAutoComplete) { m_bAutoComplete = bAutoComplete; }
    template <typename T>
    void SetCustomAutoComplete(T t)
    {
        m_bCustomAutocomplete = true;
        m_autoCompleteCustom.SetEntries(t);
    }

    enum
    {
        IDD = IDD_RENAME
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnOK() override;
    void OnCancel() override;

    afx_msg void OnEnSetfocusName();

    DECLARE_MESSAGE_MAP()

public:
    CString m_name;
    CString m_windowTitle;
    CString m_label;
    CString m_infoLabel;

private:
    bool             m_bBalloonVisible;
    bool             m_renameRequired;
    bool             m_bFsAutoComplete;
    bool             m_bAutoComplete;
    bool             m_bCustomAutocomplete;
    CString          m_originalName;
    IInputValidator* m_pInputValidator;
    CAutoComplete    m_autoCompleteCustom;
};
