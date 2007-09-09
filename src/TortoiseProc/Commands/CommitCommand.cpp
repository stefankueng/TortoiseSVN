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
#include "CommitCommand.h"

#include "CommitDlg.h"
#include "SVNProgressDlg.h"
#include "UnicodeUtils.h"
#include "Hooks.h"
#include "MessageBox.h"

bool CommitCommand::Execute()
{
	bool bFailed = true;
	CTSVNPathList selectedList;
	CString sLogMsg;
	DWORD exitcode = 0;
	CString error;
	if (CHooks::Instance().StartCommit(pathList, exitcode, error))
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
		if (parser.HasKey(_T("logmsg")))
		{
			dlg.m_sLogMessage = parser.GetVal(_T("logmsg"));
		}
		if (parser.HasKey(_T("logmsgfile")))
		{
			CString logmsgfile = parser.GetVal(_T("logmsgfile"));
			if (PathFileExists(logmsgfile))
			{
				try
				{
					CStdioFile msgfile;
					if (msgfile.Open(logmsgfile, CFile::modeRead))
					{
						CStringA filecontent;
						int filelength = (int)msgfile.GetLength();
						int bytesread = (int)msgfile.Read(filecontent.GetBuffer(filelength), filelength);
						filecontent.ReleaseBuffer(bytesread);
						dlg.m_sLogMessage = CUnicodeUtils::GetUnicode(filecontent);
						msgfile.Close();
					}
				} 
				catch (CFileException* /*pE*/)
				{
					dlg.m_sLogMessage.Empty();
				}
			}
		}
		if (parser.HasKey(_T("bugid")))
		{
			dlg.m_sBugID = parser.GetVal(_T("bugid"));
		}
		if (!sLogMsg.IsEmpty())
			dlg.m_sLogMessage = sLogMsg;
		dlg.m_pathList = pathList;
		dlg.m_checkedPathList = selectedList;
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
			CSVNProgressDlg progDlg;
			if (!dlg.m_sChangeList.IsEmpty())
				progDlg.SetChangeList(dlg.m_sChangeList, !!dlg.m_bKeepChangeList);
			progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
			progDlg.SetParams(CSVNProgressDlg::SVNProgress_Commit, dlg.m_bKeepLocks ? ProgOptKeeplocks : 0, dlg.m_pathList, _T(""), dlg.m_sLogMessage, !dlg.m_bRecursive);
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