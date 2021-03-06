// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2011, 2014-2015, 2021 - TortoiseSVN

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
#include "AddCommand.h"

#include "AddDlg.h"
#include "SVNProgressDlg.h"
#include "ShellUpdater.h"
#include "SVNStatus.h"
#include "TortoiseProc.h"

#define IDCUSTOM1 23
#define IDCUSTOM2 24

bool AddCommand::Execute()
{
    bool bRet = false;
    if (parser.HasKey(L"noui"))
    {
        SVN               svn;
        ProjectProperties props;
        if (!props.ReadPropsPathList(pathList))
            props.ReadProps(pathList.GetCommonRoot());
        bRet = !!svn.Add(pathList, &props, svn_depth_empty, true, true, false, true);
        CShellUpdater::Instance().AddPathsForUpdate(pathList);
    }
    else
    {
        if (pathList.AreAllPathsFiles())
        {
            SVNStatus            status;
            CTSVNPath            retPath;
            svn_client_status_t* s = nullptr;

            if ((s = status.GetFirstFileStatus(pathList.GetCommonDirectory(), retPath)) != 0)
            {
                do
                {
                    if (s->node_status == svn_wc_status_missing)
                    {
                        for (int i = 0; i < pathList.GetCount(); ++i)
                        {
                            if (pathList[i].IsEquivalentToWithoutCase(retPath))
                            {
                                UINT    ret = 0;
                                CString sMessage;
                                sMessage.FormatMessage(IDS_WARN_ADDCASERENAMED, static_cast<LPCWSTR>(pathList[i].GetFileOrDirectoryName()), static_cast<LPCWSTR>(retPath.GetFileOrDirectoryName()));
                                CTaskDialog taskDlg(sMessage,
                                                    CString(MAKEINTRESOURCE(IDS_WARN_ADDCASERENAMED_TASK2)),
                                                    L"TortoiseSVN",
                                                    0,
                                                    TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                                taskDlg.AddCommandControl(IDCUSTOM1, CString(MAKEINTRESOURCE(IDS_WARN_ADDCASERENAMED_TASK3)));
                                taskDlg.AddCommandControl(IDCUSTOM2, CString(MAKEINTRESOURCE(IDS_WARN_ADDCASERENAMED_TASK4)));
                                taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                                taskDlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_WARN_RELOCATEREALLY_TASK5)));
                                taskDlg.SetDefaultCommandControl(IDCUSTOM1);
                                taskDlg.SetMainIcon(TD_WARNING_ICON);
                                ret = static_cast<UINT>(taskDlg.DoModal(GetExplorerHWND()));

                                if (ret == IDCUSTOM1)
                                {
                                    // fix case of filename
                                    MoveFileEx(pathList[i].GetWinPath(), retPath.GetWinPath(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
                                    // remove it from the list
                                    pathList.RemovePath(pathList[i]);
                                }
                                else if (ret != IDCUSTOM2)
                                    return false;
                            }
                        }
                    }
                } while ((s = status.GetNextFileStatus(retPath)) != nullptr);
            }

            SVN               svn;
            ProjectProperties props;
            if (!props.ReadPropsPathList(pathList))
                props.ReadProps(pathList.GetCommonRoot());
            bRet = !!svn.Add(pathList, &props, svn_depth_empty, true, true, false, true);
            if (!bRet)
            {
                svn.ShowErrorDialog(GetExplorerHWND(), pathList.GetCommonDirectory());
            }
            CShellUpdater::Instance().AddPathsForUpdate(pathList);
        }
        else
        {
            CAddDlg dlg;
            dlg.m_pathList = pathList;
            if (dlg.DoModal() == IDOK)
            {
                if (dlg.m_pathList.GetCount() == 0)
                    return FALSE;
                CSVNProgressDlg progDlg;
                theApp.m_pMainWnd = &progDlg;
                progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Add);
                DWORD dwOpts = ProgOptForce;
                if (dlg.m_useAutoprops)
                    dwOpts |= ProgOptUseAutoprops;
                progDlg.SetOptions(dwOpts);
                progDlg.SetAutoClose(parser);
                progDlg.SetPathList(dlg.m_pathList);
                ProjectProperties props;
                if (!props.ReadPropsPathList(dlg.m_pathList))
                    props.ReadProps(dlg.m_pathList.GetCommonRoot());
                progDlg.SetProjectProperties(props);
                progDlg.SetItemCount(dlg.m_pathList.GetCount());
                progDlg.DoModal();
                bRet = !progDlg.DidErrorsOccur();
            }
        }
    }
    return bRet;
}