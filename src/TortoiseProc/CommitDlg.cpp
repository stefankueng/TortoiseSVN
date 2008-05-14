// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "CommitDlg.h"
#include "DirFileEnum.h"
#include "SVNConfig.h"
#include "SVNProperties.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "SVN.h"
#include "Registry.h"
#include "SVNStatus.h"
#include "HistoryDlg.h"
#include "Hooks.h"
#include "..\IBugTraqProvider\IBugTraqProvider_h.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT CCommitDlg::WM_AUTOLISTREADY = RegisterWindowMessage(_T("TORTOISESVN_AUTOLISTREADY_MSG"));


IMPLEMENT_DYNAMIC(CCommitDlg, CResizableStandAloneDialog)
CCommitDlg::CCommitDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCommitDlg::IDD, pParent)
	, m_bRecursive(FALSE)
	, m_bShowUnversioned(FALSE)
	, m_bBlock(FALSE)
	, m_bThreadRunning(FALSE)
	, m_bRunThread(FALSE)
	, m_pThread(NULL)
	, m_bKeepLocks(FALSE)
	, m_bKeepChangeList(TRUE)
	, m_itemsCount(0)
	, m_bSelectFilesForCommit(TRUE)
{
}

CCommitDlg::~CCommitDlg()
{
	if(m_pThread != NULL)
	{
		delete m_pThread;
	}
}

void CCommitDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_ListCtrl);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
	DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Check(pDX, IDC_KEEPLOCK, m_bKeepLocks);
	DDX_Control(pDX, IDC_SPLITTER, m_wndSplitter);
	DDX_Check(pDX, IDC_KEEPLISTS, m_bKeepChangeList);
}

