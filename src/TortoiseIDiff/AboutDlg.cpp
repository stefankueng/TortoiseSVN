// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2012-2014, 2017, 2020-2021 - TortoiseSVN

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
#include "resource.h"
#include "AboutDlg.h"
#include "Theme.h"
#include "../version.h"
#include <string>
#include <Commdlg.h>

CAboutDlg::CAboutDlg(HWND hParent)
    : m_hParent(hParent)
    , m_hHiddenWnd(nullptr)
    , m_themeCallbackId(0)
{
}

CAboutDlg::~CAboutDlg()
{
}

LRESULT CAboutDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            InitDialog(hwndDlg, IDI_TORTOISEIDIFF);
            // initialize the controls
            m_link.ConvertStaticToHyperlink(hwndDlg, IDC_WEBLINK, L"https://tortoisesvn.net");
            wchar_t verBuf[1024]  = {0};
            wchar_t maskBuf[1024] = {0};
            if (!::LoadString(hResource, IDS_VERSION, maskBuf, _countof(maskBuf)))
            {
                SecureZeroMemory(maskBuf, sizeof(maskBuf));
            }
            swprintf_s(verBuf, maskBuf, TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
            SetDlgItemText(hwndDlg, IDC_ABOUTVERSION, verBuf);
            CTheme::Instance().SetThemeForDialog(*this, CTheme::Instance().IsDarkTheme());
        }
            return TRUE;
        case WM_COMMAND:
            return DoCommand(LOWORD(wParam));
        default:
            return FALSE;
    }
}

LRESULT CAboutDlg::DoCommand(int id)
{
    switch (id)
    {
        case IDOK:
            // fall through
        case IDCANCEL:
            CTheme::Instance().SetThemeForDialog(*this, false);
            EndDialog(*this, id);
            break;
    }
    return 1;
}
