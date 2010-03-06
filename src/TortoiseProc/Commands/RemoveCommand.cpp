// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010 - TortoiseSVN

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
#include "StdAfx.h"
#include "RemoveCommand.h"

#include "MessageBox.h"
#include "ProgressDlg.h"
#include "SVN.h"
#include "InputLogDlg.h"
#include "ShellUpdater.h"

bool RemoveCommand::Execute()
{
	bool bRet = false;
	// removing items from a working copy is done item-by-item so we
	// have a chance to show a progress bar
	//
	// removing items from an URL in the repository requires that we
	// ask the user for a log message.
	bool bForce = false;
	SVN svn;
	if ((pathList.GetCount())&&(SVN::PathIsURL(pathList[0])))
	{
		// Delete using URL's, not wc paths
		svn.SetPromptApp(&theApp);
		CInputLogDlg dlg;
		CString sUUID;
		svn.GetRepositoryRootAndUUID(pathList[0], true, sUUID);
		dlg.SetUUID(sUUID);
		CString sHint;
		if (pathList.GetCount() == 1)
			sHint.Format(IDS_INPUT_REMOVEONE, (LPCTSTR)pathList[0].GetSVNPathString());
		else
			sHint.Format(IDS_INPUT_REMOVEMORE, pathList.GetCount());
		dlg.SetActionText(sHint);
		if (dlg.DoModal()==IDOK)
		{
			if (!svn.Remove(pathList, true, !!parser.HasKey(_T("keep")), dlg.GetLogMessage()))
			{
				CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return false;
			}
			return true;
		}
		return false;
	}
	else
	{
		for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
		{
			TRACE(_T("remove file %s\n"), (LPCTSTR)pathList[nPath].GetUIPathString());
			// even though SVN::Remove takes a list of paths to delete at once
			// we delete each item individually so we can prompt the user
			// if something goes wrong or unversioned/modified items are
			// to be deleted
			CTSVNPathList removePathList(pathList[nPath]);
			if ((bForce)&&(!parser.HasKey(_T("keep"))))
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
			if (!svn.Remove(removePathList, bForce, !!parser.HasKey(_T("keep"))))
			{
				if ((svn.Err->apr_err == SVN_ERR_UNVERSIONED_RESOURCE) ||
					(svn.Err->apr_err == SVN_ERR_CLIENT_MODIFIED))
				{
					CString msg, yes, no, yestoall;
					if (pathList[nPath].IsDirectory())
					{
						msg.Format(IDS_PROC_REMOVEFORCEFOLDER, pathList[nPath].GetWinPath());
					}
					else
					{
						msg.Format(IDS_PROC_REMOVEFORCE, (LPCTSTR)svn.GetLastErrorMessage());
					}
					yes.LoadString(IDS_MSGBOX_YES);
					no.LoadString(IDS_MSGBOX_NO);
					yestoall.LoadString(IDS_PROC_YESTOALL);
					UINT ret = CMessageBox::Show(hwndExplorer, msg, _T("TortoiseSVN"), 2, IDI_ERROR, yes, no, yestoall);
					if (ret == 3)
						bForce = true;
					if ((ret == 1)||(ret==3))
					{
						if (!parser.HasKey(_T("keep")))
						{
							CTSVNPath delPath = removePathList[0];
							if (!delPath.IsDirectory())
								delPath.Delete(true);
							// note: see comment for the delPath.Delete() above
						}
						if (!svn.Remove(removePathList, true, !!parser.HasKey(_T("keep"))))
						{
							CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
						else
							bRet = true;
					}
				}
				else
					CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
		}
	}
	if (bRet)
		CShellUpdater::Instance().AddPathsForUpdate(pathList);
	return bRet;
}