BEGIN_MESSAGE_MAP(CCommitDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
	ON_BN_CLICKED(IDC_BUGTRAQBUTTON, OnBnClickedBugtraqbutton)
	ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED, OnSVNStatusListCtrlItemCountChanged)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_CHECKCHANGED, &CCommitDlg::OnSVNStatusListCtrlCheckChanged)
	ON_REGISTERED_MESSAGE(WM_AUTOLISTREADY, OnAutoListReady) 
	ON_WM_TIMER()
    ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CCommitDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;

	m_History.SetMaxHistoryItems((LONG)CRegDWORD(_T("Software\\TortoiseSVN\\MaxHistoryItems"), 25));

	m_regKeepChangelists = CRegDWORD(_T("Software\\TortoiseSVN\\KeepChangeLists"), FALSE);
	m_bKeepChangeList = m_regKeepChangelists;

	SVNConfig config;
	m_bKeepLocks = config.KeepLocks();

	UpdateData(FALSE);
	
	m_ListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLTEXTSTATUS | SVNSLC_COLPROPSTATUS | SVNSLC_COLLOCK, _T("CommitDlg"));
	m_ListCtrl.SetSelectButton(&m_SelectAll);
	m_ListCtrl.SetStatLabel(GetDlgItem(IDC_STATISTICS));
	m_ListCtrl.SetCancelBool(&m_bCancelled);
	m_ListCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMIT);
	m_ListCtrl.EnableFileDrop();
	m_ListCtrl.SetBackgroundImage(IDI_COMMIT_BKG);
	
	m_ProjectProperties.ReadPropsPathList(m_pathList);
	m_cLogMessage.Init(m_ProjectProperties);
	m_cLogMessage.SetFont((CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8));
	m_cLogMessage.RegisterContextMenuHandler(this);

	OnEnChangeLogmessage();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTERNALWARNING, IDS_COMMITDLG_EXTERNALS);
	m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);
	
	m_SelectAll.SetCheck(BST_INDETERMINATE);
	
	CBugTraqAssociations bugtraq_associations;
	bugtraq_associations.Load();

	if (bugtraq_associations.FindProvider(m_pathList, &m_bugtraq_association))
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);

		CComPtr<IBugTraqProvider> pProvider;
		HRESULT hr = pProvider.CoCreateInstance(m_bugtraq_association.GetProviderClass());
		if (SUCCEEDED(hr))
		{
			BSTR temp = NULL;
			if (SUCCEEDED(hr = pProvider->GetLinkText(GetSafeHwnd(), m_bugtraq_association.GetParameters().AllocSysString(), &temp)))
			{
				SetDlgItemText(IDC_BUGTRAQBUTTON, temp);
				GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(TRUE);
				GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_SHOW);
			}

			SysFreeString(temp);
		}

		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}
	else if (!m_ProjectProperties.sMessage.IsEmpty())
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
		if (!m_ProjectProperties.sLabel.IsEmpty())
			SetDlgItemText(IDC_BUGIDLABEL, m_ProjectProperties.sLabel);
		GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUGID)->SetFocus();
		CString sBugID = m_ProjectProperties.GetBugIDFromLog(m_sLogMessage);
		if (!sBugID.IsEmpty())
		{
			SetDlgItemText(IDC_BUGID, sBugID);
		}
	}
	else
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);
		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}

	if (!m_sLogMessage.IsEmpty())
		m_cLogMessage.SetText(m_sLogMessage);
		
	GetWindowText(m_sWindowTitle);
	
	AdjustControlSize(IDC_SHOWUNVERSIONED);
	AdjustControlSize(IDC_SELECTALL);
	AdjustControlSize(IDC_KEEPLOCK);

	GetClientRect(m_DlgOrigRect);
	m_cLogMessage.GetClientRect(m_LogMsgOrigRect);

	AddAnchor(IDC_COMMITLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
	AddAnchor(IDC_BUGID, TOP_RIGHT);
	AddAnchor(IDC_BUGTRAQBUTTON, TOP_RIGHT);
	AddAnchor(IDC_COMMIT_TO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDC_EXTERNALWARNING, BOTTOM_RIGHT);
	AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_KEEPLOCK, BOTTOM_LEFT);
	AddAnchor(IDC_KEEPLISTS, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("CommitDlg"));
	DWORD yPos = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\CommitDlgSizer"));
	RECT rcDlg, rcLogMsg, rcFileList;
	GetClientRect(&rcDlg);
	m_cLogMessage.GetWindowRect(&rcLogMsg);
	ScreenToClient(&rcLogMsg);
	m_ListCtrl.GetWindowRect(&rcFileList);
	ScreenToClient(&rcFileList);
	if (yPos)
	{
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos - rectSplitter.top;
		if ((rcLogMsg.bottom + delta > rcLogMsg.top)&&(rcLogMsg.bottom + delta < rcFileList.bottom - 30))
		{
			m_wndSplitter.SetWindowPos(NULL, 0, yPos, 0, 0, SWP_NOSIZE);
			DoSize(delta);
		}
	}

	// add all directories to the watcher
	for (int i=0; i<m_pathList.GetCount(); ++i)
	{
		if (m_pathList[i].IsDirectory())
			m_pathwatcher.AddPath(m_pathList[i]);
	}

	m_updatedPathList = m_pathList;

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	InterlockedExchange(&m_bBlock, TRUE);
	m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
	CRegDWORD err = CRegDWORD(_T("Software\\TortoiseSVN\\ErrorOccurred"), FALSE);
	CRegDWORD historyhint = CRegDWORD(_T("Software\\TortoiseSVN\\HistoryHintShown"), FALSE);
	if ((((DWORD)err)!=FALSE)&&((((DWORD)historyhint)==FALSE)))
	{
		historyhint = TRUE;
		ShowBalloon(IDC_HISTORY, IDS_COMMITDLG_HISTORYHINT_TT, IDI_INFORMATION);
	}
	err = FALSE;


	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCommitDlg::OnOK()
{
	if (m_bBlock)
		return;
	if (m_bThreadRunning)
	{
		m_bCancelled = true;
		InterlockedExchange(&m_bRunThread, FALSE);
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	}
	CString id;
	GetDlgItem(IDC_BUGID)->GetWindowText(id);
	if (!m_ProjectProperties.CheckBugID(id))
	{
		ShowBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, IDI_EXCLAMATION);
		return;
	}
	m_sLogMessage = m_cLogMessage.GetText();
	if ((m_ProjectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_ProjectProperties.HasBugID(m_sLogMessage)))
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_COMMITDLG_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
			return;
	}

	CRegDWORD regUnversionedRecurse (_T("Software\\TortoiseSVN\\UnversionedRecurse"), TRUE);
	if (!(DWORD)regUnversionedRecurse)
	{
		// Find unversioned directories which are marked for commit. The user might expect them
		// to be added recursively since he cannot the the files. Let's ask the user if he knows
		// what he is doing.
		int nListItems = m_ListCtrl.GetItemCount();
		for (int j=0; j<nListItems; j++)
		{
			const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(j);
			if (entry->IsChecked() && (entry->status == svn_wc_status_unversioned) && entry->IsFolder() )
			{
				if (CMessageBox::Show(this->m_hWnd, IDS_COMMITDLG_UNVERSIONEDFOLDERWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
					return;
			}
		}
	}

	m_pathwatcher.Stop();
	InterlockedExchange(&m_bBlock, TRUE);
	CDWordArray arDeleted;
	//first add all the unversioned files the user selected
	//and check if all versioned files are selected
	int nUnchecked = 0;
	m_bRecursive = true;
	int nListItems = m_ListCtrl.GetItemCount();

	CTSVNPathList itemsToAdd;
	CTSVNPathList itemsToRemove;
	bool bCheckedInExternal = false;
	bool bHasConflicted = false;
	std::set<CString> checkedLists;
	std::set<CString> uncheckedLists;
	for (int j=0; j<nListItems; j++)
	{
		const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(j);
		if (entry->IsChecked())
		{
			if (entry->status == svn_wc_status_unversioned)
			{
				itemsToAdd.AddPath(entry->GetPath());
			}
			if (entry->status == svn_wc_status_conflicted)
			{
				bHasConflicted = true;
			}
			if (entry->status == svn_wc_status_missing)
			{
				itemsToRemove.AddPath(entry->GetPath());
			}
			if (entry->status == svn_wc_status_deleted)
			{
				arDeleted.Add(j);
			}
			if (entry->IsInExternal())
			{
				bCheckedInExternal = true;
			}
			checkedLists.insert(entry->GetChangeList());
		}
		else
		{
			if ((entry->status != svn_wc_status_unversioned)	&&
				(entry->status != svn_wc_status_ignored))
			{
				nUnchecked++;
				uncheckedLists.insert(entry->GetChangeList());
				if (m_bRecursive)
				{
					// This algorithm is for the sake of simplicity of the complexity O(N�)
					for (int k=0; k<nListItems; k++)
					{
						const CSVNStatusListCtrl::FileEntry * entryK = m_ListCtrl.GetListEntry(k);
						if (entryK->IsChecked() && entryK->GetPath().IsAncestorOf(entry->GetPath())  )
						{
							// Fall back to a non-recursive commit to prevent items being
							// committed which aren't checked although its parent is checked
							// (property change, directory deletion, ... )
							m_bRecursive = false;
							break;
						}
					}
				}
			}
		}
	}
	if (m_pathwatcher.GetNumberOfChangedPaths() && m_bRecursive)
	{
		// There are paths which got changed (touched at least).
		// We have to find out if this affects the selection in the commit dialog
		// If it could affect the selection, revert back to a non-recursive commit
		CTSVNPathList changedList = m_pathwatcher.GetChangedPaths();
		changedList.RemoveDuplicates();
		for (int i=0; i<changedList.GetCount(); ++i)
		{
			if (changedList[i].IsAdminDir())
			{
				// something inside an admin dir was changed.
				// if it's the entries file, then we have to fully refresh because
				// files may have been added/removed from version control
				if ((changedList[i].GetWinPathString().Right(7).CompareNoCase(_T("entries")) == 0) &&
					(changedList[i].GetWinPathString().Find(_T("\\tmp\\"))<0))
				{
					m_bRecursive = false;
					break;
				}
			}
			else if (!m_ListCtrl.IsPathShown(changedList[i]))
			{
				// a path which is not shown in the list has changed
				CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(changedList[i]);
				if (entry)
				{
					// check if the changed path would get committed by a recursive commit
					if ((!entry->IsFromDifferentRepository()) &&
						(!entry->IsInExternal()) &&
						(!entry->IsNested()) && 
						(!entry->IsChecked()))
					{
						m_bRecursive = false;
						break;
					}
				}
			}
		}
	}


	// Now, do all the adds - make sure that the list is sorted so that parents 
	// are added before their children
	itemsToAdd.SortByPathname();
	SVN svn;
	if (!svn.Add(itemsToAdd, &m_ProjectProperties, svn_depth_empty, FALSE, FALSE, TRUE))
	{
		CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
		Refresh();
		return;
	}

	// Remove any missing items
	// Not sure that this sort is really necessary - indeed, it might be better to do a reverse sort at this point
	itemsToRemove.SortByPathname();
	svn.Remove(itemsToRemove, TRUE);

	//the next step: find all deleted files and check if they're 
	//inside a deleted folder. If that's the case, then remove those
	//files from the list since they'll get deleted by the parent
	//folder automatically.
	m_ListCtrl.Block(TRUE, FALSE);
	INT_PTR nDeleted = arDeleted.GetCount();
	for (INT_PTR i=0; i<arDeleted.GetCount(); i++)
	{
		if (m_ListCtrl.GetCheck(arDeleted.GetAt(i)))
		{
			const CTSVNPath& path = m_ListCtrl.GetListEntry(arDeleted.GetAt(i))->GetPath();
			if (path.IsDirectory())
			{
				//now find all children of this directory
				for (int j=0; j<arDeleted.GetCount(); j++)
				{
					if (i!=j)
					{
						CSVNStatusListCtrl::FileEntry* childEntry = m_ListCtrl.GetListEntry(arDeleted.GetAt(j));
						if (childEntry->IsChecked())
						{
							if (path.IsAncestorOf(childEntry->GetPath()))
							{
								m_ListCtrl.SetEntryCheck(childEntry, arDeleted.GetAt(j), false);
								nDeleted--;
							}
						}
					}
				}
			}
		}
	} 
	m_ListCtrl.Block(FALSE, FALSE);

	if ((nUnchecked != 0)||(bCheckedInExternal)||(bHasConflicted)||(!m_bRecursive))
	{
		//save only the files the user has checked into the temporary file
		m_ListCtrl.WriteCheckedNamesToPathList(m_pathList);
	}
	m_ListCtrl.WriteCheckedNamesToPathList(m_selectedPathList);
	// the item count is used in the progress dialog to show the overall commit
	// progress.
	// deleted items only send one notification event, all others send two
	m_itemsCount = ((m_selectedPathList.GetCount() - nDeleted - itemsToRemove.GetCount()) * 2) + nDeleted + itemsToRemove.GetCount();

	if ((m_bRecursive)&&(checkedLists.size() == 1))
	{
		// all checked items belong to the same changelist
		// find out if there are any unchecked items which belong to that changelist
		if (uncheckedLists.find(*checkedLists.begin()) == uncheckedLists.end())
			m_sChangeList = *checkedLists.begin();
	}
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	if (!GetDlgItem(IDC_KEEPLOCK)->IsWindowEnabled())
		m_bKeepLocks = FALSE;
	m_regKeepChangelists = m_bKeepChangeList;
	if (!GetDlgItem(IDC_KEEPLISTS)->IsWindowEnabled())
		m_bKeepChangeList = FALSE;
	InterlockedExchange(&m_bBlock, FALSE);
	m_sBugID.Trim();
	if (!m_sBugID.IsEmpty())
	{
		m_sBugID.Replace(_T(", "), _T(","));
		m_sBugID.Replace(_T(" ,"), _T(","));
		CString sBugID = m_ProjectProperties.sMessage;
		sBugID.Replace(_T("%BUGID%"), m_sBugID);
		if (m_ProjectProperties.bAppend)
			m_sLogMessage += _T("\n") + sBugID + _T("\n");
		else
			m_sLogMessage = sBugID + _T("\n") + m_sLogMessage;
	}
	m_History.AddEntry(m_sLogMessage);
	m_History.Save();

	SaveSplitterPos();

	CResizableStandAloneDialog::OnOK();
}

void CCommitDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\CommitDlgSizer"));
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos = rectSplitter.top;
	}
}

