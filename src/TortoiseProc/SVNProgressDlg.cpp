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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include "SVNProgressDlg.h"
#include "LogDialog\LogDlg.h"
#include "TSVNPath.h"
#include "Registry.h"
#include "SVNStatus.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "SoundUtils.h"
#include "SVNDiff.h"
#include "Hooks.h"
#include "SVNLogHelper.h"
#include "RegHistory.h"
#include "ConflictResolveDlg.h"
#include "LogFile.h"
#include "ShellUpdater.h"
#include "IconMenu.h"
#include "BugTraqAssociations.h"
#include "DragDropImpl.h"
#include "SVNDataObject.h"
#include "SVNProperties.h"
#include "COMError.h"
#include "CmdLineParser.h"
#include "BstrSafeVector.h"
#include "..\..\TSVNCache\CacheInterface.h"

BOOL	CSVNProgressDlg::m_bAscending = FALSE;
int		CSVNProgressDlg::m_nSortedColumn = -1;

#define TRANSFERTIMER	100
#define VISIBLETIMER	101

#define REG_KEY_ALLOW_UNV_OBSTRUCTIONS _T("Software\\TortoiseSVN\\AllowUnversionedObstruction")

enum SVNProgressDlgContextMenuCommands
{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	ID_COMPARE = 1,
	ID_EDITCONFLICT,
	ID_CONFLICTRESOLVE,
	ID_CONFLICTUSETHEIRS,
	ID_CONFLICTUSEMINE,
	ID_LOG,
	ID_OPEN,
	ID_OPENWITH,
	ID_EXPLORE,
	ID_COPY
};

IMPLEMENT_DYNAMIC(CSVNProgressDlg, CResizableStandAloneDialog)
CSVNProgressDlg::CSVNProgressDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSVNProgressDlg::IDD, pParent)
	, m_Revision(_T("HEAD"))
	, m_RevisionEnd(0)
	, m_bLockWarning(false)
	, m_bLockExists(false)
	, m_bCancelled(FALSE)
	, m_bThreadRunning(FALSE)
	, m_nConflicts(0)
	, m_bConflictWarningShown(false)
	, m_bErrorsOccurred(FALSE)
	, m_bMergesAddsDeletesOccurred(FALSE)
	, m_pThread(NULL)
	, m_options(ProgOptNone)
	, m_dwCloseOnEnd((DWORD)-1)
	, m_bCloseLocalOnEnd((DWORD)-1)
	, m_hidden(false)
	, m_bFinishedItemAdded(false)
	, m_bLastVisible(false)
	, m_depth(svn_depth_unknown)
	, m_itemCount(-1)
	, m_itemCountTotal(-1)
	, m_AlwaysConflicted(false)
	, m_BugTraqProvider(NULL)
	, sIgnoredIncluded(MAKEINTRESOURCE(IDS_PROGRS_IGNOREDINCLUDED))
	, sExtExcluded(MAKEINTRESOURCE(IDS_PROGRS_EXTERNALSEXCLUDED))
	, sExtIncluded(MAKEINTRESOURCE(IDS_PROGRS_EXTERNALSINCLUDED))
	, sIgnoreAncestry(MAKEINTRESOURCE(IDS_PROGRS_IGNOREANCESTRY))
	, sRespectAncestry(MAKEINTRESOURCE(IDS_PROGRS_RESPECTANCESTRY))
	, sDryRun(MAKEINTRESOURCE(IDS_PROGRS_DRYRUN))
	, sRecordOnly(MAKEINTRESOURCE(IDS_MERGE_RECORDONLY))
	, sForce(MAKEINTRESOURCE(IDS_MERGE_FORCE))
{
}

CSVNProgressDlg::~CSVNProgressDlg()
{
	for (size_t i=0; i<m_arData.size(); i++)
	{
		delete m_arData[i];
	} 
	delete m_pThread;
}

void CSVNProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SVNPROGRESS, m_ProgList);
}

BEGIN_MESSAGE_MAP(CSVNProgressDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_LOGBUTTON, OnBnClickedLogbutton)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SVNPROGRESS, OnNMCustomdrawSvnprogress)
	ON_WM_CLOSE()
	ON_NOTIFY(NM_DBLCLK, IDC_SVNPROGRESS, OnNMDblclkSvnprogress)
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemclickSvnprogress)
	ON_WM_SETCURSOR()
	ON_WM_CONTEXTMENU()
	ON_REGISTERED_MESSAGE(WM_SVNPROGRESS, OnSVNProgress)
	ON_WM_TIMER()
	ON_NOTIFY(LVN_BEGINDRAG, IDC_SVNPROGRESS, &CSVNProgressDlg::OnLvnBegindragSvnprogress)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_GETDISPINFO, IDC_SVNPROGRESS, &CSVNProgressDlg::OnLvnGetdispinfoSvnprogress)
	ON_BN_CLICKED(IDC_NONINTERACTIVE, &CSVNProgressDlg::OnBnClickedNoninteractive)
	ON_MESSAGE(WM_SHOWCONFLICTRESOLVER, OnShowConflictResolver)
	ON_REGISTERED_MESSAGE(WM_TASKBARBTNCREATED, OnTaskbarBtnCreated)
END_MESSAGE_MAP()

BOOL CSVNProgressDlg::Cancel()
{
	return m_bCancelled;
}

LRESULT CSVNProgressDlg::OnShowConflictResolver(WPARAM /*wParam*/, LPARAM lParam)
{
	CConflictResolveDlg dlg(this);
	const svn_wc_conflict_description_t *description = (svn_wc_conflict_description_t *)lParam;
	if (description)
	{
		dlg.SetConflictDescription(description);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
		}
		if (dlg.DoModal() == IDOK)
		{
			if (dlg.GetResult() == svn_wc_conflict_choose_postpone)
			{
				// if the result is conflicted and the dialog returned IDOK,
				// that means we should not ask again in case of a conflict
				m_AlwaysConflicted = true;
				::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
			}
		}
		m_mergedfile = dlg.GetMergedFile();
		m_bCancelled = dlg.IsCancelled();
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
		return dlg.GetResult();
	}
	return svn_wc_conflict_choose_postpone;
}

svn_wc_conflict_choice_t CSVNProgressDlg::ConflictResolveCallback(const svn_wc_conflict_description_t *description, CString& mergedfile)
{
	// we only bother the user when merging
	if (((m_Command == SVNProgress_Merge)||(m_Command == SVNProgress_MergeAll)||(m_Command == SVNProgress_MergeReintegrate))&&(!m_AlwaysConflicted)&&(description))
	{
		// we're in a worker thread here. That means we must not show a dialog from the thread
		// but let the UI thread do it.
		// To do that, we send a message to the UI thread and let it show the conflict resolver dialog.
		LRESULT dlgResult = ::SendMessage(GetSafeHwnd(), WM_SHOWCONFLICTRESOLVER, 0, (LPARAM)description);
		mergedfile = m_mergedfile;
		return (svn_wc_conflict_choice_t)dlgResult;
	}

	return svn_wc_conflict_choose_postpone;
}

void CSVNProgressDlg::AddItemToList()
{
	int totalcount = m_ProgList.GetItemCount();

	m_ProgList.SetItemCountEx(totalcount+1, LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL);
	// make columns width fit
	if (iFirstResized < 30)
	{
		// only resize the columns for the first 30 or so entries.
		// after that, don't resize them anymore because that's an
		// expensive function call and the columns will be sized
		// close enough already.
		ResizeColumns();
		iFirstResized++;
	}

	// Make sure the item is *entirely* visible even if the horizontal
	// scroll bar is visible.
	int count = m_ProgList.GetCountPerPage();
	if (totalcount <= (m_ProgList.GetTopIndex() + count + nEnsureVisibleCount + 2))
	{
		nEnsureVisibleCount++;
		m_bLastVisible = true;
	}
	else
	{
		nEnsureVisibleCount = 0;
		if (IsIconic() == 0)
			m_bLastVisible = false;
	}
}

