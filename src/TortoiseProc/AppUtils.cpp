﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2020, 2021 - TortoiseSVN
// Copyright (C) 2015, 2019 - TortoiseGit

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
#include "TortoiseProc.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "SVNProperties.h"
#include "StringUtils.h"
#include "registry.h"
#include "TSVNPath.h"
#include "SVN.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include <intshcut.h>
#include "DirFileEnum.h"
#include "SmartHandle.h"
#include "SVNExternals.h"
#include "CmdLineParser.h"
#include "../Utils/DPIAware.h"

bool CAppUtils::GetMimeType(const CTSVNPath& file, CString& mimetype, const SVNRev& rev /*= SVNRev::REV_WC*/)
{
    // only fetch the mime-type for local paths, or for urls if a mime-type specific diff tool
    // is configured.
    if (!file.IsUrl() || CAppUtils::HasMimeTool())
    {
        SVNProperties props(file, file.IsUrl() ? rev : SVNRev::REV_WC, false, false);
        for (int i = 0; i < props.GetCount(); ++i)
        {
            if (props.GetItemName(i).compare(SVN_PROP_MIME_TYPE) == 0)
            {
                mimetype = props.GetItemValue(i).c_str();
                return true;
            }
        }
    }
    return false;
}

BOOL CAppUtils::StartExtMerge(const MergeFlags& flags,
                              const CTSVNPath& baseFile, const CTSVNPath& theirFile, const CTSVNPath& yourFile, const CTSVNPath& mergedFile, bool bSaveRequired,
                              const CString& baseName /*= CString()*/, const CString& theirName /*= CString()*/, const CString& yourName /*= CString()*/, const CString& mergedName /*= CString()*/, const CString& fileName /*= CString()*/)
{
    CRegString regCom    = CRegString(L"Software\\TortoiseSVN\\Merge");
    CString    ext       = mergedFile.GetFileExtension();
    CString    com       = regCom;
    bool       bInternal = false;

    CString mimeType;

    if (!ext.IsEmpty())
    {
        // is there an extension specific merge tool?
        CRegString mergeTool(L"Software\\TortoiseSVN\\MergeTools\\" + ext.MakeLower());
        if (!CString(mergeTool).IsEmpty())
        {
            com = mergeTool;
        }
    }
    if (HasMimeTool())
    {
        if (GetMimeType(yourFile, mimeType) || GetMimeType(theirFile, mimeType) || GetMimeType(baseFile, mimeType))
        {
            // is there a mime type specific merge tool?
            CRegString mergeTool(L"Software\\TortoiseSVN\\MergeTools\\" + mimeType);
            if (CString(mergeTool) != "")
            {
                com = mergeTool;
            }
        }
    }
    // is there a filename specific merge tool?
    CRegString mergeTool(L"Software\\TortoiseSVN\\MergeTools\\." + mergedFile.GetFilename().MakeLower());
    if (!CString(mergeTool).IsEmpty())
    {
        com = mergeTool;
    }

    if ((flags.bAlternativeTool) && (!com.IsEmpty()))
    {
        if (com.Left(1).Compare(L"#") == 0)
            com.Delete(0);
        else
            com.Empty();
    }

    if (com.IsEmpty() || (com.Left(1).Compare(L"#") == 0))
    {
        // Maybe we should use TortoiseIDiff?
        if ((ext == L".jpg") || (ext == L".jpeg") ||
            (ext == L".bmp") || (ext == L".gif") ||
            (ext == L".png") || (ext == L".ico") ||
            (ext == L".tif") || (ext == L".tiff") ||
            (ext == L".dib") || (ext == L".emf") ||
            (ext == L".cur") || (ext == L".webp"))
        {
            com = CPathUtils::GetAppDirectory() + L"TortoiseIDiff.exe";
            com = L"\"" + com + L"\"";
            com = com + L" /base:%base /theirs:%theirs /mine:%mine /result:%merged";
            com = com + L" /basetitle:%bname /theirstitle:%tname /minetitle:%yname";
        }
        else
        {
            // use TortoiseMerge
            bInternal = true;
            com       = CPathUtils::GetAppDirectory() + L"TortoiseMerge.exe";
            com       = L"\"" + com + L"\"";
            com       = com + L" /base:%base /theirs:%theirs /mine:%mine /merged:%merged";
            com       = com + L" /basename:%bname /theirsname:%tname /minename:%yname /mergedname:%mname";
        }
        if (!g_sGroupingUuid.IsEmpty())
        {
            com += L" /groupuuid:\"";
            com += g_sGroupingUuid;
            com += L"\"";
        }
        if (bSaveRequired)
        {
            com += L" /saverequired";
            CCmdLineParser parser(GetCommandLine());
            HWND           resolveMsgWnd    = parser.HasVal(L"resolvemsghwnd") ? reinterpret_cast<HWND>(parser.GetLongLongVal(L"resolvemsghwnd")) : nullptr;
            WPARAM         resolveMsgWParam = parser.HasVal(L"resolvemsgwparam") ? static_cast<WPARAM>(parser.GetLongLongVal(L"resolvemsgwparam")) : 0;
            LPARAM         resolveMsgLParam = parser.HasVal(L"resolvemsglparam") ? static_cast<LPARAM>(parser.GetLongLongVal(L"resolvemsglparam")) : 0;
            if (resolveMsgWnd)
            {
                CString s;
                s.Format(L" /resolvemsghwnd:%I64d /resolvemsgwparam:%I64d /resolvemsglparam:%I64d", reinterpret_cast<long long>(resolveMsgWnd), static_cast<long long>(resolveMsgWParam), static_cast<long long>(resolveMsgLParam));
                com += s;
            }
        }
    }
    // check if the params are set. If not, just add the files to the command line
    if ((com.Find(L"%merged") < 0) && (com.Find(L"%base") < 0) && (com.Find(L"%theirs") < 0) && (com.Find(L"%mine") < 0))
    {
        com += L" \"" + baseFile.GetWinPathString() + L"\"";
        com += L" \"" + theirFile.GetWinPathString() + L"\"";
        com += L" \"" + yourFile.GetWinPathString() + L"\"";
        com += L" \"" + mergedFile.GetWinPathString() + L"\"";
    }
    if (baseFile.IsEmpty())
    {
        com.Replace(L"/base:%base", L"");
        com.Replace(L"%base", L"");
    }
    else
        com.Replace(L"%base", L"\"" + baseFile.GetWinPathString() + L"\"");
    if (theirFile.IsEmpty())
    {
        com.Replace(L"/theirs:%theirs", L"");
        com.Replace(L"%theirs", L"");
    }
    else
        com.Replace(L"%theirs", L"\"" + theirFile.GetWinPathString() + L"\"");
    if (yourFile.IsEmpty())
    {
        com.Replace(L"/mine:%mine", L"");
        com.Replace(L"%mine", L"");
    }
    else
        com.Replace(L"%mine", L"\"" + yourFile.GetWinPathString() + L"\"");
    if (mergedFile.IsEmpty())
    {
        com.Replace(L"/merged:%merged", L"");
        com.Replace(L"%merged", L"");
    }
    else
    {
        SVNProperties props(mergedFile, SVNRev::REV_WC, false, false);
        if (props.HasProperty("svn:needs-lock"))
        {
            // remove the readonly attribute
            ::SetFileAttributes(mergedFile.GetWinPath(), FILE_ATTRIBUTE_NORMAL);
        }
        com.Replace(L"%merged", L"\"" + mergedFile.GetWinPathString() + L"\"");
    }
    if (baseName.IsEmpty())
    {
        if (baseFile.IsEmpty())
        {
            com.Replace(L"/basename:%bname", L"");
            com.Replace(L"%bname", L"");
            com.Replace(L"%nqbname", L"");
        }
        else
        {
            com.Replace(L"%bname", L"\"" + baseFile.GetUIFileOrDirectoryName() + L"\"");
            com.Replace(L"%nqbname", baseFile.GetUIFileOrDirectoryName());
        }
    }
    else
    {
        com.Replace(L"%bname", L"\"" + baseName + L"\"");
        com.Replace(L"%nqbname", baseName);
    }
    if (theirName.IsEmpty())
    {
        if (theirFile.IsEmpty())
        {
            com.Replace(L"/theirsname:%tname", L"");
            com.Replace(L"%tname", L"");
            com.Replace(L"%nqtname", L"");
        }
        else
        {
            com.Replace(L"%tname", L"\"" + theirFile.GetUIFileOrDirectoryName() + L"\"");
            com.Replace(L"%nqtname", theirFile.GetUIFileOrDirectoryName());
        }
    }
    else
    {
        com.Replace(L"%tname", L"\"" + theirName + L"\"");
        com.Replace(L"%nqtname", theirName);
    }
    if (yourName.IsEmpty())
    {
        if (yourFile.IsEmpty())
        {
            com.Replace(L"/minename:%yname", L"");
            com.Replace(L"%yname", L"");
            com.Replace(L"%nqyname", L"");
        }
        else
        {
            com.Replace(L"%yname", L"\"" + yourFile.GetUIFileOrDirectoryName() + L"\"");
            com.Replace(L"%nqyname", yourFile.GetUIFileOrDirectoryName());
        }
    }
    else
    {
        com.Replace(L"%yname", L"\"" + yourName + L"\"");
        com.Replace(L"%nqyname", yourName);
    }
    if (mergedName.IsEmpty())
    {
        if (mergedFile.IsEmpty())
        {
            com.Replace(L"/mergedname:%mname", L"");
            com.Replace(L"%mname", L"");
            com.Replace(L"%nqmname", L"");
        }
        else
        {
            com.Replace(L"%mname", L"\"" + mergedFile.GetUIFileOrDirectoryName() + L"\"");
            com.Replace(L"%nqmname", mergedFile.GetUIFileOrDirectoryName());
        }
    }
    else
    {
        com.Replace(L"%mname", L"\"" + mergedName + L"\"");
        com.Replace(L"%nqmname", mergedName);
    }
    if (fileName.IsEmpty())
    {
        com.Replace(L"%fname", L"");
        com.Replace(L"%nqfname", L"");
    }
    else
    {
        com.Replace(L"%fname", L"\"" + fileName + L"\"");
        com.Replace(L"%nqfname", fileName);
    }

    if ((flags.bReadOnly) && (bInternal))
        com += L" /readonly";
    if ((flags.bPreventSVNResolve) && (bInternal))
        com += L" /nosvnresolve";

    if (!LaunchApplication(com, IDS_ERR_EXTMERGESTART, false))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CAppUtils::StartExtPatch(const CTSVNPath& patchFile, const CTSVNPath& dir, const CString& sOriginalDescription, const CString& sPatchedDescription, BOOL bReversed, BOOL bWait)
{
    // use TortoiseMerge
    CString viewer = CPathUtils::GetAppDirectory();
    viewer += L"TortoiseMerge.exe";

    viewer = L"\"" + viewer + L"\"";
    viewer = viewer + L" /diff:\"" + patchFile.GetWinPathString() + L"\"";
    viewer = viewer + L" /patchpath:\"" + dir.GetWinPathString() + L"\"";
    if (bReversed)
        viewer += L" /reversedpatch";
    if (!sOriginalDescription.IsEmpty())
        viewer = viewer + L" /patchoriginal:\"" + sOriginalDescription + L"\"";
    if (!sPatchedDescription.IsEmpty())
        viewer = viewer + L" /patchpatched:\"" + sPatchedDescription + L"\"";
    if (!g_sGroupingUuid.IsEmpty())
    {
        viewer += L" /groupuuid:\"";
        viewer += g_sGroupingUuid;
        viewer += L"\"";
    }
    if (!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
    {
        return FALSE;
    }
    return TRUE;
}

CString CAppUtils::PickDiffTool(const CTSVNPath& file1, const CTSVNPath& file2, const CString& mimetype)
{
    CString diffTool = CRegString(L"Software\\TortoiseSVN\\DiffTools\\" + file2.GetFilename().MakeLower());
    if (!diffTool.IsEmpty())
        return diffTool;
    diffTool = CRegString(L"Software\\TortoiseSVN\\DiffTools\\" + file1.GetFilename().MakeLower());
    if (!diffTool.IsEmpty())
        return diffTool;

    if (HasMimeTool())
    {
        // Is there a mime type specific diff tool?
        CString mt = mimetype;
        if (!mt.IsEmpty() || GetMimeType(file1, mt) || GetMimeType(file2, mt))
        {
            CString mimeTool = CRegString(L"Software\\TortoiseSVN\\DiffTools\\" + mt);
            if (!mimeTool.IsEmpty())
                return mimeTool;
        }
    }

    // Is there an extension specific diff tool?
    CString ext = file2.GetFileExtension().MakeLower();
    if (!ext.IsEmpty())
    {
        CString extTool = CRegString(L"Software\\TortoiseSVN\\DiffTools\\" + ext);
        if (!extTool.IsEmpty())
            return extTool;
        // Maybe we should use TortoiseIDiff?
        if ((ext == L".jpg") || (ext == L".jpeg") ||
            (ext == L".bmp") || (ext == L".gif") ||
            (ext == L".png") || (ext == L".ico") ||
            (ext == L".tif") || (ext == L".tiff") ||
            (ext == L".dib") || (ext == L".emf") ||
            (ext == L".cur") || (ext == L".webp"))
        {
            return L"\"" + CPathUtils::GetAppDirectory() + L"TortoiseIDiff.exe" + L"\"" +
                   L" /left:%base /right:%mine /lefttitle:%bname /righttitle:%yname" +
                   L" /groupuuid:\"" + g_sGroupingUuid + L"\"";
        }
    }

    // Finally, pick a generic external diff tool
    diffTool = CRegString(L"Software\\TortoiseSVN\\Diff");
    return diffTool;
}

bool CAppUtils::StartExtDiff(const CTSVNPath& file1, const CTSVNPath& file2,
                             const CString& sName1, const CString& sName2, const DiffFlags& flags, int line, const CString& sName)
{
    return StartExtDiff(file1, file2, sName1, sName2, CTSVNPath(), CTSVNPath(), SVNRev(), SVNRev(), SVNRev(), flags, line, sName, L"");
}

bool CAppUtils::StartExtDiff(const CTSVNPath& file1, const CTSVNPath& file2,
                             const CString& sName1, const CString& sName2,
                             const CTSVNPath& url1, const CTSVNPath& url2,
                             const SVNRev& rev1, const SVNRev& rev2,
                             const SVNRev& pegRev, const DiffFlags& flags, int line, const CString& sName, const CString& mimetype)
{
    CString viewer;

    CRegDWORD blameDiff(L"Software\\TortoiseSVN\\DiffBlamesWithTortoiseMerge", FALSE);
    if (!flags.bBlame || !static_cast<DWORD>(blameDiff))
    {
        viewer = PickDiffTool(file1, file2, mimetype);
        // If registry entry for a diff program is commented out, use TortoiseMerge.
        bool bCommentedOut = viewer.Left(1) == L"#";
        if (flags.bAlternativeTool)
        {
            // Invert external vs. internal diff tool selection.
            if (bCommentedOut)
                viewer.Delete(0); // uncomment
            else
                viewer = "";
        }
        else if (bCommentedOut)
            viewer = "";
    }

    bool bInternal = viewer.IsEmpty();
    if (bInternal)
    {
        viewer =
            L"\"" + CPathUtils::GetAppDirectory() + L"TortoiseMerge.exe" + L"\"" +
            L" /base:%base /mine:%mine /basename:%bname /minename:%yname" +
            L" /basereflectedname:%burl /minereflectedname:%yurl";
        if (flags.bBlame)
            viewer += L" /blame";
        if (!g_sGroupingUuid.IsEmpty())
        {
            viewer += L" /groupuuid:\"";
            viewer += g_sGroupingUuid;
            viewer += L"\"";
        }
    }
    // check if the params are set. If not, just add the files to the command line
    if ((viewer.Find(L"%base") < 0) && (viewer.Find(L"%mine") < 0))
    {
        viewer += L" \"" + file1.GetWinPathString() + L"\"";
        viewer += L" \"" + file2.GetWinPathString() + L"\"";
    }
    if (viewer.Find(L"%base") >= 0)
    {
        viewer.Replace(L"%base", L"\"" + file1.GetWinPathString() + L"\"");
    }
    if (viewer.Find(L"%mine") >= 0)
    {
        viewer.Replace(L"%mine", L"\"" + file2.GetWinPathString() + L"\"");
    }
    if (sName1.IsEmpty())
    {
        viewer.Replace(L"%bname", L"\"" + file1.GetUIFileOrDirectoryName() + L"\"");
        viewer.Replace(L"%nqbname", file1.GetUIFileOrDirectoryName());
    }
    else
    {
        viewer.Replace(L"%bname", L"\"" + sName1 + L"\"");
        viewer.Replace(L"%nqbname", sName1);
    }

    if (sName2.IsEmpty())
    {
        viewer.Replace(L"%yname", L"\"" + file2.GetUIFileOrDirectoryName() + L"\"");
        viewer.Replace(L"%nqyname", file2.GetUIFileOrDirectoryName());
    }
    else
    {
        viewer.Replace(L"%yname", L"\"" + sName2 + L"\"");
        viewer.Replace(L"%nqyname", sName2);
    }

    if (sName.IsEmpty())
    {
        viewer.Replace(L"%fname", L"\"\"");
        viewer.Replace(L"%nqfname", L"");
    }
    else
    {
        viewer.Replace(L"%fname", L"\"" + sName + L"\"");
        viewer.Replace(L"%nqfname", sName);
    }

    if (viewer.Find(L"%burl") >= 0)
    {
        CString s = L"\"" + url1.GetSVNPathString();
        s += L"\"";
        viewer.Replace(L"%burl", s);
    }
    viewer.Replace(L"%nqburl", url1.GetSVNPathString());

    if (viewer.Find(L"%yurl") >= 0)
    {
        CString s = L"\"" + url2.GetSVNPathString();
        s += L"\"";
        viewer.Replace(L"%yurl", s);
    }
    viewer.Replace(L"%nqyurl", url2.GetSVNPathString());

    viewer.Replace(L"%brev", L"\"" + rev1.ToString() + L"\"");
    viewer.Replace(L"%nqbrev", rev1.ToString());

    viewer.Replace(L"%yrev", L"\"" + rev2.ToString() + L"\"");
    viewer.Replace(L"%nqyrev", rev2.ToString());

    viewer.Replace(L"%peg", L"\"" + pegRev.ToString() + L"\"");
    viewer.Replace(L"%nqpeg", pegRev.ToString());

    if (flags.bReadOnly && bInternal)
        viewer += L" /readonly";
    if (line > 0)
    {
        viewer += L" /line:";
        CString temp;
        temp.Format(L"%d", line);
        viewer += temp;
    }

    return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, flags.bWait);
}

BOOL CAppUtils::StartExtDiffProps(const CTSVNPath& file1, const CTSVNPath& file2, const CString& sName1, const CString& sName2, BOOL bWait, BOOL bReadOnly)
{
    CRegString diffPropsExe(L"Software\\TortoiseSVN\\DiffProps");
    CString    viewer    = diffPropsExe;
    bool       bInternal = false;
    if (viewer.IsEmpty() || (viewer.Left(1).Compare(L"#") == 0))
    {
        //no registry entry (or commented out) for a diff program
        //use TortoiseMerge
        bInternal = true;
        viewer    = CPathUtils::GetAppDirectory();
        viewer += L"TortoiseMerge.exe";
        viewer = L"\"" + viewer + L"\"";
        viewer = viewer + L" /base:%base /mine:%mine /basename:%bname /minename:%yname";
        if (!g_sGroupingUuid.IsEmpty())
        {
            viewer += L" /groupuuid:\"";
            viewer += g_sGroupingUuid;
            viewer += L"\"";
        }
    }
    // check if the params are set. If not, just add the files to the command line
    if ((viewer.Find(L"%base") < 0) && (viewer.Find(L"%mine") < 0))
    {
        viewer += L" \"" + file1.GetWinPathString() + L"\"";
        viewer += L" \"" + file2.GetWinPathString() + L"\"";
    }
    if (viewer.Find(L"%base") >= 0)
    {
        viewer.Replace(L"%base", L"\"" + file1.GetWinPathString() + L"\"");
    }
    if (viewer.Find(L"%mine") >= 0)
    {
        viewer.Replace(L"%mine", L"\"" + file2.GetWinPathString() + L"\"");
    }

    if (sName1.IsEmpty())
    {
        viewer.Replace(L"%bname", L"\"" + file1.GetUIFileOrDirectoryName() + L"\"");
        viewer.Replace(L"%nqbname", file1.GetUIFileOrDirectoryName());
    }
    else
    {
        viewer.Replace(L"%bname", L"\"" + sName1 + L"\"");
        viewer.Replace(L"%nqbname", sName1);
    }

    if (sName2.IsEmpty())
    {
        viewer.Replace(L"%yname", L"\"" + file2.GetUIFileOrDirectoryName() + L"\"");
        viewer.Replace(L"%nqyname", file2.GetUIFileOrDirectoryName());
    }
    else
    {
        viewer.Replace(L"%yname", L"\"" + sName2 + L"\"");
        viewer.Replace(L"%nqyname", sName2);
    }

    if ((bReadOnly) && (bInternal))
        viewer += L" /readonly";

    if (!LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, !!bWait))
    {
        return FALSE;
    }
    return TRUE;
}

