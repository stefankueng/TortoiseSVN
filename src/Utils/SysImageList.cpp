// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2010, 2014-2015, 2017, 2020-2021 - TortoiseSVN
// Copyright (C) 2017 - TortoiseGit

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
#include "stdafx.h"
#include "SysImageList.h"
#include "TSVNPath.h"

// Singleton constructor and destructor (private)

CSysImageList* CSysImageList::instance = nullptr;

CSysImageList::CSysImageList()
{
    SHFILEINFO ssFi;
    wchar_t    winDir[MAX_PATH] = {0};
    GetWindowsDirectory(winDir, _countof(winDir)); // MAX_PATH ok.
    hSystemImageList =
        reinterpret_cast<HIMAGELIST>(SHGetFileInfo(winDir,
                                                   0,
                                                   &ssFi, sizeof ssFi,
                                                   SHGFI_SYSICONINDEX | SHGFI_SMALLICON));

    int cx, cy;
    ImageList_GetIconSize(hSystemImageList, &cx, &cy);
    auto emptyImageList = ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK | ILC_HIGHQUALITYSCALE, ImageList_GetImageCount(hSystemImageList), 10);
    Attach(emptyImageList);

    m_dirIconIndex     = GetFileIcon(L"Doesn't matter", FILE_ATTRIBUTE_DIRECTORY, 0);
    m_dirOpenIconIndex = GetFileIcon(L"Doesn't matter", FILE_ATTRIBUTE_DIRECTORY, SHGFI_OPENICON);
    m_defaultIconIndex = GetFileIcon(L"", FILE_ATTRIBUTE_NORMAL, 0);
}

CSysImageList::~CSysImageList()
{
}

// Singleton specific operations

CSysImageList& CSysImageList::GetInstance()
{
    if (!instance)
        instance = new CSysImageList;
    return *instance;
}

void CSysImageList::Cleanup()
{
    delete instance;
    instance = nullptr;
}

// Operations

int CSysImageList::AddIcon(HICON hIcon)
{
    return this->Add(hIcon);
}

int CSysImageList::GetDirIconIndex() const
{
    return m_dirIconIndex;
}

int CSysImageList::GetDirOpenIconIndex() const
{
    return m_dirOpenIconIndex;
}

int CSysImageList::GetDefaultIconIndex() const
{
    return m_defaultIconIndex;
}

int CSysImageList::GetFileIconIndex(const CString& file)
{
    return GetFileIcon(file, FILE_ATTRIBUTE_NORMAL, 0);
}

int CSysImageList::GetPathIconIndex(const CTSVNPath& filePath)
{
    CString strExtension = filePath.GetFileExtension();
    strExtension.MakeUpper();
    auto it = m_indexCache.lower_bound(strExtension);
    if (it == m_indexCache.end() || strExtension < it->first)
    {
        // We don't have this extension in the map
        int iconIndex = GetFileIconIndex(filePath.GetFilename());
        it            = m_indexCache.emplace_hint(it, strExtension, iconIndex);
    }
    // We must have found it
    return it->second;
}

int CSysImageList::GetPathIconIndex(const CString& file)
{
    CString strExtension = file.Mid(file.ReverseFind('.') + 1);
    strExtension.MakeUpper();
    auto it = m_indexCache.lower_bound(strExtension);
    if (it == m_indexCache.end() || strExtension < it->first)
    {
        // We don't have this extension in the map
        int iconIndex = GetFileIconIndex(file);
        it            = m_indexCache.emplace_hint(it, strExtension, iconIndex);
    }
    // We must have found it
    return it->second;
}

int CSysImageList::GetFileIcon(LPCWSTR file, DWORD attributes, UINT extraFlags)
{
    SHFILEINFO sfi = {nullptr};
    SHGetFileInfo(file,
                  attributes,
                  &sfi, sizeof sfi,
                  SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES | extraFlags);

    auto hIcon = ImageList_ExtractIcon(nullptr, hSystemImageList, sfi.iIcon);
    auto index = AddIcon(hIcon);
    DestroyIcon(hIcon);
    return index;
}
