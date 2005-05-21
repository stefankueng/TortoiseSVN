// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include "DirFileEnum.h"
#include "AddDlg.h"
#include "SVNConfig.h"
#include "Registry.h"


// CAddDlg dialog

IMPLEMENT_DYNAMIC(CAddDlg, CResizableStandAloneDialog)
CAddDlg::CAddDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CAddDlg::IDD, pParent),
	m_bThreadRunning(false)
{
}

CAddDlg::~CAddDlg()
{
}

void CAddDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ADDLIST, m_addListCtrl);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}

BEGIN_MESSAGE_MAP(CAddDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()


BOOL CAddDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	//set the listcontrol to support checkboxes
	m_addListCtrl.Init(0, SVNSLC_POPALL ^ (SVNSLC_POPIGNORE|SVNSLC_POPADD));
	m_addListCtrl.SetSelectButton(&m_SelectAll);

	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("AddDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	if(AfxBeginThread(AddThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	m_bThreadRunning = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAddDlg::OnOK()
{
	if (m_bThreadRunning)
		return;

	//save only the files the user has selected into the pathlist
	m_addListCtrl.WriteCheckedNamesToPathList(m_pathList);

	CResizableStandAloneDialog::OnOK();
}

void CAddDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CAddDlg::OnBnClickedSelectall()
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
	m_addListCtrl.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

UINT CAddDlg::AddThreadEntry(LPVOID pVoid)
{
	return ((CAddDlg*)pVoid)->AddThread();
}
UINT CAddDlg::AddThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	GetDlgItem(IDOK)->EnableWindow(false);
	GetDlgItem(IDCANCEL)->EnableWindow(false);

	m_addListCtrl.GetStatus(m_pathList);
	m_addListCtrl.Show(SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWDIRECTS, SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWDIRECTS);

	GetDlgItem(IDOK)->EnableWindow(true);
	GetDlgItem(IDCANCEL)->EnableWindow(true);
	if (m_addListCtrl.GetItemCount()==0)
	{
		CMessageBox::Show(m_hWnd, IDS_ERR_NOTHINGTOADD, IDS_APPNAME, MB_ICONINFORMATION);
		GetDlgItem(IDCANCEL)->EnableWindow(true);
		m_bThreadRunning = FALSE;
		EndDialog(0);
		return (DWORD)-1;
	}
	m_bThreadRunning = FALSE;
	return 0;
}

void CAddDlg::OnBnClickedHelp()
{
	OnHelp();
}






