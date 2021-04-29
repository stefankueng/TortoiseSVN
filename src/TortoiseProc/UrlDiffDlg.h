﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009-2010, 2012, 2015-2016, 2018, 2021 - TortoiseSVN

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
#include "SVNRev.h"
#include "SVNDiffOptions.h"

/// forward declarations

class CLogDlg;

/**
 * \ingroup TortoiseProc
 * Shows a dialog to prompt the user for an URL of a branch and a revision
 * number to switch the working copy to. Also has a checkbox to
 * specify the current branch instead of a different branch url and
 * one checkbox to specify the newest revision.
 */
class CUrlDiffDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CUrlDiffDlg)

public:
    CUrlDiffDlg(CWnd* pParent = nullptr); // standard constructor
    ~CUrlDiffDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_URLDIFF
    };

protected:
    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL            OnInitDialog() override;
    void            OnOK() override;
    void            OnCancel() override;
    afx_msg void    OnBnClickedBrowse();
    afx_msg void    OnBnClickedDiffOptions();
    afx_msg void    OnBnClickedHelp();
    afx_msg void    OnEnChangeRevisionNum();
    afx_msg void    OnBnClickedLog();
    afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnCbnEditchangeUrlcombo();

    void SetRevision(const SVNRev& rev);

    DECLARE_MESSAGE_MAP()

    CString       m_rev;
    CHistoryCombo m_urlCombo;
    BOOL          m_bFolder;
    CLogDlg*      m_pLogDlg;

public:
    CString        m_path;
    CString        m_url;
    SVNRev         m_revision;
    SVNDiffOptions m_diffOptions;
    bool           m_bPrettyPrint;
};
