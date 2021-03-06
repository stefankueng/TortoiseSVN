// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2012, 2014-2015, 2021 - TortoiseSVN

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
#include "RepositoryBrowserCommand.h"

#include "RepositoryBrowser.h"
#include "URLDlg.h"
#include "SVN.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include "StringUtils.h"

bool RepositoryBrowserCommand::Execute()
{
    CString url;
    SVN svn;
    if (!cmdLinePath.IsEmpty())
    {
        if (cmdLinePath.GetSVNPathString().Left(4).CompareNoCase(L"svn:")==0)
        {
            // If the path starts with "svn:" and there is another protocol
            // found in the path (a "://" found after the "svn:") then
            // remove "svn:" from the beginning of the path.
            if (cmdLinePath.GetSVNPathString().Find(L"://", 4)>=0)
                cmdLinePath.SetFromSVN(cmdLinePath.GetSVNPathString().Mid(4));
        }

        url = svn.GetURLFromPath(cmdLinePath);

        if (url.IsEmpty())
        {
            if (SVN::PathIsURL(cmdLinePath))
                url = cmdLinePath.GetSVNPathString();
            else if (svn.IsRepository(cmdLinePath))
            {
                // The path points to a local repository.
                // Add 'file:///' so the repository browser recognizes
                // it as an URL to the local repository.
                if (cmdLinePath.GetWinPathString().GetAt(0) == '\\')    // starts with '\' means an UNC path
                {
                    CString p = cmdLinePath.GetWinPathString();
                    p.TrimLeft('\\');
                    url = L"file://"+p;
                }
                else
                    url = L"file:///"+cmdLinePath.GetWinPathString();
                url.Replace('\\', '/');
            }
        }
    }
    if (cmdLinePath.GetUIPathString().Left(7).CompareNoCase(L"file://")==0)
    {
        cmdLinePath.SetFromUnknown(cmdLinePath.GetUIPathString().Mid(7));
    }

    if (url.IsEmpty())
    {
        CURLDlg urldlg;
        if (urldlg.DoModal() != IDOK)
        {
            return false;
        }
        url = urldlg.m_url;
        cmdLinePath = CTSVNPath(url);
    }

    if (!CTSVNPath(url).IsCanonical())
    {
        CString sErr;
        sErr.Format(IDS_ERR_INVALIDURLORPATH, (LPCWSTR)url);
        ::MessageBox(GetExplorerHWND(), sErr, L"TortoiseSVN", MB_ICONERROR);
        return false;
    }
    CString val = parser.GetVal(L"rev");
    SVNRev rev(val);
    CRepositoryBrowser dlg(url, rev);
    if (parser.HasVal(L"pegrev"))
    {
        val = parser.GetVal(L"pegrev");
        SVNRev pegRev(val);
        dlg.SetPegRev(pegRev);
    }
    if (!cmdLinePath.IsUrl())
        dlg.m_projectProperties.ReadProps(cmdLinePath);
    else
    {
        if (parser.HasVal(L"projectpropertiespath"))
        {
            dlg.m_projectProperties.ReadProps(CTSVNPath(parser.GetVal(L"projectpropertiespath")));
        }
    }
    if (parser.HasKey(L"sparse"))
    {
        if (SVN::PathIsURL(cmdLinePath))
            dlg.SetSparseCheckoutMode(CTSVNPath());
        else
            dlg.SetSparseCheckoutMode(cmdLinePath);
    }
    if (parser.HasVal(L"outfile"))
    {
        dlg.SetStandaloneMode(false);
    }
    dlg.m_path = cmdLinePath;
    auto dlgret = dlg.DoModal();
    if (parser.HasVal(L"outfile"))
    {
        CString sText;
        if (dlgret == IDOK)
        {
            sText = dlg.GetPath();
            sText += L"\n";
            sText += dlg.GetRevision().ToString();
        }
        CStringUtils::WriteStringToTextFile(parser.GetVal(L"outfile"), (LPCWSTR)sText, true);
    }
    return true;
}