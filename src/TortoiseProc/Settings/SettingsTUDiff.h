// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014, 2021 - TortoiseSVN

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
 * Settings page to configure TortoiseUDiff
 */
class CSettingsUDiff : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSettingsUDiff)

public:
    CSettingsUDiff();
    ~CSettingsUDiff() override;

    UINT GetIconID() override { return IDI_TORTOISEUDIFF; }

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSUDIFF
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    BOOL         OnApply() override;
    afx_msg void OnBnClickedColor();
    afx_msg void OnChange();
    afx_msg void OnBnClickedRestore();

    DECLARE_MESSAGE_MAP()
private:
    CMFCColorButton m_cForeCommandColor;
    CMFCColorButton m_cForePositionColor;
    CMFCColorButton m_cForeHeaderColor;
    CMFCColorButton m_cForeCommentColor;
    CMFCColorButton m_cForeAddedColor;
    CMFCColorButton m_cForeRemovedColor;

    CMFCColorButton m_cBackCommandColor;
    CMFCColorButton m_cBackPositionColor;
    CMFCColorButton m_cBackHeaderColor;
    CMFCColorButton m_cBackCommentColor;
    CMFCColorButton m_cBackAddedColor;
    CMFCColorButton m_cBackRemovedColor;

    CRegDWORD m_regForeCommandColor;
    CRegDWORD m_regForePositionColor;
    CRegDWORD m_regForeHeaderColor;
    CRegDWORD m_regForeCommentColor;
    CRegDWORD m_regForeAddedColor;
    CRegDWORD m_regForeRemovedColor;

    CRegDWORD m_regBackCommandColor;
    CRegDWORD m_regBackPositionColor;
    CRegDWORD m_regBackHeaderColor;
    CRegDWORD m_regBackCommentColor;
    CRegDWORD m_regBackAddedColor;
    CRegDWORD m_regBackRemovedColor;

    CMFCFontComboBox m_cFontNames;
    CComboBox        m_cFontSizes;
    CRegDWORD        m_regFontSize;
    DWORD            m_dwFontSize;
    CRegString       m_regFontName;
    CString          m_sFontName;
    DWORD            m_dwTabSize;
    CRegDWORD        m_regTabSize;
};
