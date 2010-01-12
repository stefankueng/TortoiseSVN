// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
#include "SVNStatusListCtrl.h"
#include "ProjectProperties.h"
#include "RegHistory.h"
#include "Registry.h"
#include "SciEdit.h"
#include "SplitterControl.h"
#include "LinkControl.h"
#include "HyperLink.h"
#include "PathWatcher.h"
#include "BugTraqAssociations.h"
#include "Tooltip.h"
#include "..\IBugTraqProvider\IBugTraqProvider_h.h"
#include "PathEdit.h"
#include "AeroControls.h"

#include <regex>
using namespace std;

#define ENDDIALOGTIMER 100
#define REFRESHTIMER   101


/**
 * \ingroup TortoiseProc
 * Dialog to enter log messages used in a commit.
 */
class CCommitDlg : public CResizableStandAloneDialog, public CSciEditContextMenuInterface
{
	DECLARE_DYNAMIC(CCommitDlg)

public:
	CCommitDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCommitDlg();

	// CSciEditContextMenuInterface
	virtual void		InsertMenuItems(CMenu& mPopup, int& nCmd);
	virtual bool		HandleMenuItemClick(int cmd, CSciEdit * pSciEdit);

private:
	static UINT StatusThreadEntry(LPVOID pVoid);
	UINT StatusThread();
	void UpdateOKButton();
	void OnComError( HRESULT hr );

// Dialog Data
	enum { IDD = IDD_COMMITDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedShowexternals();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedShowunversioned();
	afx_msg void OnBnClickedHistory();
	afx_msg void OnBnClickedBugtraqbutton();
	afx_msg void OnEnChangeLogmessage();
	afx_msg void OnStnClickedExternalwarning();
	afx_msg LRESULT OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
	afx_msg LRESULT OnSVNStatusListCtrlCheckChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnSVNStatusListCtrlChangelistChanged(WPARAM count, LPARAM);
	afx_msg LRESULT OnCheck(WPARAM count, LPARAM);
	afx_msg LRESULT OnAutoListReady(WPARAM, LPARAM);
	afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnSize(UINT nType, int cx, int cy);
	void Refresh();
	void GetAutocompletionList();
	void ScanFile(const CString& sFilePath, const CString& sRegex, const CString& sExt);
	void DoSize(int delta);
	void SetSplitterRange();
	void SaveSplitterPos();
	void ParseRegexFile(const CString& sFile, std::map<CString, CString>& mapRegex);
	void UpdateCheckLinks();
	void VersionCheck();

	DECLARE_MESSAGE_MAP()


public:
	CTSVNPathList		m_pathList;
	CTSVNPathList		m_updatedPathList;
	CTSVNPathList		m_selectedPathList;
	CTSVNPathList		m_checkedPathList;
	BOOL				m_bRecursive;
	CSciEdit			m_cLogMessage;
	CString				m_sLogMessage;
	std::map<CString, CString> m_revProps;
	BOOL				m_bKeepLocks;
	CString				m_sBugID;
	CString				m_sChangeList;
	BOOL				m_bKeepChangeList;
	INT_PTR				m_itemsCount;
	bool				m_bSelectFilesForCommit;
	CComPtr<IBugTraqProvider> m_BugTraqProvider;

private:
	CWinThread*			m_pThread;
	std::set<CString>	m_autolist;
	CSVNStatusListCtrl	m_ListCtrl;
	BOOL				m_bShowUnversioned;
	BOOL				m_bShowExternals;
	volatile LONG		m_bBlock;
	volatile LONG		m_bThreadRunning;
	volatile LONG		m_bRunThread;
	CToolTips			m_tooltips;
	CRegDWORD			m_regAddBeforeCommit;
	CRegDWORD			m_regKeepChangelists;
	CRegDWORD			m_regShowExternals;
	ProjectProperties	m_ProjectProperties;
	CString				m_sWindowTitle;
	static UINT			WM_AUTOLISTREADY;
	int					m_nPopupPasteListCmd;
	CRegHistory			m_History;
	bool				m_bCancelled;
	CSplitterControl	m_wndSplitter;
	CRect				m_DlgOrigRect;
	CRect				m_LogMsgOrigRect;
	CPathWatcher		m_pathwatcher;
	CLinkControl		m_linkControl;
	CPathEdit			m_CommitTo;
	AeroControlBase		m_aeroControls;
	CBugTraqAssociation m_bugtraq_association;
	CHyperLink			m_cUpdateLink;

};
