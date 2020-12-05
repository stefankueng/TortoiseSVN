﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2020 - TortoiseSVN

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
#include "Hash.h"

std::wstring GetHashText(const void* data, const size_t data_size, HashType hashType)
{
    HCRYPTPROV hProv = NULL;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        return {};
    }

    BOOL       hash_ok = FALSE;
    HCRYPTPROV hHash   = NULL;
    switch (hashType)
    {
        case HashType::HashSha1:
            hash_ok = CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
            break;
        case HashType::HashMd5:
            hash_ok = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
            break;
        case HashType::HashSha256:
            hash_ok = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
            break;
    }

    if (!hash_ok)
    {
        CryptReleaseContext(hProv, 0);
        return {};
    }

    if (!CryptHashData(hHash, static_cast<const BYTE*>(data), static_cast<DWORD>(data_size), 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    DWORD cbHashSize = 0, dwCount = sizeof(DWORD);
    if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&cbHashSize, &dwCount, 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    std::vector<BYTE> buffer(cbHashSize);
    if (!CryptGetHashParam(hHash, HP_HASHVAL, reinterpret_cast<BYTE*>(&buffer[0]), &cbHashSize, 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    std::wostringstream oss;
    oss.fill('0');
    oss.width(2);

    for (const auto& b : buffer)
    {
        oss << std::hex << static_cast<const int>(b);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return oss.str();
}
