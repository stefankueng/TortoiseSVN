﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013, 2015, 2021 - TortoiseSVN

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
#include <string>
#include <memory>

/**
 * Loads a string from the application resources.
 */
class ResString
{
public:
    ResString(HINSTANCE hInstance, int resId)
    {
        int bufSize = 1024;
        str.clear();
        do
        {
            auto buf = std::make_unique<wchar_t[]>(bufSize);
            int  ret = ::LoadString(hInstance, resId, buf.get(), bufSize);
            if (ret == (bufSize - 1))
                bufSize *= 2;
            else
                str = buf.get();
        } while (str.empty());
    }
    operator wchar_t const *() const { return str.c_str(); }
    operator std::wstring() const { return str; }

private:
    std::wstring str;
};
