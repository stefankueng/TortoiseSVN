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

#include "ResizableDialog.h"
#include "SVN.h"

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

/** Options which can be used to configure the way the dialog box works
 *
 */
typedef enum
{
	ProgOptNone = 0,
	ProgOptRecursive = 0x01,
	ProgOptNonRecursive = 0x00,
	// The path parameter is the name of a temporary file
	ProgOptPathIsTempFile = 0x02,
	// The path parameter is a real target 
	ProgOptPathIsTarget = 0x00,		
	// Don't actually do the merge - just practice it
	ProgOptDryRun = 0x04
} ProgressOptions;


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
class CSVNProgressDlg : public CResizableDialog, SVN
{
public:
	// These names collide with functions in SVN
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

	DECLARE_DYNAMIC(CSVNProgressDlg)

public:
	class NotificationData
	{
	public:
		CTSVNPath				path;
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
	void SetParams(Command cmd, int options, const CString& path, const CString& url = CString(), const CString& message = CString(), SVNRev revision = -1); 

	CString BuildInfoString();

// Dialog Data
	enum { IDD = IDD_SVNPROGRESS };

protected:
	//implement the virtual methods from SVN base class
	virtual BOOL Notify(const CTSVNPath& path, svn_wc_notify_action_t action, svn_node_kind_t kind, const CString& mime_type, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state, LONG rev);
	virtual BOOL Cancel();
	virtual void OnCancel();

	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
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
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()

	static BOOL	m_bAscending;
	static int	m_nSortedColumn;
	CStringList m_ExtStack;
public:			//need to be public for the thread to access

private:
	static UINT ProgressThreadEntry(LPVOID pVoid);
	UINT ProgressThread();
	virtual void OnOK();
	void ReportSVNError() const;
public:

private:
	/**
	* Resizes the columns of the progress list so that the headings are visible.
	*/
	void ResizeColumns();

public:
	BOOL		m_bCloseOnEnd;
	SVNRev		m_RevisionEnd;

private:

	CArray<NotificationData *, NotificationData *>		m_arData;

	CListCtrl	m_ProgList;
	CWinThread* m_pThread;
	Command		m_Command;
	int			m_options;	// Use values from the ProgressOptions enum

	CTSVNPathList m_targetPathList;
	CTSVNPath	m_url;
	CString		m_sMessage;
	CTSVNPathList m_templist;
	SVNRev		m_Revision;
	LONG		m_nUpdateStartRev;
	BOOL		m_bCancelled;
	BOOL		m_bThreadRunning;
	BOOL		m_bRedEvents;
	int			iFirstResized;
	BOOL		bSecondResized;
	// The path of the item we will offer to show a log for, after an 'update' is complete
	CTSVNPath	m_updatedPath;

private:
	// In preparation for removing SVN as base class
	// Currently needed to avoid ambiguities with the Command Enum
	SVN* m_pSvn;
};
