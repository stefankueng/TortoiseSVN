// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009-2012, 2014-2015, 2021 - TortoiseSVN

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
#include "ResModule.h"

#include <string>
#include <vector>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

int wmain(int argc, wchar_t* argv[])
{
    bool bShowHelp   = true;
    bool bQuiet      = false;
    bool bNoUpdate   = false;
    bool bRTL        = false;
    bool bUseHeader  = false;
    bool bAdjustEOLs = false;
    //parse the command line
    std::vector<std::wstring> arguments;
    std::vector<std::wstring> switches;
    for (int i = 1; i < argc; ++i)
    {
        if ((argv[i][0] == '-') || (argv[i][0] == '/'))
        {
            std::wstring str = std::wstring(&argv[i][1]);
            switches.push_back(str);
        }
        else
        {
            std::wstring str = std::wstring(&argv[i][0]);
            arguments.push_back(str);
        }
    }

    for (auto I = switches.cbegin(); I != switches.cend(); ++I)
    {
        if (wcscmp(I->c_str(), L"?") == 0)
            bShowHelp = true;
        if (wcscmp(I->c_str(), L"help") == 0)
            bShowHelp = true;
        if (wcscmp(I->c_str(), L"quiet") == 0)
            bQuiet = true;
        if (wcscmp(I->c_str(), L"noupdate") == 0)
            bNoUpdate = true;
        if (wcscmp(I->c_str(), L"rtl") == 0)
            bRTL = true;
        if (wcscmp(I->c_str(), L"useheaderfile") == 0)
            bUseHeader = true;
        if (wcscmp(I->c_str(), L"adjusteols") == 0)
            bAdjustEOLs = true;
    }
    auto arg = arguments.cbegin();

    if (arg != arguments.cend())
    {
        if (wcscmp(arg->c_str(), L"extract") == 0)
        {
            std::wstring sPoFile;
            std::wstring sHeaderFile;
            ++arg;

            std::vector<std::wstring> filelist = arguments;
            filelist.erase(filelist.begin());
            sPoFile = std::wstring((--filelist.end())->c_str());
            filelist.erase(--filelist.end());
            if (bUseHeader)
            {
                sHeaderFile = std::wstring((--filelist.end())->c_str());
                filelist.erase(--filelist.end());
            }

            CResModule module;
            module.SetQuiet(bQuiet);
            if (!module.ExtractResources(filelist, sPoFile.c_str(), bNoUpdate, sHeaderFile.c_str()))
                return -1;
            bShowHelp = false;
        }
        else if (wcscmp(arg->c_str(), L"apply") == 0)
        {
            std::wstring sSrcDllFile;
            std::wstring sDstDllFile;
            std::wstring sPoFile;
            WORD         wLang = 0;
            ++arg;
            if (!PathFileExists(arg->c_str()))
            {
                fwprintf(stderr, L"the resource dll <%s> does not exist!\n", arg->c_str());
                return -1;
            }
            sSrcDllFile = std::wstring(arg->c_str());
            ++arg;
            sDstDllFile = std::wstring(arg->c_str());
            ++arg;
            if (!PathFileExists(arg->c_str()))
            {
                fwprintf(stderr, L"the po-file <%s> does not exist!\n", arg->c_str());
                return -1;
            }
            sPoFile = std::wstring(arg->c_str());
            ++arg;
            if (arg != arguments.end())
            {
                wLang = static_cast<WORD>(_wtoi(arg->c_str()));
            }
            CResModule module;
            module.SetQuiet(bQuiet);
            module.SetLanguage(wLang);
            module.SetRTL(bRTL);
            module.SetAdjustEOLs(bAdjustEOLs);
            if (!module.CreateTranslatedResources(sSrcDllFile.c_str(), sDstDllFile.c_str(), sPoFile.c_str()))
                return -1;
            bShowHelp = false;
        }
    }

    if (bShowHelp)
    {
        fwprintf(stdout, L"usage:\n");
        fwprintf(stdout, L"\n");
        fwprintf(stdout, L"ResText extract <resource.dll> [<resource.dll> ...] [-useheaderfile <headerfile>] <po-file> [-quiet] [-noupdate]\n");
        fwprintf(stdout, L"Extracts all strings from the resource dll and writes them to the po-file\n");
        fwprintf(stdout, L"-useheaderfile: the content of the header file instead of a default header\n");
        fwprintf(stdout, L"-quiet: don't print progress messages\n");
        fwprintf(stdout, L"-noupdate: overwrite the po-file\n");
        fwprintf(stdout, L"\n");
        fwprintf(stdout, L"ResText apply <src resource.dll> <dst resource.dll> <po-file> [langID] [-quiet][-rtl]\n");
        fwprintf(stdout, L"Replaces all strings in the dst resource.dll with the po-file translations\n");
        fwprintf(stdout, L"-quiet: don't print progress messages\n");
        fwprintf(stdout, L"-rtl  : change the controls to RTL reading\n");
        fwprintf(stdout, L"-adjusteols : if the msgid string has \\r\\n eols, enforce those for the translation too.\n");
        fwprintf(stdout, L"\n");
        fwprintf(stdout, L"Note: when extracting resources, C-resource header files can be specified\n");
        fwprintf(stdout, L"like this: <resource.dll>*<resource.h>*<resource.h>*...\n");
        fwprintf(stdout, L"If a resource header file is specified, the defines are used in the po file\n");
        fwprintf(stdout, L"as hints instead of the plain control ID number.\n");
    }

    return 0;
}