BOOL CSVNProgressDlg::Notify(const CTSVNPath& path, const CTSVNPath url, svn_wc_notify_action_t action, 
							 svn_node_kind_t kind, const CString& mime_type, 
							 svn_wc_notify_state_t content_state, 
							 svn_wc_notify_state_t prop_state, LONG rev,
							 const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
							 const CString& changelistname,
							 const CString& propertyName,
							 svn_merge_range_t * range,
							 svn_error_t * err, apr_pool_t * pool)
{
	bool bNoNotify = false;
	bool bDoAddData = true;
	NotificationData * data = new NotificationData();
	data->path = path;
	data->url = url;
	data->action = action;
	data->kind = kind;
	data->mime_type = mime_type;
	data->content_state = content_state;
	data->prop_state = prop_state;
	data->rev = rev;
	data->lock_state = lock_state;
	data->changelistname = changelistname;
	data->propertyName = propertyName;
	if ((lock)&&(lock->owner))
		data->owner = CUnicodeUtils::GetUnicode(lock->owner);
	data->sPathColumnText = path.GetUIPathString();
	if (!m_basePath.IsEmpty())
		data->basepath = m_basePath;
	if (range)
		data->merge_range = *range;
	switch (data->action)
	{
	case svn_wc_notify_add:
	case svn_wc_notify_update_add:
		if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
		{
			data->color = m_Colors.GetColor(CColors::Conflict);
			data->bConflictedActionItem = true;
			data->sActionColumnText.LoadString(IDS_SVNACTION_CONFLICTED);
			m_nConflicts++;
			m_bConflictWarningShown = false;
		}
		else
		{
			m_bMergesAddsDeletesOccurred = true;
			data->sActionColumnText.LoadString(IDS_SVNACTION_ADD);
			data->color = m_Colors.GetColor(CColors::Added);
		}
		break;
	case svn_wc_notify_commit_added:
		data->sActionColumnText.LoadString(IDS_SVNACTION_ADDING);
		data->color = m_Colors.GetColor(CColors::Added);
		break;
	case svn_wc_notify_copy:
		data->sActionColumnText.LoadString(IDS_SVNACTION_COPY);
		break;
	case svn_wc_notify_commit_modified:
		data->sActionColumnText.LoadString(IDS_SVNACTION_MODIFIED);
		data->color = m_Colors.GetColor(CColors::Modified);
		break;
	case svn_wc_notify_delete:
	case svn_wc_notify_update_delete:
		data->sActionColumnText.LoadString(IDS_SVNACTION_DELETE);
		m_bMergesAddsDeletesOccurred = true;
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_commit_deleted:
		data->sActionColumnText.LoadString(IDS_SVNACTION_DELETING);
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_restore:
		data->sActionColumnText.LoadString(IDS_SVNACTION_RESTORE);
		break;
	case svn_wc_notify_revert:
		data->sActionColumnText.LoadString(IDS_SVNACTION_REVERT);
		break;
	case svn_wc_notify_resolved:
		data->sActionColumnText.LoadString(IDS_SVNACTION_RESOLVE);
		break;
	case svn_wc_notify_update_replace:
		data->sActionColumnText.LoadString(IDS_SVNACTION_REPLACED);
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_commit_replaced:
		data->sActionColumnText.LoadString(IDS_SVNACTION_REPLACED);
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_exists:
		if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
		{
			data->color = m_Colors.GetColor(CColors::Conflict);
			data->bConflictedActionItem = true;
			m_nConflicts++;
			m_bConflictWarningShown = false;
			data->sActionColumnText.LoadString(IDS_SVNACTION_CONFLICTED);
		}
		else if ((data->content_state == svn_wc_notify_state_merged) || (data->prop_state == svn_wc_notify_state_merged))
		{
			data->color = m_Colors.GetColor(CColors::Merged);
			m_bMergesAddsDeletesOccurred = true;
			data->sActionColumnText.LoadString(IDS_SVNACTION_MERGED);
		}
		else
			data->sActionColumnText.LoadString(IDS_SVNACTION_EXISTS);
		break;
	case svn_wc_notify_update_update:
		// if this is an inoperative dir change, don't show the notification.
		// an inoperative dir change is when a directory gets updated without
		// any real change in either text or properties.
		if ((kind == svn_node_dir)
			&& ((prop_state == svn_wc_notify_state_inapplicable)
			|| (prop_state == svn_wc_notify_state_unknown)
			|| (prop_state == svn_wc_notify_state_unchanged)))
		{
			bNoNotify = true;
			break;
		}
		if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
		{
			data->color = m_Colors.GetColor(CColors::Conflict);
			data->bConflictedActionItem = true;
			m_nConflicts++;
			m_bConflictWarningShown = false;
			data->sActionColumnText.LoadString(IDS_SVNACTION_CONFLICTED);
		}
		else if ((data->content_state == svn_wc_notify_state_merged) || (data->prop_state == svn_wc_notify_state_merged))
		{
			data->color = m_Colors.GetColor(CColors::Merged);
			m_bMergesAddsDeletesOccurred = true;
			data->sActionColumnText.LoadString(IDS_SVNACTION_MERGED);
		}
		else if ((data->content_state == svn_wc_notify_state_changed) || 
			(data->prop_state == svn_wc_notify_state_changed))
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_UPDATE);
		}
		else
		{
			bNoNotify = true;
			break;
		}
		if (lock_state == svn_wc_notify_lock_state_unlocked)
		{
			CString temp(MAKEINTRESOURCE(IDS_SVNACTION_UNLOCKED));
			data->sActionColumnText += _T(", ") + temp;
		}
		break;

	case svn_wc_notify_update_external:
		// For some reason we build a list of externals...
		m_ExtStack.AddHead(path.GetUIPathString());
		data->sActionColumnText.LoadString(IDS_SVNACTION_EXTERNAL);
		data->bAuxItem = true;
		break;

	case svn_wc_notify_merge_completed:
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_COMPLETED);
			data->bAuxItem = true;
			if ((m_nConflicts>0)&&(!m_bConflictWarningShown))
			{
				// We're going to add another aux item - let's shove this current onto the list first
				// I don't really like this, but it will do for the moment.
				m_arData.push_back(data);
				AddItemToList();

				data = new NotificationData();
				data->bAuxItem = true;
				data->sActionColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED_WARNING);
				data->sPathColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED);
				data->color = m_Colors.GetColor(CColors::Conflict);
				CSoundUtils::PlayTSVNWarning();
				m_bConflictWarningShown = true;
				// This item will now be added after the switch statement
			}
			m_bFinishedItemAdded = true;
		}
		break;
	case svn_wc_notify_update_completed:
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_COMPLETED);
			data->bAuxItem = true;
			bool bEmpty = !!m_ExtStack.IsEmpty();
			if (!bEmpty)
				data->sPathColumnText.FormatMessage(IDS_PROGRS_PATHATREV, (LPCTSTR)m_ExtStack.RemoveHead(), rev);
			else
				data->sPathColumnText.Format(IDS_PROGRS_ATREV, rev);

			if ((m_nConflicts>0)&&(bEmpty)&&(!m_bConflictWarningShown))
			{
				// We're going to add another aux item - let's shove this current onto the list first
				// I don't really like this, but it will do for the moment.
				m_arData.push_back(data);
				AddItemToList();

				data = new NotificationData();
				data->bAuxItem = true;
				data->sActionColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED_WARNING);
				data->sPathColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED);
				data->color = m_Colors.GetColor(CColors::Conflict);
				CSoundUtils::PlayTSVNWarning();
				m_bConflictWarningShown = true;
				// This item will now be added after the switch statement
			}
			if (!m_basePath.IsEmpty())
				m_FinishedRevMap[m_basePath.GetSVNApiPath(pool)] = rev;
			m_RevisionEnd = rev;
			m_bFinishedItemAdded = true;
		}
		break;
	case svn_wc_notify_commit_postfix_txdelta:
		data->sActionColumnText.LoadString(IDS_SVNACTION_POSTFIX);
		break;
	case svn_wc_notify_failed_revert:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDREVERT);
		break;
	case svn_wc_notify_status_completed:
	case svn_wc_notify_status_external:
		data->sActionColumnText.LoadString(IDS_SVNACTION_STATUS);
		break;
	case svn_wc_notify_skip:
		if (content_state == svn_wc_notify_state_missing)
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_SKIPMISSING);
			data->color = m_Colors.GetColor(CColors::Conflict);
		}
		else
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_SKIP);
			if ((content_state == svn_wc_notify_state_obstructed)||(content_state == svn_wc_notify_state_conflicted))
				data->color = m_Colors.GetColor(CColors::Conflict);
		}
		break;
	case svn_wc_notify_locked:
		if ((lock)&&(lock->owner))
			data->sActionColumnText.Format(IDS_SVNACTION_LOCKEDBY, (LPCTSTR)CUnicodeUtils::GetUnicode(lock->owner));
		break;
	case svn_wc_notify_unlocked:
		data->sActionColumnText.LoadString(IDS_SVNACTION_UNLOCKED);
		break;
	case svn_wc_notify_failed_lock:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDLOCK);
		m_arData.push_back(data);
		AddItemToList();
		ReportError(SVN::GetErrorString(err));
		bDoAddData = false;
		if ((err)&&(err->apr_err == SVN_ERR_FS_OUT_OF_DATE))
			m_bLockWarning = true;
		if ((err)&&(err->apr_err == SVN_ERR_FS_PATH_ALREADY_LOCKED))
			m_bLockExists = true;
		break;
	case svn_wc_notify_failed_unlock:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDUNLOCK);
		m_arData.push_back(data);
		AddItemToList();
		ReportError(SVN::GetErrorString(err));
		bDoAddData = false;
		if ((err)&&(err->apr_err == SVN_ERR_FS_OUT_OF_DATE))
			m_bLockWarning = true;
		break;
	case svn_wc_notify_changelist_set:
		data->sActionColumnText.Format(IDS_SVNACTION_CHANGELISTSET, (LPCTSTR)data->changelistname);
		break;
	case svn_wc_notify_changelist_clear:
		data->sActionColumnText.LoadString(IDS_SVNACTION_CHANGELISTCLEAR);
		break;
	case svn_wc_notify_changelist_moved:
		data->sActionColumnText.Format(IDS_SVNACTION_CHANGELISTMOVED, (LPCTSTR)data->changelistname);
		break;
	case svn_wc_notify_foreign_merge_begin:
	case svn_wc_notify_merge_begin:
		if (range == NULL)
			data->sActionColumnText.LoadString(IDS_SVNACTION_MERGEBEGINNONE);
		else if ((data->merge_range.start == data->merge_range.end) || (data->merge_range.start == data->merge_range.end - 1))
			data->sActionColumnText.Format(IDS_SVNACTION_MERGEBEGINSINGLE, data->merge_range.end);
		else if (data->merge_range.start - 1 == data->merge_range.end)
			data->sActionColumnText.Format(IDS_SVNACTION_MERGEBEGINSINGLEREVERSE, data->merge_range.start);
		else if (data->merge_range.start < data->merge_range.end)
			data->sActionColumnText.FormatMessage(IDS_SVNACTION_MERGEBEGINMULTIPLE, data->merge_range.start + 1, data->merge_range.end);
		else
			data->sActionColumnText.FormatMessage(IDS_SVNACTION_MERGEBEGINMULTIPLEREVERSE, data->merge_range.start, data->merge_range.end + 1);
		data->bAuxItem = true;
		break;
	case svn_wc_notify_property_added:
	case svn_wc_notify_property_modified:
		data->sActionColumnText.Format(IDS_SVNACTION_PROPSET, (LPCTSTR)data->propertyName);
		break;
	case svn_wc_notify_property_deleted:
	case svn_wc_notify_property_deleted_nonexistent:
		data->sActionColumnText.Format(IDS_SVNACTION_PROPDEL, (LPCTSTR)data->propertyName);
		break;
	case svn_wc_notify_revprop_set:
		data->sActionColumnText.Format(IDS_SVNACTION_PROPSET, (LPCTSTR)data->propertyName);
		break;
	case svn_wc_notify_revprop_deleted:
		data->sActionColumnText.Format(IDS_SVNACTION_PROPDEL, (LPCTSTR)data->propertyName);
		break;
	case svn_wc_notify_tree_conflict:
		data->sActionColumnText.LoadString(IDS_SVNACTION_TREECONFLICTED);
		data->color = m_Colors.GetColor(CColors::Conflict);
		data->bConflictedActionItem = true;
		m_nConflicts++;
		m_bConflictWarningShown = false;
		break;
	case svn_wc_notify_failed_external:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDEXTERNAL);
		data->color = m_Colors.GetColor(CColors::Conflict);
		m_arData.push_back(data);
		AddItemToList();
		bDoAddData = false;
		ReportError(SVN::GetErrorString(err));
		break;
	default:
		break;
	} // switch (data->action)

	if (bNoNotify)
		delete data;
	else
	{
		if (bDoAddData)
		{
			m_arData.push_back(data);
			AddItemToList();
			if ((!data->bAuxItem)&&(m_itemCount > 0))
			{
				m_itemCount--;

				CProgressCtrl * progControl = (CProgressCtrl *)GetDlgItem(IDC_PROGRESSBAR);
				progControl->ShowWindow(SW_SHOW);
				progControl->SetPos((int)(m_itemCountTotal - m_itemCount));
				progControl->SetRange32(0, (int)m_itemCountTotal);
				if (m_pTaskbarList)
				{
					m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
					m_pTaskbarList->SetProgressValue(m_hWnd, m_itemCountTotal-m_itemCount, m_itemCountTotal);
				}

			}
		}
		if ((action == svn_wc_notify_commit_postfix_txdelta)&&(bSecondResized == FALSE))
		{
			ResizeColumns();
			bSecondResized = TRUE;
		}
	}

	return TRUE;
}

CString CSVNProgressDlg::BuildInfoString()
{
	CString infotext;
	CString temp;
	int added = 0;
	int copied = 0;
	int deleted = 0;
	int restored = 0;
	int reverted = 0;
	int resolved = 0;
	int conflicted = 0;
	int updated = 0;
	int merged = 0;
	int modified = 0;
	int skipped = 0;
	int replaced = 0;

	for (size_t i=0; i<m_arData.size(); ++i)
	{
		const NotificationData * dat = m_arData[i];
		switch (dat->action)
		{
		case svn_wc_notify_add:
		case svn_wc_notify_update_add:
		case svn_wc_notify_commit_added:
			if (dat->bConflictedActionItem)
				conflicted++;
			else
				added++;
			break;
		case svn_wc_notify_copy:
			copied++;
			break;
		case svn_wc_notify_delete:
		case svn_wc_notify_update_delete:
		case svn_wc_notify_commit_deleted:
			deleted++;
			break;
		case svn_wc_notify_restore:
			restored++;
			break;
		case svn_wc_notify_revert:
			reverted++;
			break;
		case svn_wc_notify_resolved:
			resolved++;
			break;
		case svn_wc_notify_update_update:
			if (dat->bConflictedActionItem)
				conflicted++;
			else if ((dat->content_state == svn_wc_notify_state_merged) || (dat->prop_state == svn_wc_notify_state_merged))
				merged++;
			else
				updated++;
			break;
		case svn_wc_notify_commit_modified:
			modified++;
			break;
		case svn_wc_notify_skip:
			skipped++;
			break;
		case svn_wc_notify_commit_replaced:
			replaced++;
			break;
		}
	}
	if (conflicted)
	{
		temp.LoadString(IDS_SVNACTION_CONFLICTED);
		infotext += temp;
		temp.Format(_T(":%d "), conflicted);
		infotext += temp;
	}
	if (skipped)
	{
		temp.LoadString(IDS_SVNACTION_SKIP);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), skipped);
	}
	if (merged)
	{
		temp.LoadString(IDS_SVNACTION_MERGED);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), merged);
	}
	if (added)
	{
		temp.LoadString(IDS_SVNACTION_ADD);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), added);
	}
	if (deleted)
	{
		temp.LoadString(IDS_SVNACTION_DELETE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), deleted);
	}
	if (modified)
	{
		temp.LoadString(IDS_SVNACTION_MODIFIED);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), modified);
	}
	if (copied)
	{
		temp.LoadString(IDS_SVNACTION_COPY);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), copied);
	}
	if (replaced)
	{
		temp.LoadString(IDS_SVNACTION_REPLACED);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), replaced);
	}
	if (updated)
	{
		temp.LoadString(IDS_SVNACTION_UPDATE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), updated);
	}
	if (restored)
	{
		temp.LoadString(IDS_SVNACTION_RESTORE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), restored);
	}
	if (reverted)
	{
		temp.LoadString(IDS_SVNACTION_REVERT);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), reverted);
	}
	if (resolved)
	{
		temp.LoadString(IDS_SVNACTION_RESOLVE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), resolved);
	}
	return infotext;
}

void CSVNProgressDlg::SetAutoClose(const CCmdLineParser& parser)
{
	if (parser.HasVal(_T("closeonend")))
		SetAutoClose(parser.GetLongVal(_T("closeonend")));
	if (parser.HasKey(_T("closeforlocal")))
		SetAutoCloseLocal(TRUE);
	if (parser.HasKey(_T("hideprogress")))
		SetHidden (true);
}

void CSVNProgressDlg::SetSelectedList(const CTSVNPathList& selPaths)
{
	m_selectedPaths = selPaths;
}

