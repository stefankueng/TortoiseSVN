﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014-2017, 2021 - TortoiseSVN

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
#include "DropExternalCommand.h"
#include "SVN.h"
#include "SVNProperties.h"
#include "SVNStatus.h"
#include "SVNProgressDlg.h"
#include "TortoiseProc.h"

bool DropExternalCommand::Execute()
{
    bool      bSuccess     = false;
    CString   dropPath     = parser.GetVal(L"droptarget");
    CTSVNPath droptSvnPath = CTSVNPath(dropPath);
    if (droptSvnPath.IsAdminDir())
        droptSvnPath = droptSvnPath.GetDirectory();
    if (!droptSvnPath.IsDirectory())
        droptSvnPath = droptSvnPath.GetDirectory();

    // first get the svn:externals property from the target folder
    SVNProperties props(droptSvnPath, SVNRev::REV_WC, false, false);
    std::string   sExternalsValue;
    for (int i = 0; i < props.GetCount(); ++i)
    {
        if (props.GetItemName(i).compare(SVN_PROP_EXTERNALS) == 0)
        {
            sExternalsValue = props.GetItemValue(i);
            break;
        }
    }

    // we don't add admin dirs as externals
    pathList.RemoveAdminPaths();
    if (pathList.GetCount() == 0)
        return bSuccess;

    SVNStatus status;
    status.GetStatus(droptSvnPath);
    CString sTargetRepoRootUrl;
    if (status.status && status.status->repos_root_url)
    {
        sTargetRepoRootUrl = CUnicodeUtils::GetUnicode(status.status->repos_root_url);
    }
    if (sTargetRepoRootUrl.IsEmpty())
    {
        // failed to get the status and/or the repo root url
        CString messageString;
        messageString.Format(IDS_ERR_NOURLOFFILE, droptSvnPath.GetWinPath());
        ::MessageBox(GetExplorerHWND(), messageString, L"TortoiseSVN", MB_ICONERROR);
    }
    SVN svn;
    for (auto i = 0; i < pathList.GetCount(); ++i)
    {
        CTSVNPath destPath = droptSvnPath;
        destPath.AppendPathString(pathList[i].GetFileOrDirectoryName());
        bool bExists = !!PathFileExists(destPath.GetWinPath());
        if (!bExists)
        {
            SVNStatus sourceStatus;
            sourceStatus.GetStatus(pathList[i]);
            if (sourceStatus.status && sourceStatus.status->repos_root_url)
            {
                CString sExternalRootUrl     = CUnicodeUtils::GetUnicode(sourceStatus.status->repos_root_url);
                CString sExternalUrl         = svn.GetURLFromPath(pathList[i]);
                CString sFileOrDirectoryName = pathList[i].GetFileOrDirectoryName();
                if (sFileOrDirectoryName.Find(L" ") > 0)
                    sFileOrDirectoryName = L"\"" + sFileOrDirectoryName + L"\"";
                CString sExtValue = sExternalUrl + L" " + sFileOrDirectoryName;
                // check if the url is from the same repo as the target, and if it is
                // use a relative external url instead of a full url
                if (sTargetRepoRootUrl.Compare(sExternalRootUrl) == 0)
                {
                    sExtValue = L"^" + sExternalUrl.Mid(sTargetRepoRootUrl.GetLength()) + L" " + sFileOrDirectoryName;
                }
                if (!sExternalsValue.empty())
                {
                    if (sExternalsValue[sExternalsValue.size() - 1] != '\n')
                        sExternalsValue += "\n";
                }
                sExternalsValue += CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(sExtValue));
                bSuccess = true;
            }
        }
        else
        {
            // the file already exists, there can't be an external with the
            // same name.

            CString messageString;
            messageString.Format(IDS_DROPEXT_FILEEXISTS, static_cast<LPCWSTR>(pathList[i].GetFileOrDirectoryName()));
            ::MessageBox(GetExplorerHWND(), messageString, L"TortoiseSVN", MB_ICONERROR);
            bSuccess = false;
        }
    }
    if (bSuccess)
    {
        bSuccess = !!props.Add(SVN_PROP_EXTERNALS, sExternalsValue, true);
        if (bSuccess)
        {
            CString sInfo;
            sInfo.Format(IDS_DROPEXT_UPDATE_TASK1, static_cast<LPCWSTR>(droptSvnPath.GetFileOrDirectoryName()));
            CTaskDialog taskDlg(sInfo,
                                CString(MAKEINTRESOURCE(IDS_DROPEXT_UPDATE_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_DROPEXT_UPDATE_TASK3)));
            taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_DROPEXT_UPDATE_TASK4)));
            taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskDlg.SetDefaultCommandControl(200);
            taskDlg.SetMainIcon(TD_WARNING_ICON);
            bool doUpdate = (taskDlg.DoModal(GetExplorerHWND()) == 100);
            if (doUpdate)
            {
                DWORD             exitCode = 0;
                CString           error;
                ProjectProperties pprops;
                pprops.ReadPropsPathList(pathList);
                CHooks::Instance().SetProjectProperties(droptSvnPath, pprops);
                if (CHooks::Instance().StartUpdate(GetExplorerHWND(), pathList, exitCode, error))
                {
                    if (exitCode)
                    {
                        CString temp;
                        temp.Format(IDS_ERR_HOOKFAILED, static_cast<LPCWSTR>(error));
                        ::MessageBox(GetExplorerHWND(), temp, L"TortoiseSVN", MB_ICONERROR);
                        return FALSE;
                    }
                }

                CSVNProgressDlg progDlg;
                theApp.m_pMainWnd = &progDlg;
                progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Update);
                progDlg.SetAutoClose(parser);
                progDlg.SetOptions(ProgOptSkipPreChecks);
                progDlg.SetPathList(CTSVNPathList(droptSvnPath));
                progDlg.SetRevision(SVNRev(L"HEAD"));
                progDlg.SetProjectProperties(pprops);
                progDlg.SetDepth(svn_depth_unknown);
                progDlg.DoModal();
                return !progDlg.DidErrorsOccur();
            }
        }
        else
        {
            // adding the svn:externals property failed, remove all the copied files
            props.ShowErrorDialog(GetExplorerHWND());
        }
    }

    return bSuccess != false;
}
