// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008, 2010, 2014 - TortoiseSVN

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
#include <fstream>
#include "PersonalDictionary.h"
#include "PathUtils.h"

CPersonalDictionary::CPersonalDictionary(LONG lLanguage /* = 0*/) :
    m_bLoaded(false)
{
    m_lLanguage = lLanguage;
}

CPersonalDictionary::~CPersonalDictionary()
{
}

bool CPersonalDictionary::Load()
{
    CString sWord;
    TCHAR line[PDICT_MAX_WORD_LENGTH + 1];

    if (m_bLoaded)
        return true;
    TCHAR path[MAX_PATH] = { 0 };       //MAX_PATH ok here.
    wcscpy_s (path, CPathUtils::GetAppDataDirectory());

    if (m_lLanguage==0)
        m_lLanguage = GetUserDefaultLCID();

    TCHAR sLang[10] = { 0 };
    swprintf_s(sLang, L"%ld", m_lLanguage);
    wcscat_s(path, sLang);
    wcscat_s(path, L".dic");

    std::wifstream File;
    char filepath[MAX_PATH + 1] = { 0 };
    SecureZeroMemory(filepath, sizeof(filepath));
    WideCharToMultiByte(CP_ACP, NULL, path, -1, filepath, _countof(filepath)-1, NULL, NULL);
    File.open(filepath);
    if (!File.good())
    {
        return false;
    }
    do
    {
        File.getline(line, _countof(line));
        sWord = line;
        dict.insert(sWord);
    } while (File.gcount() > 0);
    File.close();
    m_bLoaded = true;
    return true;
}

bool CPersonalDictionary::AddWord(const CString& sWord)
{
    if (!m_bLoaded)
        Load();
    if (sWord.GetLength() >= PDICT_MAX_WORD_LENGTH)
        return false;
    dict.insert(sWord);
    return true;
}

bool CPersonalDictionary::FindWord(const CString& sWord)
{
    if (!m_bLoaded)
        Load();
    // even if the load failed for some reason, we mark it as loaded
    // and just assume an empty personal dictionary
    m_bLoaded = true;
    std::set<CString>::iterator it;
    it = dict.find(sWord);
    return (it != dict.end());
}

bool CPersonalDictionary::Save()
{
    if (!m_bLoaded)
        return false;
    TCHAR path[MAX_PATH] = { 0 };       //MAX_PATH ok here.
    wcscpy_s (path, CPathUtils::GetAppDataDirectory());

    if (m_lLanguage==0)
        m_lLanguage = GetUserDefaultLCID();

    TCHAR sLang[10] = { 0 };
    swprintf_s(sLang, L"%ld", m_lLanguage);
    wcscat_s(path, sLang);
    wcscat_s(path, L".dic");

    std::wofstream File;
    char filepath[MAX_PATH + 1] = { 0 };
    SecureZeroMemory(filepath, sizeof(filepath));
    WideCharToMultiByte(CP_ACP, NULL, path, -1, filepath, _countof(filepath)-1, NULL, NULL);
    File.open(filepath, std::ios_base::binary);
    for (std::set<CString>::iterator it = dict.begin(); it != dict.end(); ++it)
    {
        File << (LPCTSTR)*it << L"\n";
    }
    File.close();
    return true;
}