void CSVNProgressDlg::ResizeColumns()
{
	m_ProgList.SetRedraw(FALSE);

	TCHAR textbuf[MAX_PATH];

	int maxcol = ((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
	{
		// find the longest width of all items
		int count = m_ProgList.GetItemCount();
		HDITEM hdi = {0};
		hdi.mask = HDI_TEXT;
		hdi.pszText = textbuf;
		hdi.cchTextMax = sizeof(textbuf);
		((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItem(col, &hdi);
		int cx = m_ProgList.GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin

		for (int index = 0; index<count; ++index)
		{
			// get the width of the string and add 12 pixels for the column separator and margins
			int linewidth = cx;
			switch (col)
			{
			case 0:
				linewidth = m_ProgList.GetStringWidth(m_arData[index]->sActionColumnText) + 12;
				break;
			case 1:
				linewidth = m_ProgList.GetStringWidth(m_arData[index]->sPathColumnText) + 12;
				break;
			case 2:
				linewidth = m_ProgList.GetStringWidth(m_arData[index]->mime_type) + 12;
				break;
			}
			if (cx < linewidth)
				cx = linewidth;
		}
		m_ProgList.SetColumnWidth(col, cx);
	}

	m_ProgList.SetRedraw(TRUE);	
}

BOOL CSVNProgressDlg::OnInitDialog()
{
	__super::OnInitDialog();

	ExtendFrameIntoClientArea(IDC_SVNPROGRESS, IDC_SVNPROGRESS, IDC_SVNPROGRESS, IDC_SVNPROGRESS);
	m_aeroControls.SubclassControl(this, IDC_PROGRESSLABEL);
	m_aeroControls.SubclassControl(this, IDC_PROGRESSBAR);
	m_aeroControls.SubclassControl(this, IDC_INFOTEXT);
	m_aeroControls.SubclassControl(this, IDC_NONINTERACTIVE);
	m_aeroControls.SubclassControl(this, IDC_LOGBUTTON);
	m_aeroControls.SubclassOkCancel(this);

	// Let the TaskbarButtonCreated message through the UIPI filter. If we don't
	// do this, Explorer would be unable to send that message to our window if we
	// were running elevated. It's OK to make the call all the time, since if we're
	// not elevated, this is a no-op.
	CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
	typedef BOOL STDAPICALLTYPE ChangeWindowMessageFilterExDFN(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
	HMODULE hUser = ::LoadLibrary(_T("user32.dll"));
	if (hUser)
	{
		ChangeWindowMessageFilterExDFN *pfnChangeWindowMessageFilterEx = (ChangeWindowMessageFilterExDFN*)GetProcAddress(hUser, "ChangeWindowMessageFilterEx");
		if (pfnChangeWindowMessageFilterEx)
		{
			pfnChangeWindowMessageFilterEx(m_hWnd, WM_TASKBARBTNCREATED, MSGFLT_ALLOW, &cfs);
		}
		FreeLibrary(hUser);
	}
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);

	m_ProgList.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_ProgList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ProgList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_PROGRS_ACTION);
	m_ProgList.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGRS_PATH);
	m_ProgList.InsertColumn(1, temp);
	temp.LoadString(IDS_PROGRS_MIMETYPE);
	m_ProgList.InsertColumn(2, temp);

	m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}

	UpdateData(FALSE);

	// Call this early so that the column headings aren't hidden before any
	// text gets added.
	ResizeColumns();

	SetTimer(VISIBLETIMER, 300, NULL);

	AddAnchor(IDC_SVNPROGRESS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESSLABEL, BOTTOM_LEFT, BOTTOM_CENTER);
	AddAnchor(IDC_PROGRESSBAR, BOTTOM_CENTER, BOTTOM_RIGHT);
	AddAnchor(IDC_INFOTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_NONINTERACTIVE, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGBUTTON, BOTTOM_RIGHT);
	SetPromptParentWindow(this->m_hWnd);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("SVNProgressDlg"));
	GetDlgItem(IDOK)->SetFocus();

	return FALSE;
}

bool CSVNProgressDlg::SetBackgroundImage(UINT nID)
{
	return CAppUtils::SetListCtrlBackgroundImage(m_ProgList.GetSafeHwnd(), nID);
}

void CSVNProgressDlg::ReportSVNError()
{
	ReportError(GetLastErrorMessage());
}

void CSVNProgressDlg::ReportError(const CString& sError)
{
	CSoundUtils::PlayTSVNError();
	ReportString(sError, CString(MAKEINTRESOURCE(IDS_ERR_ERROR)), m_Colors.GetColor(CColors::Conflict));
	m_bErrorsOccurred = true;
}

void CSVNProgressDlg::ReportHookFailed(const CString& error)
{
	CString temp;
	temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
	ReportError(temp);
}

void CSVNProgressDlg::ReportWarning(const CString& sWarning)
{
	CSoundUtils::PlayTSVNWarning();
	ReportString(sWarning, CString(MAKEINTRESOURCE(IDS_WARN_WARNING)), m_Colors.GetColor(CColors::Conflict));
}

void CSVNProgressDlg::ReportNotification(const CString& sNotification)
{
	CSoundUtils::PlayTSVNNotification();
	ReportString(sNotification, CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
}

void CSVNProgressDlg::ReportCmd(const CString& sCmd)
{
	ReportString(sCmd, CString(MAKEINTRESOURCE(IDS_PROGRS_CMDINFO)), m_Colors.GetColor(CColors::Cmd));
}

void CSVNProgressDlg::ReportString(CString sMessage, const CString& sMsgKind, COLORREF color)
{
	// instead of showing a dialog box with the error message or notification,
	// just insert the error text into the list control.
	// that way the user isn't 'interrupted' by a dialog box popping up!

	// the message may be split up into different lines
	// so add a new entry for each line of the message
	while (!sMessage.IsEmpty())
	{
		NotificationData * data = new NotificationData();
		data->bAuxItem = true;
		data->sActionColumnText = sMsgKind;
		if (sMessage.Find('\n')>=0)
			data->sPathColumnText = sMessage.Left(sMessage.Find('\n'));
		else
			data->sPathColumnText = sMessage;		
		data->sPathColumnText.Trim(_T("\n\r"));
		data->color = color;
		if (sMessage.Find('\n')>=0)
		{
			sMessage = sMessage.Mid(sMessage.Find('\n')+1);
		}
		else
			sMessage.Empty();
		m_arData.push_back(data);
		AddItemToList();
	}
}

UINT CSVNProgressDlg::ProgressThreadEntry(LPVOID pVoid)
{
	return ((CSVNProgressDlg*)pVoid)->ProgressThread();
}

UINT CSVNProgressDlg::ProgressThread()
{
	if (m_hidden)
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW|
			SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);

	// The SetParams function should have loaded something for us

	CString temp;
	CString sWindowTitle;
	bool localoperation = false;
	bool bSuccess = false;
	m_AlwaysConflicted = false;

	DialogEnableWindow(IDOK, FALSE);
	DialogEnableWindow(IDCANCEL, TRUE);
	SetAndClearProgressInfo(m_hWnd);
	m_itemCount = m_itemCountTotal;

	InterlockedExchange(&m_bThreadRunning, TRUE);
	iFirstResized = 0;
	bSecondResized = FALSE;
	m_bFinishedItemAdded = false;
	CTime startTime = CTime::GetCurrentTime();
	if (m_pTaskbarList)
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);

	switch (m_Command)
	{
	case SVNProgress_Add:
		bSuccess = CmdAdd(sWindowTitle, localoperation);
		break;
	case SVNProgress_Checkout:
		bSuccess = CmdCheckout(sWindowTitle, localoperation);
		break;
	case SVNProgress_SingleFileCheckout:
		bSuccess = CmdSingleFileCheckout(sWindowTitle, localoperation);
		break;
	case SVNProgress_Commit:
		bSuccess = CmdCommit(sWindowTitle, localoperation);
		break;
	case SVNProgress_Copy:
		bSuccess = CmdCopy(sWindowTitle, localoperation);
		break;
	case SVNProgress_Export:
		bSuccess = CmdExport(sWindowTitle, localoperation);
		break;
	case SVNProgress_Import:
		bSuccess = CmdImport(sWindowTitle, localoperation);
		break;
	case SVNProgress_Lock:
		bSuccess = CmdLock(sWindowTitle, localoperation);
		break;
	case SVNProgress_Merge:
		bSuccess = CmdMerge(sWindowTitle, localoperation);
		break;
	case SVNProgress_MergeAll:
		bSuccess = CmdMergeAll(sWindowTitle, localoperation);
		break;
	case SVNProgress_MergeReintegrate:
		bSuccess = CmdMergeReintegrate(sWindowTitle, localoperation);
		break;
	case SVNProgress_Rename:
		bSuccess = CmdRename(sWindowTitle, localoperation);
		break;
	case SVNProgress_Resolve:
		bSuccess = CmdResolve(sWindowTitle, localoperation);
		break;
	case SVNProgress_Revert:
		bSuccess = CmdRevert(sWindowTitle, localoperation);
		break;
	case SVNProgress_Switch:
		bSuccess = CmdSwitch(sWindowTitle, localoperation);
		break;
	case SVNProgress_Unlock:
		bSuccess = CmdUnlock(sWindowTitle, localoperation);
		break;
	case SVNProgress_Update:
		bSuccess = CmdUpdate(sWindowTitle, localoperation);
		break;
	}
	if (!bSuccess)
		temp.LoadString(IDS_PROGRS_TITLEFAILED);
	else
		temp.LoadString(IDS_PROGRS_TITLEFIN);
	sWindowTitle = sWindowTitle + _T(" ") + temp;
	SetWindowText(sWindowTitle);

	KillTimer(TRANSFERTIMER);
	KillTimer(VISIBLETIMER);

	DialogEnableWindow(IDCANCEL, FALSE);
	DialogEnableWindow(IDOK, TRUE);

	CString info = BuildInfoString();
	if (!bSuccess)
		info.LoadString(IDS_PROGRS_INFOFAILED);
	SetDlgItemText(IDC_INFOTEXT, info);
	ResizeColumns();
	SendMessage(DM_SETDEFID, IDOK);
	GetDlgItem(IDOK)->SetFocus();	

	CString sFinalInfo;
	if (!m_sTotalBytesTransferred.IsEmpty())
	{
		CTimeSpan time = CTime::GetCurrentTime() - startTime;
		temp.FormatMessage(IDS_PROGRS_TIME, (LONG)time.GetTotalMinutes(), (LONG)time.GetSeconds());
		sFinalInfo.FormatMessage(IDS_PROGRS_FINALINFO, m_sTotalBytesTransferred, (LPCTSTR)temp);
		SetDlgItemText(IDC_PROGRESSLABEL, sFinalInfo);
	}
	else
		GetDlgItem(IDC_PROGRESSLABEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_PROGRESSBAR)->ShowWindow(SW_HIDE);

	if (!m_bFinishedItemAdded)
	{
		// there's no "finished: xxx" line at the end. We add one here to make
		// sure the user sees that the command is actually finished.
		NotificationData * data = new NotificationData();
		data->bAuxItem = true;
		data->sActionColumnText.LoadString(IDS_PROGRS_FINISHED);
		m_arData.push_back(data);
		AddItemToList();
	}

	int count = m_ProgList.GetItemCount();
	if ((count > 0)&&(m_bLastVisible))
		m_ProgList.EnsureVisible(count-1, FALSE);

	CLogFile logfile;
	if (logfile.Open())
	{
		logfile.AddTimeLine();
		for (size_t i=0; i<m_arData.size(); i++)
		{
			NotificationData * data = m_arData[i];
			temp.Format(_T("%-20s : %s"), (LPCTSTR)data->sActionColumnText, (LPCTSTR)data->sPathColumnText);
			logfile.AddLine(temp);
		}
		if (!sFinalInfo.IsEmpty())
			logfile.AddLine(sFinalInfo);
		logfile.Close();
	}

	m_bCancelled = TRUE;
	InterlockedExchange(&m_bThreadRunning, FALSE);
	RefreshCursor();

	if (m_pTaskbarList)
	{
		if (DidErrorsOccur())
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
			m_pTaskbarList->SetProgressValue(m_hWnd, 100, 100);
		}
		else
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
	}
	DWORD dwAutoClose = CRegStdDWORD(_T("Software\\TortoiseSVN\\AutoClose"));
	BOOL bAutoCloseLocal = CRegStdDWORD(_T("Software\\TortoiseSVN\\AutoCloseLocal"), FALSE);
	if ((m_options & ProgOptDryRun)||(!m_bLastVisible))
	{
		// don't autoclose for dry-runs or if the user scrolled the
		// content of the list control
		dwAutoClose = 0;
		bAutoCloseLocal = FALSE;
	}
	if (m_dwCloseOnEnd != (DWORD)-1)
		dwAutoClose = m_dwCloseOnEnd;		// command line value has priority over setting value
	if (m_bCloseLocalOnEnd != (DWORD)-1)
		bAutoCloseLocal = m_bCloseLocalOnEnd;

	bool sendClose = false;
	if ((dwAutoClose == CLOSE_NOERRORS)&&(!m_bErrorsOccurred))
		sendClose = true;
	if ((dwAutoClose == CLOSE_NOCONFLICTS)&&(!m_bErrorsOccurred)&&(m_nConflicts==0))
		sendClose = true;
	if ((dwAutoClose == CLOSE_NOMERGES)&&(!m_bErrorsOccurred)&&(m_nConflicts==0)&&(!m_bMergesAddsDeletesOccurred))
		sendClose = true;
	// kept for compatibility with pre 1.7 clients
	if ((dwAutoClose == CLOSE_LOCAL)&&(!m_bErrorsOccurred)&&(m_nConflicts==0)&&(localoperation))
		sendClose = true;

	if ((bAutoCloseLocal)&&(!m_bErrorsOccurred)&&(m_nConflicts==0)&&(localoperation))
		sendClose = true;

	if (sendClose)
		PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);
	else if (m_hidden)
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_SHOWWINDOW|
			SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);

	//Don't do anything here which might cause messages to be sent to the window
	//The window thread is probably now blocked in OnOK if we've done an auto close
	return 0;
}

