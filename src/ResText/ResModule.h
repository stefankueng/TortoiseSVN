﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2019 - TortoiseGit
// Copyright (C) 2003-2007, 2011-2012, 2014-2017, 2021 - TortoiseSVN

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
#include <vector>
#include <string>
#include <map>
#include "POFile.h"

#define GET_WORD(ptr)        (*(WORD*)(ptr))
#define GET_DWORD(ptr)       (*(DWORD*)(ptr))
#define ALIGN_DWORD(type, p) ((type)(((DWORD)p + 3) & ~3))

#define MAX_STRING_LENGTH (32 * 1024)

// DIALOG CONTROL INFORMATION
typedef struct TagDlgItemInfo
{
    DWORD   style;
    DWORD   exStyle;
    DWORD   helpId;
    short   x;
    short   y;
    short   cx;
    short   cy;
    WORD    id;
    LPCWSTR className;
    LPCWSTR windowName;
    LPVOID  data;
} DLGITEMINFO, *LPDLGITEMINFO;

// DIALOG TEMPLATE
typedef struct TagDialogInfo
{
    DWORD   style;
    DWORD   exStyle;
    DWORD   helpId;
    WORD    nbItems;
    short   x;
    short   y;
    short   cx;
    short   cy;
    LPCWSTR menuName;
    LPCWSTR className;
    LPCWSTR caption;
    WORD    pointSize;
    WORD    weight;
    BOOL    italic;
    LPCWSTR faceName;
    BOOL    dialogEx;
} DIALOGINFO, *LPDIALOGINFO;
// MENU resource
typedef struct TagMenuEntry
{
    WORD         wID;
    std::wstring reference;
    std::wstring msgstr;
} MENUENTRY, *LPMENUENTRY;

/**
 * \ingroup ResText
 * Class to handle a resource module (*.exe or *.dll file).
 *
 * Provides methods to extract and apply resource strings.
 */
class CResModule
{
public:
    CResModule();
    ~CResModule();

    BOOL ExtractResources(LPCWSTR lpszSrcLangDllPath, LPCWSTR lpszPOFilePath, BOOL bNoUpdate, LPCWSTR lpszHeaderFile);
    BOOL ExtractResources(const std::vector<std::wstring>& fileList, LPCWSTR lpszPoFilePath, BOOL bNoUpdate, LPCWSTR lpszHeaderFile);
    BOOL CreateTranslatedResources(LPCWSTR lpszSrcLangDllPath, LPCWSTR lpszDestLangDllPath, LPCWSTR lpszPoFilePath);
    void SetQuiet(BOOL bQuiet = TRUE)
    {
        m_bQuiet = bQuiet;
        m_stringEntries.SetQuiet(bQuiet);
    }
    void SetLanguage(WORD wLangID) { m_wTargetLang = wLangID; }
    void SetRTL(bool bRTL = true) { m_bRTL = bRTL; }
    void SetAdjustEOLs(bool bAdjustEOLs = true) { m_bAdjustEOLs = bAdjustEOLs; }

private:
    static BOOL CALLBACK EnumResNameCallback(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam);
    static BOOL CALLBACK EnumResNameWriteCallback(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam);
    static BOOL CALLBACK EnumResWriteLangCallback(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, WORD wLanguage, LONG_PTR lParam);
    BOOL                 ExtractString(LPCWSTR lpszType);
    BOOL                 ExtractDialog(LPCWSTR lpszType);
    BOOL                 ExtractMenu(LPCWSTR lpszType);
    BOOL                 ExtractRibbon(LPCWSTR lpszType);
    BOOL                 ReplaceString(LPCWSTR lpszType, WORD wLanguage);
    BOOL                 ReplaceDialog(LPCWSTR lpszType, WORD wLanguage);
    BOOL                 ReplaceMenu(LPCWSTR lpszType, WORD wLanguage);
    BOOL                 ExtractAccelerator(LPCWSTR lpszType);
    BOOL                 ReplaceAccelerator(LPCWSTR lpszType, WORD wLanguage);
    BOOL                 ReplaceRibbon(LPCWSTR lpszType, WORD wLanguage);

    template <size_t Size>
    inline std::wstring ReplaceWithRegex(WCHAR (&pBuf)[Size])
    {
        return ReplaceWithRegex(pBuf, Size);
    }
    std::wstring ReplaceWithRegex(WCHAR* pBuf, size_t bufferSize);
    std::wstring ReplaceWithRegex(std::wstring& s);

    const WORD*        ParseMenuResource(const WORD* res);
    const WORD*        CountMemReplaceMenuResource(const WORD* res, size_t* wordcount, WORD* newMenu);
    const WORD*        ParseMenuExResource(const WORD* res);
    const WORD*        CountMemReplaceMenuExResource(const WORD* res, size_t* wordCount, WORD* newMenu);
    static const WORD* GetControlInfo(const WORD* p, LPDLGITEMINFO lpDlgItemInfo, BOOL dialogEx, LPBOOL bIsID);
    static const WORD* GetDialogInfo(const WORD* pTemplate, LPDIALOGINFO lpDlgInfo);
    const WORD*        CountMemReplaceDialogResource(const WORD* res, size_t* wordCount, WORD* newMenu);
    const WORD*        ReplaceControlInfo(const WORD* res, size_t* wordCount, WORD* newDialog, BOOL bEx);

    void ReplaceStr(LPCWSTR src, WORD* dest, size_t* count, int* translated, int* def);

    size_t      ScanHeaderFile(const std::wstring& filepath);
    void        InsertResourceIDs(LPCWSTR lpType, INT_PTR mainId, TagResourceEntry& entry, INT_PTR id, LPCWSTR infoText);
    static bool AdjustCheckSum(const std::wstring& resFile);
    static void RemoveSignatures(LPCWSTR lpszDestLangDllPath);

    HMODULE                         m_hResDll;
    HANDLE                          m_hUpdateRes;
    CPOFile                         m_stringEntries;
    std::map<WORD, MENUENTRY>       m_menuEntries;
    std::wstring                    sDestFile;
    std::map<INT_PTR, std::wstring> m_currentHeaderDataDialogs;
    std::map<INT_PTR, std::wstring> m_currentHeaderDataStrings;
    std::map<INT_PTR, std::wstring> m_currentHeaderDataMenus;
    BOOL                            m_bQuiet;

    bool m_bRTL;
    bool m_bAdjustEOLs;

    int m_bTranslatedStrings;
    int m_bDefaultStrings;
    int m_bTranslatedDialogStrings;
    int m_bDefaultDialogStrings;
    int m_bTranslatedMenuStrings;
    int m_bDefaultMenuStrings;
    int m_bTranslatedAcceleratorStrings;
    int m_bDefaultAcceleratorStrings;
    int m_bTranslatedRibbonTexts;
    int m_bDefaultRibbonTexts;

    WORD m_wTargetLang;
};
