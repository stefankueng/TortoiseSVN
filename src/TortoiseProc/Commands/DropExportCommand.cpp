// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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
#include "DropExportCommand.h"
#include "MessageBox.h"
#include "SVN.h"

bool DropExportCommand::Execute()
{
	CString droppath = parser.GetVal(_T("droptarget"));
	if (CTSVNPath(droppath).IsAdminDir())
		return false;
	SVN svn;
	for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
	{
		CString dropper = droppath + _T("\\") + pathList[nPath].GetFileOrDirectoryName();
		if (PathFileExists(dropper))
		{
			CString sMsg;
			CString sBtn1(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_OVERWRITE));
			CString sBtn2(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_RENAME));
			CString sBtn3(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_CANCEL));
			sMsg.Format(IDS_PROC_OVERWRITEEXPORT, dropper);
			UINT ret = CMessageBox::Show(hwndExplorer, sMsg, _T("TortoiseSVN"), MB_DEFBUTTON1, IDI_QUESTION, sBtn1, sBtn2, sBtn3);
			if (ret==2)
			{
				dropper.Format(IDS_PROC_EXPORTFOLDERNAME, droppath, pathList[nPath].GetFileOrDirectoryName());
				int exportcount = 1;
				while (PathFileExists(dropper))
				{
					dropper.Format(IDS_PROC_EXPORTFOLDERNAME2, droppath, exportcount++, pathList[nPath].GetFileOrDirectoryName());
				}
			}
			else if (ret == 3)
				return false;
		}
		if (!svn.Export(pathList[nPath], CTSVNPath(dropper), SVNRev::REV_WC ,SVNRev::REV_WC, FALSE, FALSE, svn_depth_infinity, hwndExplorer, parser.HasKey(_T("extended"))))
		{
			CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
		}
	}
	return true;
}