void CSVNProgressDlg::OnBnClickedLogbutton()
{
	if (m_targetPathList.GetCount() != 1)
		return;
	StringRevMap::iterator it = m_UpdateStartRevMap.begin();
	svn_revnum_t rev = -1;
	if (it != m_UpdateStartRevMap.end())
	{
		rev = it->second;
	}
	CLogDlg dlg;
	dlg.SetParams(m_targetPathList[0], m_RevisionEnd, m_RevisionEnd, rev, 0, TRUE);
	dlg.DoModal();
}


void CSVNProgressDlg::OnClose()
{
	if (m_bCancelled)
	{
		TerminateThread(m_pThread->m_hThread, (DWORD)-1);
		InterlockedExchange(&m_bThreadRunning, FALSE);
	}
	else
	{
		m_bCancelled = TRUE;
		return;
	}
	DialogEnableWindow(IDCANCEL, TRUE);
	__super::OnClose();
}

void CSVNProgressDlg::OnOK()
{
	if (GetFocus() != GetDlgItem(IDOK))
		return;	// if the "OK" button doesn't have the focus, do nothing: this prevents closing the dialog when pressing enter
	if ((m_bCancelled)&&(!m_bThreadRunning))
	{
		// I have made this wait a sensible amount of time (10 seconds) for the thread to finish
		// You must be careful in the thread that after posting the WM_COMMAND/IDOK message, you 
		// don't do any more operations on the window which might require message passing
		// If you try to send windows messages once we're waiting here, then the thread can't finished
		// because the Window's message loop is blocked at this wait
		WaitForSingleObject(m_pThread->m_hThread, 10000);
		__super::OnOK();
	}
	m_bCancelled = TRUE;
}

void CSVNProgressDlg::OnCancel()
{
	if ((m_bCancelled)&&(!m_bThreadRunning))
		__super::OnCancel();
	m_bCancelled = TRUE;
}

void CSVNProgressDlg::OnLvnGetdispinfoSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	if (pDispInfo)
	{
		if (pDispInfo->item.mask & LVIF_TEXT)
		{
			if (pDispInfo->item.iItem < (int)m_arData.size())
			{
				const NotificationData * data = m_arData[pDispInfo->item.iItem];
				if (data)
				{
					switch (pDispInfo->item.iSubItem)
					{
					case 0:
						lstrcpyn(m_columnbuf, data->sActionColumnText, MAX_PATH);
						break;
					case 1:
						lstrcpyn(m_columnbuf, data->sPathColumnText, pDispInfo->item.cchTextMax);
						if (!data->bAuxItem)
						{
							int cWidth = m_ProgList.GetColumnWidth(1);
							cWidth = max(12, cWidth-12);
							CDC * pDC = m_ProgList.GetDC();
							if (pDC != NULL)
							{
								CFont * pFont = pDC->SelectObject(m_ProgList.GetFont());
								PathCompactPath(pDC->GetSafeHdc(), m_columnbuf, cWidth);
								pDC->SelectObject(pFont);
								ReleaseDC(pDC);
							}
						}
						break;
					case 2:
						lstrcpyn(m_columnbuf, data->mime_type, MAX_PATH);
						break;
					default:
						m_columnbuf[0] = 0;
					}
				}
				else
					m_columnbuf[0] = 0;

				pDispInfo->item.pszText = m_columnbuf;
			}
		}
	}
	*pResult = 0;
}

void CSVNProgressDlg::OnNMCustomdrawSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		ASSERT(pLVCD->nmcd.dwItemSpec <  m_arData.size());
		if(pLVCD->nmcd.dwItemSpec >= m_arData.size())
		{
			return;
		}
		const NotificationData * data = m_arData[pLVCD->nmcd.dwItemSpec];
		ASSERT(data != NULL);
		if (data == NULL)
			return;

		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = data->color;
	}
}

void CSVNProgressDlg::OnNMDblclkSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (pNMLV->iItem < 0)
		return;
	if (m_options & ProgOptDryRun)
		return;	//don't do anything in a dry-run.

	const NotificationData * data = m_arData[pNMLV->iItem];
	if (data == NULL)
		return;

	if (data->bConflictedActionItem)
	{
		// We've double-clicked on a conflicted item - do a three-way merge on it
		CString sCmd;
		sCmd.Format(_T("\"%s\" /command:conflicteditor /path:\"%s\""),
			(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), data->path.GetWinPath());
		if (!data->path.IsUrl())
		{
			sCmd += _T(" /propspath:\"");
			sCmd += data->path.GetWinPathString();
			sCmd += _T("\"");
		}	
		CAppUtils::LaunchApplication(sCmd, NULL, false);
	}
	else if ((data->action == svn_wc_notify_update_update) && ((data->content_state == svn_wc_notify_state_merged)||(SVNProgress_Merge == m_Command)) || (data->action == svn_wc_notify_resolved))
	{
		// This is a modified file which has been merged on update. Diff it against base
		CTSVNPath temporaryFile;
		SVNDiff diff(this, this->m_hWnd, true);
		diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		svn_revnum_t baseRev = 0;
		diff.DiffFileAgainstBase(data->path, baseRev);
	}
	else if ((!data->bAuxItem)&&(data->path.Exists())&&(!data->path.IsDirectory()))
	{
		bool bOpenWith = false;
		int ret = (int)ShellExecute(m_hWnd, NULL, data->path.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
		if (ret <= HINSTANCE_ERROR)
			bOpenWith = true;
		if (bOpenWith)
		{
			CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
			cmd += data->path.GetWinPathString() + _T(" ");
			CAppUtils::LaunchApplication(cmd, NULL, false);
		}
	}
}

void CSVNProgressDlg::OnHdnItemclickSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	if (m_bThreadRunning)
		return;
	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = phdr->iItem;
	Sort();

	CString temp;
	m_ProgList.SetRedraw(FALSE);
	m_ProgList.DeleteAllItems();
	m_ProgList.SetItemCountEx (static_cast<int>(m_arData.size()));

	m_ProgList.SetRedraw(TRUE);

	*pResult = 0;
}

bool CSVNProgressDlg::NotificationDataIsAux(const NotificationData* pData)
{
	return pData->bAuxItem;
}

LRESULT CSVNProgressDlg::OnSVNProgress(WPARAM /*wParam*/, LPARAM lParam)
{
	SVNProgress * pProgressData = (SVNProgress *)lParam;
	CProgressCtrl * progControl = (CProgressCtrl *)GetDlgItem(IDC_PROGRESSBAR);
	if ((pProgressData->total > 1000)&&(!progControl->IsWindowVisible()))
	{
		progControl->ShowWindow(SW_SHOW);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
	}
	if (((pProgressData->total < 0)&&(pProgressData->progress > 1000)&&(progControl->IsWindowVisible()))&&(m_itemCountTotal<0))
	{
		progControl->ShowWindow(SW_HIDE);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
	}
	if (!GetDlgItem(IDC_PROGRESSLABEL)->IsWindowVisible())
		GetDlgItem(IDC_PROGRESSLABEL)->ShowWindow(SW_SHOW);
	SetTimer(TRANSFERTIMER, 2000, NULL);
	if ((pProgressData->total > 0)&&(pProgressData->progress > 1000))
	{
		progControl->SetPos((int)pProgressData->progress);
		progControl->SetRange32(0, (int)pProgressData->total);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, pProgressData->progress, pProgressData->total);
		}
	}
	CString progText;
	if (pProgressData->overall_total < 1024)
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALBYTESTRANSFERRED, pProgressData->overall_total);	
	else if (pProgressData->overall_total < 1200000)
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALTRANSFERRED, pProgressData->overall_total / 1024);
	else
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALMBTRANSFERRED, (double)((double)pProgressData->overall_total / 1024000.0));
	progText.FormatMessage(IDS_SVN_PROGRESS_TOTALANDSPEED, (LPCTSTR)m_sTotalBytesTransferred, (LPCTSTR)pProgressData->SpeedString);
	SetDlgItemText(IDC_PROGRESSLABEL, progText);
	return 0;
}

void CSVNProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TRANSFERTIMER)
	{
		CString progText;
		CString progSpeed;
		progSpeed.Format(IDS_SVN_PROGRESS_BYTES_SEC, 0i64);
		progText.FormatMessage(IDS_SVN_PROGRESS_TOTALANDSPEED, (LPCTSTR)m_sTotalBytesTransferred, (LPCTSTR)progSpeed);
		SetDlgItemText(IDC_PROGRESSLABEL, progText);
		KillTimer(TRANSFERTIMER);
	}
	if (nIDEvent == VISIBLETIMER)
	{
		if (nEnsureVisibleCount)
			m_ProgList.EnsureVisible(m_ProgList.GetItemCount()-1, false);
		nEnsureVisibleCount = 0;
	}
}

void CSVNProgressDlg::Sort()
{
	if(m_arData.size() < 2)
	{
		return;
	}

	// We need to sort the blocks which lie between the auxiliary entries
	// This is so that any aux data stays where it was
	NotificationDataVect::iterator actionBlockBegin;
	NotificationDataVect::iterator actionBlockEnd = m_arData.begin();	// We start searching from here

	for(;;)
	{
		// Search to the start of the non-aux entry in the next block
		actionBlockBegin = std::find_if(actionBlockEnd, m_arData.end(), std::not1(std::ptr_fun(&CSVNProgressDlg::NotificationDataIsAux)));
		if(actionBlockBegin == m_arData.end())
		{
			// There are no more actions
			break;
		}
		// Now search to find the end of the block
		actionBlockEnd = std::find_if(actionBlockBegin+1, m_arData.end(), std::ptr_fun(&CSVNProgressDlg::NotificationDataIsAux));
		// Now sort the block
		std::sort(actionBlockBegin, actionBlockEnd, &CSVNProgressDlg::SortCompare);
	}
}

bool CSVNProgressDlg::SortCompare(const NotificationData * pData1, const NotificationData * pData2)
{
	int result = 0;
	switch (m_nSortedColumn)
	{
	case 0:		//action column
		result = pData1->sActionColumnText.Compare(pData2->sActionColumnText);
		break;
	case 1:		//path column
		// Compare happens after switch()
		break;
	case 2:		//mime-type column
		result = pData1->mime_type.Compare(pData2->mime_type);
		break;
	default:
		break;
	}

	// Sort by path if everything else is equal
	if (result == 0)
	{
		result = CTSVNPath::Compare(pData1->path, pData2->path);
	}

	if (!m_bAscending)
		result = -result;
	return result < 0;
}

