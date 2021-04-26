﻿// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2010, 2013-2014, 2016, 2020-2021 - TortoiseSVN

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
#include "resource.h"
#include "HistoryCombo.h"
#include "StandAloneDlg.h"

#define REPLACEMSGSTRINGW L"TortoiseMerge_FindReplace"

/**
 * \ingroup TortoiseMerge
 * Find dialog used in TortoiseMerge.
 */
class CFindDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CFindDlg)

public:
    CFindDlg(CWnd* pParent = nullptr); // standard constructor
    ~CFindDlg() override;
    void    Create(CWnd* pParent = nullptr, int id = 0);
    bool    IsTerminating() const { return m_bTerminating; }
    bool    FindNext() const { return m_bFindNext; }
    bool    MatchCase() const { return !!m_bMatchCase; }
    bool    LimitToDiffs() const { return !!m_bLimitToDiffs; }
    bool    WholeWord() const { return !!m_bWholeWord; }
    bool    SearchUp() const { return !!m_bSearchUp; }
    CString GetFindString() const { return m_findCombo.GetString(); }
    CString GetReplaceString() const { return m_replaceCombo.GetString(); }
    void    SetFindString(const CString& str)
    {
        if (!str.IsEmpty())
        {
            m_findCombo.SetWindowText(str);
        }
    }
    void SetStatusText(const CString& str, COLORREF color = RGB(0, 0, 255));
    void SetReadonly(bool bReadonly);
    // Dialog Data
    enum
    {
        IDD = IDD_FIND
    };

    enum FindType
    {
        Find,
        Count,
        Replace,
        ReplaceAll
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support

    DECLARE_MESSAGE_MAP()
    void           OnCancel() override;
    void           PostNcDestroy() override;
    void           OnOK() override;
    BOOL           OnInitDialog() override;
    afx_msg void   OnCbnEditchangeFindcombo();
    afx_msg void   OnBnClickedCount();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnBnClickedReplace();
    afx_msg void   OnBnClickedReplaceall();
    void           SaveWindowPos(CWnd* pParent) const;

private:
    UINT          m_findMsg;
    bool          m_bTerminating;
    bool          m_bFindNext;
    BOOL          m_bMatchCase;
    BOOL          m_bLimitToDiffs;
    BOOL          m_bWholeWord;
    BOOL          m_bSearchUp;
    CHistoryCombo m_findCombo;
    CHistoryCombo m_replaceCombo;
    CStatic       m_findStatus;
    CWnd*         m_pParent;
    CRegDWORD     m_regMatchCase;
    CRegDWORD     m_regLimitToDiffs;
    CRegDWORD     m_regWholeWord;
    COLORREF      m_clrFindStatus;
    bool          m_bReadonly;
    int           m_id;
};
