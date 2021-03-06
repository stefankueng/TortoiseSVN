// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2016, 2018, 2021 - TortoiseSVN

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
#include "SVNDiffOptions.h"

/**
 * \ingroup TortoiseProc
 * Shows the patch dialog where the user can select the files/folders to be
 * included in the resulting patch (unified diff) file.
 */
class CCreatePatch : public CResizableStandAloneDialog //CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CCreatePatch)

public:
    CCreatePatch(CWnd* pParent = nullptr); // standard constructor
    ~CCreatePatch() override;

    enum
    {
        IDD = IDD_CREATEPATCH
    };

protected:
    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL            OnInitDialog() override;
    BOOL            PreTranslateMessage(MSG* pMsg) override;
    void            OnCancel() override;
    void            OnOK() override;
    afx_msg void    OnBnClickedSelectall();
    afx_msg void    OnBnClickedShowunversioned();
    afx_msg void    OnBnClickedHelp();
    afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
    afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnBnClickedDiffoptions();

    DECLARE_MESSAGE_MAP()

private:
    static UINT PatchThreadEntry(LPVOID pVoid);
    UINT        PatchThread();
    DWORD       ShowMask() const;

private:
    CSVNStatusListCtrl m_patchList;
    LONG               m_bThreadRunning;
    CButton            m_selectAll;
    bool               m_bCancelled;
    BOOL               m_bShowUnversioned;
    CRegDWORD          m_regAddBeforeCommit;

public:
    /// the list of files to include in the patch
    CTSVNPathList m_pathList;
    /**
     * The files which have to be reverted after the patch was created.
     * That's necessary if the user selected an unversioned file - such files
     * are added automatically to version control so they can be included in
     * the patch, but then must be reverted later.
     */
    CTSVNPathList  m_filesToRevert;
    SVNDiffOptions m_diffOptions;
    bool           m_bPrettyPrint;
};
