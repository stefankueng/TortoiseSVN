// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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

#pragma once
#include "afxcmn.h"
#include "resizer.h"



/**
 * \ingroup TortoiseProc
 * a simple dialog to show the user all unversioned
 * files below a specified folder.
 *
 * \par requirements
 * win95 or later\n
 * winNT4 or later\n
 * MFC\n
 *
 * \version 1.0
 * first version
 *
 * \date 03-06-2003
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class CAddDlg : public CDialog
{
	DECLARE_DYNAMIC(CAddDlg)

	DECLARE_RESIZER;

public:
	CAddDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAddDlg();

// Dialog Data
	enum { IDD = IDD_ADD };

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl		m_addListCtrl;
	CString			m_sPath;
	CStringArray	m_arFileList;

	virtual BOOL OnInitDialog();
private:
	HANDLE			m_hThread;

};

DWORD WINAPI AddThread(LPVOID pVoid);
