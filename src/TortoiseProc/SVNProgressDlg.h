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
#include "svn.h"
#include "promptdlg.h"

#include "ResizableDialog.h"

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

typedef enum
{
	Checkout = 1,
	Update = 2,
	Enum_Update = 2,
	Commit = 3,
	Add = 4,
	Revert = 5,
	Enum_Revert = 5,
	Resolve = 6,
	Import = 7,
	Switch = 8,
	Export = 9,
	Merge = 10,
	Enum_Merge = 10,
	Copy = 11,
	Relocate = 12
} Command;

/**
 * \ingroup TortoiseProc
 * Handles different Subversion commands and shows the notify messages
 * in a listbox. Since several Subversion commands have similar notify
 * messages they are grouped together in this single class.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 10-20-2002
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
class CSVNProgressDlg : public CResizableDialog, public SVN
{
	DECLARE_DYNAMIC(CSVNProgressDlg)

public:
	class Data
	{
	public:
		CString					path;
		svn_wc_notify_action_t	action;
		svn_node_kind_t			kind;
		CString					mime_type;
		svn_wc_notify_state_t	content_state;
		svn_wc_notify_state_t	prop_state;
		LONG					rev;
	};

	CSVNProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSVNProgressDlg();

	virtual BOOL OnInitDialog();
	/**
	 * Sets the needed parameters for the commands to execute. Call this method
	 * before DoModal()
	 * \param cmd the command to execute on DoModal()
	 * \param path local path to the working copy (directory or file) or to a tempfile containing the filenames
	 * \param url the url of the repository
	 * \param revision the revision to work on or to get
	 */
	void SetParams(Command cmd, BOOL isTempFile, CString path, CString url = _T(""), CString message = _T(""), SVNRev revision = -1, CString modName = _T(""));

// Dialog Data
	enum { IDD = IDD_SVNPROGRESS };

protected:
	//implement the virtual methods from SVN base class
	virtual BOOL Notify(CString path, svn_wc_notify_action_t action, svn_node_kind_t kind, CString mime_type, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state, LONG rev);
	virtual BOOL Cancel();
	virtual void OnCancel();

	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	/**
	 * Resizes the columns of the progress list so that the headings are visible.
	 */
	void ResizeColumns();
	void Sort();
	static int __cdecl SortCompare(const void * pElem1, const void * pElem2);

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnNMCustomdrawSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedLogbutton();
	afx_msg void OnBnClickedOk();
	afx_msg void OnHdnItemclickSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnClose();

	DECLARE_MESSAGE_MAP()

	static BOOL	m_bAscending;
	static int	m_nSortedColumn;
public:			//need to be public for the thread to access
	virtual void OnOK();
	CArray<Data *, Data *>		m_arData;

	CListCtrl	m_ProgList;
	HANDLE		m_hThread;
	Command		m_Command;

	CString		m_sPath;
	CString		m_sUrl;
	CString		m_sMessage;
	CStringArray m_templist;
	SVNRev		m_Revision;
	SVNRev		m_RevisionEnd;
	LONG		m_nUpdateStartRev;
	BOOL		m_IsTempFile;
	BOOL		m_bCancelled;
	BOOL		m_bThreadRunning;
	BOOL		m_bCloseOnEnd;
	BOOL		m_bRedEvents;
	CString		m_sModName;
};
DWORD WINAPI ProgressThread(LPVOID pVoid);
