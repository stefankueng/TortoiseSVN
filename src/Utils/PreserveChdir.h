// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2010, 2012, 2017, 2021 - TortoiseSVN

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

/**
 * \ingroup TortoiseShell
 * Helper class to keep the current directory.
 * When created, the current directory is saved and restored
 * as soon as the object is destroyed.
 * Since some of the SVN functions change the current directory
 * which when in the explorer process space causes problems / crashes,
 * this class is used to reset the current directory after calling
 * those functions.
 */
class PreserveChdir
{
public:
    PreserveChdir();  ///< saves originalCurrentDirectory
    ~PreserveChdir(); ///< restores originalCurrentDirectory

private:
    PreserveChdir(const PreserveChdir&) = delete;            ///< non-copyable
    PreserveChdir& operator=(const PreserveChdir&) = delete; ///< non-assignable

    const size_t               m_size;                     ///< size of originalCurrentDirectory
    std::unique_ptr<wchar_t[]> m_originalCurrentDirectory; ///< %CD% at ctor time
};
