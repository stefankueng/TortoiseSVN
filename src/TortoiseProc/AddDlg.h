// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2011 - TortoiseSVN

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
 * a simple dialog to show the user all unversioned
 * files below a specified folder.
 */
class CAddDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CAddDlg)

public:
    CAddDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CAddDlg();

// Dialog Data
    enum { IDD = IDD_ADD };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    afx_msg void OnBnClickedSelectall();
    afx_msg void OnBnClickedHelp();
    afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();

private:
    static UINT AddThreadEntry(LPVOID pVoid);
    UINT AddThread();
    afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()

public:
    /** holds all the selected files/folders the user wants to add to version
     * control on exit */
    CTSVNPathList   m_pathList;
    BOOL            m_UseAutoprops;

private:
    CSVNStatusListCtrl  m_addListCtrl;
    volatile LONG   m_bThreadRunning;
    CButton         m_SelectAll;
    bool            m_bCancelled;
};

