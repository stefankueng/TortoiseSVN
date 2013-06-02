// Copyright (C) 2007, 2011-2013 - TortoiseSVN

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
#pragma once
#include "apr_pools.h"
#include <string>
/**
 * \ingroup SubWCRev
 * Converts an ANSI string to UTF8.
 */
char * AnsiToUtf8(const char * pszAnsi, apr_pool_t *pool);

/**
 * \ingroup SubWCRev
 * Converts an UTF16 string to UTF8
 */
char * Utf16ToUtf8(const WCHAR *pszUtf16, apr_pool_t *pool);

/**
 * \ingroup SubWCRev
 * Converts an UTF8 string to UTF16
 */
std::wstring Utf8ToWide(const std::string& string);

