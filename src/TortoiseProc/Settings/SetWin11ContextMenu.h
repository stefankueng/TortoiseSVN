﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2021-2022 - TortoiseSVN

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
#include "Globals.h"

/**
 * \ingroup TortoiseProc
 * Settings page for the Win11 top context menu.
 */
class CSetWin11ContextMenu : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSetWin11ContextMenu)

public:
    CSetWin11ContextMenu();
    ~CSetWin11ContextMenu() override;

    UINT GetIconID() override { return IDI_MISC; }

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSWIN11CONTEXTMENU
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnApply() override;
    afx_msg void OnLvnItemchangedMenulist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedRegister();

    DECLARE_MESSAGE_MAP()
public:
    BOOL OnInitDialog() override;

private:
    void InsertItem(UINT nTextID, UINT nIconID, TSVNContextMenuEntries dwFlags, int iconWidth, int iconHeight);

    CRegQWORD m_regTopMenu;

    CImageList             m_imgList;
    CListCtrl              m_cMenuList;
    BOOL                   m_bModified;
    TSVNContextMenuEntries m_topMenu;
    bool                   m_bBlock;
};
