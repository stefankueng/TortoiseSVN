﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2011, 2014-2015, 2021 - TortoiseSVN

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
#include "DropMoveCommand.h"

#include "ProgressDlg.h"
#include "SVN.h"
#include "RenameDlg.h"
#include "ShellUpdater.h"

#define IDYESTOALL 19
#define IDNOTOALL  20

bool DropMoveCommand::Execute()
{
    CString dropPath = parser.GetVal(L"droptarget");
    if (CTSVNPath(dropPath).IsAdminDir())
        return FALSE;
    SVN           svn;
    unsigned long count = 0;
    pathList.RemoveAdminPaths();
    CString sNewName;
    if ((parser.HasKey(L"rename")) && (pathList.GetCount() == 1))
    {
        // ask for a new name of the source item
        CRenameDlg renDlg;
        renDlg.SetFileSystemAutoComplete();
        renDlg.SetInputValidator(this);
        renDlg.m_windowTitle.LoadString(IDS_PROC_MOVERENAME);
        renDlg.m_name = pathList[0].GetFileOrDirectoryName();
        if (renDlg.DoModal() != IDOK)
        {
            return FALSE;
        }
        sNewName = renDlg.m_name;
    }
    CProgressDlg progress;
    if (progress.IsValid())
    {
        progress.SetTitle(IDS_PROC_MOVING);
        progress.SetTime(true);
        progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
    }
    UINT    msgRet             = IDNO;
    INT_PTR msgRetNonVersioned = 0;
    for (int nPath = 0; nPath < pathList.GetCount(); nPath++)
    {
        CTSVNPath destPath;
        if (sNewName.IsEmpty())
            destPath = CTSVNPath(dropPath + L"\\" + pathList[nPath].GetFileOrDirectoryName());
        else
            destPath = CTSVNPath(dropPath + L"\\" + sNewName);
        // path the same but case-changed is ok: results in a case-rename
        if (!(pathList[nPath].IsEquivalentToWithoutCase(destPath) && !pathList[nPath].IsEquivalentTo(destPath)))
        {
            if (destPath.Exists())
            {
                progress.Stop();

                CString name = pathList[nPath].GetFileOrDirectoryName();
                if (!sNewName.IsEmpty())
                    name = sNewName;
                progress.Stop();
                CRenameDlg dlg;
                dlg.SetFileSystemAutoComplete();
                dlg.SetInputValidator(this);
                dlg.m_name = name;
                dlg.m_windowTitle.Format(IDS_PROC_NEWNAMEMOVE, static_cast<LPCWSTR>(name));
                if (dlg.DoModal() != IDOK)
                {
                    return FALSE;
                }
                destPath.SetFromWin(dropPath + L"\\" + dlg.m_name);

                progress.EnsureValid();
                progress.SetTitle(IDS_PROC_MOVING);
                progress.SetTime(true);
                progress.SetProgress(count, pathList.GetCount());
                progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
            }
        }
        if (!svn.Move(CTSVNPathList(pathList[nPath]), destPath))
        {
            if ((svn.GetSVNError() && svn.GetSVNError()->apr_err == SVN_ERR_ENTRY_EXISTS) && (destPath.Exists()))
            {
                if ((msgRet != IDYESTOALL) && (msgRet != IDNOTOALL))
                {
                    progress.Stop();
                    // target file already exists. Ask user if he wants to replace the file
                    CString sReplace;
                    sReplace.Format(IDS_PROC_REPLACEEXISTING, destPath.GetWinPath());
                    CTaskDialog taskDlg(sReplace,
                                        CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK2)),
                                        L"TortoiseSVN",
                                        0,
                                        TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                    taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK3)));
                    taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK4)));
                    taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                    taskDlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK5)));
                    taskDlg.SetVerificationCheckbox(false);
                    taskDlg.SetDefaultCommandControl(200);
                    taskDlg.SetMainIcon(TD_WARNING_ICON);
                    INT_PTR ret = taskDlg.DoModal(GetExplorerHWND());
                    if (ret == 100) // replace
                        msgRet = taskDlg.GetVerificationCheckboxState() ? IDYESTOALL : IDYES;
                    else
                        msgRet = taskDlg.GetVerificationCheckboxState() ? IDNOTOALL : IDNO;

                    progress.EnsureValid();
                    progress.SetTitle(IDS_PROC_MOVING);
                    progress.SetTime(true);
                    progress.SetProgress(count, pathList.GetCount());
                    progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
                }

                if ((msgRet == IDYES) || (msgRet == IDYESTOALL))
                {
                    if (!svn.Remove(CTSVNPathList(destPath), true, false))
                    {
                        destPath.Delete(true);
                    }
                    if (!svn.Move(CTSVNPathList(pathList[nPath]), destPath))
                    {
                        progress.Stop();
                        svn.ShowErrorDialog(GetExplorerHWND(), pathList[nPath]);
                        return FALSE; //get out of here
                    }
                    CShellUpdater::Instance().AddPathForUpdate(destPath);
                }
            }
            else if (svn.GetSVNError() && svn.GetSVNError()->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
            {
                INT_PTR ret = 0;
                if (msgRetNonVersioned == 0)
                {
                    progress.Stop();

                    CString sReplace;
                    sReplace.Format(IDS_PROC_MOVEUNVERSIONED_TASK1, destPath.GetWinPath());
                    CTaskDialog taskDlg(sReplace,
                                        CString(MAKEINTRESOURCE(IDS_PROC_MOVEUNVERSIONED_TASK2)),
                                        L"TortoiseSVN",
                                        TDCBF_CANCEL_BUTTON,
                                        TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                    taskDlg.AddCommandControl(101, CString(MAKEINTRESOURCE(IDS_PROC_MOVEUNVERSIONED_TASK3)));
                    taskDlg.AddCommandControl(102, CString(MAKEINTRESOURCE(IDS_PROC_MOVEUNVERSIONED_TASK4)));
                    taskDlg.AddCommandControl(103, CString(MAKEINTRESOURCE(IDS_PROC_MOVEUNVERSIONED_TASK5)));
                    taskDlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_PROC_MOVEUNVERSIONED_TASK6)));
                    taskDlg.SetVerificationCheckbox(false);
                    taskDlg.SetDefaultCommandControl(103);
                    taskDlg.SetMainIcon(TD_WARNING_ICON);
                    ret = taskDlg.DoModal(GetExplorerHWND());
                    if (taskDlg.GetVerificationCheckboxState())
                        msgRetNonVersioned = ret;

                    progress.EnsureValid();
                    progress.SetTitle(IDS_PROC_MOVING);
                    progress.SetTime(true);
                    progress.SetProgress(count, pathList.GetCount());
                    progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
                }
                else
                {
                    ret = msgRetNonVersioned;
                }
                switch (ret)
                {
                    case 101: // move
                        MoveFile(pathList[nPath].GetWinPath(), destPath.GetWinPath());
                        break;
                    case 102: // move and add
                        MoveFile(pathList[nPath].GetWinPath(), destPath.GetWinPath());
                        if (!svn.Add(CTSVNPathList(destPath), nullptr, svn_depth_infinity, true, false, false, false))
                        {
                            progress.Stop();
                            svn.ShowErrorDialog(GetExplorerHWND(), destPath);
                            return FALSE; //get out of here
                        }
                        break;
                    case 103: // skip
                    default:
                        break;
                }
            }
            else
            {
                progress.Stop();
                svn.ShowErrorDialog(GetExplorerHWND(), pathList[nPath]);
                return FALSE; //get out of here
            }
        }
        else
            CShellUpdater::Instance().AddPathForUpdate(destPath);
        count++;
        if (progress.IsValid())
        {
            progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, pathList[nPath].GetWinPath());
            progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, destPath.GetWinPath());
            progress.SetProgress(count, pathList.GetCount());
        }
        if ((progress.IsValid()) && (progress.HasUserCancelled()))
        {
            progress.Stop();
            TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_SVN_USERCANCELLED), nullptr, TDCBF_OK_BUTTON, TD_INFORMATION_ICON, nullptr);
            return FALSE;
        }
    }
    return true;
}

CString DropMoveCommand::Validate(int /*nID*/, const CString& input)
{
    CString sError;

    CString sDropPath = parser.GetVal(L"droptarget");
    if (input.IsEmpty())
        sError.LoadString(IDS_ERR_NOVALIDPATH);
    else if (!CTSVNPath(sDropPath + L"\\" + input).IsValidOnWindows())
        sError.LoadString(IDS_ERR_NOVALIDPATH);

    return sError;
}
