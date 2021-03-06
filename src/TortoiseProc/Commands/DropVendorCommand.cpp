// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013-2015, 2017-2018, 2021 - TortoiseSVN

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
#include "DropVendorCommand.h"
#include "SVNStatus.h"
#include "DirFileEnum.h"
#include "SVN.h"
#include "SVNAdminDir.h"
#include "ProgressDlg.h"
#include "FormatMessageWrapper.h"
#include "SmartHandle.h"
#include "PathUtils.h"

bool DropVendorCommand::Execute()
{
    CString   dropPath    = parser.GetVal(L"droptarget");
    CTSVNPath dropSvnPath = CTSVNPath(dropPath);
    if (dropSvnPath.IsAdminDir())
        return FALSE;
    if (!parser.HasKey(L"noui"))
    {
        CString sAsk;
        sAsk.Format(IDS_PROC_VENDORDROP_CONFIRM, static_cast<LPCWSTR>(dropSvnPath.GetFileOrDirectoryName()));
        if (MessageBox(GetExplorerHWND(), sAsk, L"TortoiseSVN", MB_YESNO | MB_ICONQUESTION) != IDYES)
            return FALSE;
    }

    CProgressDlg progress;
    progress.SetTitle(IDS_PROC_VENDORDROP_TITLE);
    progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_VENDORDROP_GETDATA1)));
    if (!parser.HasKey(L"noprogressui"))
        progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));

    std::map<CString, bool> versionedFiles;
    CTSVNPath               path;
    SVNStatus               st;
    svn_client_status_t*    status = st.GetFirstFileStatus(dropSvnPath, path, false, svn_depth_infinity, true, true);
    if (status)
    {
        while ((status = st.GetNextFileStatus(path)) != nullptr && !progress.HasUserCancelled())
        {
            if (status->node_status == svn_wc_status_deleted)
            {
                if (PathFileExists(path.GetWinPath()))
                {
                    path = CTSVNPath(CPathUtils::GetLongPathname(path.GetWinPath()).c_str());
                }
                else
                    continue;
            }
            versionedFiles[path.GetWinPathString().Mid(dropPath.GetLength() + 1)] = status->kind == svn_node_dir;
        }
    }

    pathList.RemoveAdminPaths();
    std::map<CString, bool> vendorFiles;
    CTSVNPath               root    = pathList.GetCommonRoot();
    int                     rootLen = root.GetWinPathString().GetLength() + 1;
    progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_VENDORDROP_GETDATA2)));
    for (int nPath = 0; (nPath < pathList.GetCount()) && (!progress.HasUserCancelled()); nPath++)
    {
        CDirFileEnum crawler(pathList[nPath].GetWinPathString());
        CString      sPath;
        bool         bDir = false;
        while (crawler.NextFile(sPath, &bDir, true))
        {
            if (!g_SVNAdminDir.IsAdminDirPath(sPath))
                vendorFiles[sPath.Mid(rootLen)] = bDir;
        }
    }

    // now start copying the vendor files over the versioned files
    ProjectProperties projectProperties;
    projectProperties.ReadPropsPathList(pathList);
    SVN svn;
    progress.SetTime(true);
    progress.SetProgress(0, static_cast<DWORD>(vendorFiles.size()));
    DWORD count = 0;
    for (auto it = vendorFiles.cbegin(); (it != vendorFiles.cend()) && (!progress.HasUserCancelled()); ++it)
    {
        CString srcPath = root.GetWinPathString() + L"\\" + it->first;
        CString dstPath = dropPath + L"\\" + it->first;

        progress.FormatPathLine(1, IDS_PROC_COPYINGPROG, static_cast<LPCWSTR>(srcPath));
        progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, static_cast<LPCWSTR>(dstPath));
        progress.SetProgress(++count, static_cast<DWORD>(vendorFiles.size()));

        auto found = versionedFiles.find(it->first);
        if (found != versionedFiles.end())
        {
            versionedFiles.erase(found);
            if (!it->second)
            {
                if (!CopyFileHandleReadOnly(srcPath, dstPath))
                {
                    if (!parser.HasKey(L"noui"))
                    {
                        CFormatMessageWrapper error;
                        progress.Stop();
                        CString sErr;
                        sErr.Format(IDS_ERR_COPYFAILED, static_cast<LPCWSTR>(srcPath), static_cast<LPCWSTR>(dstPath), static_cast<LPCWSTR>(error));
                        MessageBox(progress.GetHwnd(), sErr, L"TortoiseSVN", MB_ICONERROR);
                    }
                    return FALSE;
                }
            }
        }
        else
        {
            if (PathFileExists(dstPath))
            {
                // case rename!
                for (auto v = versionedFiles.begin(); v != versionedFiles.end(); ++v)
                {
                    if (v->first.CompareNoCase(it->first) == 0)
                    {
                        CString dstSrc = dropPath + L"\\" + v->first;
                        if (!svn.Move(CTSVNPathList(CTSVNPath(dstSrc)), CTSVNPath(dstPath)))
                        {
                            progress.Stop();
                            svn.ShowErrorDialog(GetExplorerHWND());
                            return FALSE;
                        }
                        if (!it->second)
                        {
                            if (!CopyFileHandleReadOnly(srcPath, dstPath))
                            {
                                if (!parser.HasKey(L"noui"))
                                {
                                    CFormatMessageWrapper error;
                                    progress.Stop();
                                    CString sErr;
                                    sErr.Format(IDS_ERR_COPYFAILED, static_cast<LPCWSTR>(srcPath), static_cast<LPCWSTR>(dstPath), static_cast<LPCWSTR>(error));
                                    MessageBox(progress.GetHwnd(), sErr, L"TortoiseSVN", MB_ICONERROR);
                                }
                                return FALSE;
                            }
                            versionedFiles.erase(v);
                        }
                        else
                        {
                            versionedFiles.erase(v);
                            // adjust all paths inside the renamed folder
                            std::map<CString, bool> versionedFilesTemp;
                            for (auto vv = versionedFiles.begin(); vv != versionedFiles.end();)
                            {
                                if ((vv->first.Left(it->first.GetLength()).CompareNoCase(it->first) == 0) && (vv->first[it->first.GetLength()] == '\\'))
                                {
                                    versionedFilesTemp[it->first + vv->first.Mid(it->first.GetLength())] = vv->second;
                                    auto curIter                                                         = vv++;
                                    versionedFiles.erase(curIter);
                                }
                                else
                                {
                                    ++vv;
                                }
                            }
                            versionedFiles.insert(versionedFilesTemp.begin(), versionedFilesTemp.end());
                        }
                        break;
                    }
                }
            }
            else
            {
                CTSVNPathList plist = CTSVNPathList(CTSVNPath(dstPath));
                if (!it->second)
                {
                    if (!CopyFileHandleReadOnly(srcPath, dstPath))
                    {
                        if (!parser.HasKey(L"noui"))
                        {
                            CFormatMessageWrapper error;
                            progress.Stop();
                            CString sErr;
                            sErr.Format(IDS_ERR_COPYFAILED, static_cast<LPCWSTR>(srcPath), static_cast<LPCWSTR>(dstPath), static_cast<LPCWSTR>(error));
                            MessageBox(progress.GetHwnd(), sErr, L"TortoiseSVN", MB_ICONERROR);
                        }
                        return FALSE;
                    }
                }
                else
                    CreateDirectory(dstPath, nullptr);
                if (!svn.Add(plist, &projectProperties, svn_depth_infinity, true, true, true, true))
                {
                    progress.Stop();
                    if (!parser.HasKey(L"noui"))
                        svn.ShowErrorDialog(progress.GetHwnd());
                    return FALSE;
                }
            }
        }
    }
    if (pathList.GetCount() > 1)
    {
        // remove the source paths from the list:
        // they're definitely not missing
        for (int nPath = 0; nPath < pathList.GetCount(); nPath++)
            versionedFiles.erase(pathList[nPath].GetWinPathString().Mid(rootLen));
    }

    // now go through all files still in the versionedFiles map
    progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_VENDORDROP_REMOVE)));
    progress.SetLine(2, L"");
    progress.SetProgress(0, 0);
    while (!versionedFiles.empty())
    {
        auto it = versionedFiles.begin();
        // first move the file to the recycle bin so it's possible to retrieve it later
        // again in case removing it was done accidentally
        CTSVNPath delpath = CTSVNPath(dropPath + L"\\" + it->first);
        bool      isDir   = delpath.IsDirectory();
        delpath.Delete(true);
        // create the deleted file or directory again to avoid svn throwing an error
        if (isDir)
            CreateDirectory(delpath.GetWinPath(), nullptr);
        else
        {
            CAutoFile hFile = CreateFile(delpath.GetWinPath(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        }
        if (!svn.Remove(CTSVNPathList(delpath), true, false))
        {
            progress.Stop();
            if (!parser.HasKey(L"noui"))
                svn.ShowErrorDialog(progress.GetHwnd());
            return FALSE;
        }
        if (isDir)
        {
            // remove all files within this directory from the list
            auto p = it->first;
            versionedFiles.erase(it);
            for (auto delit = versionedFiles.begin(); delit != versionedFiles.end(); ++delit)
            {
                if ((delit->first.Left(p.GetLength()).CompareNoCase(p) == 0) && (delit->first[p.GetLength()] == '\\'))
                {
                    versionedFiles.erase(delit);
                    delit = versionedFiles.begin();
                }
            }
        }
        else
            versionedFiles.erase(it);
    }

    return TRUE;
}

bool DropVendorCommand::CopyFileHandleReadOnly(LPCWSTR lpExistingFilename, LPCWSTR lpNewFilename)
{
    if (!CopyFile(lpExistingFilename, lpNewFilename, FALSE))
    {
        // try again with the readonly attribute removed
        auto attribs = ::GetFileAttributes(lpNewFilename);
        OnOutOfScope(::SetFileAttributes(lpNewFilename, attribs));

        ::SetFileAttributes(lpNewFilename, FILE_ATTRIBUTE_NORMAL);
        if (!CopyFile(lpExistingFilename, lpNewFilename, FALSE))
            return false;
    }
    return true;
}
