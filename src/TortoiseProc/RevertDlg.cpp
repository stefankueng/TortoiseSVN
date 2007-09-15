// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "messagebox.h"
#include "Revertdlg.h"
#include "SVN.h"
#include "Registry.h"
#include ".\revertdlg.h"

IMPLEMENT_DYNAMIC(CRevertDlg, CResizableStandAloneDialog)
CRevertDlg::CRevertDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRevertDlg::IDD, pParent)
	, m_bSelectAll(TRUE)
	, m_bThreadRunning(FALSE)
	, m_bCancelled(false)
{
}

CRevertDlg::~CRevertDlg()
{
}

void CRevertDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REVERTLIST, m_RevertList);
	DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CRevertDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
END_MESSAGE_MAP()



BOOL CRevertDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_RevertList.Init(SVNSLC_COLTEXTSTATUS | SVNSLC_COLPROPSTATUS, _T("RevertDlg"));
	m_RevertList.SetConfirmButton((CButton*)GetDlgItem(IDOK));
	m_RevertList.SetSelectButton(&m_SelectAll);
	m_RevertList.SetCancelBool(&m_bCancelled);
	m_RevertList.SetBackgroundImage(IDI_REVERT_BKG);

	GetWindowText(m_sWindowTitle);
	
	AdjustControlSize(IDC_SELECTALL);

	AddAnchor(IDC_REVERTLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("RevertDlg"));

	// first start a thread to obtain the file list with the status without
	// blocking the dialog
	if (AfxBeginThread(RevertThreadEntry, this)==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	InterlockedExchange(&m_bThreadRunning, TRUE);

	return TRUE;
}

UINT CRevertDlg::RevertThreadEntry(LPVOID pVoid)
{
	return ((CRevertDlg*)pVoid)->RevertThread();
}

UINT CRevertDlg::RevertThread()
{
	// get the status of all selected file/folders recursively
	// and show the ones which can be reverted to the user
	// in a listcontrol. 
	DialogEnableWindow(IDOK, false);
	m_bCancelled = false;

	if (!m_RevertList.GetStatus(m_pathList))
	{
		CMessageBox::Show(m_hWnd, m_RevertList.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
	}
	m_RevertList.Show(SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWDIRECTFILES, 
						// do not select all files, only the ones the user has selected directly
						SVNSLC_SHOWDIRECTFILES);

	CTSVNPath commonDir = m_RevertList.GetCommonDirectory(false);
	SetWindowText(m_sWindowTitle + _T(" - ") + commonDir.GetWinPathString());

	InterlockedExchange(&m_bThreadRunning, FALSE);
	RefreshCursor();

	return 0;
}

void CRevertDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	// save only the files the user has selected into the temporary file
	m_bRecursive = TRUE;
	for (int i=0; i<m_RevertList.GetItemCount(); ++i)
	{
		if (!m_RevertList.GetCheck(i))
		{
			m_bRecursive = FALSE;
		}
		else 
		{
			CSVNStatusListCtrl::FileEntry * entry = m_RevertList.GetListEntry(i);
			// add all selected entries to the list, except the ones with 'added'
			// status: we later *delete* all the entries in the list before
			// the actual revert is done (so the user has the reverted files
			// still in the trashbin to recover from), but it's not good to
			// delete added files because they're not restored by the revert.
			if (entry->status != svn_wc_status_added)
				m_selectedPathList.AddPath(entry->GetPath());
			// if an entry inside an external is selected, we can't revert
			// recursively anymore because the recursive revert stops at the
			// external boundaries.
			if (entry->IsInExternal())
				m_bRecursive = FALSE;
		}
	}
	if (!m_bRecursive)
	{
		m_RevertList.WriteCheckedNamesToPathList(m_pathList);
	}
	m_selectedPathList.SortByPathname();

	CResizableStandAloneDialog::OnOK();
}

void CRevertDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CRevertDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CRevertDlg::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == BST_INDETERMINATE)
	{
		// It is not at all useful to manually place the checkbox into the indeterminate state...
		// We will force this on to the unchecked state
		state = BST_UNCHECKED;
		m_SelectAll.SetCheck(state);
	}
	theApp.DoWaitCursor(1);
	m_RevertList.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

BOOL CRevertDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
					return TRUE;
				}
			}
			break;
		case VK_F5:
			{
				if (!m_bThreadRunning)
				{
					if (AfxBeginThread(RevertThreadEntry, this)==0)
					{
						CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
					}
					else
						InterlockedExchange(&m_bThreadRunning, TRUE);
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CRevertDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if (AfxBeginThread(RevertThreadEntry, this)==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}
