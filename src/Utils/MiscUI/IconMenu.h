// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2009, 2011, 2021 - TortoiseSVN

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
#include "IconBitmapUtils.h"

/**
 * \ingroup utils
 * Extends the CMenu class with a convenience method to insert a menu
 * entry with an icon.
 *
 * The icons are loaded from the resources and converted to a bitmap.
 * The bitmaps are destroyed together with the CIconMenu object.
 */
class CIconMenu : public CMenu
{
public:
    CIconMenu();
    ~CIconMenu() override;

    // ReSharper disable once CppHidingFunction
    BOOL CreateMenu();
    // ReSharper disable once CppHidingFunction
    BOOL CreatePopupMenu();
    BOOL AppendMenuIcon(UINT_PTR nIDNewItem, LPCWSTR lpszNewItem, UINT uIcon = 0);
    BOOL AppendMenuIcon(UINT_PTR nIDNewItem, UINT_PTR nNewItem, UINT uIcon = 0);
    BOOL AppendMenuIcon(UINT_PTR nIDNewItem, UINT_PTR nNewItem, HICON hIcon);
    void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override;
    void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) override;

private:
    BOOL SetMenuStyle();

private:
    IconBitmapUtils           m_bitmapUtils;
    std::map<UINT_PTR, UINT>  m_icons;
    std::map<UINT_PTR, HICON> m_iconHandles;
    bool                      m_bShowIcons;
};
