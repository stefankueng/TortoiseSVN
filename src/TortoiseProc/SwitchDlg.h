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
#pragma once
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "LogDlg.h"
#include "SVNRev.h"

/**
 * \ingroup TortoiseProc
 * Shows a dialog to prompt the user for an URL of a branch and a revision
 * number to switch the working copy to. Also has a checkbox to
 * specify the current branch instead of a different branch url and
 * one checkbox to specify the newest revision.
 */
class CSwitchDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSwitchDlg)

public:
	CSwitchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSwitchDlg();

	/**
	 * Sets the text for the dialog title.
	 * \remark this method must be called before the dialog is shown!
	 */
	void SetDialogTitle(const CString& sTitle);

	/**
	 * Sets the label in front of the URL combobox.
	 * \remark this method must be called before the dialog is shown!
	 */
	void SetUrlLabel(const CString& sLabel);

// Dialog Data
	enum { IDD = IDD_SWITCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnChangeRevisionNum();
	afx_msg void OnBnClickedLog();
	afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);

	void		SetRevision(const SVNRev& rev);

	DECLARE_MESSAGE_MAP()

	CString			m_rev;
	CHistoryCombo	m_URLCombo;
	BOOL			m_bFolder;
	CString			m_sTitle;
	CString			m_sLabel;
	CLogDlg *		m_pLogDlg;

public:
	CString			m_path;
	CString			m_URL;
	SVNRev			Revision;
};
