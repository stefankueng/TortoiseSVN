// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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

#include "resource.h"
#include "svn.h"
#include "ProjectProperties.h"
#include "StandAloneDlg.h"
#include "TSVNPath.h"
#include "registry.h"
#include "SplitterControl.h"
#include "Colors.h"
#include "MenuButton.h"
#include "LogDlgHelper.h"
#include "FilterEdit.h"
#include "SVNRev.h"
#include "SciEdit.h"


#define MERGE_REVSELECTSTART	 1
#define MERGE_REVSELECTEND       2
#define MERGE_REVSELECTSTARTEND  3		///< both
#define MERGE_REVSELECTMINUSONE  4		///< first with N-1

#define LOGFILTER_ALL      1
#define LOGFILTER_MESSAGES 2
#define LOGFILTER_PATHS    3
#define LOGFILTER_AUTHORS  4
#define LOGFILTER_REVS	   5
#define LOGFILTER_REGEX	   6


#define LOGFILTER_TIMER		101

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

/**
 * \ingroup TortoiseProc
 * Shows log messages of a single file or folder in a listbox. 
 */
class CLogDlg : public CResizableStandAloneDialog, public SVN, IFilterEditValidator
{
	DECLARE_DYNAMIC(CLogDlg)
	
	friend class CStoreSelection;

public:
	CLogDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLogDlg();


	void SetParams(const CTSVNPath& path, SVNRev pegrev, SVNRev startrev, SVNRev endrev, int limit, 
		BOOL bStrict = CRegDWORD(_T("Software\\TortoiseSVN\\LastLogStrict"), FALSE), BOOL bSaveStrict = TRUE);
	void SetIncludeMerge(bool bInclude = true) {m_bIncludeMerges = bInclude;}
	void SetProjectPropertiesPath(const CTSVNPath& path) {m_ProjectProperties.ReadProps(path);}
	bool IsThreadRunning() {return !!m_bThreadRunning;}
	void SetDialogTitle(const CString& sTitle) {m_sTitle = sTitle;}
	void SetSelect(bool bSelect) {m_bSelect = bSelect;}
	void ContinuousSelection(bool bCont = true) {m_bSelectionMustBeContinuous = bCont;}
	void SetMergePath(const CTSVNPath& mergepath) {m_mergePath = mergepath;}

