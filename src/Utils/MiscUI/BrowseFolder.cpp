// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2003-2014, 2016, 2021 - TortoiseSVN

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
#include <windowsx.h>
#include "BrowseFolder.h"
#include "OnOutOfScope.h"
#include "FileDlgEventHandler.h"
#include <strsafe.h>

BOOL    CBrowseFolder::m_bCheck  = FALSE;
BOOL    CBrowseFolder::m_bCheck2 = FALSE;
CString CBrowseFolder::m_sDefaultPath;

class BrowseFolderDlgEventHandler : public CFileDlgEventHandler
{
public:
    BrowseFolderDlgEventHandler()
        : m_disableCheckbox2WhenCheckbox1IsChecked(false)
    {
    }

    bool m_disableCheckbox2WhenCheckbox1IsChecked;

    STDMETHODIMP OnCheckButtonToggled(IFileDialogCustomize* pfdc,
                                      DWORD                 dwIDCtl,
                                      BOOL                  bChecked) override
    {
        if (m_disableCheckbox2WhenCheckbox1IsChecked && dwIDCtl == 101)
        {
            if (bChecked)
                pfdc->SetControlState(102, CDCS_VISIBLE | CDCS_INACTIVE);
            else
                pfdc->SetControlState(102, CDCS_VISIBLE | CDCS_ENABLED);
        }
        return S_OK;
    }
};

CBrowseFolder::CBrowseFolder()
    : m_style(0)
    , m_disableCheckbox2WhenCheckbox1IsChecked(false)
{
    SecureZeroMemory(&m_title, sizeof(m_title));
}

CBrowseFolder::~CBrowseFolder()
{
}

//show the dialog
CBrowseFolder::RetVal CBrowseFolder::Show(HWND parent, LPWSTR path, size_t pathlen, LPCWSTR szDefaultPath /* = NULL */)
{
    CString temp = path;
    CString sDefault;
    if (szDefaultPath)
        sDefault = szDefaultPath;
    CBrowseFolder::RetVal ret = Show(parent, temp, sDefault);
    wcscpy_s(path, pathlen, temp);
    return ret;
}
CBrowseFolder::RetVal CBrowseFolder::Show(HWND parent, CString& path, const CString& sDefaultPath /* = CString() */)
{
    m_sDefaultPath = sDefaultPath;
    if (m_sDefaultPath.IsEmpty() && !path.IsEmpty())
    {
        while (!PathFileExists(path) && !path.IsEmpty())
        {
            CString p = path.Left(path.ReverseFind('\\'));
            if ((p.GetLength() == 2) && (p[1] == ':'))
            {
                p += L"\\";
                if (p.Compare(path) == 0)
                    p.Empty();
            }
            if (p.GetLength() < 2)
                p.Empty();
            path = p;
        }
        // if the result path already contains a path, use that as the default path
        m_sDefaultPath = path;
    }

    // Create a new common open file dialog
    CComPtr<IFileOpenDialog> pfd;
    if (FAILED(pfd.CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER)))
        return CANCEL;

    // Set the dialog as a folder picker
    DWORD dwOptions;
    if (FAILED(pfd->GetOptions(&dwOptions)))
        return CANCEL;
    if (FAILED(pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST)))
        return CANCEL;

    // Set a title
    auto* nl = wcschr(m_title, '\n');
    if (nl)
        *nl = 0;
    pfd->SetTitle(m_title);

    // set the default folder
    CComPtr<IShellItem> psiDefault;
    if (FAILED(SHCreateItemFromParsingName(m_sDefaultPath, nullptr, IID_PPV_ARGS(&psiDefault))))
        return CANCEL;
    if (FAILED(pfd->SetFolder(psiDefault)))
        return CANCEL;

    CComObjectStackEx<BrowseFolderDlgEventHandler> cbk;
    cbk.m_disableCheckbox2WhenCheckbox1IsChecked = m_disableCheckbox2WhenCheckbox1IsChecked;
    CComQIPtr<IFileDialogEvents> pEvents         = cbk.GetUnknown();

    if (!m_checkText.IsEmpty())
    {
        CComPtr<IFileDialogCustomize> pfdCustomize;
        if (FAILED(pfd.QueryInterface(&pfdCustomize)))
            return CANCEL;

        pfdCustomize->StartVisualGroup(100, L"");
        pfdCustomize->AddCheckButton(101, m_checkText, FALSE);
        if (!m_checkText2.IsEmpty())
            pfdCustomize->AddCheckButton(102, m_checkText2, FALSE);
        pfdCustomize->EndVisualGroup();
    }

    DWORD eventsCookie;
    if (FAILED(pfd->Advise(pEvents, &eventsCookie)))
        return CANCEL;

    OnOutOfScope(pfd->Unadvise(eventsCookie););

    // Show the open file dialog
    if (FAILED(pfd->Show(parent)))
        return CANCEL;

    // Get the selection from the user
    CComPtr<IShellItem> psiResult;
    if (FAILED(pfd->GetResult(&psiResult)))
        return CANCEL;

    PWSTR pszPath = nullptr;
    if (SUCCEEDED(psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
    {
        path = pszPath;
        CoTaskMemFree(pszPath);
    }

    CComPtr<IFileDialogCustomize> pfdCustomize;
    if (SUCCEEDED(pfd.QueryInterface(&pfdCustomize)))
    {
        pfdCustomize->GetCheckButtonState(101, &m_bCheck);
        pfdCustomize->GetCheckButtonState(102, &m_bCheck2);
    }

    return OK;
}

void CBrowseFolder::SetInfo(LPCWSTR title)
{
    ASSERT(title);

    if (title)
        wcscpy_s(m_title, title);
}

void CBrowseFolder::SetCheckBoxText(LPCWSTR checktext)
{
    ASSERT(checktext);

    if (checktext)
        m_checkText = checktext;
}

void CBrowseFolder::SetCheckBoxText2(LPCWSTR checktext)
{
    ASSERT(checktext);

    if (checktext)
        m_checkText2 = checktext;
}