UINT CCommitDlg::StatusThreadEntry(LPVOID pVoid)
{
	return ((CCommitDlg*)pVoid)->StatusThread();
}

UINT CCommitDlg::StatusThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a list control. 
	InterlockedExchange(&m_bBlock, TRUE);
	InterlockedExchange(&m_bThreadRunning, TRUE);// so the main thread knows that this thread is still running
	InterlockedExchange(&m_bRunThread, TRUE);	// if this is set to FALSE, the thread should stop
	m_bCancelled = false;

	DialogEnableWindow(IDOK, false);
	DialogEnableWindow(IDC_SHOWUNVERSIONED, false);
	GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_HIDE);
	DialogEnableWindow(IDC_EXTERNALWARNING, false);

    // read the list of recent log entries before querying the WC for status
    // -> the user may select one and modify / update it while we are crawling the WC
	if (m_History.GetCount()==0)
	{
		CString reg;
		if (m_ListCtrl.m_sUUID.IsEmpty())
		{
			SVN svn;
			reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), (LPCTSTR)svn.GetUUIDFromPath(m_pathList[0]));
		}
		else
			reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), (LPCTSTR)m_ListCtrl.m_sUUID);
		m_History.Load(reg, _T("logmsgs"));
	}

    // Initialise the list control with the status of the files/folders below us
	BOOL success = m_ListCtrl.GetStatus(m_pathList);
	m_ListCtrl.CheckIfChangelistsArePresent(false);

	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWLOCKS | SVNSLC_SHOWINCHANGELIST;
	dwShow |= DWORD(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
	if (success)
	{
		if (m_checkedPathList.GetCount())
			m_ListCtrl.Show(dwShow, m_checkedPathList);
		else
		{
			DWORD dwCheck = m_bSelectFilesForCommit ? SVNSLC_SHOWDIRECTS|SVNSLC_SHOWMODIFIED|SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|SVNSLC_SHOWLOCKS : 0;
			m_ListCtrl.Show(dwShow, dwCheck);
			m_bSelectFilesForCommit = true;
		}

		if (m_ListCtrl.HasExternalsFromDifferentRepos())
		{
			GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_SHOW);
			DialogEnableWindow(IDC_EXTERNALWARNING, TRUE);
		}
		SetDlgItemText(IDC_COMMIT_TO, m_ListCtrl.m_sURL);
		m_tooltips.AddTool(GetDlgItem(IDC_STATISTICS), m_ListCtrl.GetStatisticsString());
	}
	CString logmsg;
	GetDlgItem(IDC_LOGMESSAGE)->GetWindowText(logmsg);
	DialogEnableWindow(IDOK, logmsg.GetLength() >= m_ProjectProperties.nMinLogSize);
	if (!success)
	{
		m_ListCtrl.SetEmptyString(m_ListCtrl.GetLastErrorMessage());
		InterlockedExchange(&m_bBlock, FALSE);
		InterlockedExchange(&m_bThreadRunning, FALSE);
		SetTimer(ENDDIALOGTIMER, 100, NULL);
		return (DWORD)-1;
	}
	if ((m_ListCtrl.GetItemCount()==0)&&(m_ListCtrl.HasUnversionedItems()))
	{
		if (CMessageBox::Show(m_hWnd, IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
		{
			m_bShowUnversioned = TRUE;
			GetDlgItem(IDC_SHOWUNVERSIONED)->SendMessage(BM_SETCHECK, BST_CHECKED);
			DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWLOCKS;
			m_ListCtrl.Show(dwShow);
		}
	}

	CTSVNPath commonDir = m_ListCtrl.GetCommonDirectory(false);
	SetWindowText(m_sWindowTitle + _T(" - ") + commonDir.GetWinPathString());

	m_autolist.clear();
	// we don't have to block the commit dialog while we fetch the
	// auto completion list.
	m_pathwatcher.ClearChangedPaths();
	InterlockedExchange(&m_bBlock, FALSE);
	if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\Autocompletion"), TRUE)==TRUE)
	{
		m_ListCtrl.Block(TRUE, TRUE);
		GetAutocompletionList();
		m_ListCtrl.Block(FALSE, FALSE);
	}
	if (m_bRunThread)
	{
		DialogEnableWindow(IDC_SHOWUNVERSIONED, true);
		if (m_ListCtrl.HasChangeLists())
			DialogEnableWindow(IDC_KEEPLISTS, true);
		if (m_ListCtrl.HasLocks())
			DialogEnableWindow(IDC_KEEPLOCK, true);
		// we have the list, now signal the main thread about it
		SendMessage(WM_AUTOLISTREADY);	// only send the message if the thread wasn't told to quit!
	}
	InterlockedExchange(&m_bRunThread, FALSE);
	InterlockedExchange(&m_bThreadRunning, FALSE);
	// force the cursor to normal
	RefreshCursor();
	return 0;
}

void CCommitDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bBlock)
		return;
	m_pathwatcher.Stop();
	if (m_bThreadRunning)
	{
		InterlockedExchange(&m_bRunThread, FALSE);
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	}
	UpdateData();
	m_sBugID.Trim();
	m_sLogMessage = m_cLogMessage.GetText();
	if (!m_sBugID.IsEmpty())
	{
		m_sBugID.Replace(_T(", "), _T(","));
		m_sBugID.Replace(_T(" ,"), _T(","));
		CString sBugID = m_ProjectProperties.sMessage;
		sBugID.Replace(_T("%BUGID%"), m_sBugID);
		if (m_ProjectProperties.bAppend)
			m_sLogMessage += _T("\n") + sBugID + _T("\n");
		else
			m_sLogMessage = sBugID + _T("\n") + m_sLogMessage;
	}
	m_History.AddEntry(m_sLogMessage);
	m_History.Save();
	SaveSplitterPos();
	CResizableStandAloneDialog::OnCancel();
}

