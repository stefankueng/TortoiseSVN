// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2011, 2013-2016, 2020-2021 - TortoiseSVN

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
#include "RemoveCommand.h"
#include "SVN.h"
#include "InputLogDlg.h"
#include "ShellUpdater.h"
#include "TortoiseProc.h"

#define IDYESTOALL 19

bool RemoveCommand::Execute()
{
    bool bRet = false;
    // removing items from a working copy is done item-by-item so we
    // have a chance to show a progress bar
    //
    // removing items from an URL in the repository requires that we
    // ask the user for a log message.
    SVN svn;
    if ((pathList.GetCount()) && (SVN::PathIsURL(pathList[0])))
    {
        // Delete using URL's, not wc paths
        svn.SetPromptApp(&theApp);
        CInputLogDlg dlg;
        CString      sUuid;
        svn.GetRepositoryRootAndUUID(pathList[0], true, sUuid);
        dlg.SetUUID(sUuid);
        CString sHint;
        if (pathList.GetCount() == 1)
            sHint.Format(IDS_INPUT_REMOVEONE, static_cast<LPCWSTR>(pathList[0].GetSVNPathString()));
        else
            sHint.Format(IDS_INPUT_REMOVEMORE, pathList.GetCount());
        dlg.SetActionText(sHint);
        if (dlg.DoModal() == IDOK)
        {
            if (!svn.Remove(pathList, true, !!parser.HasKey(L"keep"), dlg.GetLogMessage()))
            {
                svn.ShowErrorDialog(GetExplorerHWND(), pathList.GetCommonDirectory());
                return false;
            }
            return true;
        }
        return false;
    }
    else
    {
        bool bForce    = false;
        bool bDoForAll = false;
        for (int nPath = 0; nPath < pathList.GetCount(); nPath++)
        {
            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": remove file %s\n", static_cast<LPCWSTR>(pathList[nPath].GetUIPathString()));
            // even though SVN::Remove takes a list of paths to delete at once
            // we delete each item individually so we can prompt the user
            // if something goes wrong or unversioned/modified items are
            // to be deleted
            CTSVNPathList removePathList(pathList[nPath]);
            if ((bForce) && (!parser.HasKey(L"keep")))
            {
                CTSVNPath delPath = removePathList[0];
                if (!delPath.IsDirectory())
                    delPath.Delete(true);
                // note: we don't move folders to the trash bin, so they can't
                // get restored anymore - svn removes *all* files in a removed
                // folder, even modified and unversioned ones
                // We could move the folders here to the trash bin too, but then
                // the folder would be gone and will require a recursive commit.
                // Of course: a solution would be to first create a copy of the folder,
                // move the original folder to the trash, then rename the copied folder
                // to the original name, then let svn delete the folder - but
                // that would just take too much time for bigger folders...
            }
            if (!svn.Remove(removePathList, bForce, !!parser.HasKey(L"keep")))
            {
                if ((svn.GetSVNError()->apr_err == SVN_ERR_UNVERSIONED_RESOURCE) ||
                    (svn.GetSVNError()->apr_err == SVN_ERR_CLIENT_MODIFIED))
                {
                    if (!bDoForAll)
                    {
                        UINT    ret = 0;
                        CString msg;
                        if (pathList[nPath].IsDirectory())
                            msg.Format(IDS_PROC_REMOVEFORCE_TASK1_2, static_cast<LPCWSTR>(svn.GetLastErrorMessage(0)), static_cast<LPCWSTR>(pathList[nPath].GetFileOrDirectoryName()));
                        else
                            msg.Format(IDS_PROC_REMOVEFORCE_TASK1, static_cast<LPCWSTR>(svn.GetLastErrorMessage(0)), static_cast<LPCWSTR>(pathList[nPath].GetFileOrDirectoryName()));
                        CTaskDialog taskDlg(msg,
                                            CString(MAKEINTRESOURCE(IDS_PROC_REMOVEFORCE_TASK2)),
                                            L"TortoiseSVN",
                                            0,
                                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                        taskDlg.AddCommandControl(IDYES, CString(MAKEINTRESOURCE(IDS_PROC_REMOVEFORCE_TASK3)));
                        taskDlg.AddCommandControl(IDNO, CString(MAKEINTRESOURCE(IDS_PROC_REMOVEFORCE_TASK4)));
                        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                        taskDlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_PROC_REMOVEFORCE_TASK5)));
                        taskDlg.SetDefaultCommandControl(IDNO);
                        taskDlg.SetMainIcon(TD_WARNING_ICON);
                        ret = static_cast<UINT>(taskDlg.DoModal(GetExplorerHWND()));
                        if (taskDlg.GetVerificationCheckboxState())
                            bDoForAll = true;
                        if (ret == IDYES)
                        {
                            if (bDoForAll)
                                bForce = true;
                            if (!parser.HasKey(L"keep"))
                            {
                                CTSVNPath delPath = removePathList[0];
                                if (!delPath.IsDirectory())
                                    delPath.Delete(true);
                                // note: see comment for the delPath.Delete() above
                                // don't try to 'svn rm' the file below if the file is unversioned: we just removed it here
                                if (!delPath.IsDirectory() && (svn.GetSVNError()->apr_err == SVN_ERR_UNVERSIONED_RESOURCE))
                                    continue;
                            }
                            if (!svn.Remove(removePathList, true, !!parser.HasKey(L"keep")))
                            {
                                svn.ShowErrorDialog(GetExplorerHWND(), removePathList.GetCommonDirectory());
                            }
                            else
                                bRet = true;
                        }
                    }
                }
                else
                {
                    // only show an error if we don't force all: if the filename is bad we
                    // come here because the file was unversioned but deleted above
                    if (!bDoForAll || !bForce || (svn.GetSVNError()->apr_err != SVN_ERR_BAD_FILENAME))
                        svn.ShowErrorDialog(GetExplorerHWND(), removePathList.GetCommonDirectory());
                }
            }
        }
    }
    if (bRet)
        CShellUpdater::Instance().AddPathsForUpdate(pathList);
    return bRet;
}
