// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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
#include "CommitCommand.h"

#include "CommitDlg.h"
#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "Hooks.h"
#include "MessageBox.h"

CString CommitCommand::LoadLogMessage()
{
	CString msg;
	if (parser.HasKey(_T("logmsg")))
	{
		msg = parser.GetVal(_T("logmsg"));
	}
	if (parser.HasKey(_T("logmsgfile")))
	{
		CString logmsgfile = parser.GetVal(_T("logmsgfile"));
		CStringUtils::ReadStringFromTextFile(logmsgfile, msg);
	}
	return msg;
}

bool CommitCommand::Execute()
{
	bool bFailed = true;
	CTSVNPathList selectedList;
	CString sLogMsg = LoadLogMessage();
	bool bSelectFilesForCommit = !!DWORD(CRegStdWORD(_T("Software\\TortoiseSVN\\SelectFilesForCommit"), TRUE));
	DWORD exitcode = 0;
	CString error;
	if (CHooks::Instance().StartCommit(pathList, sLogMsg, exitcode, error))
	{
		if (exitcode)
		{
			CString temp;
			temp.Format(IDS_ERR_HOOKFAILED, error);
			CMessageBox::Show(hwndExplorer, temp, _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
	}
	while (bFailed)
	{
		bFailed = false;
		CCommitDlg dlg;
		if (parser.HasKey(_T("bugid")))
		{
			dlg.m_sBugID = parser.GetVal(_T("bugid"));
		}
		dlg.m_sLogMessage = sLogMsg;
		dlg.m_pathList = pathList;
		dlg.m_checkedPathList = selectedList;
		dlg.m_bSelectFilesForCommit = bSelectFilesForCommit;
		if (dlg.DoModal() == IDOK)
		{
			if (dlg.m_pathList.GetCount()==0)
				return false;
			// if the user hasn't changed the list of selected items
			// we don't use that list. Because if we would use the list
			// of pre-checked items, the dialog would show different
			// checked items on the next startup: it would only try
			// to check the parent folder (which might not even show)
			// instead, we simply use an empty list and let the
			// default checking do its job.
			if (!dlg.m_pathList.IsEqual(pathList))
				selectedList = dlg.m_pathList;
			sLogMsg = dlg.m_sLogMessage;
			bSelectFilesForCommit = true;
			CSVNProgressDlg progDlg;
			if (!dlg.m_sChangeList.IsEmpty())
				progDlg.SetChangeList(dlg.m_sChangeList, !!dlg.m_bKeepChangeList);
			progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
			progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Commit);
			progDlg.SetOptions(dlg.m_bKeepLocks ? ProgOptKeeplocks : ProgOptNone);
			progDlg.SetPathList(dlg.m_pathList);
			progDlg.SetCommitMessage(dlg.m_sLogMessage);
			progDlg.SetDepth(dlg.m_bRecursive ? svn_depth_infinity : svn_depth_empty);
			progDlg.SetSelectedList(dlg.m_selectedPathList);
			progDlg.SetItemCount(dlg.m_itemsCount);
			progDlg.DoModal();
			CRegDWORD err = CRegDWORD(_T("Software\\TortoiseSVN\\ErrorOccurred"), FALSE);
			err = (DWORD)progDlg.DidErrorsOccur();
			bFailed = progDlg.DidErrorsOccur();
			CRegDWORD bFailRepeat = CRegDWORD(_T("Software\\TortoiseSVN\\CommitReopen"), FALSE);
			if (DWORD(bFailRepeat)==0)
				bFailed = false;		// do not repeat if the user chose not to in the settings.
		}
	}
	return true;
}
