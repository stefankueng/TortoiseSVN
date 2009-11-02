// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009 - TortoiseSVN

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
#include "StandAloneDlg.h"
#include "RegHistory.h"
#include "AeroControls.h"

/**
 * \ingroup TortoiseProc
 * Dialog showing the log message history.
 */
class CHistoryDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CHistoryDlg)
public:
	CHistoryDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CHistoryDlg();

	/// Returns the text of the selected entry.
	CString GetSelectedText() const {return m_SelectedText;}
	/// Sets the history object to use
	void SetHistory(CRegHistory& history) {m_history = &history;}
	// Dialog Data
	enum { IDD = IDD_HISTORYDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnLbnDblclkHistorylist();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
private:
	CListBox		m_List;
	CString			m_SelectedText;
	CRegHistory*	m_history;
	AeroControlBase m_aeroControls;
};