	const SVNRevList&	GetSelectedRevList() {return m_selectedRevs;}

// Dialog Data
	enum { IDD = IDD_LOGMESSAGE };

protected:
	//implement the virtual methods from SVN base class
	virtual BOOL Log(svn_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies, DWORD actions);
	virtual BOOL Log(svn_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies, DWORD actions, BOOL haschildren);
	virtual BOOL Cancel();
	virtual bool Validate(LPCTSTR string);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClickedInfoIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClickedCancelFilter(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnBnClickedGetall();
	afx_msg void OnNMDblclkChangedFileList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedStatbutton();
	afx_msg void OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawChangedFileList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetdispinfoChangedFileList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeSearchedit();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDtnDatetimechangeDateto(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDtnDatetimechangeDatefrom(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnColumnclickChangedFileList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedNexthundred();
	afx_msg void OnBnClickedHidepaths();
	afx_msg void OnBnClickedCheckStoponcopy();
	afx_msg void OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDtnDropdownDatefrom(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDtnDropdownDateto(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedIncludemerge();
	afx_msg void OnBnClickedRefresh();
	afx_msg void OnRefresh();
	afx_msg void OnFind();
	afx_msg void OnFocusFilter();
	afx_msg void OnEditCopy();

	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void	FillLogMessageCtrl(bool bShow = true);
	void	DoDiffFromLog(int selIndex, svn_revnum_t rev1, svn_revnum_t rev2, bool blame, bool unified);

	DECLARE_MESSAGE_MAP()

private:
	static UINT LogThreadEntry(LPVOID pVoid);
	UINT LogThread();
	void Refresh();
	BOOL IsDiffPossible(LogChangedPath * changedpath, svn_revnum_t rev);
	BOOL Open(bool bOpenWith, CString changedpath, svn_revnum_t rev);
	void EditAuthor(int index);
	void EditLogMessage(int index);
	void DoSizeV1(int delta);
	void DoSizeV2(int delta);
	void AdjustMinSize();
	void SetSplitterRange();
	void SetFilterCueText();
	BOOL IsEntryInDateRange(int i);
	void CopySelectionToClipBoard();
	void CopyChangedSelectionToClipBoard();
	CTSVNPathList GetChangedPathsFromSelectedRevisions(bool bRelativePaths = false, bool bUseFilter = true);
    void SortShownListArray();
	void RecalculateShownList(CPtrArray * pShownlist);
    void SetSortArrow(CListCtrl * control, int nColumn, bool bAscending);
	void SortByColumn(int nSortColumn, bool bAscending);
	bool IsSelectionContinuous();
	void EnableOKButton();
	void GetAll(bool bForceAll = false);
	void UpdateLogInfoLabel();
	void SaveSplitterPos();
	bool ValidateRegexp(LPCTSTR regexp_str, rpattern& pat, bool bMatchCase = false);
	void CheckRegexpTooltip();
	void GetChangedPaths(std::vector<CString>& changedpaths, std::vector<LogChangedPath*>& changedlogpaths);
	/**
	 * Extracts part of commit message suitable for displaying in revision list.
	 */
	CString MakeShortMessage(const CString& message);
	inline int ShownCountWithStopped() const { return m_arShownList.GetCount() + (m_bStrictStopped ? 1 : 0); }


	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	static int __cdecl	SortCompare(const void * pElem1, const void * pElem2);	///< sort callback function

	void ResizeAllListCtrlCols(CListCtrl &list);

	void ShowContextMenuForRevisions(CWnd* pWnd, CPoint point);
	void ShowContextMenuForChangedpaths(CWnd* pWnd, CPoint point);
public:
	CWnd *				m_pNotifyWindow;
	ProjectProperties	m_ProjectProperties;
	WORD				m_wParam;
private:
	CString				m_sRelativeRoot;
	CString				m_sRepositoryRoot;
	CListCtrl			m_LogList;
	CListCtrl			m_ChangedFileListCtrl;
	CFilterEdit			m_cFilter;
	CProgressCtrl		m_LogProgress;
	CMenuButton			m_btnShow;
	CTSVNPath			m_path;
	CTSVNPath			m_mergePath;
	SVNRev				m_pegrev;
	SVNRev				m_startrev;
	SVNRev				m_LogRevision;
	SVNRev				m_endrev;
	SVNRevList			m_selectedRevs;
	bool				m_bSelectionMustBeContinuous;
	long				m_logcounter;
	bool				m_bCancelled;
	volatile LONG 		m_bThreadRunning;
	BOOL				m_bStrict;
	bool				m_bStrictStopped;
	BOOL				m_bIncludeMerges;
	svn_revnum_t		m_lowestRev;
	BOOL				m_bSaveStrict;
	LogChangedPathArray * m_currentChangedArray;
	LogChangedPathArray m_CurrentFilteredChangedArray;
	CTSVNPathList		m_currentChangedPathList;
	CPtrArray			m_arShownList;
	bool				m_hasWC;
	int					m_nSearchIndex;
	bool				m_bFilterWithRegex;
	static const UINT	m_FindDialogMessage;
	CFindReplaceDialog *m_pFindDialog;
	CString				m_sMessageBuf;
	CSplitterControl	m_wndSplitter1;
	CSplitterControl	m_wndSplitter2;
	CRect				m_DlgOrigRect;
	CRect				m_MsgViewOrigRect;
	CRect				m_LogListOrigRect;
	CString				m_sFilterText;
	int					m_nSelectedFilter;
	volatile LONG		m_bNoDispUpdates;
	CDateTimeCtrl		m_DateFrom;
	CDateTimeCtrl		m_DateTo;
	DWORD				m_tFrom;
	DWORD				m_tTo;
	int					m_limit;
	int					m_limitcounter;
	int                 m_nSortColumn;
	bool                m_bAscending;
	static int			m_nSortColumnPathList;
	static bool			m_bAscendingPathList;
	CRegDWORD			m_regLastStrict;
	CButton				m_cHidePaths;
	bool				m_bShowedAll;
	CString				m_sTitle;
	bool				m_bSelect;
	bool				m_bShowBugtraqColumn;
	CString				m_sLogInfo;
	CSciEdit			m_cLogMessage;
	std::set<svn_revnum_t> m_mergedRevs;

	CBalloon			m_tooltips;

	CTime				m_timFrom;
	CTime				m_timTo;
	CColors				m_Colors;
	CImageList			m_imgList;
	HICON				m_hModifiedIcon;
	HICON				m_hReplacedIcon;
	HICON				m_hAddedIcon;
	HICON				m_hDeletedIcon;

	DWORD				m_childCounter;
	DWORD				m_maxChild;
	HACCEL				m_hAccel;

	CStoreSelection*	m_pStoreSelection;
    CLogDataVector		m_logEntries;
};
static UINT WM_REVSELECTED = RegisterWindowMessage(_T("TORTOISESVN_REVSELECTED_MSG"));
static UINT WM_REVLIST = RegisterWindowMessage(_T("TORTOISESVN_REVLIST_MSG"));