BOOL CAppUtils::CheckForEmptyDiff(const CTSVNPath& sDiffPath)
{
    DWORD     length = 0;
    CAutoFile hFile  = ::CreateFile(sDiffPath.GetWinPath(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, NULL, nullptr);
    if (!hFile)
        return TRUE;
    length = ::GetFileSize(hFile, nullptr);
    if (length < 4)
        return TRUE;
    return FALSE;
}

CString CAppUtils::GetLogFontName()
{
    return static_cast<CString>(CRegString(L"Software\\TortoiseSVN\\LogFontName", L"Consolas"));
}

DWORD CAppUtils::GetLogFontSize()
{
    return static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\LogFontSize", 9));
}

void CAppUtils::CreateFontForLogs(HWND hWnd, CFont& fontToCreate)
{
    LOGFONT logFont;
    logFont.lfHeight         = -CDPIAware::Instance().PointsToPixels(hWnd, GetLogFontSize());
    logFont.lfWidth          = 0;
    logFont.lfEscapement     = 0;
    logFont.lfOrientation    = 0;
    logFont.lfWeight         = FW_NORMAL;
    logFont.lfItalic         = 0;
    logFont.lfUnderline      = 0;
    logFont.lfStrikeOut      = 0;
    logFont.lfCharSet        = DEFAULT_CHARSET;
    logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality        = DRAFT_QUALITY;
    logFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
    wcscpy_s(logFont.lfFaceName, static_cast<LPCWSTR>(GetLogFontName()));
    VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

/**
* Launch the external blame viewer
*/
bool CAppUtils::LaunchTortoiseBlame(const CString& sBlameFile,
                                    const CString& sOriginalFile,
                                    const CString& sParams,
                                    const SVNRev&  startRev,
                                    const SVNRev&  endRev,
                                    const SVNRev&  pegRev)
{
    CString viewer = L"\"";
    viewer += CPathUtils::GetAppDirectory();
    viewer += L"TortoiseBlame.exe\"";
    viewer += L" \"" + sBlameFile + L"\"";
    viewer += L" \"" + sOriginalFile + L"\"";
    viewer += L" " + sParams;

    if (startRev.IsValid() && endRev.IsValid())
        viewer += L" /revrange:\"" + startRev.ToString() + L"-" + endRev.ToString() + L"\"";

    if (pegRev.IsValid())
        viewer += L" /pegrev:" + pegRev.ToString();

    if (!g_sGroupingUuid.IsEmpty())
    {
        viewer += L" /groupuuid:\"";
        viewer += g_sGroupingUuid;
        viewer += L"\"";
    }

    return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, false);
}

bool CAppUtils::FormatTextInRichEditControl(CWnd* pWnd)
{
    CString sText;
    if (pWnd == nullptr)
        return false;
    bool bStyled = false;
    pWnd->GetWindowText(sText);
    // the rich edit control doesn't count the CR char!
    // to be exact: CRLF is treated as one char.
    sText.Remove('\r');

    // style each line separately
    int offset = 0;
    int nNewlinePos;
    do
    {
        nNewlinePos   = sText.Find('\n', offset);
        CString sLine = nNewlinePos >= 0
                            ? sText.Mid(offset, nNewlinePos - offset)
                            : sText.Mid(offset);

        int start = 0;
        int end   = 0;
        while (FindStyleChars(sLine, '*', start, end))
        {
            CHARRANGE range = {static_cast<LONG>(start) + offset, static_cast<LONG>(end) + offset};
            pWnd->SendMessage(EM_EXSETSEL, NULL, reinterpret_cast<LPARAM>(&range));
            SetCharFormat(pWnd, CFM_BOLD, CFE_BOLD);
            bStyled = true;
            start   = end;
        }
        start = 0;
        end   = 0;
        while (FindStyleChars(sLine, '^', start, end))
        {
            CHARRANGE range = {static_cast<LONG>(start) + offset, static_cast<LONG>(end) + offset};
            pWnd->SendMessage(EM_EXSETSEL, NULL, reinterpret_cast<LPARAM>(&range));
            SetCharFormat(pWnd, CFM_ITALIC, CFE_ITALIC);
            bStyled = true;
            start   = end;
        }
        start = 0;
        end   = 0;
        while (FindStyleChars(sLine, '_', start, end))
        {
            CHARRANGE range = {static_cast<LONG>(start) + offset, static_cast<LONG>(end) + offset};
            pWnd->SendMessage(EM_EXSETSEL, NULL, reinterpret_cast<LPARAM>(&range));
            SetCharFormat(pWnd, CFM_UNDERLINE, CFE_UNDERLINE);
            bStyled = true;
            start   = end;
        }
        offset = nNewlinePos + 1;
    } while (nNewlinePos >= 0);
    return bStyled;
}

std::vector<CHARRANGE>
    CAppUtils::FindRegexMatches(const std::wstring& text, const CString& matchString, const CString& matchSubString /* = L".*"*/)
{
    std::vector<CHARRANGE> result;
    if (matchString.IsEmpty())
        return result;

    try
    {
        const std::wregex           regMatch(matchString, std::regex_constants::icase | std::regex_constants::ECMAScript);
        const std::wregex           regSubMatch(matchSubString, std::regex_constants::icase | std::regex_constants::ECMAScript);
        const std::wsregex_iterator end;
        for (std::wsregex_iterator it(text.begin(), text.end(), regMatch); it != end; ++it)
        {
            // (*it)[0] is the matched string
            std::wstring matchedString = (*it)[0];
            ptrdiff_t    matchPos      = it->position(0);
            for (std::wsregex_iterator it2(matchedString.begin(), matchedString.end(), regSubMatch); it2 != end; ++it2)
            {
                ATLTRACE(L"matched id : %s\n", (*it2)[0].str().c_str());
                ptrdiff_t matchPosId = it2->position(0);
                CHARRANGE range      = {static_cast<LONG>(matchPos + matchPosId), static_cast<LONG>(matchPos + matchPosId + (*it2)[0].str().size())};
                result.push_back(range);
            }
        }
    }
    catch (std::exception&)
    {
    }

    return result;
}

// from CSciEdit
namespace
{
bool IsValidURLChar(wchar_t ch)
{
    return iswalnum(ch) ||
           ch == L'_' || ch == L'/' || ch == L';' || ch == L'?' || ch == L'&' || ch == L'=' ||
           ch == L'%' || ch == L':' || ch == L'.' || ch == L'#' || ch == L'-' || ch == L'+' ||
           ch == L'|' || ch == L'>' || ch == L'<' || ch == L'!' || ch == L'@' || ch == L'~';
}

bool IsUrlOrEmail(const CString& sText)
{
    if (!PathIsURLW(sText))
    {
        auto atPos = sText.Find('@');
        if (atPos <= 0)
            return false;
        if (sText.ReverseFind('.') > atPos)
            return true;
        return false;
    }
    if (sText.Find(L"://") >= 0)
        return true;
    return false;
}
} // namespace

/**
* implements URL searching with the same logic as CSciEdit::StyleURLs
*/
std::vector<CHARRANGE> CAppUtils::FindURLMatches(const CString& msg)
{
    std::vector<CHARRANGE> result;

    int len      = msg.GetLength();
    int startUrl = -1;

    for (int i = 0; i <= msg.GetLength(); ++i)
    {
        if ((i < len) && IsValidURLChar(msg[i]))
        {
            if (startUrl < 0)
                startUrl = i;
        }
        else
        {
            if (startUrl >= 0)
            {
                bool strip = true;
                if (msg[startUrl] == '<' && i < len) // try to detect and do not strip URLs put within <>
                {
                    while (startUrl <= i && msg[startUrl] == '<') // strip leading '<'
                        ++startUrl;
                    strip = false;
                    i     = startUrl;
                    while (i < len && msg[i] != '\r' && msg[i] != '\n' && msg[i] != '>') // find first '>' or new line after resetting i to start position
                        ++i;
                }
                int skipTrailing = 0;
                while (strip && i - skipTrailing - 1 > startUrl && (msg[i - skipTrailing - 1] == '.' || msg[i - skipTrailing - 1] == '-' || msg[i - skipTrailing - 1] == '?' || msg[i - skipTrailing - 1] == ';' || msg[i - skipTrailing - 1] == ':' || msg[i - skipTrailing - 1] == '>' || msg[i - skipTrailing - 1] == '<' || msg[i - skipTrailing - 1] == '!'))
                    ++skipTrailing;
                if (!IsUrlOrEmail(msg.Mid(startUrl, i - startUrl - skipTrailing)))
                {
                    startUrl = -1;
                    continue;
                }
                CHARRANGE range = {startUrl, i - skipTrailing};
                result.push_back(range);
            }
            startUrl = -1;
        }
    }

    return result;
}

bool CAppUtils::FindStyleChars(const CString& sText, wchar_t stylechar, int& start, int& end)
{
    int     i            = start;
    int     last         = sText.GetLength() - 1;
    bool    bFoundMarker = false;
    wchar_t c            = i == 0 ? '\0' : sText[i - 1];
    wchar_t nextChar     = i >= last ? '\0' : sText[i + 1];

    // find a starting marker
    while (i < last)
    {
        wchar_t prevChar = c;
        c                = nextChar;
        nextChar         = sText[i + 1];

        // IsCharAlphaNumeric can be somewhat expensive.
        // Long lines of "*****" or "----" will be pre-empted efficiently
        // by the (c != nextChar) condition.

        if ((c == stylechar) && (c != nextChar))
        {
            if (IsCharAlphaNumeric(nextChar) && !IsCharAlphaNumeric(prevChar))
            {
                start        = ++i;
                bFoundMarker = true;
                break;
            }
        }
        i++;
    }
    if (!bFoundMarker)
        return false;

    // find ending marker
    // c == sText[i-1]

    bFoundMarker = false;
    while (i <= last)
    {
        wchar_t prevChar = c;
        c                = sText[i];
        if (c == stylechar)
        {
            if ((i == last) || (!IsCharAlphaNumeric(sText[i + 1]) && IsCharAlphaNumeric(prevChar)))
            {
                end = i;
                i++;
                bFoundMarker = true;
                break;
            }
        }
        i++;
    }
    return bFoundMarker;
}

bool CAppUtils::BrowseRepository(const CString& repoRoot, CHistoryCombo& combo, CWnd* pParent, SVNRev& rev)
{
    bool    bExternalUrl = false;
    CString strUrl;
    combo.GetWindowText(strUrl);
    if (strUrl.GetLength() && strUrl[0] == '^')
    {
        bExternalUrl = true;
        strUrl       = strUrl.Mid(1);
    }
    strUrl.Replace('\\', '/');
    strUrl.Replace(L"%", L"%25");
    strUrl.TrimLeft('/');

    CString trimmedRoot = repoRoot;
    trimmedRoot.TrimRight('/');

    strUrl = trimmedRoot + L"/" + strUrl;

    if (strUrl.Left(7) == L"file://")
    {
        // browse repository - show repository browser
        SVN::preparePath(strUrl);
        CRepositoryBrowser browser(strUrl, rev, pParent);
        if (browser.DoModal() == IDOK)
        {
            combo.SetCurSel(-1);
            if (bExternalUrl)
                combo.SetWindowText(L"^" + browser.GetPath().Mid(CPathUtils::PathUnescape(repoRoot).GetLength()));
            else
                combo.SetWindowText(browser.GetPath().Mid(CPathUtils::PathUnescape(repoRoot).GetLength()));
            combo.SetFocus();
            rev = browser.GetRevision();
            return true;
        }
    }
    else if ((strUrl.Left(7) == L"http://" || (strUrl.Left(8) == L"https://") || (strUrl.Left(6) == L"svn://") || (strUrl.Left(4) == L"svn+")) && strUrl.GetLength() > 6)
    {
        // browse repository - show repository browser
        CRepositoryBrowser browser(strUrl, rev, pParent);
        if (browser.DoModal() == IDOK)
        {
            combo.SetCurSel(-1);
            if (bExternalUrl)
                combo.SetWindowText(L"^" + browser.GetPath().Mid(CPathUtils::PathUnescape(repoRoot).GetLength()));
            else
                combo.SetWindowText(browser.GetPath().Mid(CPathUtils::PathUnescape(repoRoot).GetLength()));
            combo.SetFocus();
            rev = browser.GetRevision();
            return true;
        }
    }
    combo.SetFocus();
    return false;
}

bool CAppUtils::BrowseRepository(CHistoryCombo& combo, CWnd* pParent, SVNRev& rev, bool multiSelection, const CString& root, const CString& selUrl)
{
    CString strUrLs;
    combo.GetWindowText(strUrLs);
    if (strUrLs.IsEmpty())
        strUrLs = combo.GetString();
    strUrLs.Replace('\\', '/');
    strUrLs.Replace(L"%", L"%25");

    CString strUrl = strUrLs;
    if (multiSelection)
    {
        CTSVNPathList paths;
        paths.LoadFromAsteriskSeparatedString(strUrLs);

        strUrl = paths.GetCommonRoot().GetSVNPathString();
    }

    if (strUrl.GetLength() > 1)
    {
        strUrl = SVNExternals::GetFullExternalUrl(strUrl, root, selUrl);
        // check if the url has a revision appended to it
        auto atPosUrl = strUrl.ReverseFind('@');
        if (atPosUrl >= 0)
        {
            CString sRev = strUrl.Mid(atPosUrl + 1);
            SVNRev  urlRev(sRev);
            if (urlRev.IsValid())
            {
                strUrl = strUrl.Left(atPosUrl);
                if (!rev.IsValid() || rev.IsHead())
                    rev = urlRev;
            }
        }
    }

    if (strUrl.Left(7) == L"file://")
    {
        CString strFile(strUrl);
        SVN::UrlToPath(strFile);

        SVN svn;
        if (svn.IsRepository(CTSVNPath(strFile)))
        {
            // browse repository - show repository browser
            SVN::preparePath(strUrl);
            CRepositoryBrowser browser(strUrl, rev, pParent);
            if (browser.DoModal() == IDOK)
            {
                combo.SetCurSel(-1);
                combo.SetWindowText(multiSelection ? browser.GetSelectedURLs() : browser.GetPath());
                combo.SetFocus();
                rev = browser.GetRevision();
                return true;
            }
        }
        else
        {
            // browse local directories
            CBrowseFolder folderBrowser;
            folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
            // remove the 'file:///' so the shell can recognize the local path
            SVN::UrlToPath(strUrl);
            if (folderBrowser.Show(pParent->GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
            {
                SVN::PathToUrl(strUrl);

                combo.SetCurSel(-1);
                combo.SetWindowText(strUrl);
                combo.SetFocus();
                return true;
            }
        }
    }
    else if ((strUrl.Left(7) == L"http://" || (strUrl.Left(8) == L"https://") || (strUrl.Left(6) == L"svn://") || (strUrl.Left(4) == L"svn+")) && strUrl.GetLength() > 6)
    {
        // browse repository - show repository browser
        CRepositoryBrowser browser(strUrl, rev, pParent);
        if (browser.DoModal() == IDOK)
        {
            combo.SetCurSel(-1);
            combo.SetWindowText(multiSelection ? browser.GetSelectedURLs() : browser.GetPath());
            combo.SetFocus();
            rev = browser.GetRevision();
            return true;
        }
    }
    else
    {
        SVN svn;
        if (svn.IsRepository(CTSVNPath(strUrl)))
        {
            // browse repository - show repository browser
            SVN::PathToUrl(strUrl);
            CRepositoryBrowser browser(strUrl, rev, pParent);
            if (browser.DoModal() == IDOK)
            {
                combo.SetCurSel(-1);
                combo.SetWindowText(multiSelection ? browser.GetSelectedURLs() : browser.GetPath());
                combo.SetFocus();
                rev = browser.GetRevision();
                return true;
            }
        }
        else
        {
            // browse local directories
            CBrowseFolder folderBrowser;
            folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
            if (folderBrowser.Show(pParent->GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
            {
                SVN::PathToUrl(strUrl);

                combo.SetCurSel(-1);
                combo.SetWindowText(strUrl);
                return true;
            }
        }
    }

    combo.SetFocus();
    return false;
}

CString CAppUtils::GetProjectNameFromURL(CString url)
{
    CString name;
    while ((name.IsEmpty() || (name.CompareNoCase(L"branches") == 0) ||
            (name.CompareNoCase(L"tags") == 0) ||
            (name.CompareNoCase(L"trunk") == 0)) &&
           (!url.IsEmpty()))
    {
        auto rFound = url.ReverseFind('/');
        if (rFound < 0)
            break;
        name = url.Mid(rFound + 1);
        url  = url.Left(rFound);
    }
    if ((name.Compare(L"svn") == 0) || (name.Compare(L"svnroot") == 0))
    {
        // a name of svn or svnroot indicates that it's not really the project name. In that
        // case, we try the first part of the URL
        // of course, this won't work in all cases (but it works for Google project hosting)
        url.Replace(L"http://", L"");
        url.Replace(L"https://", L"");
        url.Replace(L"svn://", L"");
        url.Replace(L"svn+ssh://", L"");
        url.TrimLeft(L"/");
        name = url.Left(url.Find('.'));
    }
    return name;
}

bool CAppUtils::StartShowUnifiedDiff(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1,
                                     const CTSVNPath& url2, const SVNRev& rev2,
                                     const SVNRev& peg, const SVNRev& headPeg,
                                     bool prettyPrint, const CString& options,
                                     bool bAlternateDiff /* = false */, bool bIgnoreAncestry /* = false */, bool blame /* = false */, bool bIgnoreProperties /* = true*/)
{
    CString sCmd = L"/command:showcompare /unified";
    sCmd += L" /url1:\"" + url1.GetSVNPathString();
    sCmd += L"\"";
    if (rev1.IsValid())
        sCmd += L" /revision1:" + rev1.ToString();
    sCmd += L" /url2:\"" + url2.GetSVNPathString();
    sCmd += L"\"";
    if (rev2.IsValid())
        sCmd += L" /revision2:" + rev2.ToString();
    if (peg.IsValid())
        sCmd += L" /pegrevision:" + peg.ToString();
    if (headPeg.IsValid())
        sCmd += L" /headpegrevision:" + headPeg.ToString();

    if (!options.IsEmpty())
        sCmd += L" /diffoptions:\"" + options + L"\"";

    if (bAlternateDiff)
        sCmd += L" /alternatediff";

    if (bIgnoreAncestry)
        sCmd += L" /ignoreancestry";

    if (blame)
        sCmd += L" /blame";

    if (bIgnoreProperties)
        sCmd += L" /ignoreprops";

    if (prettyPrint)
        sCmd += L" /prettyprint";

    if (hWnd)
    {
        sCmd += L" /hwnd:";
        wchar_t buf[30] = {0};
        swprintf_s(buf, L"%p", static_cast<void*>(hWnd));
        sCmd += buf;
    }

    return CAppUtils::RunTortoiseProc(sCmd);
}

bool CAppUtils::StartShowCompare(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1,
                                 const CTSVNPath& url2, const SVNRev& rev2,
                                 const SVNRev& peg, const SVNRev& headpeg,
                                 bool ignoreprops, bool prettyprint, const CString& options,
                                 bool bAlternateDiff /*= false*/, bool bIgnoreAncestry /*= false*/,
                                 bool blame /*= false*/, svn_node_kind_t nodekind /*= svn_node_unknown*/,
                                 int line /*= 0*/)
{
    CString sCmd;
    sCmd.Format(L"/command:showcompare /nodekind:%d", nodekind);
    sCmd += L" /url1:\"" + url1.GetSVNPathString();
    sCmd += L"\"";
    if (rev1.IsValid())
        sCmd += L" /revision1:" + rev1.ToString();
    sCmd += L" /url2:\"" + url2.GetSVNPathString();
    sCmd += L"\"";
    if (rev2.IsValid())
        sCmd += L" /revision2:" + rev2.ToString();
    if (peg.IsValid())
        sCmd += L" /pegrevision:" + peg.ToString();
    if (headpeg.IsValid())
        sCmd += L" /headpegrevision:" + headpeg.ToString();
    if (!options.IsEmpty())
        sCmd += L" /diffoptions:\"" + options + L"\"";
    if (bAlternateDiff)
        sCmd += L" /alternatediff";
    if (bIgnoreAncestry)
        sCmd += L" /ignoreancestry";
    if (blame)
        sCmd += L" /blame";
    if (ignoreprops)
        sCmd += L" /ignoreprops";
    if (prettyprint)
        sCmd += L" /prettyprint";

    if (hWnd)
    {
        sCmd += L" /hwnd:";
        wchar_t buf[30] = {0};
        swprintf_s(buf, L"%p", static_cast<void*>(hWnd));
        sCmd += buf;
    }

    if (line > 0)
    {
        sCmd += L" /line:";
        CString temp;
        temp.Format(L"%d", line);
        sCmd += temp;
    }

    return CAppUtils::RunTortoiseProc(sCmd);
}

bool CAppUtils::SetupDiffScripts(bool force, const CString& type)
{
    CString scriptsDir = CPathUtils::GetAppParentDirectory();
    scriptsDir += L"Diff-Scripts";
    CSimpleFileFind files(scriptsDir);
    while (files.FindNextFileNoDirectories())
    {
        CString file     = files.GetFilePath();
        CString fileName = files.GetFileName();
        CString ext      = file.Mid(file.ReverseFind('-') + 1);
        ext              = L"." + ext.Left(ext.ReverseFind('.'));
        std::set<CString> extensions;
        extensions.insert(ext);
        CString kind;
        if (file.Right(3).CompareNoCase(L"vbs") == 0)
        {
            kind = L" //E:vbscript";
        }
        if (file.Right(2).CompareNoCase(L"js") == 0)
        {
            kind = L" //E:javascript";
        }
        // open the file, read the first line and find possible extensions
        // this script can handle
        try
        {
            CStdioFile f(file, CFile::modeRead | CFile::shareDenyNone);
            CString    extLine;
            if (f.ReadString(extLine))
            {
                if ((extLine.GetLength() > 15) &&
                    ((extLine.Left(15).Compare(L"// extensions: ") == 0) ||
                     (extLine.Left(14).Compare(L"' extensions: ") == 0)))
                {
                    if (extLine[0] == '/')
                        extLine = extLine.Mid(15);
                    else
                        extLine = extLine.Mid(14);
                    CString sToken;
                    int     curPos = 0;
                    sToken         = extLine.Tokenize(L";", curPos);
                    while (!sToken.IsEmpty())
                    {
                        if (!sToken.IsEmpty())
                        {
                            if (sToken[0] != '.')
                                sToken = L"." + sToken;
                            extensions.insert(sToken);
                        }
                        sToken = extLine.Tokenize(L";", curPos);
                    }
                }
            }
            f.Close();
        }
        catch (CFileException* e)
        {
            e->Delete();
        }

        for (std::set<CString>::const_iterator it = extensions.begin(); it != extensions.end(); ++it)
        {
            if (type.IsEmpty() || (type.Compare(L"Diff") == 0))
            {
                if (fileName.Left(5).CompareNoCase(L"diff-") == 0)
                {
                    CRegString diffReg       = CRegString(L"Software\\TortoiseSVN\\DiffTools\\" + *it);
                    CString    diffRegString = diffReg;
                    if (force || (diffRegString.IsEmpty()) || (diffRegString.Find(fileName) >= 0))
                        diffReg = L"wscript.exe \"" + file + L"\" %base %mine" + kind;
                }
            }
            if (type.IsEmpty() || (type.Compare(L"Merge") == 0))
            {
                if (fileName.Left(6).CompareNoCase(L"merge-") == 0)
                {
                    CRegString diffReg       = CRegString(L"Software\\TortoiseSVN\\MergeTools\\" + *it);
                    CString    diffRegString = diffReg;
                    if (force || (diffRegString.IsEmpty()) || (diffRegString.Find(fileName) >= 0))
                        diffReg = L"wscript.exe \"" + file + L"\" %merged %theirs %mine %base" + kind;
                }
            }
        }
    }
    // Initialize "Software\\TortoiseSVN\\DiffProps" once with the same value as "Software\\TortoiseSVN\\Diff"
    CRegString regDiffPropsPath = CRegString(L"Software\\TortoiseSVN\\DiffProps", L"non-existent");
    CString    strDiffPropsPath = regDiffPropsPath;
    if (force || strDiffPropsPath == L"non-existent")
    {
        CString strDiffPath = CRegString(L"Software\\TortoiseSVN\\Diff");
        regDiffPropsPath    = strDiffPath;
    }

    return true;
}

void CAppUtils::SetCharFormat(CWnd* window, DWORD mask, DWORD effects, const std::vector<CHARRANGE>& positions)
{
    CHARFORMAT2 format = {0};
    format.cbSize      = sizeof(CHARFORMAT2);
    format.dwMask      = mask;
    format.dwEffects   = effects;
    format.crTextColor = effects;

    for (auto iter = positions.begin(), end = positions.end(); iter != end; ++iter)
    {
        CHARRANGE range = *iter;
        window->SendMessage(EM_EXSETSEL, NULL, reinterpret_cast<LPARAM>(&range));
        window->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
    }
}

void CAppUtils::SetCharFormat(CWnd* window, DWORD mask, DWORD effects)
{
    CHARFORMAT2 format = {0};
    format.cbSize      = sizeof(CHARFORMAT2);
    format.dwMask      = mask;
    format.dwEffects   = effects;
    window->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
}

bool CAppUtils::AskToUpdate(HWND hParent, LPCWSTR error)
{
    CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TASK1)),
                        CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TITLE)),
                        L"TortoiseSVN",
                        0,
                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
    taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TASK3)));
    taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TASK4)));
    taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskDlg.SetDefaultCommandControl(1);
    CString details;
    details.Format(IDS_MSG_NEEDSUPDATE_ERRORDETAILS, error);
    taskDlg.SetExpansionArea(details);
    taskDlg.SetMainIcon(TD_WARNING_ICON);
    return (taskDlg.DoModal(hParent) == 100);
}

