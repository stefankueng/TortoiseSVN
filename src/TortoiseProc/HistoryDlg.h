// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2010, 2013, 2017, 2021 - TortoiseSVN

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
#include "RegHistory.h"
#include "FilterEdit.h"
#include "HintCtrl.h"

/**
 * \ingroup TortoiseProc
 * Dialog showing the log message history.
 */
class CHistoryDlg : public CResizableStandAloneDialog
    , IFilterEditValidator
{
    DECLARE_DYNAMIC(CHistoryDlg)
public:
    CHistoryDlg(CWnd* pParent = nullptr); // standard constructor
    ~CHistoryDlg() override;

    /// Returns the text of the selected entry.
    CString GetSelectedText() const { return m_selectedText; }
    /// Sets the history object to use
    void SetHistory(CRegHistory& history) { m_history = &history; }
    // Dialog Data
    enum
    {
        IDD = IDD_HISTORYDLG
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    BOOL PreTranslateMessage(MSG* pMsg) override;
    bool Validate(LPCWSTR string) override;

    afx_msg void OnBnClickedOk();
    afx_msg void OnLbnDblclkHistorylist();
    afx_msg void OnEnChangeSearchedit();

    afx_msg LRESULT OnClickedCancelFilter(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

    void UpdateMessageList();

private:
    CHintCtrl<CListBox> m_list;
    CString             m_selectedText;
    CRegHistory*        m_history;
    CFilterEdit         m_cFilter;
};
