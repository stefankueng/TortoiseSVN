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
#include "DiffCommand.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "ChangedDlg.h"
#include "SVNDiff.h"
#include "SVNStatus.h"

bool DiffCommand::Execute()
{
	CString path2 = CPathUtils::GetLongPathname(parser.GetVal(_T("path2")));
	bool bAlternativeTool = !!parser.HasKey(_T("alternative"));
	bool bBlame = !!parser.HasKey(_T("blame"));
	if (path2.IsEmpty())
	{
		if (cmdLinePath.IsDirectory())
		{
			CChangedDlg dlg;
			dlg.m_pathList = CTSVNPathList(cmdLinePath);
			dlg.DoModal();
		}
		else
		{
			SVNDiff diff;
			diff.SetAlternativeTool(bAlternativeTool);
			if ( parser.HasKey(_T("startrev")) && parser.HasKey(_T("endrev")) )
			{
				LONG nStartRevision = parser.GetLongVal(_T("startrev"));
				LONG nEndRevision = parser.GetLongVal(_T("endrev"));
				diff.ShowCompare(cmdLinePath, nStartRevision, cmdLinePath, nEndRevision, SVNRev(), false, bBlame);
			}
			else
			{
				diff.DiffFileAgainstBase(cmdLinePath);
			}
		}
	} 
	else
		CAppUtils::StartExtDiff(
			CTSVNPath(path2), cmdLinePath, CString(), CString(),
			CAppUtils::DiffFlags().AlternativeTool(bAlternativeTool));
	return true;
}