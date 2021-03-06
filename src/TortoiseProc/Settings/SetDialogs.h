// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009, 2012-2013, 2015, 2021 - TortoiseSVN

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
#include "SettingsPropPage.h"
#include "registry.h"

/**
 * \ingroup TortoiseProc
 * Settings page responsible for dialog settings.
 */
class CSetDialogs : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSetDialogs)

public:
    CSetDialogs();
    ~CSetDialogs() override;

    UINT GetIconID() override { return IDI_DIALOGS; }

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSDIALOGS
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    BOOL         OnApply() override;
    afx_msg void OnChange();
    afx_msg void OnCbnSelchangeAutoclosecombo();
    afx_msg void OnBnClickedBrowsecheckoutpath();

    DECLARE_MESSAGE_MAP()

private:
    BOOL             m_bShortDateFormat;
    CRegDWORD        m_regShortDateFormat;
    BOOL             m_bUseSystemLocaleForDates;
    CRegDWORD        m_regUseSystemLocaleForDates;
    CRegDWORD        m_regAutoClose;
    DWORD            m_dwAutoClose;
    CRegDWORD        m_regAutoCloseLocal;
    BOOL             m_bAutoCloseLocal;
    CRegDWORD        m_regDefaultLogs;
    CString          m_sDefaultLogs;
    CMFCFontComboBox m_cFontNames;
    CComboBox        m_cFontSizes;
    CRegDWORD        m_regFontSize;
    DWORD            m_dwFontSize;
    CRegString       m_regFontName;
    CString          m_sFontName;
    CComboBox        m_cAutoClose;
    CRegDWORD        m_regUseWcUrl;
    BOOL             m_bUseWcUrl;
    CRegString       m_regDefaultCheckoutPath;
    CString          m_sDefaultCheckoutPath;
    CRegString       m_regDefaultCheckoutUrl;
    CString          m_sDefaultCheckoutUrl;
    CRegDWORD        m_regDiffByDoubleClick;
    BOOL             m_bDiffByDoubleClick;
    CRegDWORD        m_regUseRecycleBin;
    BOOL             m_bUseRecycleBin;
};
