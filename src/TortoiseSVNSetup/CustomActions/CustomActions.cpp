﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010, 2012-2014, 2017, 2021 - TortoiseSVN

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

/* BIG FAT WARNING: Do not use any functions which require the C-Runtime library
   in this custom action dll! The runtimes might not be installed yet!
*/

#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "shell32")

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;

#ifdef _WIN64
#    define TSVN_CACHE_WINDOW_NAME L"TSVNCacheWindow64"
#else
#    define TSVN_CACHE_WINDOW_NAME L"TSVNCacheWindow"
#endif

BOOL APIENTRY DllMain(HANDLE /*hModule*/,
                      DWORD /*ul_reason_for_call*/,
                      LPVOID /*lpReserved*/
)
{
    return TRUE;
}

UINT __stdcall TerminateCache(MSIHANDLE /*hModule*/)
{
    HWND hWnd = FindWindow(TSVN_CACHE_WINDOW_NAME, TSVN_CACHE_WINDOW_NAME);
    if (hWnd)
    {
        PostMessage(hWnd, WM_CLOSE, NULL, NULL);
        for (int i = 0; i < 10; ++i)
        {
            Sleep(500);
            if (!IsWindow(hWnd))
            {
                // Cache is gone!
                return ERROR_SUCCESS;
            }
        }
        // Don't return ERROR_FUNCTION_FAILED, because even if the cache is still
        // running, the installer will overwrite the file, and we require a
        // reboot anyway after upgrading.
        return ERROR_SUCCESS;
    }
    // cache wasn't even running
    return ERROR_SUCCESS;
}

UINT __stdcall OpenDonatePage(MSIHANDLE /*hModule*/)
{
    ShellExecute(nullptr, L"open", L"https://tortoisesvn.net/donate.html", nullptr, nullptr, SW_SHOW);
    return ERROR_SUCCESS;
}

UINT __stdcall MsgBox(MSIHANDLE /*hModule*/)
{
    MessageBox(nullptr, L"CustomAction \"MsgBox\" running", L"Installer", MB_ICONINFORMATION);
    return ERROR_SUCCESS;
}

UINT __stdcall SetLanguage(MSIHANDLE hModule)
{
    wchar_t codebuf[30] = {0};
    DWORD   count       = _countof(codebuf);
    MsiGetProperty(hModule, L"COUNTRYID", codebuf, &count);
    DWORD lang = _wtol(codebuf);
    SHRegSetUSValue(L"Software\\TortoiseSVN", L"LanguageID", REG_DWORD, &lang, sizeof(DWORD), SHREGSET_FORCE_HKCU);
    return ERROR_SUCCESS;
}

UINT __stdcall RegisterSparsePackage(MSIHANDLE hModule)
{
    DWORD len = 0;
    MsiGetPropertyW(hModule, L"INSTALLDIR", L"", &len);
    auto sparseExtPath = std::make_unique<wchar_t[]>(len + 1LL);
    len += 1;
    MsiGetPropertyW(hModule, L"INSTALLDIR", sparseExtPath.get(), &len);

    len = 0;
    MsiGetPropertyW(hModule, L"SPARSEPACKAGEFILE", L"", &len);
    auto sparsePackageFile = std::make_unique<wchar_t[]>(len + 1LL);
    len += 1;
    MsiGetPropertyW(hModule, L"SPARSEPACKAGEFILE", sparsePackageFile.get(), &len);

    std::wstring sSparsePackagePath = sparseExtPath.get();
    sSparsePackagePath += L"\\bin\\";
    sSparsePackagePath += sparsePackageFile.get();

    PackageManager    manager;
    AddPackageOptions options;
    Uri               externalUri(sparseExtPath.get());
    Uri               packageUri(sSparsePackagePath.c_str());
    options.ExternalLocationUri(externalUri);
    auto deploymentOperation = manager.AddPackageByUriAsync(packageUri, options);

    auto deployResult = deploymentOperation.get();

    if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
    {
        // Deployment failed
        return deployResult.ExtendedErrorCode();
    }
    return ERROR_SUCCESS;
}

UINT __stdcall UnregisterSparsePackage(MSIHANDLE hModule)
{
    DWORD len = 0;
    MsiGetPropertyW(hModule, L"SPARSEPACKAGENAME", L"", &len);
    auto sparsePackageName = std::make_unique<wchar_t[]>(len + 1LL);
    len += 1;
    MsiGetPropertyW(hModule, L"SPARSEPACKAGENAME", sparsePackageName.get(), &len);

    PackageManager packageManager;

    auto           packages = packageManager.FindPackages();
    winrt::hstring fullName = sparsePackageName.get();
    for (const auto& package : packages)
    {
        if (package.Id().Name() == sparsePackageName.get())
            fullName = package.Id().FullName();
    }

    auto deploymentOperation = packageManager.RemovePackageAsync(fullName, RemovalOptions::None);
    auto deployResult        = deploymentOperation.get();
    if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
    {
        // Deployment failed
        return deployResult.ExtendedErrorCode();
    }

    return ERROR_SUCCESS;
}