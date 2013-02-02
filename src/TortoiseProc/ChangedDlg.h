// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2012 - TortoiseSVN

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
#include "SVN.h"
#include "SVNStatusListCtrl.h"
#include "registry.h"
#include "Tooltip.h"


/**
 * \ingroup TortoiseProc
 * Shows the "check for modifications" dialog.
 */
class CChangedDlg : public CResizableStandAloneDialog, public SVN
{
    DECLARE_DYNAMIC(CChangedDlg)

public:
    CChangedDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CChangedDlg();
    void ContactRepository(bool bContact) {m_bContactRepository = bContact;}

// Dialog Data
    enum { IDD = IDD_CHANGEDFILES };

protected:
    virtual void            DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL            OnInitDialog();
    virtual void            OnOK();
    virtual void            OnCancel();
    virtual BOOL            PreTranslateMessage(MSG* pMsg);
    afx_msg void            OnBnClickedCheckrepo();
    afx_msg void            OnBnClickedRefresh();
    afx_msg void            OnBnClickedShowunversioned();
    afx_msg void            OnBnClickedShowUnmodified();
    afx_msg void            OnBnClickedShowignored();
    afx_msg void            OnBnClickedShowexternals();
    afx_msg void            OnBnClickedShowUserProps();
    afx_msg void            OnBnClickedShowfolders();
    afx_msg void            OnBnClickedShowfiles();
    afx_msg BOOL            OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg LRESULT         OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
    afx_msg LRESULT         OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()

private:
    static UINT             ChangedStatusThreadEntry(LPVOID pVoid);
    UINT                    ChangedStatusThread();
    void                    UpdateStatistics();
    DWORD                   UpdateShowFlags();

public:
    CTSVNPathList           m_pathList;

private:
    CRegDWORD               m_regAddBeforeCommit;
    CRegDWORD               m_regShowUserProps;
    CSVNStatusListCtrl      m_FileListCtrl;
    bool                    m_bRemote;
    BOOL                    m_bShowUnversioned;
    int                     m_iShowUnmodified;
    volatile LONG           m_bBlock;
    CString                 m_sTitle;
    bool                    m_bCanceled;
    BOOL                    m_bShowIgnored;
    BOOL                    m_bShowExternals;
    BOOL                    m_bShowUserProps;
    BOOL                    m_bShowDirs;
    BOOL                    m_bShowFiles;
    bool                    m_bDepthInfinity;
    CToolTips               m_tooltips;
    bool                    m_bContactRepository;

    /// temp. set when the "Properties" was clicked last
    bool                    m_bShowPropertiesClicked;
};

