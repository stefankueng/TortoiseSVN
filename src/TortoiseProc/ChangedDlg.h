// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#pragma once

#include "afxtempl.h"
#include "Registry.h"
#include "ResizableDialog.h"
#include "afxcmn.h"
#include "SVNStatusListCtrl.h"


// CChangedDlg dialog

class CChangedDlg : public CResizableDialog, public SVN
{
	DECLARE_DYNAMIC(CChangedDlg)

public:
	CChangedDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CChangedDlg();

// Dialog Data
	enum { IDD = IDD_CHANGEDFILES };

protected:
	virtual void			DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void			OnPaint();
	afx_msg HCURSOR			OnQueryDragIcon();
	afx_msg void			OnBnClickedCheckrepo();
	afx_msg void			OnBnClickedShowunversioned();
	virtual BOOL			OnInitDialog();
	virtual void			OnOK();
	virtual void			OnCancel();

	DECLARE_MESSAGE_MAP()

protected:
	HICON			m_hIcon;
	HANDLE			m_hThread;
	BOOL			m_bShowUnversioned;

public:
	CSVNStatusListCtrl	m_FileListCtrl;
	CString			m_path;
	bool			m_bRemote;
};

DWORD WINAPI ChangedStatusThread(LPVOID pVoid);