void CCommitDlg::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == BST_INDETERMINATE)
	{
		// It is not at all useful to manually place the checkbox into the indeterminate state...
		// We will force this on to the unchecked state
		state = BST_UNCHECKED;
		m_SelectAll.SetCheck(state);
	}
	m_ListCtrl.SelectAll(state == BST_CHECKED);
}

BOOL CCommitDlg::PreTranslateMessage(MSG* pMsg)
{
	if (!m_bBlock)
		m_tooltips.RelayEvent(pMsg);
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (m_bBlock)
					return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
				Refresh();
			}
			break;
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
					return TRUE;
				}
				if ( GetFocus()==GetDlgItem(IDC_BUGID) )
				{
					// Pressing RETURN in the bug id control
					// moves the focus to the message
					GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
					return TRUE;
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCommitDlg::Refresh()
{
	if (m_bThreadRunning)
		return;

	InterlockedExchange(&m_bBlock, TRUE);
	m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
}

void CCommitDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CCommitDlg::OnBnClickedShowunversioned()
{
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	if (!m_bBlock)
	{
		DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWLOCKS;
		dwShow |= DWORD(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
		m_ListCtrl.Show(dwShow);
	}
}

void CCommitDlg::OnEnChangeLogmessage()
{
	UpdateOKButton();
}

LRESULT CCommitDlg::OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
	if ((m_ListCtrl.GetItemCount() == 0)&&(m_ListCtrl.HasUnversionedItems())&&(!m_bShowUnversioned))
	{
		if (CMessageBox::Show(*this, IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
		{
			m_bShowUnversioned = TRUE;
			DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWLOCKS;
			m_ListCtrl.Show(dwShow);
			UpdateData(FALSE);
		}
	}
	return 0;
}

LRESULT CCommitDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	Refresh();
	return 0;
}

LRESULT CCommitDlg::OnFileDropped(WPARAM, LPARAM lParam)
{
	BringWindowToTop();
	SetForegroundWindow();
	SetActiveWindow();
	// if multiple files/folders are dropped
	// this handler is called for every single item
	// separately.
	// To avoid creating multiple refresh threads and
	// causing crashes, we only add the items to the
	// list control and start a timer.
	// When the timer expires, we start the refresh thread,
	// but only if it isn't already running - otherwise we
	// restart the timer.
	CTSVNPath path;
	path.SetFromWin((LPCTSTR)lParam);

	// just add all the items we get here.
	// if the item is versioned, the add will fail but nothing
	// more will happen.
	SVN svn;
	svn.Add(CTSVNPathList(path), &m_ProjectProperties, svn_depth_empty, false, true, true);

	if (!m_ListCtrl.HasPath(path))
	{
		if (m_pathList.AreAllPathsFiles())
		{
			m_pathList.AddPath(path);
			m_pathList.RemoveDuplicates();
			m_updatedPathList.AddPath(path);
			m_updatedPathList.RemoveDuplicates();
		}
		else
		{
			// if the path list contains folders, we have to check whether
			// our just (maybe) added path is a child of one of those. If it is
			// a child of a folder already in the list, we must not add it. Otherwise
			// that path could show up twice in the list.
			bool bHasParentInList = false;
			for (int i=0; i<m_pathList.GetCount(); ++i)
			{
				if (m_pathList[i].IsAncestorOf(path))
				{
					bHasParentInList = true;
					break;
				}
			}
			if (!bHasParentInList)
			{
				m_pathList.AddPath(path);
				m_pathList.RemoveDuplicates();
				m_updatedPathList.AddPath(path);
				m_updatedPathList.RemoveDuplicates();
			}
		}
	}
	
	// Always start the timer, since the status of an existing item might have changed
	SetTimer(REFRESHTIMER, 200, NULL);
	ATLTRACE(_T("Item %s dropped, timer started\n"), path.GetWinPath());
	return 0;
}

LRESULT CCommitDlg::OnAutoListReady(WPARAM, LPARAM)
{
	m_cLogMessage.SetAutoCompletionList(m_autolist, '*');
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// functions which run in the status thread
//////////////////////////////////////////////////////////////////////////

void CCommitDlg::ParseRegexFile(const CString& sFile, std::map<CString, CString>& mapRegex)
{
	CString strLine;
	try
	{
		CStdioFile file(sFile, CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
		while (m_bRunThread && file.ReadString(strLine))
		{
			int eqpos = strLine.Find('=');
			CString rgx;
			rgx = strLine.Mid(eqpos+1).Trim();

			int pos = -1;
			while (((pos = strLine.Find(','))>=0)&&(pos < eqpos))
			{
				mapRegex[strLine.Left(pos)] = rgx;
				strLine = strLine.Mid(pos+1).Trim();
			}
			mapRegex[strLine.Left(strLine.Find('=')).Trim()] = rgx;
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException loading auto list regex file\n");
		pE->Delete();
		return;
	}
}
void CCommitDlg::GetAutocompletionList()
{
	// the auto completion list is made of strings from each selected files.
	// the strings used are extracted from the files with regexes found
	// in the file "autolist.txt".
	// the format of that file is:
	// file extensions separated with commas '=' regular expression to use
	// example:
	// .h, .hpp = (?<=class[\s])\b\w+\b|(\b\w+(?=[\s ]?\(\);))
	// .cpp = (?<=[^\s]::)\b\w+\b
	
	std::map<CString, CString> mapRegex;
	CString sRegexFile = CPathUtils::GetAppDirectory();
	CRegDWORD regtimeout = CRegDWORD(_T("Software\\TortoiseSVN\\AutocompleteParseTimeout"), 5);
	DWORD timeoutvalue = regtimeout*1000;
	sRegexFile += _T("autolist.txt");
	if (!m_bRunThread)
		return;
	ParseRegexFile(sRegexFile, mapRegex);
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, sRegexFile.GetBuffer(MAX_PATH+1));
	sRegexFile.ReleaseBuffer();
	sRegexFile += _T("\\TortoiseSVN\\autolist.txt");
	if (PathFileExists(sRegexFile))
	{
		ParseRegexFile(sRegexFile, mapRegex);
	}
	DWORD starttime = GetTickCount();

	// now we have two arrays of strings, where the first array contains all
	// file extensions we can use and the second the corresponding regex strings
	// to apply to those files.

	// the next step is to go over all files shown in the commit dialog
	// and scan them for strings we can use
	int nListItems = m_ListCtrl.GetItemCount();

	for (int i=0; i<nListItems && m_bRunThread; ++i)
	{
		// stop parsing after timeout
		if ((!m_bRunThread) || (GetTickCount() - starttime > timeoutvalue))
			return;
		const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(i);
		if (!entry)
			continue;
		
		// add the path parts to the auto completion list too
		CString sPartPath = entry->GetRelativeSVNPath();
		m_autolist.insert(sPartPath);

		int pos = 0;
		int lastPos = 0;
		while ((pos = sPartPath.Find('/', pos)) >= 0)
		{
			pos++;
			lastPos = pos;
			m_autolist.insert(sPartPath.Mid(pos));
		}

		// Last inserted entry is a file name.
		// Some users prefer to also list file name without extension.
		if (CRegDWORD(_T("Software\\TortoiseSVN\\AutocompleteRemovesExtensions"), FALSE))
		{
			int dotPos = sPartPath.ReverseFind('.');
			if ((dotPos >= 0) && (dotPos > lastPos))
				m_autolist.insert(sPartPath.Mid(lastPos, dotPos - lastPos));
		}

		if ((entry->status <= svn_wc_status_normal)||(entry->status == svn_wc_status_ignored))
			continue;

		CString sExt = entry->GetPath().GetFileExtension();
		sExt.MakeLower();
		// find the regex string which corresponds to the file extension
		CString rdata = mapRegex[sExt];
		if (rdata.IsEmpty())
			continue;

		ScanFile(entry->GetPath().GetWinPathString(), rdata);
		if ((entry->textstatus != svn_wc_status_unversioned) &&
			(entry->textstatus != svn_wc_status_none) &&
			(entry->textstatus != svn_wc_status_ignored) &&
			(entry->textstatus != svn_wc_status_added) &&
			(entry->textstatus != svn_wc_status_normal))
		{
			CTSVNPath basePath = SVN::GetPristinePath(entry->GetPath());
			if (!basePath.IsEmpty())
				ScanFile(basePath.GetWinPathString(), rdata);
		}
	}
	ATLTRACE(_T("Auto completion list loaded in %d msec\n"), GetTickCount() - starttime);
}

void CCommitDlg::ScanFile(const CString& sFilePath, const CString& sRegex)
{
	wstring sFileContent;
	HANDLE hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(hFile, NULL);
		if (size > 1000000L)
		{
			// no files bigger than 1 Meg
			CloseHandle(hFile);
			return;
		}
		// allocate memory to hold file contents
		char * buffer = new char[size];
		DWORD readbytes;
		ReadFile(hFile, buffer, size, &readbytes, NULL);
		CloseHandle(hFile);
		int opts = 0;
		IsTextUnicode(buffer, readbytes, &opts);
		if (opts & IS_TEXT_UNICODE_NULL_BYTES)
		{
			delete buffer;
			return;
		}
		if (opts & IS_TEXT_UNICODE_UNICODE_MASK)
		{
			sFileContent = wstring((wchar_t*)buffer, readbytes/sizeof(WCHAR));
		}
		if ((opts & IS_TEXT_UNICODE_NOT_UNICODE_MASK)||(opts == 0))
		{
			int ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)buffer, readbytes, NULL, 0);
			wchar_t * pWideBuf = new wchar_t[ret];
			int ret2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)buffer, readbytes, pWideBuf, ret);
			if (ret2 == ret)
				sFileContent = wstring(pWideBuf, ret);
			delete [] pWideBuf;
		}
		delete [] buffer;
	}
	if (sFileContent.empty()|| !m_bRunThread)
	{
		return;
	}

	try
	{
		const tr1::wregex regCheck(sRegex, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
		const tr1::wsregex_iterator end;
		wstring s = sFileContent;
		for (tr1::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
		{
			const tr1::wsmatch match = *it;
			for (size_t i=1; i<match.size(); ++i)
			{
				if (match[i].second-match[i].first)
				{
					ATLTRACE(_T("matched keyword : %s\n"), wstring(match[i]).c_str());
					m_autolist.insert(wstring(match[i]).c_str());
				}
			}
		}
	}
	catch (exception) {}
}

// CSciEditContextMenuInterface
void CCommitDlg::InsertMenuItems(CMenu& mPopup, int& nCmd)
{
	CString sMenuItemText(MAKEINTRESOURCE(IDS_COMMITDLG_POPUP_PASTEFILELIST));
	m_nPopupPasteListCmd = nCmd++;
	mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPasteListCmd, sMenuItemText);
}

