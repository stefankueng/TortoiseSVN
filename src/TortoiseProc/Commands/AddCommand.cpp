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
#include "AddCommand.h"

#include "AddDlg.h"
#include "SVNProgressDlg.h"
#include "ShellUpdater.h"

bool AddCommand::Execute()
{
	bool bRet = false;
	if (parser.HasKey(_T("noui")))
	{
		SVN svn;
		ProjectProperties props;
		props.ReadPropsPathList(pathList);
		bRet = !!svn.Add(pathList, &props, svn_depth_empty, FALSE, FALSE, TRUE);
		CShellUpdater::Instance().AddPathsForUpdate(pathList);
	}
	else
	{
		if (pathList.AreAllPathsFiles())
		{
			SVN svn;
			ProjectProperties props;
			props.ReadPropsPathList(pathList);
			bRet = !!svn.Add(pathList, &props, svn_depth_empty, FALSE, FALSE, TRUE);
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
				if (parser.HasVal(_T("closeonend")))
					progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
				progDlg.SetPathList(dlg.m_pathList);
				ProjectProperties props;
				props.ReadPropsPathList(dlg.m_pathList);
				progDlg.SetProjectProperties(props);
				progDlg.SetItemCount(dlg.m_pathList.GetCount());
				progDlg.DoModal();
				bRet = !progDlg.DidErrorsOccur();
			}
		}
	}
	return bRet;
}
