// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "MessageBox.h"
#include "CreatePatch.h"
#include "SVN.h"
#include ".\createpatch.h"


// CCreatePatch dialog

IMPLEMENT_DYNAMIC(CCreatePatch, CResizableStandAloneDialog)
CCreatePatch::CCreatePatch(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCreatePatch::IDD, pParent)
	, m_bThreadRunning(FALSE)
	, m_bCancelled(false)
{
}

CCreatePatch::~CCreatePatch()
{
}

void CCreatePatch::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PATCHLIST, m_PatchList);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CCreatePatch, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
END_MESSAGE_MAP()


// CCreatePatch message handlers

BOOL CCreatePatch::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	//set the listcontrol to support checkboxes
	m_PatchList.Init(0, _T("CreatePatchDlg"), SVNSLC_POPALL ^ (SVNSLC_POPIGNORE|SVNSLC_POPCOMMIT));
	m_PatchList.SetConfirmButton((CButton*)GetDlgItem(IDOK));
	m_PatchList.SetSelectButton(&m_SelectAll);
	m_PatchList.SetCancelBool(&m_bCancelled);

	AddAnchor(IDC_PATCHLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("CreatePatchDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	if(AfxBeginThread(PatchThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	InterlockedExchange(&m_bThreadRunning, TRUE);

	return TRUE;
}

UINT CCreatePatch::PatchThreadEntry(LPVOID pVoid)
{
	return ((CCreatePatch*)pVoid)->PatchThread();
}
UINT CCreatePatch::PatchThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	DialogEnableWindow(IDOK, false);
	m_bCancelled = false;

	if (!m_PatchList.GetStatus(m_pathList))
	{
		CMessageBox::Show(m_hWnd, m_PatchList.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
	}

	m_PatchList.Show(SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS, 
						SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS);

	DialogEnableWindow(IDOK, true);
	InterlockedExchange(&m_bThreadRunning, FALSE);
	return 0;
}

BOOL CCreatePatch::PreTranslateMessage(MSG* pMsg)
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
					if(AfxBeginThread(PatchThreadEntry, this) == NULL)
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

void CCreatePatch::OnBnClickedSelectall()
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
	m_PatchList.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

void CCreatePatch::OnBnClickedHelp()
{
	OnHelp();
}

void CCreatePatch::OnCancel()
{
	m_bCancelled = true;
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CCreatePatch::OnOK()
{
	if (m_bThreadRunning)
		return;

	int nAddedFolders = 0;
	int nListItems = m_PatchList.GetItemCount();
	m_filesToRevert.Clear();
	
	for (int j=0; j<nListItems; j++)
	{
		const CSVNStatusListCtrl::FileEntry * entry = m_PatchList.GetListEntry(j);
		if (entry->IsChecked())
		{
			if (entry->status == svn_wc_status_added)
			{
				if (entry->IsFolder())
					nAddedFolders++;
				else
				{
					// an added file. Is it inside an added folder?
					for (int i=0; i<nListItems; ++i)
					{
						const CSVNStatusListCtrl::FileEntry * parententry = m_PatchList.GetListEntry(i);
						if (parententry->status == svn_wc_status_added)
						{
							CTSVNPath checkpath = entry->GetPath().GetContainingDirectory();
							while (!checkpath.IsEmpty())
							{
								if (checkpath.IsEquivalentTo(parententry->GetPath()))
									nAddedFolders++;
								checkpath = checkpath.GetContainingDirectory();
							}
						}
					}
				}	
			}
			// Unversioned files are not included in the resulting patchfile!
			// We add those files to a list which will be used to add those files
			// before creating the patch.
			if ((entry->status == svn_wc_status_none)||(entry->status == svn_wc_status_unversioned))
			{
				m_filesToRevert.AddPath(entry->GetPath());
			}
		}
	}

	if (nAddedFolders != 0)
	{
		if (CMessageBox::Show(m_hWnd, IDS_CREATEPATCH_ADDEDFOLDERS, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)!=IDYES)
			return;
	}

	if (m_filesToRevert.GetCount())
	{
		// add all unversioned files to version control
		// so they're included in the resulting patch
		// NOTE: these files must be reverted after the patch
		// has been created! Since this dialog doesn't create the patch
		// itself, the calling function is responsible to revert these files!
		SVN svn;
		svn.Add(m_filesToRevert, false);
	}
	
	//save only the files the user has selected into the pathlist
	m_PatchList.WriteCheckedNamesToPathList(m_pathList);

	CResizableStandAloneDialog::OnOK();
}

LRESULT CCreatePatch::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if(AfxBeginThread(PatchThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}