bool CCommitDlg::HandleMenuItemClick(int cmd, CSciEdit * pSciEdit)
{
	if (m_bBlock)
		return false;
	if (cmd == m_nPopupPasteListCmd)
	{
		CString logmsg;
		TCHAR buf[MAX_STATUS_STRING_LENGTH];
		int nListItems = m_ListCtrl.GetItemCount();
		for (int i=0; i<nListItems; ++i)
		{
			CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(i);
			if (entry->IsChecked())
			{
				CString line;
				svn_wc_status_kind status = entry->status;
				if (status == svn_wc_status_unversioned)
					status = svn_wc_status_added;
				if (status == svn_wc_status_missing)
					status = svn_wc_status_deleted;
				WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
				if (m_ProjectProperties.bFileListInEnglish)
					langID = 1033;
				SVNStatus::GetStatusString(AfxGetResourceHandle(), status, buf, sizeof(buf)/sizeof(TCHAR), langID);
				line.Format(_T("%-10s %s\r\n"), buf, (LPCTSTR)m_ListCtrl.GetItemText(i,0));
				logmsg += line;
			}
		}
		pSciEdit->InsertText(logmsg);
		return true;
	}
	return false;
}

void CCommitDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case ENDDIALOGTIMER:
		KillTimer(ENDDIALOGTIMER);
		EndDialog(0);
		break;
	case REFRESHTIMER:
		if (m_bThreadRunning)
		{
			SetTimer(REFRESHTIMER, 200, NULL);
			ATLTRACE("Wait some more before refreshing\n");
		}
		else
		{
			KillTimer(REFRESHTIMER);
			ATLTRACE("Refreshing after items dropped\n");
			Refresh();
		}
		break;
	}
	__super::OnTimer(nIDEvent);
}

