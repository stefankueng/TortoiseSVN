// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011 - TortoiseSVN

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
#include "StdAfx.h"
#include "UsageStats.h"
#include "PathUtils.h"

UsageStats::UsageStats(void)
{
    statFilePath = CPathUtils::GetAppDataDirectory();
    statFilePath += L"\\usagestats";
    Load(statFilePath.c_str());
}

UsageStats::~UsageStats(void)
{
    Save(statFilePath.c_str());
}

UsageStats& UsageStats::Instance()
{
    static UsageStats instance;
    return instance;
}

int UsageStats::SetEvent( LPCWSTR name )
{
    int value = (*this)[name];
    value++;
    (*this)[name] = value;
    return value;
}