BOOL CSVNProgressDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (!GetDlgItem(IDOK)->IsWindowEnabled())
	{
		if (!IsCursorOverWindowBorder() && ((pWnd)&&(pWnd != GetDlgItem(IDCANCEL))))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CSVNProgressDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
		{
			// pressing the ESC key should close the dialog. But since we disabled the escape
			// key (so the user doesn't get the idea that he could simply undo an e.g. update)
			// this won't work.
			// So if the user presses the ESC key, change it to VK_RETURN so the dialog gets
			// the impression that the OK button was pressed.
			if ((!m_bThreadRunning)&&(!GetDlgItem(IDCANCEL)->IsWindowEnabled())
				&&(GetDlgItem(IDOK)->IsWindowEnabled())&&(GetDlgItem(IDOK)->IsWindowVisible()))
			{
				// since we convert ESC to RETURN, make sure the OK button has the focus.
				GetDlgItem(IDOK)->SetFocus();
				pMsg->wParam = VK_RETURN;
			}
		}
		if (pMsg->wParam == 'A')
		{
			if (GetKeyState(VK_CONTROL)&0x8000)
			{
				// Ctrl-A -> select all
				m_ProgList.SetSelectionMark(0);
				for (int i=0; i<m_ProgList.GetItemCount(); ++i)
				{
					m_ProgList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
		}
		if ((pMsg->wParam == 'C')||(pMsg->wParam == VK_INSERT))
		{
			int selIndex = m_ProgList.GetSelectionMark();
			if (selIndex >= 0)
			{
				if (GetKeyState(VK_CONTROL)&0x8000)
				{
					//Ctrl-C -> copy to clipboard
					CString sClipdata;
					POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
					if (pos != NULL)
					{
						while (pos)
						{
							int nItem = m_ProgList.GetNextSelectedItem(pos);
							CString sAction = m_ProgList.GetItemText(nItem, 0);
							CString sPath = m_ProgList.GetItemText(nItem, 1);
							CString sMime = m_ProgList.GetItemText(nItem, 2);
							CString sLogCopyText;
							sLogCopyText.Format(_T("%s: %s  %s\r\n"),
								(LPCTSTR)sAction, (LPCTSTR)sPath, (LPCTSTR)sMime);
							sClipdata +=  sLogCopyText;
						}
						CStringUtils::WriteAsciiStringToClipboard(sClipdata);
					}
				}
			}
		} 
	} // if (pMsg->message == WM_KEYDOWN)
	return __super::PreTranslateMessage(pMsg);
}

void CSVNProgressDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (m_options & ProgOptDryRun)
		return;	// don't do anything in a dry-run.

	if (pWnd != &m_ProgList)
		return;

	int selIndex = m_ProgList.GetSelectionMark();
	if ((point.x == -1) && (point.y == -1))
	{
		// Menu was invoked from the keyboard rather than by right-clicking
		CRect rect;
		m_ProgList.GetItemRect(selIndex, &rect, LVIR_LABEL);
		m_ProgList.ClientToScreen(&rect);
		point = rect.CenterPoint();
	}

	if ((selIndex < 0)||(m_bThreadRunning))
		return;

	// entry is selected, thread has finished with updating so show the popup menu
	CIconMenu popup;
	if (!popup.CreatePopupMenu())
		return;

	bool bAdded = false;
	NotificationData * data = m_arData[selIndex];
	if ((data)&&(!data->path.IsDirectory()))
	{
		if (data->action == svn_wc_notify_update_update || data->action == svn_wc_notify_resolved)
		{
			if (m_ProgList.GetSelectedCount() == 1)
			{
				popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
				bAdded = true;
			}
		}
		if (data->bConflictedActionItem)
		{
			if (m_ProgList.GetSelectedCount() == 1)
			{
				popup.AppendMenuIcon(ID_EDITCONFLICT, IDS_MENUCONFLICT,IDI_CONFLICT);
				popup.SetDefaultItem(ID_EDITCONFLICT, FALSE);
				popup.AppendMenuIcon(ID_CONFLICTRESOLVE, IDS_SVNPROGRESS_MENUMARKASRESOLVED,IDI_RESOLVE);
			}
			popup.AppendMenuIcon(ID_CONFLICTUSETHEIRS, IDS_SVNPROGRESS_MENUUSETHEIRS,IDI_RESOLVE);
			popup.AppendMenuIcon(ID_CONFLICTUSEMINE, IDS_SVNPROGRESS_MENUUSEMINE,IDI_RESOLVE);
		}
		else if ((data->content_state == svn_wc_notify_state_merged)||(SVNProgress_Merge == m_Command)||(data->action == svn_wc_notify_resolved))
			popup.SetDefaultItem(ID_COMPARE, FALSE);
			
		if (m_ProgList.GetSelectedCount() == 1)
		{
			if ((data->action == svn_wc_notify_add)||
				(data->action == svn_wc_notify_update_add)||
				(data->action == svn_wc_notify_commit_added)||
				(data->action == svn_wc_notify_commit_modified)||
				(data->action == svn_wc_notify_restore)||
				(data->action == svn_wc_notify_revert)||
				(data->action == svn_wc_notify_resolved)||
				(data->action == svn_wc_notify_commit_replaced)||
				(data->action == svn_wc_notify_commit_modified)||
				(data->action == svn_wc_notify_commit_postfix_txdelta)||
				(data->action == svn_wc_notify_update_update))
			{
				popup.AppendMenuIcon(ID_LOG, IDS_MENULOG,IDI_LOG);
				if (data->action == svn_wc_notify_update_update)
					popup.AppendMenu(MF_SEPARATOR, NULL);
				popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
				popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
				bAdded = true;
			}
		}
	}

	if (m_ProgList.GetSelectedCount() == 1)
	{
		if (data)
		{
			CString sPath = GetPathFromColumnText(data->sPathColumnText);
			if ((!sPath.IsEmpty())&&(!SVN::PathIsURL(CTSVNPath(sPath))))
			{
				CTSVNPath path = CTSVNPath(sPath);
				if (path.GetDirectory().Exists())
				{
					popup.AppendMenuIcon(ID_EXPLORE, IDS_SVNPROGRESS_MENUOPENPARENT, IDI_EXPLORER);
					bAdded = true;
				}
			}
		}
	}
	if (m_ProgList.GetSelectedCount() > 0)
	{
		if (bAdded)
			popup.AppendMenu(MF_SEPARATOR, NULL);
		popup.AppendMenuIcon(ID_COPY, IDS_LOG_POPUP_COPYTOCLIPBOARD,IDI_COPYCLIP);
		bAdded = true;
	}
	if (!bAdded)
		return;

	int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
	DialogEnableWindow(IDOK, FALSE);
	this->SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	bool bOpenWith = false;
	switch (cmd)
	{
	case ID_COPY:
		{
			CString sLines;
			POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
			while (pos)
			{
				int nItem = m_ProgList.GetNextSelectedItem(pos);
				NotificationData * data2 = m_arData[nItem];
				if (data2)
				{
					sLines += data2->sPathColumnText;
					sLines += _T("\r\n");
				}
			}
			sLines.TrimRight();
			if (!sLines.IsEmpty())
			{
				CStringUtils::WriteAsciiStringToClipboard(sLines, GetSafeHwnd());
			}
		}
		break;
	case ID_EXPLORE:
		{
			if (data == NULL)
				break;
			CString sPath = GetPathFromColumnText(data->sPathColumnText);
			CTSVNPath path = CTSVNPath(sPath);
			ShellExecute(m_hWnd, _T("explore"), path.GetDirectory().GetWinPath(), NULL, path.GetDirectory().GetWinPath(), SW_SHOW);
		}
		break;
	case ID_COMPARE:
		{
			svn_revnum_t rev = -1;
			StringRevMap::iterator it = m_UpdateStartRevMap.end();
			if (data->basepath.IsEmpty())
				it = m_UpdateStartRevMap.begin();
			else
				it = m_UpdateStartRevMap.find(data->basepath.GetSVNApiPath(pool));
			if (it != m_UpdateStartRevMap.end())
				rev = it->second;
			// if the file was merged during update, do a three way diff between OLD, MINE, THEIRS
			if (data->content_state == svn_wc_notify_state_merged)
			{
				CTSVNPath basefile = CTempFiles::Instance().GetTempFilePath(false, data->path, rev);
				CTSVNPath newfile = CTempFiles::Instance().GetTempFilePath(false, data->path, SVNRev::REV_HEAD);
				SVN svn;
				if (!svn.Cat(data->path, SVNRev(SVNRev::REV_WC), rev, basefile))
				{
					CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					DialogEnableWindow(IDOK, TRUE);
					break;
				}
				// If necessary, convert the line-endings on the file before diffing
				if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\ConvertBase"), TRUE))
				{
					CTSVNPath temporaryFile = CTempFiles::Instance().GetTempFilePath(false, data->path, SVNRev::REV_BASE);
					if (!svn.Cat(data->path, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE), temporaryFile))
					{
						temporaryFile.Reset();
						break;
					}
					newfile = temporaryFile;
				}

				SetFileAttributes(newfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
				SetFileAttributes(basefile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
				CString revname, wcname, basename;
				revname.Format(_T("%s Revision %ld"), (LPCTSTR)data->path.GetUIFileOrDirectoryName(), rev);
				wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
				basename.Format(IDS_DIFF_BASENAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
				CAppUtils::MergeFlags flags;
				flags.bAlternativeTool = (GetKeyState(VK_SHIFT)&0x8000) != 0;
				flags.bReadOnly = true;
				CAppUtils::StartExtMerge(flags,	basefile, newfile, data->path, data->path, basename, revname, wcname);
			}
			else
			{
				CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, data->path, rev);
				SVN svn;
				if (!svn.Cat(data->path, SVNRev(SVNRev::REV_WC), rev, tempfile))
				{
					CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					DialogEnableWindow(IDOK, TRUE);
					break;
				}
				SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
				CString revname, wcname;
				revname.Format(_T("%s Revision %ld"), (LPCTSTR)data->path.GetUIFileOrDirectoryName(), rev);
				wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
				CAppUtils::StartExtDiff(
					tempfile, data->path, revname, wcname,
						CAppUtils::DiffFlags().AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000)), 0);
			}
		}
		break;
	case ID_EDITCONFLICT:
		{
			CString sPath = GetPathFromColumnText(data->sPathColumnText);
			CString sCmd;
			sCmd.Format(_T("\"%s\" /command:conflicteditor /path:\"%s\""),
				(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)sPath);
			CAppUtils::LaunchApplication(sCmd, NULL, false);
		}
		break;
	case ID_CONFLICTUSETHEIRS:
	case ID_CONFLICTUSEMINE:
	case ID_CONFLICTRESOLVE:
		{
			svn_wc_conflict_choice_t result = svn_wc_conflict_choose_merged;
			switch (cmd)
			{
				case ID_CONFLICTUSETHEIRS:
					result = svn_wc_conflict_choose_theirs_full;
					break;
				case ID_CONFLICTUSEMINE:
					result = svn_wc_conflict_choose_mine_full;
					break;
				case ID_CONFLICTRESOLVE:
					result = svn_wc_conflict_choose_merged;
					break;
			}
			SVN svn;
			POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
			CString sResolvedPaths;
			while (pos)
			{
				int nItem = m_ProgList.GetNextSelectedItem(pos);
				NotificationData * data2 = m_arData[nItem];
				if (data2)
				{
					if (data2->bConflictedActionItem)
					{
						if (!svn.Resolve(data2->path, result, FALSE))
						{
							CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							DialogEnableWindow(IDOK, TRUE);
							break;
						}
						data2->color = ::GetSysColor(COLOR_WINDOWTEXT);
						data2->action = svn_wc_notify_resolved;
						data2->sActionColumnText.LoadString(IDS_SVNACTION_RESOLVE);
						data2->bConflictedActionItem = false;
						m_nConflicts--;

						if (m_nConflicts==0)
						{
							// When the last conflict is resolved we remove
							// the warning which we assume is in the last line.
							int nIndex = m_ProgList.GetItemCount()-1;
							VERIFY(m_ProgList.DeleteItem(nIndex));

							delete m_arData[nIndex];
							m_arData.pop_back();
						}
						sResolvedPaths += data2->path.GetWinPathString() + _T("\n");
					}
				}
			}
			m_ProgList.Invalidate();
			CString info = BuildInfoString();
			SetDlgItemText(IDC_INFOTEXT, info);

			if (!sResolvedPaths.IsEmpty())
			{
				CString msg;
				msg.Format(IDS_SVNPROGRESS_RESOLVED, (LPCTSTR)sResolvedPaths);
				CMessageBox::Show(m_hWnd, msg, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
			}
		}
		break;
	case ID_LOG:
		{
			CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
			int limit = (int)(DWORD)reg;
			svn_revnum_t rev = m_RevisionEnd;
			if (!data->basepath.IsEmpty())
			{
				StringRevMap::iterator it = m_FinishedRevMap.find(data->basepath.GetSVNApiPath(pool));
				if (it != m_FinishedRevMap.end())
					rev = it->second;
			}
			CLogDlg dlg;
			// fetch the log from HEAD, not the revision we updated to:
			// the path might be inside an external folder which has its own
			// revisions.
			CString sPath = GetPathFromColumnText(data->sPathColumnText);
			dlg.SetParams(CTSVNPath(sPath), SVNRev(), SVNRev::REV_HEAD, 1, limit, TRUE);
			dlg.DoModal();
		}
		break;
	case ID_OPENWITH:
		bOpenWith = true;
	case ID_OPEN:
		{
			CString sWinPath = GetPathFromColumnText(data->sPathColumnText);
			if (!bOpenWith)
            {
				const int ret = (int)ShellExecute(this->m_hWnd, NULL, (LPCTSTR)sWinPath, NULL, NULL, SW_SHOWNORMAL);
			    if ((ret <= HINSTANCE_ERROR)||bOpenWith)
			    {
				    CString c = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
				    c += sWinPath + _T(" ");
				    CAppUtils::LaunchApplication(c, NULL, false);
			    }
            }
		}
	}
	DialogEnableWindow(IDOK, TRUE);
	theApp.DoWaitCursor(-1);
}

