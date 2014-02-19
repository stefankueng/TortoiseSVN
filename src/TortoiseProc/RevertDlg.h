// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2014 - TortoiseSVN

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
 * Dialog showing a list of versioned files which don't have the status 'normal'.
 * The dialog effectively shows a list of files which can be reverted.
 */
class CRevertDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CRevertDlg)

public:
    CRevertDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CRevertDlg();

    enum { IDD = IDD_REVERT };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnCancel();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnBnClickedHelp();
    afx_msg void OnBnClickedSelectall();
    afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
    afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBnClickedDelunversioned();
    afx_msg void OnBnClickedOk();

    DECLARE_MESSAGE_MAP()

private:
    static UINT RevertThreadEntry(LPVOID pVoid);
    UINT        RevertThread();

public:
    CTSVNPathList       m_pathList;
    CTSVNPathList       m_selectedPathList;
    BOOL                m_bRecursive;
    BOOL                m_bClearChangeLists;

private:
    BOOL                m_bSelectAll;
    CString             m_sWindowTitle;
    volatile LONG       m_bThreadRunning;
    CSVNStatusListCtrl  m_RevertList;
    CButton             m_SelectAll;
    bool                m_bCancelled;
};

