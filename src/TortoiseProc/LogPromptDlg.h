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

#pragma once
#include "afxcmn.h"
#include "ResizableDialog.h"

/**
 * \ingroup TortoiseProc
 * Dialog to enter log messages used in a commit.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 10-25-2002
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CLogPromptDlg : public CResizableDialog
{
	DECLARE_DYNAMIC(CLogPromptDlg)

public:
	CLogPromptDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLogPromptDlg();

// Dialog Data
	enum { IDD = IDD_LOGPROMPT };

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLvnItemchangedFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedSelectall();

	void StartDiff(int fileindex);
	DECLARE_MESSAGE_MAP()
public:
	CString			m_sLogMessage;
	CListCtrl		m_ListCtrl;
	CString			m_sPath;
	CStringArray	m_arFileList;
	CStringArray	m_templist;
	CDWordArray		m_arFileStatus;
private:
	HANDLE			m_hThread;
	BOOL			m_bSelectAll;
	CFont			m_logFont;
};

DWORD WINAPI StatusThread(LPVOID pVoid);