void CAppUtils::ReportFailedHook(HWND hWnd, const CString& sError)
{
    std::wstring str = static_cast<LPCWSTR>(sError);
    std::wregex  rx(L"((https?|ftp|file)://[-A-Z0-9+&@#/%?=~_|!:,.;]*[-A-Z0-9+&@#/%=~_|])", std::regex_constants::icase | std::regex_constants::ECMAScript);
    std::wstring replacement = L"<A HREF=\"$1\">$1</A>";
    std::wstring str2        = std::regex_replace(str, rx, replacement);

    CTaskDialog taskDlg(str2.c_str(),
                        CString(MAKEINTRESOURCE(IDS_COMMITDLG_CHECKCOMMIT_TASK1)),
                        L"TortoiseSVN",
                        0,
                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
    taskDlg.SetCommonButtons(TDCBF_OK_BUTTON);
    taskDlg.SetMainIcon(TD_ERROR_ICON);
    taskDlg.DoModal(hWnd);
}

bool CAppUtils::HasMimeTool()
{
    CRegistryKey diffList(L"Software\\TortoiseSVN\\DiffTools");
    CStringList  diffValues;
    diffList.getValues(diffValues);
    bool hasMimeTool = false;
    for (POSITION pos = diffValues.GetHeadPosition(); pos != nullptr;)
    {
        auto s = diffValues.GetNext(pos);
        if (s.Find('/') >= 0)
        {
            hasMimeTool = true;
            break;
        }
    }
    return hasMimeTool;
}