void CSVNProgressDlg::OnLvnBegindragSvnprogress(NMHDR* pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	int selIndex = m_ProgList.GetSelectionMark();
	if (selIndex < 0)
		return;

	CTSVNPathList pathList;
	int index;
	POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
	while ( (index = m_ProgList.GetNextSelectedItem(pos)) >= 0 )
	{
		NotificationData * data = m_arData[index];

		if ( data->kind==svn_node_file || data->kind==svn_node_dir )
		{
			CString sPath = GetPathFromColumnText(data->sPathColumnText);
			pathList.AddPath(CTSVNPath(sPath));
		}
	}

	if (pathList.GetCount() == 0)
	{
		return;
	}

	CIDropSource* pdsrc = new CIDropSource;
	if (pdsrc == NULL)
		return;
	pdsrc->AddRef();


	SVNDataObject* pdobj = new SVNDataObject(pathList, SVNRev::REV_WC, SVNRev::REV_WC);
	if (pdobj == NULL)
	{
		delete pdsrc;
		return;
	}
	pdobj->AddRef();

	CDragSourceHelper dragsrchelper;

	dragsrchelper.InitializeFromWindow(m_ProgList.GetSafeHwnd(), pNMLV->ptAction, pdobj);

	pdsrc->m_pIDataObj = pdobj;
	pdsrc->m_pIDataObj->AddRef();

	// Initiate the Drag & Drop
	DWORD dwEffect;
	::DoDragDrop(pdobj, pdsrc, DROPEFFECT_MOVE|DROPEFFECT_COPY, &dwEffect);
	pdsrc->Release();
	pdobj->Release();
}

void CSVNProgressDlg::OnSize(UINT nType, int cx, int cy)
{
	CResizableStandAloneDialog::OnSize(nType, cx, cy);
	if ((nType == SIZE_RESTORED)&&(m_bLastVisible))
	{
		int count = m_ProgList.GetItemCount();
		if (count > 0)
			m_ProgList.EnsureVisible(count-1, false);
	}
}

//////////////////////////////////////////////////////////////////////////
/// commands
//////////////////////////////////////////////////////////////////////////
bool CSVNProgressDlg::CmdAdd(CString& sWindowTitle, bool& localoperation)
{
	localoperation = true;
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_ADD);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_ADD_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_ADD)));
	if (!Add(m_targetPathList, &m_ProjectProperties, svn_depth_empty, false, true, true))
	{
		ReportSVNError();
		return false;
	}
	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
	return true;
}

bool CSVNProgressDlg::CmdCheckout(CString& sWindowTitle, bool& /*localoperation*/)
{
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_CHECKOUT);
	SetBackgroundImage(IDI_CHECKOUT_BKG);
	CTSVNPathList urls;
	urls.LoadFromAsteriskSeparatedString(m_url.GetSVNPathString());
	for (int i=0; i<urls.GetCount(); ++i)
	{
		sWindowTitle = urls[i].GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
		SetWindowText(sWindowTitle);
		CTSVNPath checkoutdir = m_targetPathList[0];
		if (urls.GetCount() > 1)
		{
			CString fileordir = urls[i].GetFileOrDirectoryName();
			fileordir = CPathUtils::PathUnescape(fileordir);
			checkoutdir.AppendPathString(fileordir);
		}
		CString sCmdInfo;
		sCmdInfo.Format(IDS_PROGRS_CMD_CHECKOUT, 
			(LPCTSTR)urls[i].GetSVNPathString(), (LPCTSTR)m_Revision.ToString(), 
			(LPCTSTR)SVNStatus::GetDepthString(m_depth), 
			m_options & ProgOptIgnoreExternals ? (LPCTSTR)sExtExcluded : (LPCTSTR)sExtIncluded);
		ReportCmd(sCmdInfo);

		SendCacheCommand(TSVNCACHECOMMAND_BLOCK, checkoutdir.GetWinPath());
		if (!Checkout(urls[i], checkoutdir, m_Revision, m_Revision, m_depth, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(REG_KEY_ALLOW_UNV_OBSTRUCTIONS, true))))
		{
			if (m_ProgList.GetItemCount()>1)
			{
				ReportSVNError();
				SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, checkoutdir.GetWinPath());
				return false;
			}
			// if the checkout fails with the peg revision set to the checkout revision,
			// try again with HEAD as the peg revision.
			else
			{
				if (!Checkout(urls[i], checkoutdir, SVNRev::REV_HEAD, m_Revision, m_depth, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(REG_KEY_ALLOW_UNV_OBSTRUCTIONS, true))))
				{
					ReportSVNError();
					SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, checkoutdir.GetWinPath());
					return false;
				}
			}
		}
		SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, checkoutdir.GetWinPath());
	}
	return true;
}

bool CSVNProgressDlg::CmdSingleFileCheckout(CString& sWindowTitle, bool& localoperation)
{
	// get urls to c/o

	CTSVNPathList urls;
	urls.LoadFromAsteriskSeparatedString(m_url.GetSVNPathString());
	if (urls.GetCount() == 0)
		return true;

	// preparation

	CTSVNPath parentURL = urls[0].GetContainingDirectory();
	CTSVNPath checkoutdir = m_targetPathList[0];

	sWindowTitle.LoadString(IDS_PROGRS_TITLE_CHECKOUT);
	SetBackgroundImage(IDI_CHECKOUT_BKG);

	sWindowTitle = checkoutdir.GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
	SetWindowText(sWindowTitle);

	// c/o base folder

	SVNInfo info;
	const SVNInfoData* infoData 
		= info.GetFirstFileInfo (checkoutdir, SVNRev(), SVNRev());

	bool allow_obstructions 
		= !!DWORD (CRegDWORD (REG_KEY_ALLOW_UNV_OBSTRUCTIONS , true));

	bool revisionIsHead = m_Revision.IsHead() || !m_Revision.IsValid();
	if ((infoData == NULL) || (infoData->url != parentURL.GetSVNPathString()))
	{
		CPathUtils::MakeSureDirectoryPathExists (checkoutdir.GetWinPath());

		if (   !Checkout ( parentURL
			  		     , checkoutdir
					     , m_Revision
					     , m_Revision
					     , svn_depth_empty
					     , (m_options & ProgOptIgnoreExternals) != 0
					     , allow_obstructions)

			 // retry with HEAD as pegRev
			&& (   revisionIsHead
				|| !Checkout ( parentURL
			  				 , checkoutdir
							 , SVNRev::REV_HEAD
							 , m_Revision
							 , svn_depth_empty
							 , (m_options & ProgOptIgnoreExternals) != 0
							 , allow_obstructions)))
		{
			ReportSVNError();
			return false;
		}
	}

	// add the sub-items

	m_url = CTSVNPath();
	for (int i=0; i<urls.GetCount(); ++i)
	{
		CTSVNPath filePath = checkoutdir;
		filePath.AppendPathString (urls[i].GetFilename());
		m_targetPathList = CTSVNPathList (filePath);
		m_options = ProgOptNone;

		if (!CmdUpdate (sWindowTitle, localoperation))
			return false;
	}

	return true;
}

bool CSVNProgressDlg::CmdCommit(CString& sWindowTitle, bool& /*localoperation*/)
{
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_COMMIT);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_COMMIT_BKG);
	if (m_targetPathList.GetCount()==0)
	{
		SetWindowText(sWindowTitle);

		DialogEnableWindow(IDCANCEL, FALSE);
		DialogEnableWindow(IDOK, TRUE);

		InterlockedExchange(&m_bThreadRunning, FALSE);
		return true;
	}
	if (m_targetPathList.GetCount()==1)
	{
		sWindowTitle = m_targetPathList[0].GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
		SetWindowText(sWindowTitle);
	}
	if (IsCommittingToTag())
	{
		if (CMessageBox::Show(m_hWnd, IDS_PROGRS_COMMITT_TRUNK, IDS_APPNAME, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION)==IDNO)
			return false;
	}
	DWORD exitcode = 0;
	CString error;
	if (CHooks::Instance().PreCommit(m_selectedPaths, m_depth, m_sMessage, exitcode, error))
	{
		if (exitcode)
		{
			ReportHookFailed(error);
			return false;
		}
	}

	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_COMMIT)));
	CStringArray changelists;
    if (!m_changelist.IsEmpty())
	    changelists.Add(m_changelist);
	bool commitSuccessful = true;
	if (!Commit(m_targetPathList, m_sMessage, changelists, m_keepchangelist, 
		m_depth, (m_options & ProgOptKeeplocks) != 0, m_revProps))
	{
		ReportSVNError();
		error = GetLastErrorMessage();
		// if a non-recursive commit failed with SVN_ERR_UNSUPPORTED_FEATURE,
		// that means a folder deletion couldn't be committed.
		if ((m_Revision != 0)&&(Err)&&(Err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE))
		{
			ReportError(CString(MAKEINTRESOURCE(IDS_PROGRS_NONRECURSIVEHINT)));
		}
		commitSuccessful = false;
		return false;
	}
	if (!PostCommitErr.IsEmpty())
	{
		ReportError(PostCommitErr);
	}
	if (commitSuccessful)
	{
		if (m_BugTraqProvider)
		{
			CComPtr<IBugTraqProvider2> pProvider;
			HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider);
			if (SUCCEEDED(hr))
			{
				ATL::CComBSTR commonRoot(m_selectedPaths.GetCommonRoot().GetDirectory().GetWinPath());
				CBstrSafeVector pathList(m_selectedPaths.GetCount());

				for (LONG index = 0; index < m_selectedPaths.GetCount(); ++index)
					pathList.PutElement(index, m_selectedPaths[index].GetSVNPathString());

				ATL::CComBSTR logMessage;
				logMessage.Attach(m_sMessage.AllocSysString());

				ATL::CComBSTR temp;
				if (FAILED(hr = pProvider->OnCommitFinished(GetSafeHwnd(), 
					commonRoot,
					pathList,
					logMessage,
					(LONG)m_RevisionEnd,
					&temp)))
				{
					CString sErr = temp;
					if (!sErr.IsEmpty())
						ReportError(sErr);
					else
					{
						COMError ce(hr);
						sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, ce.GetSource().c_str(), ce.GetMessageAndDescription().c_str());
						ReportError(sErr);
					}
				}
			}
		}
	}
	if (CHooks::Instance().PostCommit(m_selectedPaths, m_depth, m_RevisionEnd, m_sMessage, exitcode, error))
	{
		if (exitcode)
		{
			ReportHookFailed(error);
			return false;
		}
	}
	return true;
}

bool CSVNProgressDlg::CmdCopy(CString& sWindowTitle, bool& /*localoperation*/)
{
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_COPY);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_COPY_BKG);

	if (m_Revision.IsWorking())
	{
		// adjust the svn:externals property before we do
		// the actual copy
		m_externals.TagExternals(false);
	}

	CString sCmdInfo;
	sCmdInfo.Format(IDS_PROGRS_CMD_COPY, 
		m_targetPathList[0].IsUrl() ? (LPCTSTR)m_targetPathList[0].GetSVNPathString() : m_targetPathList[0].GetWinPath(),
		(LPCTSTR)m_url.GetSVNPathString(), (LPCTSTR)m_Revision.ToString());
	ReportCmd(sCmdInfo);

	if (!Copy(m_targetPathList, m_url, m_Revision, m_pegRev, m_sMessage, false, false, (m_options & ProgOptIgnoreExternals) != 0, m_revProps))
	{
		ReportSVNError();
		return false;
	}
	if (!PostCommitErr.IsEmpty())
	{
		ReportError(PostCommitErr);
	}
	if (m_Revision.IsWorking())
	{
		// restore the svn:externals property
		m_externals.RestoreExternals();
	}
	else if (m_Revision.IsNumber() || m_Revision.IsDate() || m_Revision.IsHead())
	{
		if (m_externals.size())
		{
			sCmdInfo.LoadString(IDS_PROGRS_CMD_TAGEXTERNALS);
			ReportCmd(sCmdInfo);
			m_externals.TagExternals(true, CString(MAKEINTRESOURCE(IDS_COPY_COMMITMSG)), GetCommitRevision(), m_targetPathList[0], CTSVNPath(m_url));
		}
	}

	if (m_options & ProgOptSwitchAfterCopy)
	{
		sCmdInfo.Format(IDS_PROGRS_CMD_SWITCH, 
			m_targetPathList[0].GetWinPath(),
			(LPCTSTR)m_url.GetSVNPathString(), (LPCTSTR)m_Revision.ToString());
		ReportCmd(sCmdInfo);
		if (!Switch(m_targetPathList[0], m_url, SVNRev::REV_HEAD, SVNRev::REV_HEAD, m_depth, true, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\AllowUnversionedObstruction"), true))))
		{
			if (m_ProgList.GetItemCount()>1)
			{
				ReportSVNError();
				return false;
			}
			else if (!Switch(m_targetPathList[0], m_url, SVNRev::REV_HEAD, m_Revision, m_depth, true, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\AllowUnversionedObstruction"), true))))
			{
				ReportSVNError();
				return false;
			}
		}
	}
	else
	{
		if (SVN::PathIsURL(m_url))
		{
			CString sMsg(MAKEINTRESOURCE(IDS_PROGRS_COPY_WARNING));
			ReportNotification(sMsg);
		}
	}
	return true;
}

bool CSVNProgressDlg::CmdExport(CString& sWindowTitle, bool& /*localoperation*/)
{
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_EXPORT);
	sWindowTitle = m_url.GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_EXPORT_BKG);
	CString eol;
	if (m_options & ProgOptEolCRLF)
		eol = _T("CRLF");
	if (m_options & ProgOptEolLF)
		eol = _T("LF");
	if (m_options & ProgOptEolCR)
		eol = _T("CR");
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_EXPORT)));
	SendCacheCommand(TSVNCACHECOMMAND_BLOCK, m_targetPathList[0].GetWinPath());
	if (!Export(m_url, m_targetPathList[0], m_Revision, m_Revision, true, (m_options & ProgOptIgnoreExternals) != 0, m_depth, NULL, false, eol))
	{
		ReportSVNError();
		SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
		return false;
	}
	SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
	return true;
}

