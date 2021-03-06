// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016, 2021 - TortoiseSVN

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
#include "SVNDiffOptions.h"

SVNDiffOptions::SVNDiffOptions()
    : m_bIgnoreEOL(false)
    , m_ignoreSpace(svn_diff_file_ignore_space_none)
{
}

CString SVNDiffOptions::GetOptionsString() const
{
    CString opts;
    if (m_bIgnoreEOL)
        opts.Append(L"--ignore-eol-style ");

    switch (m_ignoreSpace)
    {
        case svn_diff_file_ignore_space_change:
            opts.Append(L"-b");
            break;
        case svn_diff_file_ignore_space_all:
            opts.Append(L"-w");
            break;
        case svn_diff_file_ignore_space_none:
            break;
        default:
            break;
    }
    opts.Trim();
    return opts;
}
