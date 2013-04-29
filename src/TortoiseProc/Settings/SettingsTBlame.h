// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010, 2012-2013 - TortoiseSVN

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
#include "../../TortoiseBlame/BlameIndexColors.h"


/**
 * \ingroup TortoiseProc
 * Settings page to configure TortoiseBlame
 */
class CSettingsTBlame : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSettingsTBlame)

public:
    CSettingsTBlame();
    virtual ~CSettingsTBlame();

    UINT GetIconID() override {return IDI_TORTOISEBLAME;}

// Dialog Data
    enum { IDD = IDD_SETTINGSTBLAME };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    afx_msg void OnBnClickedColor();
    afx_msg void OnChange();
    afx_msg void OnBnClickedRestore();

    DECLARE_MESSAGE_MAP()
private:
    CMFCColorButton m_cNewLinesColor;
    CMFCColorButton m_cOldLinesColor;
    CMFCColorButton m_cNewLinesColorBar;
    CMFCColorButton m_cOldLinesColorBar;
    CRegDWORD       m_regNewLinesColor;
    CRegDWORD       m_regOldLinesColor;
    CRegDWORD       m_regNewLinesColorBar;
    CRegDWORD       m_regOldLinesColorBar;

    CMFCColorButton m_indexColors[MAX_BLAMECOLORS];
    CRegDWORD       m_regIndexColors[MAX_BLAMECOLORS];

    CMFCFontComboBox    m_cFontNames;
    CComboBox       m_cFontSizes;
    CRegDWORD       m_regFontSize;
    DWORD           m_dwFontSize;
    CRegString      m_regFontName;
    CString         m_sFontName;
    DWORD           m_dwTabSize;
    CRegDWORD       m_regTabSize;
};