bool CSVNProgressDlg::CmdImport(CString& sWindowTitle, bool& /*localoperation*/)
{
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_IMPORT);
	sWindowTitle = m_targetPathList[0].GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_IMPORT_BKG);
	CString sCmdInfo;
	sCmdInfo.Format(IDS_PROGRS_CMD_IMPORT, 
		m_targetPathList[0].GetWinPath(), (LPCTSTR)m_url.GetSVNPathString(), 
		m_options & ProgOptIncludeIgnored ? (LPCTSTR)(_T(", ") + sIgnoredIncluded) : _T(""));
	ReportCmd(sCmdInfo);
	if (!Import(m_targetPathList[0], m_url, m_sMessage, &m_ProjectProperties, svn_depth_infinity, m_options & ProgOptIncludeIgnored ? true : false, false, m_revProps))
	{
		ReportSVNError();
		return false;
	}
	return true;
}

bool CSVNProgressDlg::CmdLock(CString& sWindowTitle, bool& /*localoperation*/)
{
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_LOCK);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_LOCK_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_LOCK)));
	if (!Lock(m_targetPathList, (m_options & ProgOptForce) != 0, m_sMessage))
	{
		ReportSVNError();
		return false;
	}
	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
	if (m_bLockWarning)
	{
		// the lock failed, because the file was outdated.
		// ask the user whether to update the file and try again
		if (CMessageBox::Show(m_hWnd, IDS_WARN_LOCKOUTDATED, IDS_APPNAME, MB_ICONQUESTION|MB_YESNO)==IDYES)
		{
			ReportString(CString(MAKEINTRESOURCE(IDS_SVNPROGRESS_UPDATEANDRETRY)), CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
			if (!Update(m_targetPathList, SVNRev::REV_HEAD, svn_depth_files, false, true, !!DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\AllowUnversionedObstruction"), true))))
			{
				ReportSVNError();
				return false;
			}
			if (!Lock(m_targetPathList, (m_options & ProgOptForce) != 0, m_sMessage))
			{
				ReportSVNError();
				return false;
			}
		}
	}
	if (m_bLockExists)
	{
		// the locking failed because there already is a lock.
		// if the locking-dialog is skipped in the settings, tell the
		// user how to steal the lock anyway (i.e., how to get the lock
		// dialog back without changing the settings)
		if (!DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\ShowLockDlg"), TRUE)))
		{
			ReportString(CString(MAKEINTRESOURCE(IDS_SVNPROGRESS_LOCKHINT)), CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
		}
		return false;
	}
	return true;
}

bool CSVNProgressDlg::CmdMerge(CString& sWindowTitle, bool& /*localoperation*/)
{
	bool bFailed = false;
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_MERGE);
	SetBackgroundImage(IDI_MERGE_BKG);
	if (m_options & ProgOptDryRun)
	{
		sWindowTitle += _T(" ") + sDryRun;
	}
	if (m_options & ProgOptRecordOnly)
	{
		sWindowTitle += _T(" ") + sRecordOnly;
	}
	SetWindowText(sWindowTitle);

	GetDlgItem(IDC_INFOTEXT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_SHOW);
	CRegDWORD nonint = CRegDWORD(_T("Software\\TortoiseSVN\\MergeNonInteractive"), FALSE);
	if (DWORD(nonint))
	{
		::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
		m_AlwaysConflicted = true;
	}
	// we only accept a revision list to merge for peg merges
	ATLASSERT((m_revisionArray.GetCount()==0) || (m_revisionArray.GetCount() && (m_url.IsEquivalentTo(m_url2))));

	if (m_url.IsEquivalentTo(m_url2))
	{
		CString sSuggestedMessage;
		CString sMergedLogMessage;
		CString sSeparator = CRegString(_T("Software\\TortoiseSVN\\MergeLogSeparator"), _T("........"));
		CString temp;

		// Merging revisions %s of %s to %s into %s, %s%s
		CString sCmdInfo;
		sCmdInfo.Format(IDS_PROGRS_CMD_MERGEPEG, 
			(LPCTSTR)m_revisionArray.ToListString(),
			(LPCTSTR)m_url.GetSVNPathString(),
			m_targetPathList[0].GetWinPath(),
			m_options & ProgOptIgnoreAncestry ? (LPCTSTR)sIgnoreAncestry : (LPCTSTR)sRespectAncestry,
			m_options & ProgOptDryRun ? ((LPCTSTR)_T(", ") + sDryRun) : _T(""),
			m_options & ProgOptForce ? ((LPCTSTR)_T(", ") + sForce) : _T(""));
		ReportCmd(sCmdInfo);

		SVNRev firstRevOfRange;
		if (m_revisionArray.GetCount())
		{
			firstRevOfRange = m_revisionArray[0].GetStartRevision();
		}
		SendCacheCommand(TSVNCACHECOMMAND_BLOCK, m_targetPathList[0].GetWinPath());
		if (!PegMerge(m_url, m_revisionArray, 
			m_pegRev.IsValid() ? m_pegRev : (m_url.IsUrl() ? firstRevOfRange : SVNRev(SVNRev::REV_WC)),
			m_targetPathList[0], !!(m_options & ProgOptForce), m_depth, m_diffoptions, !!(m_options & ProgOptIgnoreAncestry), !!(m_options & ProgOptDryRun), !!(m_options & ProgOptRecordOnly)))
		{
			if (m_ProgList.GetItemCount()>1)
			{
				ReportSVNError();
				SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
				bFailed = true;
			}
			// if the merge fails with the peg revision set,
			// try again with HEAD as the peg revision.
			else if (!PegMerge(m_url, m_revisionArray, SVNRev::REV_HEAD,
				m_targetPathList[0], !!(m_options & ProgOptForce), m_depth, m_diffoptions, !!(m_options & ProgOptIgnoreAncestry), !!(m_options & ProgOptDryRun), !!(m_options & ProgOptRecordOnly)))
			{
				ReportSVNError();
				SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
				bFailed = true;
			}
		}
	}
	else
	{
		CString sCmdInfo;
		sCmdInfo.Format(IDS_PROGRS_CMD_MERGEURL, 
			(LPCTSTR)m_url.GetSVNPathString(), (LPCTSTR)m_Revision.ToString(), 
			(LPCTSTR)m_url2.GetSVNPathString(), (LPCTSTR)m_RevisionEnd.ToString(),
			m_targetPathList[0].GetWinPath(),
			m_options & ProgOptIgnoreAncestry ? (LPCTSTR)sIgnoreAncestry : (LPCTSTR)sRespectAncestry,
			m_options & ProgOptDryRun ? ((LPCTSTR)_T(", ") + sDryRun) : _T(""),
			m_options & ProgOptForce ? ((LPCTSTR)_T(", ") + sForce) : _T(""));
		ReportCmd(sCmdInfo);

		SendCacheCommand(TSVNCACHECOMMAND_BLOCK, m_targetPathList[0].GetWinPath());
		if (!Merge(m_url, m_Revision, m_url2, m_RevisionEnd, m_targetPathList[0], 
			!!(m_options & ProgOptForce), m_depth, m_diffoptions, !!(m_options & ProgOptIgnoreAncestry), !!(m_options & ProgOptDryRun), !!(m_options & ProgOptRecordOnly)))
		{
			ReportSVNError();
			SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
			bFailed = true;
		}
	}
	SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
	GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_INFOTEXT)->ShowWindow(SW_SHOW);
	return !bFailed;
}

bool CSVNProgressDlg::CmdMergeAll(CString& sWindowTitle, bool& /*localoperation*/)
{
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_MERGE);
	SetBackgroundImage(IDI_MERGE_BKG);
	SetWindowText(sWindowTitle);

	ATLASSERT(m_targetPathList.GetCount() == 1);

	CString sCmdInfo;
	sCmdInfo.LoadString(IDS_PROGRS_INFOGETTINGINFO);
	ReportCmd(sCmdInfo);
	CTSVNPathList suggestedSources;
	if (!SuggestMergeSources(m_targetPathList[0], m_Revision, suggestedSources))
	{
		ReportSVNError();
		return false;
	}

	if (suggestedSources.GetCount() == 0)
	{
		CString sErr;
		sErr.Format(IDS_PROGRS_MERGEALLNOSOURCES, m_targetPathList[0].GetWinPath());
		ReportError(sErr);
		return false;
	}
	sCmdInfo.Format(IDS_PROGRS_CMD_MERGEALL, 
		(LPCTSTR)suggestedSources[0].GetSVNPathString(),
		m_targetPathList[0].GetWinPath(),
		m_options & ProgOptIgnoreAncestry ? (LPCTSTR)sIgnoreAncestry : (LPCTSTR)sRespectAncestry,
		m_options & ProgOptForce ? ((LPCTSTR)_T(", ") + sForce) : _T(""));
	ReportCmd(sCmdInfo);

	GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_SHOW);
	CRegDWORD nonint = CRegDWORD(_T("Software\\TortoiseSVN\\MergeNonInteractive"), FALSE);
	if (DWORD(nonint))
	{
		::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
		m_AlwaysConflicted = true;
	}

	SVNRevRangeArray revarray;
	SendCacheCommand(TSVNCACHECOMMAND_BLOCK, m_targetPathList[0].GetWinPath());
	if (!PegMerge(suggestedSources[0], revarray, 
		SVNRev::REV_HEAD,
		m_targetPathList[0], !!(m_options & ProgOptForce), m_depth, m_diffoptions, !!(m_options & ProgOptIgnoreAncestry), FALSE))
	{
		GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
		ReportSVNError();
		SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
		return false;
	}

	SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
	GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
	return true;
}

bool CSVNProgressDlg::CmdMergeReintegrate(CString& sWindowTitle, bool& /*localoperation*/)
{
	ASSERT(m_targetPathList.GetCount() == 1);
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_MERGEREINTEGRATE);
	SetBackgroundImage(IDI_MERGE_BKG);
	SetWindowText(sWindowTitle);

	CString sCmdInfo;
	sCmdInfo.Format(IDS_PROGRS_CMD_MERGEREINTEGRATE, 
		(LPCTSTR)m_url.GetSVNPathString(),
		m_targetPathList[0].GetWinPath());
	ReportCmd(sCmdInfo);

	GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_SHOW);
	CRegDWORD nonint = CRegDWORD(_T("Software\\TortoiseSVN\\MergeNonInteractive"), FALSE);
	if (DWORD(nonint))
	{
		::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
		m_AlwaysConflicted = true;
	}

	SendCacheCommand(TSVNCACHECOMMAND_BLOCK, m_targetPathList[0].GetWinPath());
	if (!MergeReintegrate(m_url, SVNRev::REV_HEAD, m_targetPathList[0], !!(m_options & ProgOptDryRun), m_diffoptions))
	{
		ReportSVNError();
		GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
		SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
		return false;
	}

	SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
	GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
	return true;
}

bool CSVNProgressDlg::CmdRename(CString& sWindowTitle, bool& localoperation)
{
	ASSERT(m_targetPathList.GetCount() == 1);
	if ((!m_targetPathList[0].IsUrl())&&(!m_url.IsUrl()))
		localoperation = true;
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_RENAME);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_RENAME_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_RENAME)));
	if (!Move(m_targetPathList, m_url, (m_options & ProgOptForce) != 0, m_sMessage, false, false, m_revProps))
	{
		ReportSVNError();
		return false;
	}
	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
	return true;
}

