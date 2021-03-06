// Copyright (C) 2007, 2010-2012, 2021 - TortoiseSVN

// this program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// this program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#include "stdafx.h"
#include "UnicodeUtils.h"
#include <memory>

char *AnsiToUtf8(const char *pszAnsi, apr_pool_t *pool)
{
    // convert ANSI --> UTF16
    int  utf16Count = MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, nullptr, 0);
    auto pwc        = std::make_unique<wchar_t[]>(utf16Count);
    MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, pwc.get(), utf16Count);

    // and now from URF16 --> UTF-8
    int   utf8Count = WideCharToMultiByte(CP_UTF8, 0, pwc.get(), utf16Count, nullptr, 0, nullptr, nullptr);
    char *pch       = static_cast<char *>(apr_palloc(pool, utf8Count));
    WideCharToMultiByte(CP_UTF8, 0, pwc.get(), utf16Count, pch, utf8Count, nullptr, nullptr);
    return pch;
}

char *Utf16ToUtf8(const wchar_t *pszUtf16, apr_pool_t *pool)
{
    // from URF16 --> UTF-8
    int   utf8Count = WideCharToMultiByte(CP_UTF8, 0, pszUtf16, -1, nullptr, 0, nullptr, nullptr);
    char *pch       = static_cast<char *>(apr_palloc(pool, utf8Count));
    WideCharToMultiByte(CP_UTF8, 0, pszUtf16, -1, pch, utf8Count, nullptr, nullptr);
    return pch;
}

std::wstring Utf8ToWide(const std::string &string)
{
    const size_t len = string.size();
    auto         buf = std::make_unique<wchar_t[]>(len * 4 + 1);
    SecureZeroMemory(buf.get(), (len * 4 + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, buf.get(), static_cast<int>(len) * 4);
    std::wstring ret = std::wstring(buf.get());
    return ret;
}