void CCommitDlg::OnBnClickedHistory()
{
	if (m_pathList.GetCount() == 0)
		return;
	CHistoryDlg historyDlg;
	historyDlg.SetHistory(m_History);
	if (historyDlg.DoModal() != IDOK)
		return;

	CString sMsg = historyDlg.GetSelectedText();
	if (sMsg != m_cLogMessage.GetText().Left(sMsg.GetLength()))
	{
		CString sBugID = m_ProjectProperties.GetBugIDFromLog(sMsg);
		if (!sBugID.IsEmpty())
		{
			SetDlgItemText(IDC_BUGID, sBugID);
		}
		if (m_ProjectProperties.sLogTemplate.Compare(m_cLogMessage.GetText())!=0)
			m_cLogMessage.InsertText(sMsg, !m_cLogMessage.GetText().IsEmpty());
		else
			m_cLogMessage.SetText(sMsg);
	}

	UpdateOKButton();
	GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
}

void CCommitDlg::OnBnClickedBugtraqbutton()
{
	CString sMsg = m_cLogMessage.GetText();

	CComPtr<IBugTraqProvider> pProvider;
	HRESULT hr = pProvider.CoCreateInstance(m_bugtraq_association.GetProviderClass());
	if (FAILED(hr))
	{
		CString sErr;
		sErr.Format(IDS_ERR_FAILEDISSUETRACKERCOM, m_bugtraq_association.GetProviderName(), _com_error(hr).ErrorMessage());
		CMessageBox::Show(m_hWnd, sErr, _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}

	BSTR parameters = m_bugtraq_association.GetParameters().AllocSysString();
	BSTR commonRoot = SysAllocString(m_pathList.GetCommonRoot().GetDirectory().GetWinPath());
	SAFEARRAY *pathList = SafeArrayCreateVector(VT_BSTR, 0, m_pathList.GetCount());

	for (LONG index = 0; index < m_pathList.GetCount(); ++index)
		SafeArrayPutElement(pathList, &index, m_pathList[index].GetSVNPathString().AllocSysString());

	BSTR originalMessage = sMsg.AllocSysString();
	BSTR temp = NULL;
	if (FAILED(hr = pProvider->GetCommitMessage(GetSafeHwnd(), parameters, commonRoot, pathList, originalMessage, &temp)))
	{
		CString sErr;
		sErr.Format(IDS_ERR_FAILEDISSUETRACKERCOM, m_bugtraq_association.GetProviderName(), _com_error(hr).ErrorMessage());
		CMessageBox::Show(m_hWnd, sErr, _T("TortoiseSVN"), MB_ICONERROR);
	}
	else
		m_cLogMessage.SetText(temp);

	m_cLogMessage.SetFocus();

	SysFreeString(temp);
}

LRESULT CCommitDlg::OnSVNStatusListCtrlCheckChanged(WPARAM, LPARAM)
{
	UpdateOKButton();
	return 0;
}

void CCommitDlg::UpdateOKButton()
{
	BOOL bValidLogSize = FALSE;

    if (m_cLogMessage.GetText().GetLength() >= m_ProjectProperties.nMinLogSize)
		bValidLogSize = !m_bBlock;

	LONG nSelectedItems = m_ListCtrl.GetSelected();
	DialogEnableWindow(IDOK, bValidLogSize && nSelectedItems>0);
}


LRESULT CCommitDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_SPLITTER)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSize(pHdr->delta);
		}
		break;
	}

	return __super::DefWindowProc(message, wParam, lParam);
}