bool CSVNProgressDlg::CmdResolve(CString& sWindowTitle, bool& localoperation)
{
	localoperation = true;
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_RESOLVE);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_RESOLVE_BKG);
	// check if the file may still have conflict markers in it.
	BOOL bMarkers = FALSE;
	if ((m_options & ProgOptSkipConflictCheck) == 0)
	{
		try
		{
			for (INT_PTR fileindex=0; (fileindex<m_targetPathList.GetCount()) && (bMarkers==FALSE); ++fileindex)
			{
				CTSVNPath targetPath = m_targetPathList[fileindex];
				bool doCheck = true;
				if (targetPath.Exists() && !targetPath.IsDirectory())	// only check existing files
				{
					if (targetPath.GetFileSize() < 100*1024)			// only check files smaller than 100kBytes
					{
						SVNProperties props(targetPath, SVNRev::REV_WC, false);
						for (int i=0; i<props.GetCount(); i++)
						{
							if (props.GetItemName(i).compare(SVN_PROP_MIME_TYPE)==0)
							{
								CString mimetype = CUnicodeUtils::GetUnicode((char *)props.GetItemValue(i).c_str());
								if ((!mimetype.IsEmpty())&&(mimetype.Left(4).CompareNoCase(_T("text"))))
								{
									doCheck = false;	// do not check files with a non-text mimetype
									break;
								}
							}
						}
						if (doCheck)
						{
							CStdioFile file(targetPath.GetWinPath(), CFile::typeBinary | CFile::modeRead);
							CString strLine = _T("");
							while (file.ReadString(strLine))
							{
								if (strLine.Find(_T("<<<<<<<"))==0)
								{
									bMarkers = TRUE;
									break;
								}
							}
							file.Close();
						}
					}
				}
			}
		} 
		catch (CFileException* pE)
		{
			TRACE(_T("CFileException in Resolve!\n"));
			pE->Delete();
		}
	}

	UINT showRet = IDYES;	// default to yes
	if (bMarkers)
	{
		showRet = CMessageBox::Show(m_hWnd, IDS_PROGRS_REVERTMARKERS, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION);
	}
	if (showRet == IDYES)
	{
		ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_RESOLVE)));
		for (INT_PTR fileindex=0; fileindex<m_targetPathList.GetCount(); ++fileindex)
		{
			if (!Resolve(m_targetPathList[fileindex], svn_wc_conflict_choose_merged, true))
			{
				ReportSVNError();
			}
		}
	}
	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
	return true;
}

bool CSVNProgressDlg::CmdRevert(CString& sWindowTitle, bool& localoperation)
{
	localoperation = true;
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_REVERT);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_REVERT_BKG);

	CTSVNPathList delList = m_selectedPaths;
	if (DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\RevertWithRecycleBin"), TRUE)))
		delList.DeleteAllPaths(true, true);

	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_REVERT)));
	if (!Revert(m_targetPathList, CStringArray(), (m_options & ProgOptRecursive)!=0))
	{
		ReportSVNError();
		return false;
	}
	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
	return true;
}

bool CSVNProgressDlg::CmdSwitch(CString& sWindowTitle, bool& /*localoperation*/)
{
	ASSERT(m_targetPathList.GetCount() == 1);
	SVNStatus st;
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_SWITCH);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_SWITCH_BKG);
	LONG rev = 0;
	if (st.GetStatus(m_targetPathList[0]) != (-2))
	{
		if (st.status->entry != NULL)
		{
			rev = st.status->entry->revision;
		}
	}

	CString sCmdInfo;
	sCmdInfo.Format(IDS_PROGRS_CMD_SWITCH, 
		m_targetPathList[0].GetWinPath(), (LPCTSTR)m_url.GetSVNPathString(),
		(LPCTSTR)m_Revision.ToString());
	ReportCmd(sCmdInfo);

	bool depthIsSticky = true;
	if (m_depth == svn_depth_unknown)
		depthIsSticky = false;
	SendCacheCommand(TSVNCACHECOMMAND_BLOCK, m_targetPathList[0].GetWinPath());
	if (!Switch(m_targetPathList[0], m_url, m_Revision, m_Revision, m_depth, depthIsSticky, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\AllowUnversionedObstruction"), true))))
	{
		ReportSVNError();
		SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
		return false;
	}
	SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
	m_UpdateStartRevMap[m_targetPathList[0].GetSVNApiPath(pool)] = rev;
	if ((m_RevisionEnd >= 0)&&(rev >= 0)
		&&((LONG)m_RevisionEnd > (LONG)rev))
	{
		GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
	}
	return true;
}

bool CSVNProgressDlg::CmdUnlock(CString& sWindowTitle, bool& /*localoperation*/)
{
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_UNLOCK);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_UNLOCK_BKG);
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_UNLOCK)));
	if (!Unlock(m_targetPathList, (m_options & ProgOptForce) != 0))
	{
		ReportSVNError();
		return false;
	}
	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
	return true;
}

bool CSVNProgressDlg::CmdUpdate(CString& sWindowTitle, bool& /*localoperation*/)
{
	sWindowTitle.LoadString(IDS_PROGRS_TITLE_UPDATE);
	SetWindowText(sWindowTitle);
	SetBackgroundImage(IDI_UPDATE_BKG);

	int targetcount = m_targetPathList.GetCount();
	CString sfile;
	CStringA uuid;
	StringRevMap uuidmap;
	SVNRev revstore = m_Revision;
	int nUUIDs = 0;
	for(int nItem = 0; nItem < targetcount; nItem++)
	{
		const CTSVNPath& targetPath = m_targetPathList[nItem];
		SVNStatus st;
		LONG headrev = -1;
		m_Revision = revstore;
		if (m_Revision.IsHead())
		{
			if ((targetcount > 1)&&((headrev = st.GetStatus(targetPath, true)) != (-2)))
			{
				if (st.status->entry != NULL)
				{

					m_UpdateStartRevMap[targetPath.GetSVNApiPath(pool)] = st.status->entry->cmt_rev;
					if (st.status->entry->uuid)
					{
						uuid = st.status->entry->uuid;
						StringRevMap::iterator iter = uuidmap.lower_bound(uuid);
						if (iter == uuidmap.end() || iter->first != uuid)
						{
							uuidmap.insert(iter, std::make_pair(uuid, headrev));
							nUUIDs++;
						}
						else
							headrev = iter->second;
						m_Revision = headrev;
					}
					else
						m_Revision = headrev;
				}
			}
			else
			{
				if ((headrev = st.GetStatus(targetPath, FALSE)) != (-2))
				{
					if (st.status->entry != NULL)
						m_UpdateStartRevMap[targetPath.GetSVNApiPath(pool)] = st.status->entry->cmt_rev;
				}
			}
			if (uuidmap.size() > 1)
				m_Revision = SVNRev::REV_HEAD;
		} // if (m_Revision.IsHead()) 
	} // for(int nItem = 0; nItem < targetcount; nItem++) 
	sWindowTitle = m_targetPathList.GetCommonRoot().GetWinPathString()+_T(" - ")+sWindowTitle;
	SetWindowText(sWindowTitle);

	DWORD exitcode = 0;
	CString error;
	if (CHooks::Instance().PreUpdate(m_targetPathList, m_depth, nUUIDs > 1 ? revstore : m_Revision, exitcode, error))
	{
		if (exitcode)
		{
			ReportHookFailed(error);
			return false;
		}
	}
	ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_UPDATE)));
	if (nUUIDs > 1)
	{
		// the selected items are from different repositories,
		// so we have to update them separately
		for(int nItem = 0; nItem < targetcount; nItem++)
		{
			const CTSVNPath& targetPath = m_targetPathList[nItem];
			m_basePath = targetPath;
			CString sNotify;
			sNotify.Format(IDS_PROGRS_UPDATEPATH, m_basePath.GetWinPath());
			ReportString(sNotify, CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
			SendCacheCommand(TSVNCACHECOMMAND_BLOCK, targetPath.GetWinPath());
			if (!Update(CTSVNPathList(targetPath), revstore, m_depth, true, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\AllowUnversionedObstruction"), true))))
			{
				ReportSVNError();
				SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, targetPath.GetWinPath());
				return false;
			}
			SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, targetPath.GetWinPath());
		}
	}
	else 
	{
		// if we have a target path, but that target path does not exist,
		// we have to check whether at least the parent path exists. If not,
		// then we have to update all paths in between the first path that exists and the
		// parent path of the one we want to update
		// This is required so a user can create a sparse checkout without having
		// to update all intermediate folders manually
		for (int pathindex = 0; pathindex < m_targetPathList.GetCount(); ++pathindex)
		{
			if (!m_targetPathList[pathindex].Exists())
			{
				CTSVNPath wcPath = m_targetPathList[pathindex].GetContainingDirectory();
				CTSVNPath existingParentPath = wcPath.GetContainingDirectory();
				while (!existingParentPath.Exists() && (existingParentPath.GetWinPathString().GetLength() > 2))
				{
					existingParentPath = existingParentPath.GetContainingDirectory();
				}
				if (existingParentPath.GetWinPathString().GetLength() && !existingParentPath.IsEquivalentTo(wcPath))
				{
					// update all intermediate directories with depth 'empty'
					CTSVNPath intermediatepath = existingParentPath;
					bool bSuccess = true;
					while (bSuccess && intermediatepath.IsAncestorOf(wcPath) && !intermediatepath.IsEquivalentTo(wcPath))
					{
						CString childname = wcPath.GetWinPathString().Mid(intermediatepath.GetWinPathString().GetLength(),
							wcPath.GetWinPathString().Find('\\', intermediatepath.GetWinPathString().GetLength()+1)-intermediatepath.GetWinPathString().GetLength());
						if (childname.IsEmpty())
							intermediatepath = wcPath;
						else
							intermediatepath.AppendPathString(childname);
						bSuccess = Update(CTSVNPathList(intermediatepath), m_Revision, svn_depth_empty, false, true, !!DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\AllowUnversionedObstruction"), true)));
					}

					if (!bSuccess)
					{
						ReportSVNError();
						return false;
					}
				}
			}
		}
		SendCacheCommand(TSVNCACHECOMMAND_BLOCK, m_targetPathList[0].GetWinPath());
		if (!Update(m_targetPathList, m_Revision, m_depth, true, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\AllowUnversionedObstruction"), true))))
		{
			ReportSVNError();
			SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
			return false;
		}
		SendCacheCommand(TSVNCACHECOMMAND_UNBLOCK, m_targetPathList[0].GetWinPath());
	}
	if (CHooks::Instance().PostUpdate(m_targetPathList, m_depth, m_RevisionEnd, exitcode, error))
	{
		if (exitcode)
		{
			ReportHookFailed(error);
			return false;
		}
	}

	// after an update, show the user the log button, but only if only one single item was updated
	// (either a file or a directory)
	if ((m_targetPathList.GetCount() == 1)&&(m_UpdateStartRevMap.size()>0))
		GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);

	return true;
}

void CSVNProgressDlg::OnBnClickedNoninteractive()
{
	LRESULT res = ::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_GETCHECK, 0, 0);
	m_AlwaysConflicted = (res == BST_CHECKED);
	CRegDWORD nonint = CRegDWORD(_T("Software\\TortoiseSVN\\MergeNonInteractive"), FALSE);
	nonint = m_AlwaysConflicted;
}

CString CSVNProgressDlg::GetPathFromColumnText(const CString& sColumnText)
{
	// First check if the text in the column actually *is* already
	// a valid path
	if (PathFileExists(sColumnText))
	{
		return sColumnText;
	}
	CString sPath = CPathUtils::ParsePathInString(sColumnText);
	if (sPath.Find(':')<0)
	{
		// the path is not absolute: add the common root of all paths to it
		sPath = m_targetPathList.GetCommonRoot().GetDirectory().GetWinPathString() + _T("\\") + CPathUtils::ParsePathInString(sColumnText);
	}
	return sPath;
}

LRESULT CSVNProgressDlg::OnTaskbarBtnCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	return 0;
}

bool CSVNProgressDlg::IsCommittingToTag()
{
	bool isTag = false;
	bool bURLFetched = false;
	CString url;
	for (int i=0; i<m_targetPathList.GetCount(); ++i)
	{
		if (bURLFetched)
			continue;

		url = GetURLFromPath(m_targetPathList[i]);
		if (!url.IsEmpty())
			bURLFetched = true;
		CString urllower = url;
		urllower.MakeLower();
		// test if the commit goes to a tag.
		// now since Subversion doesn't force users to
		// create tags in the recommended /tags/ folder
		// only a warning is shown. This won't work if the tags
		// are stored in a non-recommended place, but the check
		// still helps those who do.
		CRegString regTagsPattern (_T("Software\\TortoiseSVN\\RevisionGraph\\TagsPattern"), _T("tags"));
		CString sTags = regTagsPattern;
		int pos = 0;
		CString temp;
		while (!isTag)
		{
			temp = sTags.Tokenize(_T(";"), pos);
			if (temp.IsEmpty())
				break;

			int urlpos = 0;
			CString temp2;
			for(;;)
			{
				temp2 = urllower.Tokenize(_T("/"), urlpos);
				if (temp2.IsEmpty())
					break;

				if (CStringUtils::WildCardMatch(temp, temp2))
				{
					isTag = true;
					break;
				}
			}
		} 
		break;
	}
	return isTag;
}