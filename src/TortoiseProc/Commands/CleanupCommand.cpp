// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "CleanupCommand.h"

#include "MessageBox.h"
#include "ProgressDlg.h"
#include "SVN.h"
#include "SVNGlobal.h"
#include "SVNAdminDir.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"

bool CleanupCommand::Execute()
{
	CProgressDlg progress;
	progress.SetTitle(IDS_PROC_CLEANUP);
	progress.SetAnimation(IDR_CLEANUPANI);
	progress.SetShowProgressBar(false);
	progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO1)));
	progress.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO2)));
	progress.ShowModeless(hwndExplorer);
	
	CString strSuccessfullPaths, strFailedPaths;
	for (int i=0; i<pathList.GetCount(); ++i)
	{
		SVN svn;
		if (!svn.CleanUp(pathList[i]))
		{
			strFailedPaths += _T("- ") + pathList[i].GetWinPathString() + _T("\n");
		}
		else
		{
			strSuccessfullPaths += _T("- ") + pathList[i].GetWinPathString() + _T("\n");

			// after the cleanup has finished, crawl the path downwards and send a change
			// notification for every directory to the shell. This will update the
			// overlays in the left treeview of the explorer.
			CDirFileEnum crawler(pathList[i].GetWinPathString());
			CString sPath;
			bool bDir = false;
			CTSVNPathList updateList;
			while (crawler.NextFile(sPath, &bDir))
			{
				if ((bDir) && (!g_SVNAdminDir.IsAdminDirPath(sPath)))
				{
					updateList.AddPath(CTSVNPath(sPath));
				}
			}
			updateList.AddPath(pathList[i]);
			CShellUpdater::Instance().AddPathsForUpdate(updateList);
			CShellUpdater::Instance().Flush();
			updateList.SortByPathname(true);
			for (INT_PTR i=0; i<updateList.GetCount(); ++i)
			{
				SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, updateList[i].GetWinPath(), NULL);
				ATLTRACE(_T("notify change for path %s\n"), updateList[i].GetWinPath());
			}
		}
	}
	progress.Stop();
	
	CString strMessage;
	if ( !strSuccessfullPaths.IsEmpty() )
	{
		CString tmp;
		tmp.Format(IDS_PROC_CLEANUPFINISHED, strSuccessfullPaths);
		strMessage += tmp;
	}
	if ( !strFailedPaths.IsEmpty() )
	{
		if (!strMessage.IsEmpty())
			strMessage += _T("\n");
		CString tmp;
		tmp.Format(IDS_PROC_CLEANUPFINISHED_FAILED, strFailedPaths);
		strMessage += tmp;
	}
	CMessageBox::Show(hwndExplorer, strMessage, _T("TortoiseSVN"), MB_OK | (strFailedPaths.IsEmpty()?MB_ICONINFORMATION:MB_ICONERROR));

	return true;
}
