// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2016, 2020-2021 - TortoiseSVN

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
#include <afxcmn.h>
#include "StandAloneDlg.h"
#include "SVN.h"
#include "TSVNPath.h"
#include "Blame.h"
#include "HintCtrl.h"
#include "Colors.h"
#include "FilterEdit.h"
#include "JobScheduler.h"
#include "ThemeControls.h"

#define IDT_FILTER 101

/**
 * \ingroup TortoiseProc
 * Dialog which fetches and shows the difference between two urls in the
 * repository. It shows a list of files/folders which were changed in those
 * two revisions.
 */
class CFileDiffDlg : public CResizableStandAloneDialog
    , public SVN
{
    DECLARE_DYNAMIC(CFileDiffDlg)
public:
    class FileDiff
    {
    public:
        CTSVNPath                        path;
        svn_client_diff_summarize_kind_t kind;
        bool                             propchanged;
        svn_node_kind_t                  node;
    };

public:
    CFileDiffDlg(CWnd* pParent = nullptr);
    ~CFileDiffDlg() override;

    void SetDiff(const CTSVNPath& path1, const SVNRev& rev1, const CTSVNPath& path2, const SVNRev& rev2, svn_depth_t depth, bool ignoreancestry);
    void SetDiff(const CTSVNPath& path, const SVNRev& peg, const SVNRev& rev1, const SVNRev& rev2, svn_depth_t depth, bool ignoreancestry);

    void DoBlame(bool blame = true) { m_bBlame = blame; }

    enum
    {
        IDD = IDD_DIFFFILES
    };

protected:
    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    void            OnCancel() override;
    BOOL            OnInitDialog() override;
    BOOL            PreTranslateMessage(MSG* pMsg) override;
    afx_msg void    OnNMDblclkFilelist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnGetInfoTipFilelist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMCustomdrawFilelist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnGetdispinfoFilelist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void    OnEnSetfocusSecondurl();
    afx_msg void    OnEnSetfocusFirsturl();
    afx_msg void    OnBnClickedSwitchleftright();
    afx_msg void    OnHdnItemclickFilelist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnBnClickedRev1btn();
    afx_msg void    OnBnClickedRev2btn();
    afx_msg LRESULT OnClickedCancelFilter(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnEnChangeFilter();
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnSetFocus(CWnd* pOldWnd);

    DECLARE_MESSAGE_MAP()

    svn_error_t* DiffSummarizeCallback(const CTSVNPath&                 path,
                                       svn_client_diff_summarize_kind_t kind,
                                       bool                             propchanged,
                                       svn_node_kind_t                  node) override;

    void DoDiff(int selIndex, bool bText, bool bProps, bool blame, bool bDefault);
    void DiffProps(int selIndex);
    void SetURLLabels();
    void Filter(CString sFilterText);
    void CopySelectionToClipboard() const;

private:
    static UINT DiffThreadEntry(LPVOID pVoid);
    UINT        DiffThread();
    static UINT ExportThreadEntry(LPVOID pVoid);
    UINT        ExportThread();
    void        GetSelectedPaths(CTSVNPathList& urls1, CTSVNPathList& urls2);

    BOOL Cancel() override { return m_bCancelled; }

    CButton     m_cRev1Btn;
    CButton     m_cRev2Btn;
    CFilterEdit m_cFilter;

    CThemeMFCButton            m_switchButton;
    CColors                    m_colors;
    CHintCtrl<CListCtrl>       m_cFileList;
    wchar_t                    m_columnBuf[MAX_PATH];
    bool                       m_bBlame;
    CBlame                     m_blamer;
    std::vector<FileDiff>      m_arFileList;
    std::vector<FileDiff>      m_arFilteredList;
    CArray<FileDiff, FileDiff> m_arSelectedFileList;

    CString       m_strExportDir;
    CProgressDlg* m_pProgDlg;

    int m_nIconFolder;

    CTSVNPath            m_path1;
    SVNRev               m_peg;
    SVNRev               m_rev1;
    CTSVNPath            m_path2;
    SVNRev               m_rev2;
    svn_depth_t          m_depth;
    bool                 m_bIgnoreancestry;
    bool                 m_bDoPegDiff;
    volatile LONG        m_bThreadRunning;
    async::CJobScheduler netScheduler;

    bool m_bCancelled;

    void        Sort();
    static bool SortCompare(const FileDiff& data1, const FileDiff& data2);

    static BOOL m_bAscending;
    static int  m_nSortedColumn;
};