void CCommitDlg::SetSplitterRange()
{
	if ((m_ListCtrl)&&(m_cLogMessage))
	{
		CRect rcTop;
		m_cLogMessage.GetWindowRect(rcTop);
		ScreenToClient(rcTop);
		CRect rcMiddle;
		m_ListCtrl.GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);
		if (rcMiddle.Height() && rcMiddle.Width())
			m_wndSplitter.SetRange(rcTop.top+60, rcMiddle.bottom-80);
	}
}

void CCommitDlg::DoSize(int delta)
{
	RemoveAnchor(IDC_MESSAGEGROUP);
	RemoveAnchor(IDC_LOGMESSAGE);
	RemoveAnchor(IDC_SPLITTER);
	RemoveAnchor(IDC_LISTGROUP);
	RemoveAnchor(IDC_FILELIST);
	CSplitterControl::ChangeHeight(&m_cLogMessage, delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MESSAGEGROUP), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(&m_ListCtrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_LISTGROUP), -delta, CW_BOTTOMALIGN);
	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	ArrangeLayout();
	// adjust the minimum size of the dialog to prevent the resizing from
	// moving the list control too far down.
	CRect rcLogMsg;
	m_cLogMessage.GetClientRect(rcLogMsg);
	SetMinTrackSize(CSize(m_DlgOrigRect.Width(), m_DlgOrigRect.Height()-m_LogMsgOrigRect.Height()+rcLogMsg.Height()));

	SetSplitterRange();
	m_cLogMessage.Invalidate();
	GetDlgItem(IDC_LOGMESSAGE)->Invalidate();
}

void CCommitDlg::OnSize(UINT nType, int cx, int cy)
{
    // first, let the resizing take place
    __super::OnSize(nType, cx, cy);

    //set range
    SetSplitterRange();
}
