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
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once


// For base class
#include "SVNRev.h"
#include "StandAloneDlg.h"

/**
 * \ingroup TortoiseProc
 * A simple dialog box asking the user for a revision number.
 */
class CRevisionDlg : public CDialog, public SVNRev
{
	DECLARE_DYNAMIC(CRevisionDlg)

public:
	CRevisionDlg(CWnd* pParent = NULL);
	virtual ~CRevisionDlg();

	enum { IDD = IDD_REVISION };

	CString GetEnteredRevisionString() {return m_sRevision;}
	void	AllowWCRevs(bool bAllowWCRevs = true) {m_bAllowWCRevs = bAllowWCRevs;}
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnEnChangeRevnum();

	DECLARE_MESSAGE_MAP()

	CString m_sRevision;
	bool	m_bAllowWCRevs;
};
