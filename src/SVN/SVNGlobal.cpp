// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009, 2013-2014 - TortoiseSVN

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
#include "stdafx.h"
#include "registry.h"
#include "UnicodeUtils.h"
#include "SVNGlobal.h"

char * g_pConfigDir = NULL;
SVNGlobal g_SVNGlobal;

SVNGlobal::SVNGlobal()
{
    CRegStdString regConfigDir = CRegStdString(L"Software\\TortoiseSVN\\ConfigDir");
    tstring sConfigDir = regConfigDir;
    if (!sConfigDir.empty())
    {
        g_pConfigDir = StrDupA(CUnicodeUtils::StdGetUTF8(sConfigDir).c_str());
    }
}

SVNGlobal::~SVNGlobal()
{
    if (g_pConfigDir)
        LocalFree(g_pConfigDir);
}

void SVNGlobal::SetConfigDir(CString sConfigDir)
{
    if (g_pConfigDir)
    {
        LocalFree(g_pConfigDir);
        g_pConfigDir = NULL;
    }
    if (sConfigDir.IsEmpty())
        return;
    g_pConfigDir = StrDupA((LPCSTR)CUnicodeUtils::GetUTF8(sConfigDir));
}