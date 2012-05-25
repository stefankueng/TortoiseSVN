// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012 - TortoiseSVN

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
#include "SVNRev.h"
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "FileDropEdit.h"
#include "Tooltip.h"
#include "TSVNPath.h"

/// forward declarations

class CLogDlg;

/**
 * \ingroup TortoiseProc
 * Prompts the user for required information for a checkout command. The information
 * is the module name and the repository url.
 */
class CCheckoutDlg : public CResizableStandAloneDialog //CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CCheckoutDlg)

public:
    CCheckoutDlg(CWnd* pParent = NULL);   ///< standard constructor
    virtual ~CCheckoutDlg();

// Dialog Data
    enum { IDD = IDD_CHECKOUT };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnBnClickedBrowse();
    afx_msg void OnBnClickedCheckoutdirectoryBrowse();
    afx_msg void OnEnChangeCheckoutdirectory();
    afx_msg void OnBnClickedHelp();
    afx_msg void OnBnClickedShowlog();
    afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void OnEnChangeRevisionNum();
    afx_msg void OnCbnEditchangeUrlcombo();
    afx_msg void OnCbnSelchangeDepth();
    afx_msg void OnBnClickedSparse();

    DECLARE_MESSAGE_MAP()

    void UpdateURLsFromCombo();
    bool IsStandardCheckout();
    void SetRevision(const SVNRev& rev);
    SVNRev GetSelectedRevision();
    SVNRev GetSelectedRevisionOrHead();

protected:
    CToolTips       m_tooltips;
    CString         m_sRevision;
    CString         m_sCheckoutDirOrig;
    bool            m_bAutoCreateTargetName;
    CComboBox       m_depthCombo;
    bool            m_bBlockMessages;

public:
    CHistoryCombo   m_URLCombo;
    CTSVNPathList   m_URLs;
    SVNRev          Revision;
    BOOL            m_bNoExternals;
    BOOL            m_bIndependentWCs;
    CButton         m_butBrowse;
    CEdit           m_editRevision;
    CString         m_strCheckoutDirectory;
    CFileDropEdit   m_cCheckoutEdit;
    CLogDlg *       m_pLogDlg;
    svn_depth_t     m_depth;
    BOOL            m_blockPathAdjustments;

    std::map<CString,svn_depth_t> m_checkoutDepths;
    bool            m_standardCheckout; ///< true if only one path got selected and that URL path is a folder
    bool            m_parentExists; ///< W/C for parent folder already exists. Only valid if \ref m_standardCheckout is false.
};
