﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2020-2021 - TortoiseSVN

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
#include "SVNConfig.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "SVN.h"
#include "registry.h"
#include "SVNStatus.h"
#include "HistoryDlg.h"
#include "Hooks.h"
#include "COMError.h"
#include "../version.h"
#include "BstrSafeVector.h"
#include "SmartHandle.h"
#include "DPIAware.h"

#ifdef _DEBUG
// ReSharper disable once CppInconsistentNaming
#    define new DEBUG_NEW
#    undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ReSharper disable once CppInconsistentNaming
UINT CCommitDlg::WM_AUTOLISTREADY = RegisterWindowMessage(L"TORTOISESVN_AUTOLISTREADY_MSG");

IMPLEMENT_DYNAMIC(CCommitDlg, CResizableStandAloneDialog)
CCommitDlg::CCommitDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CCommitDlg::IDD, pParent)
    , m_bRecursive(FALSE)
    , m_bUnchecked(false)
    , m_bKeepLocks(FALSE)
    , m_bKeepChangeList(TRUE)
    , m_itemsCount(0)
    , m_bSelectFilesForCommit(TRUE)
    , m_pThread(nullptr)
    , m_bShowUnversioned(FALSE)
    , m_bShowExternals(FALSE)
    , m_bBlock(FALSE)
    , m_bThreadRunning(FALSE)
    , m_bRunThread(FALSE)
    , m_nPopupPasteListCmd(0)
    , m_bCancelled(FALSE)
{
}

CCommitDlg::~CCommitDlg()
{
    delete m_pThread;
}

void CCommitDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_FILELIST, m_listCtrl);
    DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
    DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
    DDX_Check(pDX, IDC_SHOWEXTERNALS, m_bShowExternals);
    DDX_Text(pDX, IDC_BUGID, m_sBugID);
    DDX_Check(pDX, IDC_KEEPLOCK, m_bKeepLocks);
    DDX_Control(pDX, IDC_SPLITTER, m_wndSplitter);
    DDX_Check(pDX, IDC_KEEPLISTS, m_bKeepChangeList);
    DDX_Control(pDX, IDC_COMMIT_TO, m_commitTo);
    DDX_Control(pDX, IDC_NEWVERSIONLINK, m_cUpdateLink);
    DDX_Control(pDX, IDC_CHECKALL, m_checkAll);
    DDX_Control(pDX, IDC_CHECKNONE, m_checkNone);
    DDX_Control(pDX, IDC_CHECKUNVERSIONED, m_checkUnversioned);
    DDX_Control(pDX, IDC_CHECKVERSIONED, m_checkVersioned);
    DDX_Control(pDX, IDC_CHECKADDED, m_checkAdded);
    DDX_Control(pDX, IDC_CHECKDELETED, m_checkDeleted);
    DDX_Control(pDX, IDC_CHECKMODIFIED, m_checkModified);
    DDX_Control(pDX, IDC_CHECKFILES, m_checkFiles);
    DDX_Control(pDX, IDC_CHECKDIRECTORIES, m_checkDirectories);
}

BEGIN_MESSAGE_MAP(CCommitDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
    ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
    ON_BN_CLICKED(IDC_BUGTRAQBUTTON, OnBnClickedBugtraqbutton)
    ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED, OnSVNStatusListCtrlItemCountChanged)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_CHECKCHANGED, &CCommitDlg::OnSVNStatusListCtrlCheckChanged)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_CHANGELISTCHANGED, &CCommitDlg::OnSVNStatusListCtrlChangelistChanged)
    ON_REGISTERED_MESSAGE(CLinkControl::LK_LINKITEMCLICKED, &CCommitDlg::OnCheck)
    ON_REGISTERED_MESSAGE(WM_AUTOLISTREADY, OnAutoListReady)
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_STN_CLICKED(IDC_EXTERNALWARNING, &CCommitDlg::OnStnClickedExternalwarning)
    ON_BN_CLICKED(IDC_SHOWEXTERNALS, &CCommitDlg::OnBnClickedShowexternals)
    ON_BN_CLICKED(IDC_LOG, &CCommitDlg::OnBnClickedLog)
    ON_WM_QUERYENDSESSION()
    ON_BN_CLICKED(IDC_RUNHOOK, &CCommitDlg::OnBnClickedRunhook)
END_MESSAGE_MAP()

BOOL CCommitDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);
    ReadPersistedDialogSettings();
    ExtendFrameIntoClientArea(IDC_DWM);
    SubclassControls();
    UpdateData(FALSE);
    InitializeListControl();
    InitializeLogMessageControl();
    SetControlAccessibilityProperties();
    SetupToolTips();
    HideAndDisableBugTraqButton();
    SetupBugTraqControlsIfConfigured();
    ShowOrHideBugIdAndLabelControls();
    VersionCheck();
    SetupLogMessageDefaultText();
    SetCommitWindowTitleAndEnableStatus();
    AdjustControlSizes();
    LineupControlsAndAdjustSizes();
    SaveDialogAndLogMessageControlRectangles();
    AddAnchorsToFacilitateResizing();
    CenterWindowWhenLaunchedFromExplorer();
    AdjustDialogSizeAndPanes();
    AddDirectoriesToPathWatcher();
    StartStatusThread();
    ShowBalloonInCaseOfError();
    GetDlgItem(IDC_RUNHOOK)->ShowWindow(CHooks::Instance().IsHookPresent(Manual_Precommit, m_pathList) ? SW_SHOW : SW_HIDE);
    return FALSE; // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CCommitDlg::OnOK()
{
    if (m_bBlock)
        return;
    if (m_bThreadRunning)
    {
        m_bCancelled = true;
        StopStatusThread();
    }
    CString id;
    GetDlgItemText(IDC_BUGID, id);
    id.Trim(L"\n\r");
    if (!m_projectProperties.CheckBugID(id))
    {
        ShowEditBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, TTI_ERROR);
        return;
    }
    m_sLogMessage = m_cLogMessage.GetText();
    if ((m_projectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_projectProperties.HasBugID(m_sLogMessage)))
    {
        CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK1)),
                            CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK3)));
        taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK4)));
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetDefaultCommandControl(2);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        if (taskDlg.DoModal(m_hWnd) != 100)
            return;
    }

    CRegDWORD regUnversionedRecurse(L"Software\\TortoiseSVN\\UnversionedRecurse", TRUE);
    if (!static_cast<DWORD>(regUnversionedRecurse))
    {
        // Find unversioned directories which are marked for commit. The user might expect them
        // to be added recursively since he cannot see the files. Let's ask the user if he knows
        // what he is doing.
        int  nListItems        = m_listCtrl.GetItemCount();
        bool bCheckUnversioned = true;
        for (int j = 0; j < nListItems; j++)
        {
            const CSVNStatusListCtrl::FileEntry* entry = m_listCtrl.GetConstListEntry(j);
            if (bCheckUnversioned && entry->IsChecked() && (entry->status == svn_wc_status_unversioned) && entry->IsFolder())
            {
                CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK1)),
                                    CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK3)));
                taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK4)));
                taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskDlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK5)));
                taskDlg.SetVerificationCheckbox(false);
                taskDlg.SetDefaultCommandControl(2);
                taskDlg.SetMainIcon(TD_WARNING_ICON);
                if (taskDlg.DoModal(m_hWnd) != 100)
                    return;
                if (taskDlg.GetVerificationCheckboxState())
                    bCheckUnversioned = false;
            }
        }
    }

    m_pathWatcher.Stop();
    InterlockedExchange(&m_bBlock, TRUE);
    CDWordArray arDeleted;
    //first add all the unversioned files the user selected
    //and check if all versioned files are selected
    int nUnchecked = 0;
    m_bUnchecked   = false;
    m_bRecursive   = true;
    int nListItems = m_listCtrl.GetItemCount();

    CTSVNPathList itemsToAdd;
    CTSVNPathList itemsToRemove;
    CTSVNPathList itemsToUnlock;

    bool              bCheckedInExternal = false;
    bool              bHasConflicted     = false;
    bool              bHasDirCopyPlus    = false;
    std::set<CString> checkedLists;
    std::set<CString> uncheckedLists;
    m_restorePaths.clear();
    m_restorePaths             = m_listCtrl.GetRestorePaths();
    bool bAllowPeggedExternals = !static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\BlockPeggedExternals", TRUE));
    for (int j = 0; j < nListItems; j++)
    {
        const CSVNStatusListCtrl::FileEntry* entry = m_listCtrl.GetConstListEntry(j);
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
            if (entry->IsPeggedExternal())
            {
                bCheckedInExternal = true;
            }
            if (entry->IsLocked() && entry->status == svn_wc_status_normal)
            {
                itemsToUnlock.AddPath(entry->GetPath());
            }
            if (entry->IsCopied() && entry->IsFolder())
            {
                bHasDirCopyPlus = true;
            }
            checkedLists.insert(entry->GetChangeList());
        }
        else
        {
            if ((entry->status != svn_wc_status_unversioned) &&
                (entry->status != svn_wc_status_ignored) &&
                (!entry->IsFromDifferentRepository()))
            {
                nUnchecked++;
                uncheckedLists.insert(entry->GetChangeList());
                if (m_bRecursive)
                {
                    // items which are not modified, i.e. won't get committed can
                    // still be shown in a changelist, e.g. the 'don't commit' changelist.
                    if ((entry->status > svn_wc_status_normal) ||
                        (entry->propStatus > svn_wc_status_normal) ||
                        (entry->IsCopied()))
                    {
                        // items that can't be checked don't count as unchecked.
                        if (!(static_cast<DWORD>(m_regShowExternals) && (entry->IsFromDifferentRepository() || entry->IsNested() || (!bAllowPeggedExternals && entry->IsPeggedExternal()))))
                            m_bUnchecked = true;
                        // This algorithm is for the sake of simplicity of the complexity O(N²)
                        for (int k = 0; k < nListItems; k++)
                        {
                            const CSVNStatusListCtrl::FileEntry* entryK = m_listCtrl.GetConstListEntry(k);
                            if (entryK->IsChecked() && entryK->GetPath().IsAncestorOf(entry->GetPath()))
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
    }
    if (m_pathWatcher.LimitReached())
        m_bRecursive = false;
    else if (m_pathWatcher.GetNumberOfChangedPaths() && m_bRecursive)
    {
        // There are paths which got changed (touched at least).
        // We have to find out if this affects the selection in the commit dialog
        // If it could affect the selection, revert back to a non-recursive commit
        CTSVNPathList changedList = m_pathWatcher.GetChangedPaths();
        changedList.RemoveDuplicates();
        for (int i = 0; i < changedList.GetCount(); ++i)
        {
            if (changedList[i].IsAdminDir())
            {
                // something inside an admin dir was changed.
                // if it's the entries file, then we have to fully refresh because
                // files may have been added/removed from version control
                m_bRecursive = false;
                break;
            }
            else if (!m_listCtrl.IsPathShown(changedList[i]))
            {
                // a path which is not shown in the list has changed
                const CSVNStatusListCtrl::FileEntry* entry = m_listCtrl.GetConstListEntry(changedList[i]);
                if (entry)
                {
                    // check if the changed path would get committed by a recursive commit
                    if ((!entry->IsFromDifferentRepository()) &&
                        (!entry->IsInExternal()) &&
                        (!entry->IsNested()) &&
                        (!entry->IsChecked()) &&
                        (entry->IsVersioned()))
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
    if (!svn.Add(itemsToAdd, &m_projectProperties, svn_depth_empty, true, true, false, true))
    {
        svn.ShowErrorDialog(m_hWnd);
        InterlockedExchange(&m_bBlock, FALSE);
        Refresh();
        return;
    }

    // Remove any missing items
    // Not sure that this sort is really necessary - indeed, it might be better to do a reverse sort at this point
    itemsToRemove.SortByPathname();
    svn.Remove(itemsToRemove, true);

    //the next step: find all deleted files and check if they're
    //inside a deleted folder. If that's the case, then remove those
    //files from the list since they'll get deleted by the parent
    //folder automatically.
    m_listCtrl.BusyCursor(true);
    INT_PTR nDeleted = arDeleted.GetCount();
    for (INT_PTR i = 0; i < arDeleted.GetCount(); i++)
    {
        if (!m_listCtrl.GetCheck(arDeleted.GetAt(i)))
            continue;
        const CTSVNPath& path = m_listCtrl.GetConstListEntry(arDeleted.GetAt(i))->GetPath();
        if (!path.IsDirectory())
            continue;
        //now find all children of this directory
        for (int j = 0; j < arDeleted.GetCount(); j++)
        {
            if (i == j)
                continue;
            const CSVNStatusListCtrl::FileEntry* childEntry = m_listCtrl.GetConstListEntry(arDeleted.GetAt(j));
            if (childEntry->IsChecked())
            {
                if (path.IsAncestorOf(childEntry->GetPath()))
                {
                    m_listCtrl.SetEntryCheck(arDeleted.GetAt(j), false);
                    nDeleted--;
                }
            }
        }
    }
    m_listCtrl.BusyCursor(false);

    if ((nUnchecked != 0) || (bCheckedInExternal) || (bHasConflicted) || (!m_bRecursive))
    {
        //save only the files the user has checked into the temporary file
        m_listCtrl.WriteCheckedNamesToPathList(m_pathList);
    }
    m_listCtrl.WriteCheckedNamesToPathList(m_selectedPathList);
    // the item count is used in the progress dialog to show the overall commit
    // progress.
    // deleted items only send one notification event, all others send two
    m_itemsCount = ((m_selectedPathList.GetCount() - nDeleted - itemsToRemove.GetCount()) * 2) + nDeleted + itemsToRemove.GetCount();

    if ((m_bRecursive) && (checkedLists.size() == 1))
    {
        // all checked items belong to the same changelist
        // find out if there are any unchecked items which belong to that changelist
        if (uncheckedLists.find(*checkedLists.begin()) == uncheckedLists.end())
            m_sChangeList = *checkedLists.begin();
    }

    if ((!m_bRecursive) && (bHasDirCopyPlus))
    {
        CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_COMMITDLG_COPYDEPTH_TASK1)),
                            CString(MAKEINTRESOURCE(IDS_COMMITDLG_COPYDEPTH_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_COMMITDLG_COPYDEPTH_TASK3)));
        taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_COMMITDLG_COPYDEPTH_TASK4)));
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetDefaultCommandControl(1);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        if (taskDlg.DoModal(m_hWnd) != 100)
        {
            InterlockedExchange(&m_bBlock, FALSE);
            return;
        }
    }

    UpdateData();
    // Release all locks to unchanged items - they won't be released by commit.
    if (!m_bKeepLocks && !svn.Unlock(itemsToUnlock, false))
    {
        svn.ShowErrorDialog(m_hWnd);
        InterlockedExchange(&m_bBlock, FALSE);
        Refresh();
        return;
    }

    m_regAddBeforeCommit = m_bShowUnversioned;
    m_regShowExternals   = m_bShowExternals;
    if (!GetDlgItem(IDC_KEEPLOCK)->IsWindowEnabled())
        m_bKeepLocks = FALSE;
    m_regKeepChangelists = m_bKeepChangeList;
    if (!GetDlgItem(IDC_KEEPLISTS)->IsWindowEnabled())
        m_bKeepChangeList = FALSE;
    InterlockedExchange(&m_bBlock, FALSE);
    m_sBugID.Trim();
    CString sExistingBugID = m_projectProperties.FindBugID(m_sLogMessage);
    sExistingBugID.Trim();
    if (!m_sBugID.IsEmpty() && m_sBugID.Compare(sExistingBugID))
    {
        m_sBugID.Replace(L", ", L",");
        m_sBugID.Replace(L" ,", L",");
        CString sBugID = m_projectProperties.sMessage;
        sBugID.Replace(L"%BUGID%", m_sBugID);
        if (m_projectProperties.bAppend)
            m_sLogMessage += L"\n" + sBugID + L"\n";
        else
            m_sLogMessage = sBugID + L"\n" + m_sLogMessage;
    }

    // now let the bugtraq plugin check the commit message
    CComPtr<IBugTraqProvider2> pProvider2 = nullptr;
    if (m_bugTraqProvider)
    {
        HRESULT hr = m_bugTraqProvider.QueryInterface(&pProvider2);
        if (SUCCEEDED(hr))
        {
            ATL::CComBSTR temp;
            CString       common = m_listCtrl.GetCommonURL(true).GetSVNPathString();
            ATL::CComBSTR repositoryRoot;
            repositoryRoot.Attach(common.AllocSysString());
            ATL::CComBSTR parameters;
            parameters.Attach(m_bugtraqAssociation.GetParameters().AllocSysString());
            ATL::CComBSTR commonRoot(m_selectedPathList.GetCommonRoot().GetDirectory().GetWinPath());
            ATL::CComBSTR commitMessage;
            commitMessage.Attach(m_sLogMessage.AllocSysString());
            CBstrSafeVector pathList(m_selectedPathList.GetCount());

            for (LONG index = 0; index < m_selectedPathList.GetCount(); ++index)
                pathList.PutElement(index, m_selectedPathList[index].GetSVNPathString());

            hr = pProvider2->CheckCommit(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, commitMessage, &temp);
            if (FAILED(hr))
            {
                OnComError(hr);
            }
            else
            {
                CString sError = temp == 0 ? L"" : static_cast<LPCWSTR>(temp);
                if (!sError.IsEmpty())
                {
                    CAppUtils::ReportFailedHook(m_hWnd, sError);
                    return;
                }
            }
        }
    }

    // run the check-commit hook script
    CTSVNPathList checkedItems;
    m_listCtrl.WriteCheckedNamesToPathList(checkedItems);
    DWORD   exitCode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(checkedItems.GetCommonRoot(), m_projectProperties);
    if (CHooks::Instance().CheckCommit(m_hWnd, checkedItems, m_sLogMessage, exitCode, error))
    {
        if (exitCode)
        {
            CString sErrorMsg;
            sErrorMsg.Format(IDS_HOOK_ERRORMSG, static_cast<LPCWSTR>(error));

            CTaskDialog taskDlg(sErrorMsg,
                                CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK3)));
            taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK4)));
            taskDlg.SetDefaultCommandControl(1);
            taskDlg.SetMainIcon(TD_ERROR_ICON);
            bool doIt = (taskDlg.DoModal(GetSafeHwnd()) != 200);

            if (doIt)
            {
                // parse the error message and select all mentioned paths if there are any
                // the paths must be separated by newlines!
                CTSVNPathList errorPaths;
                error.Replace(L"\r\n", L"*");
                error.Replace(L"\n", L"*");
                errorPaths.LoadFromAsteriskSeparatedString(error);
                m_listCtrl.SetSelectionMark(-1);
                for (int i = 0; i < errorPaths.GetCount(); ++i)
                {
                    CTSVNPath errorPath       = errorPaths[i];
                    int       nListItemsCount = m_listCtrl.GetItemCount();
                    for (int j = 0; j < nListItemsCount; j++)
                    {
                        const CSVNStatusListCtrl::FileEntry* entry = m_listCtrl.GetConstListEntry(j);
                        if (entry->GetPath().IsEquivalentTo(errorPath))
                        {
                            m_listCtrl.SetItemState(j, TVIS_SELECTED, TVIS_SELECTED);
                            break;
                        }
                    }
                }
                return;
            }
        }
    }

    if (!m_sLogMessage.IsEmpty())
    {
        m_history.AddEntry(m_sLogMessage);
        m_history.Save();
    }

    SaveSplitterPos();

    CResizableStandAloneDialog::OnOK();
}

void CCommitDlg::SaveSplitterPos() const
{
    if (!IsIconic())
    {
        CRegDWORD regPos(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\CommitDlgSizer");
        RECT      rectSplitter;
        m_wndSplitter.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        regPos = rectSplitter.top;
    }
}

UINT CCommitDlg::StatusThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashThread;
    return static_cast<CCommitDlg*>(pVoid)->StatusThread();
}

UINT CCommitDlg::StatusThread()
{
    //get the status of all selected file/folders recursively
    //and show the ones which have to be committed to the user
    //in a list control.
    m_bCancelled = false;
    CoInitialize(nullptr);
    OnOutOfScope(CoUninitialize());

    DialogEnableWindow(IDOK, false);
    DialogEnableWindow(IDC_SHOWUNVERSIONED, false);
    GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_HIDE);
    DialogEnableWindow(IDC_EXTERNALWARNING, false);

    DialogEnableWindow(IDC_CHECKALL, false);
    DialogEnableWindow(IDC_CHECKNONE, false);
    DialogEnableWindow(IDC_CHECKUNVERSIONED, false);
    DialogEnableWindow(IDC_CHECKVERSIONED, false);
    DialogEnableWindow(IDC_CHECKADDED, false);
    DialogEnableWindow(IDC_CHECKDELETED, false);
    DialogEnableWindow(IDC_CHECKMODIFIED, false);
    DialogEnableWindow(IDC_CHECKFILES, false);
    DialogEnableWindow(IDC_CHECKDIRECTORIES, false);

    // read the list of recent log entries before querying the WC for status
    // -> the user may select one and modify / update it while we are crawling the WC
    if (m_history.GetCount() == 0)
    {
        CString reg;
        SVN     svn;
        reg.Format(L"Software\\TortoiseSVN\\History\\commit%s", static_cast<LPCWSTR>(svn.GetUUIDFromPath(m_pathList[0])));
        m_history.Load(reg, L"logmsgs");
    }

    // Initialize the list control with the status of the files/folders below us
    BOOL success = m_listCtrl.GetStatus(m_pathList);
    m_listCtrl.CheckIfChangelistsArePresent(false);

    DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWLOCKS | SVNSLC_SHOWINCHANGELIST | SVNSLC_SHOWEXTERNAL | SVNSLC_SHOWINEXTERNALS;
    dwShow |= static_cast<DWORD>(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
    dwShow |= static_cast<DWORD>(m_regShowExternals) ? SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO | SVNSLC_SHOWEXTDISABLED : 0;
    if (success)
    {
        m_cLogMessage.SetRepositoryRoot(m_listCtrl.m_sRepositoryRoot);

        if (m_checkedPathList.GetCount())
            m_listCtrl.Show(dwShow, m_checkedPathList, 0, true, true);
        else
        {
            DWORD dwCheck = m_bSelectFilesForCommit ? SVNSLC_SHOWDIRECTS | SVNSLC_SHOWMODIFIED | SVNSLC_SHOWADDED | SVNSLC_SHOWADDEDHISTORY | SVNSLC_SHOWREMOVED | SVNSLC_SHOWREPLACED | SVNSLC_SHOWMERGED | SVNSLC_SHOWLOCKS : 0;
            m_listCtrl.Show(dwShow, CTSVNPathList(), dwCheck, true, true);
            m_bSelectFilesForCommit = true;
        }

        if (m_listCtrl.HasExternalsFromDifferentRepos())
        {
            GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_SHOW);
            DialogEnableWindow(IDC_EXTERNALWARNING, TRUE);
        }
        m_commitTo.SetBold();
        m_commitTo.SetWindowText(m_listCtrl.m_sURL);
        m_tooltips.AddTool(GetDlgItem(IDC_STATISTICS), m_listCtrl.GetStatisticsString());

        {
            // compare all url parts against the branches pattern and if it matches,
            // change the background image of the list control to indicate
            // a commit to a branch
            CRegString branchesPattern(L"Software\\TortoiseSVN\\RevisionGraph\\BranchPattern", L"branches");
            CString    sBranches = branchesPattern;
            int        pos       = 0;
            bool       bFound    = false;
            while (!bFound)
            {
                CString temp = sBranches.Tokenize(L";", pos);
                if (temp.IsEmpty())
                    break;

                int     urlPos = 0;
                CString temp2;
                for (;;)
                {
                    temp2 = m_listCtrl.m_sURL.Tokenize(L"/", urlPos);
                    if (temp2.IsEmpty())
                        break;

                    if (CStringUtils::WildCardMatch(temp, temp2))
                    {
                        m_listCtrl.SetBackgroundImage(IDI_COMMITBRANCHES_BKG);
                        bFound = true;
                        break;
                    }
                }
            }
        }
    }
    if (!success)
    {
        if (!m_listCtrl.GetLastErrorMessage().IsEmpty())
            m_listCtrl.SetEmptyString(m_listCtrl.GetLastErrorMessage());
        m_listCtrl.Show(dwShow, CTSVNPathList(), 0, true, true);
    }

    CTSVNPath commonDir = m_listCtrl.GetCommonDirectory(false);
    CAppUtils::SetWindowTitle(m_hWnd, commonDir.GetWinPathString(), m_sWindowTitle);

    // we don't have to block the commit dialog while we fetch the
    // auto completion list.
    m_pathWatcher.ClearChangedPaths();
    InterlockedExchange(&m_bBlock, FALSE);
    UpdateOKButton();
    std::map<CString, int> autolist;
    if (static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\Autocompletion", TRUE)) == TRUE)
    {
        m_listCtrl.BusyCursor(true);
        GetAutocompletionList(autolist);
        m_listCtrl.BusyCursor(false);
    }
    if (m_bRunThread)
    {
        DialogEnableWindow(IDC_BUGTRAQBUTTON, TRUE);
        DialogEnableWindow(IDC_SHOWUNVERSIONED, true);
        if (m_listCtrl.HasChangeLists())
            DialogEnableWindow(IDC_KEEPLISTS, true);
        if (m_listCtrl.HasExternalsFromDifferentRepos())
            DialogEnableWindow(IDC_SHOWEXTERNALS, true);
        if (m_listCtrl.HasLocks())
            DialogEnableWindow(IDC_KEEPLOCK, true);

        UpdateCheckLinks();
        // we have the list, now signal the main thread about it
        SendMessage(WM_AUTOLISTREADY, 0, reinterpret_cast<LPARAM>(&autolist)); // only send the message if the thread wasn't told to quit!
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
    m_pathWatcher.Stop();
    if (m_bThreadRunning)
    {
        StopStatusThread();
    }
    UpdateData();
    m_sBugID.Trim();
    m_sLogMessage = m_cLogMessage.GetText();
    if (!m_sBugID.IsEmpty())
    {
        m_sBugID.Replace(L", ", L",");
        m_sBugID.Replace(L" ,", L",");
        CString sBugID = m_projectProperties.sMessage;
        sBugID.Replace(L"%BUGID%", m_sBugID);
        if (m_projectProperties.bAppend)
            m_sLogMessage += L"\n" + sBugID + L"\n";
        else
            m_sLogMessage = sBugID + L"\n" + m_sLogMessage;
    }
    if ((m_projectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATECOMMIT).Compare(m_sLogMessage) != 0) &&
        !m_sLogMessage.IsEmpty())
    {
        m_history.AddEntry(m_sLogMessage);
        m_history.Save();
    }
    m_restorePaths = m_listCtrl.GetRestorePaths();
    if (!m_restorePaths.empty())
    {
        SVN svn;
        for (auto it = m_restorePaths.cbegin(); it != m_restorePaths.cend(); ++it)
        {
            CopyFile(it->first, std::get<0>(it->second), FALSE);
            CPathUtils::Touch(std::get<0>(it->second));
            svn.AddToChangeList(CTSVNPathList(CTSVNPath(std::get<0>(it->second))), std::get<1>(it->second), svn_depth_empty);
        }
    }

    SaveSplitterPos();
    CResizableStandAloneDialog::OnCancel();
}

BOOL CCommitDlg::PreTranslateMessage(MSG* pMsg)
{
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
                if (OnEnterPressed())
                    return TRUE;
                if (GetFocus() == GetDlgItem(IDC_BUGID))
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

    StartStatusThread();
}

void CCommitDlg::StartStatusThread()
{
    if (InterlockedExchange(&m_bBlock, TRUE) != FALSE)
        return;

    delete m_pThread;

    m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    if (m_pThread == nullptr)
    {
        OnCantStartThread();
        InterlockedExchange(&m_bBlock, FALSE);
    }
    else
    {
        InterlockedExchange(&m_bThreadRunning, TRUE); // so the main thread knows that this thread is still running
        InterlockedExchange(&m_bRunThread, TRUE);     // if this is set to FALSE, the thread should stop
        m_pThread->m_bAutoDelete = FALSE;
        m_pThread->ResumeThread();
    }
}

void CCommitDlg::StopStatusThread()
{
    InterlockedExchange(&m_bRunThread, FALSE);
    WaitForSingleObject(m_pThread->m_hThread, 1000);
    if (m_bThreadRunning)
    {
        // we gave the thread a chance to quit. Since the thread didn't
        // listen to us we have to kill it.
        TerminateThread(m_pThread->m_hThread, static_cast<DWORD>(-1));
        InterlockedExchange(&m_bThreadRunning, FALSE);
    }
}

void CCommitDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CCommitDlg::OnBnClickedShowunversioned()
{
    m_tooltips.Pop(); // hide the tooltips
    UpdateData();
    m_regAddBeforeCommit = m_bShowUnversioned;
    if (!m_bBlock)
    {
        DWORD dwShow = m_listCtrl.GetShowFlags();
        if (static_cast<DWORD>(m_regAddBeforeCommit))
            dwShow |= SVNSLC_SHOWUNVERSIONED;
        else
            dwShow &= ~SVNSLC_SHOWUNVERSIONED;
        m_listCtrl.Show(dwShow, CTSVNPathList(), 0, true, true);
        UpdateCheckLinks();
    }
}

void CCommitDlg::OnStnClickedExternalwarning()
{
    m_tooltips.Popup();
}

void CCommitDlg::OnEnChangeLogmessage()
{
    UpdateOKButton();
}

LRESULT CCommitDlg::OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
    if ((m_listCtrl.GetItemCount() == 0) && (m_listCtrl.HasUnversionedItems()) && (!m_bShowUnversioned))
    {
        m_listCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED);
        m_listCtrl.Invalidate();
    }
    else
    {
        m_listCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMIT);
        m_listCtrl.Invalidate();
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
    path.SetFromWin(reinterpret_cast<LPCWSTR>(lParam));

    // just add all the items we get here.
    // if the item is versioned, the add will fail but nothing
    // more will happen.
    SVN svn;
    svn.Add(CTSVNPathList(path), &m_projectProperties, svn_depth_empty, false, true, true, true);

    if (!m_listCtrl.HasPath(path))
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
            for (int i = 0; i < m_pathList.GetCount(); ++i)
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
    SetTimer(REFRESHTIMER, 200, nullptr);
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
    return 0;
}

LRESULT CCommitDlg::OnAutoListReady(WPARAM, LPARAM lParam)
{
    std::map<CString, int>* autoList = reinterpret_cast<std::map<CString, int>*>(lParam);
    m_cLogMessage.SetAutoCompletionList(std::move(*autoList), '*');
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// functions which run in the status thread
//////////////////////////////////////////////////////////////////////////

void CCommitDlg::ParseRegexFile(const CString& sFile, std::map<CString, CString>& mapRegex) const
{
    CString strLine;
    try
    {
        CStdioFile file(sFile, CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
        while (m_bRunThread && !m_bCancelled && file.ReadString(strLine))
        {
            int     eqPos = strLine.Find('=');
            CString rgx   = strLine.Mid(eqPos + 1).Trim();

            int pos = -1;
            while (((pos = strLine.Find(',')) >= 0) && (pos < eqPos))
            {
                mapRegex[strLine.Left(pos)] = rgx;
                strLine                     = strLine.Mid(pos + 1).Trim();
            }
            mapRegex[strLine.Left(strLine.Find('=')).Trim()] = rgx;
        }
        file.Close();
    }
    catch (CFileException* pE)
    {
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException loading auto list regex file\n");
        pE->Delete();
    }
}

void CCommitDlg::ParseSnippetFile(const CString& sFile, std::map<CString, CString>& mapSnippet) const
{
    try
    {
        CString    strLine;
        CStdioFile file(sFile, CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
        while (m_bRunThread && !m_bCancelled && file.ReadString(strLine))
        {
            if (strLine.IsEmpty())
                continue;
            if (strLine.Left(1) == _T('#')) // comment char
                continue;
            int     eqPos = strLine.Find('=');
            CString key   = strLine.Left(eqPos);
            CString value = strLine.Mid(eqPos + 1);
            value.Replace(_T("\\\t"), _T("\t"));
            value.Replace(_T("\\\r"), _T("\r"));
            value.Replace(_T("\\\n"), _T("\n"));
            value.Replace(_T("\\\\"), _T("\\"));
            mapSnippet[key] = value;
        }
        file.Close();
    }
    catch (CFileException* pE)
    {
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException loading snippet file\n");
        pE->Delete();
    }
}

void CCommitDlg::GetAutocompletionList(std::map<CString, int>& autolist)
{
    // the auto completion list is made of strings from each selected files.
    // the strings used are extracted from the files with regexes found
    // in the file "autolist.txt".
    // the format of that file is:
    // file extensions separated with commas '=' regular expression to use
    // example:
    // .h, .hpp = (?<=class[\s])\b\w+\b|(\b\w+(?=[\s ]?\(\);))
    // .cpp = (?<=[^\s]::)\b\w+\b

    autolist.clear();
    std::map<CString, CString> mapRegex;
    CString                    sRegexFile   = CPathUtils::GetAppDirectory();
    CRegDWORD                  regTimeout   = CRegDWORD(L"Software\\TortoiseSVN\\AutocompleteParseTimeout", 5);
    ULONGLONG                  timeoutValue = static_cast<ULONGLONG>(static_cast<DWORD>(regTimeout)) * 1000UL;
    if (timeoutValue == 0)
        return;
    sRegexFile += L"autolist.txt";
    if (!m_bRunThread)
        return;
    if (PathFileExists(sRegexFile))
        ParseRegexFile(sRegexFile, mapRegex);
    sRegexFile = CPathUtils::GetAppDataDirectory();
    sRegexFile += L"\\autolist.txt";
    if (PathFileExists(sRegexFile))
        ParseRegexFile(sRegexFile, mapRegex);

    m_snippet.clear();
    CString sSnippetFile = CPathUtils::GetAppDirectory();
    sSnippetFile += _T("snippet.txt");
    ParseSnippetFile(sSnippetFile, m_snippet);
    sSnippetFile = CPathUtils::GetAppDataDirectory();
    sSnippetFile += _T("snippet.txt");
    if (PathFileExists(sSnippetFile))
        ParseSnippetFile(sSnippetFile, m_snippet);
    for (auto snip : m_snippet)
        autolist.emplace(snip.first, AUTOCOMPLETE_SNIPPET);

    ULONGLONG startTime = GetTickCount64();

    // now we have two arrays of strings, where the first array contains all
    // file extensions we can use and the second the corresponding regex strings
    // to apply to those files.

    // the next step is to go over all files shown in the commit dialog
    // and scan them for strings we can use
    int       nListItems = m_listCtrl.GetItemCount();
    CRegDWORD removedExtension(L"Software\\TortoiseSVN\\AutocompleteRemovesExtensions", FALSE);
    for (int i = 0; i < nListItems && m_bRunThread && !m_bCancelled; ++i)
    {
        // stop parsing after timeout
        if ((!m_bRunThread) || m_bCancelled || (GetTickCount64() - startTime > timeoutValue))
            return;
        const CSVNStatusListCtrl::FileEntry* entry = m_listCtrl.GetConstListEntry(i);
        if (!entry)
            continue;
        if (!entry->IsChecked() && (entry->GetChangeList().Compare(SVNSLC_IGNORECHANGELIST) == 0))
            continue;

        // add the path parts to the auto completion list too
        CString sPartPath = entry->GetRelativeSVNPath(false);
        autolist.emplace(sPartPath, AUTOCOMPLETE_FILENAME);

        int pos     = 0;
        int lastPos = 0;
        while ((pos = sPartPath.Find('/', pos)) >= 0)
        {
            pos++;
            lastPos = pos;
            autolist.emplace(sPartPath.Mid(pos), AUTOCOMPLETE_FILENAME);
        }

        // Last inserted entry is a file name.
        // Some users prefer to also list file name without extension.
        if (static_cast<DWORD>(removedExtension))
        {
            int dotPos = sPartPath.ReverseFind('.');
            if ((dotPos >= 0) && (dotPos > lastPos))
                autolist.emplace(sPartPath.Mid(lastPos, dotPos - lastPos), AUTOCOMPLETE_FILENAME);
        }
    }
    for (int i = 0; i < nListItems && m_bRunThread && !m_bCancelled; ++i)
    {
        // stop parsing after timeout
        if ((!m_bRunThread) || m_bCancelled || (GetTickCount64() - startTime > timeoutValue))
            return;
        const CSVNStatusListCtrl::FileEntry* entry = m_listCtrl.GetConstListEntry(i);
        if (!entry)
            continue;
        if (!entry->IsChecked() && (entry->GetChangeList().Compare(SVNSLC_IGNORECHANGELIST) == 0))
            continue;
        if ((entry->status <= svn_wc_status_normal) || (entry->status == svn_wc_status_ignored))
            continue;

        CString sExt = entry->GetPath().GetFileExtension();
        sExt.MakeLower();
        // find the regex string which corresponds to the file extension
        CString rData = mapRegex[sExt];
        if (rData.IsEmpty())
            continue;

        ScanFile(autolist, entry->GetPath().GetWinPathString(), rData, sExt);
        if ((entry->status != svn_wc_status_unversioned) &&
            (entry->status != svn_wc_status_none) &&
            (entry->status != svn_wc_status_ignored) &&
            (entry->status != svn_wc_status_added) &&
            (entry->textStatus != svn_wc_status_normal))
        {
            CTSVNPath basePath = SVN::GetPristinePath(entry->GetPath());
            if (!basePath.IsEmpty())
                ScanFile(autolist, basePath.GetWinPathString(), rData, sExt);
        }
    }
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Auto completion list loaded in %d msec\n", GetTickCount64() - startTime);
}

void CCommitDlg::ScanFile(std::map<CString, int>& autolist, const CString& sFilePath, const CString& sRegex, const CString& sExt) const
{
    static std::map<CString, std::wregex> regexMap;

    std::wstring sFileContent;
    CAutoFile    hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, NULL, nullptr);
    if (hFile)
    {
        DWORD size = GetFileSize(hFile, nullptr);
        if (size > 300000L)
        {
            // no files bigger than 300k
            return;
        }
        // allocate memory to hold file contents
        auto  buffer    = std::make_unique<char[]>(size);
        DWORD readBytes = 0;
        if (!ReadFile(hFile, buffer.get(), size, &readBytes, nullptr))
            return;
        int opts = 0;
        IsTextUnicode(buffer.get(), readBytes, &opts);
        if (opts & IS_TEXT_UNICODE_NULL_BYTES)
        {
            return;
        }
        if (opts & IS_TEXT_UNICODE_UNICODE_MASK)
        {
            sFileContent = std::wstring(reinterpret_cast<wchar_t*>(buffer.get()), readBytes / sizeof(WCHAR));
        }
        if ((opts & IS_TEXT_UNICODE_NOT_UNICODE_MASK) || (opts == 0))
        {
            const int ret      = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, static_cast<LPCSTR>(buffer.get()), readBytes, nullptr, 0);
            auto      pWideBuf = std::make_unique<wchar_t[]>(ret);
            const int ret2     = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, static_cast<LPCSTR>(buffer.get()), readBytes, pWideBuf.get(), ret);
            if (ret2 == ret)
                sFileContent = std::wstring(pWideBuf.get(), ret);
        }
    }
    if (sFileContent.empty() || !m_bRunThread)
    {
        return;
    }

    try
    {
        std::wregex                                    regCheck;
        std::map<CString, std::wregex>::const_iterator regIt;
        if ((regIt = regexMap.find(sExt)) != regexMap.end())
            regCheck = regIt->second;
        else
        {
            regCheck       = std::wregex(sRegex, std::regex_constants::icase | std::regex_constants::ECMAScript);
            regexMap[sExt] = regCheck;
        }
        const std::wsregex_iterator end;
        for (std::wsregex_iterator it(sFileContent.begin(), sFileContent.end(), regCheck); it != end; ++it)
        {
            if (m_bCancelled)
                break;
            const std::wsmatch match = *it;
            for (size_t i = 1; i < match.size(); ++i)
            {
                if (match[i].second - match[i].first)
                {
                    autolist.emplace(std::wstring(match[i]).c_str(), AUTOCOMPLETE_PROGRAMCODE);
                }
            }
        }
    }
    catch (std::exception&)
    {
    }
}

// CSciEditContextMenuInterface
void CCommitDlg::InsertMenuItems(CMenu& mPopup, int& nCmd)
{
    CString sMenuItemText(MAKEINTRESOURCE(IDS_COMMITDLG_POPUP_PASTEFILELIST));
    m_nPopupPasteListCmd = nCmd++;
    mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPasteListCmd, sMenuItemText);
}

bool CCommitDlg::HandleMenuItemClick(int cmd, CSciEdit* pSciEdit)
{
    if (m_bBlock)
        return false;
    if (cmd == m_nPopupPasteListCmd)
    {
        CString logMsg;
        wchar_t buf[MAX_STATUS_STRING_LENGTH] = {0};
        int     nListItems                    = m_listCtrl.GetItemCount();
        for (int i = 0; i < nListItems; ++i)
        {
            const CSVNStatusListCtrl::FileEntry* entry = m_listCtrl.GetConstListEntry(i);
            if (entry->IsChecked())
            {
                CString            line;
                svn_wc_status_kind status = entry->status;
                if (status == svn_wc_status_unversioned)
                    status = svn_wc_status_added;
                if (status == svn_wc_status_missing)
                    status = svn_wc_status_deleted;
                WORD langID = static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", GetUserDefaultLangID()));
                if (m_projectProperties.bFileListInEnglish)
                    langID = 1033;
                SVNStatus::GetStatusString(AfxGetResourceHandle(), status, buf, _countof(buf), langID);
                line.Format(L"%-10s %s\r\n", buf, static_cast<LPCWSTR>(m_listCtrl.GetItemText(i, 0)));
                logMsg += line;
            }
        }
        pSciEdit->InsertText(logMsg);
        return true;
    }
    return false;
}

void CCommitDlg::HandleSnippet(int type, const CString& text, CSciEdit* pSciEdit)
{
    if (type == AUTOCOMPLETE_SNIPPET)
    {
        CString target = m_snippet[text];
        pSciEdit->GetWordUnderCursor(true);
        pSciEdit->InsertText(target, false);
    }
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
                SetTimer(REFRESHTIMER, 200, nullptr);
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Wait some more before refreshing\n");
            }
            else
            {
                KillTimer(REFRESHTIMER);
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Refreshing after items dropped\n");
                Refresh();
            }
            break;
    }
    __super::OnTimer(nIDEvent);
}

void CCommitDlg::OnBnClickedHistory()
{
    m_tooltips.Pop(); // hide the tooltips
    if (m_pathList.GetCount() == 0)
        return;
    CHistoryDlg historyDlg(this);
    historyDlg.SetHistory(m_history);
    if (historyDlg.DoModal() != IDOK)
        return;

    CString sMsg = historyDlg.GetSelectedText();
    if (sMsg != m_cLogMessage.GetText().Left(sMsg.GetLength()))
    {
        CString sBugID = m_projectProperties.FindBugID(sMsg);
        if ((!sBugID.IsEmpty()) && ((GetDlgItem(IDC_BUGID)->IsWindowVisible())))
        {
            SetDlgItemText(IDC_BUGID, sBugID);
        }
        if (m_projectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATECOMMIT).Compare(m_cLogMessage.GetText()) != 0)
            m_cLogMessage.InsertText(sMsg, !m_cLogMessage.GetText().IsEmpty());
        else
            m_cLogMessage.SetText(sMsg);
    }

    UpdateOKButton();
    GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
}

void CCommitDlg::OnBnClickedBugtraqbutton()
{
    m_tooltips.Pop(); // hide the tooltips
    CString sMsg = m_cLogMessage.GetText();

    if (m_bugTraqProvider == nullptr)
        return;

    ATL::CComBSTR parameters;
    parameters.Attach(m_bugtraqAssociation.GetParameters().AllocSysString());
    ATL::CComBSTR   commonRoot(m_pathList.GetCommonRoot().GetDirectory().GetWinPath());
    CBstrSafeVector pathList(m_pathList.GetCount());

    for (LONG index = 0; index < m_pathList.GetCount(); ++index)
        pathList.PutElement(index, m_pathList[index].GetSVNPathString());

    ATL::CComBSTR originalMessage;
    originalMessage.Attach(sMsg.AllocSysString());
    ATL::CComBSTR temp;
    m_revProps.clear();

    // first try the IBugTraqProvider2 interface
    CComPtr<IBugTraqProvider2> pProvider2  = nullptr;
    HRESULT                    hr          = m_bugTraqProvider.QueryInterface(&pProvider2);
    bool                       bugIdOutSet = false;
    if (SUCCEEDED(hr))
    {
        CString       common = m_listCtrl.GetCommonURL(false).GetSVNPathString();
        ATL::CComBSTR repositoryRoot;
        repositoryRoot.Attach(common.AllocSysString());
        ATL::CComBSTR bugIDOut;
        GetDlgItemText(IDC_BUGID, m_sBugID);
        ATL::CComBSTR bugID;
        bugID.Attach(m_sBugID.AllocSysString());
        CBstrSafeVector revPropNames;
        CBstrSafeVector revPropValues;
        if (FAILED(hr = pProvider2->GetCommitMessage2(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, originalMessage, bugID, &bugIDOut, &revPropNames, &revPropValues, &temp)))
        {
            OnComError(hr);
        }
        else
        {
            if (bugIDOut)
            {
                bugIdOutSet = true;
                m_sBugID    = bugIDOut;
                SetDlgItemText(IDC_BUGID, m_sBugID);
            }
            m_cLogMessage.SetText(static_cast<LPCWSTR>(temp));
            BSTR* pbRevNames;
            BSTR* pbRevValues;

            HRESULT hr1 = SafeArrayAccessData(revPropNames, reinterpret_cast<void**>(&pbRevNames));
            if (SUCCEEDED(hr1))
            {
                HRESULT hr2 = SafeArrayAccessData(revPropValues, reinterpret_cast<void**>(&pbRevValues));
                if (SUCCEEDED(hr2))
                {
                    if (revPropNames->rgsabound->cElements == revPropValues->rgsabound->cElements)
                    {
                        for (ULONG i = 0; i < revPropNames->rgsabound->cElements; i++)
                        {
                            m_revProps[pbRevNames[i]] = pbRevValues[i];
                        }
                    }
                    SafeArrayUnaccessData(revPropValues);
                }
                SafeArrayUnaccessData(revPropNames);
            }
        }
    }
    else
    {
        // if IBugTraqProvider2 failed, try IBugTraqProvider
        CComPtr<IBugTraqProvider> pProvider = nullptr;
        hr                                  = m_bugTraqProvider.QueryInterface(&pProvider);
        if (FAILED(hr))
        {
            OnComError(hr);
            return;
        }

        if (FAILED(hr = pProvider->GetCommitMessage(GetSafeHwnd(), parameters, commonRoot, pathList, originalMessage, &temp)))
        {
            OnComError(hr);
        }
        else
            m_cLogMessage.SetText(static_cast<LPCWSTR>(temp));
    }
    m_sLogMessage = m_cLogMessage.GetText();
    if (!m_projectProperties.sMessage.IsEmpty())
    {
        CString sBugID = m_projectProperties.FindBugID(m_sLogMessage);
        if (!sBugID.IsEmpty() && !bugIdOutSet)
        {
            SetDlgItemText(IDC_BUGID, sBugID);
        }
    }

    m_cLogMessage.SetFocus();
}

void CCommitDlg::OnBnClickedLog()
{
    CString   sCmd;
    CTSVNPath root = m_pathList.GetCommonRoot();
    if (root.IsEmpty())
    {
        SVN svn;
        root = svn.GetWCRootFromPath(m_pathList[0]);
    }
    sCmd.Format(L"/command:log /path:\"%s\"", root.GetWinPath());
    CAppUtils::RunTortoiseProc(sCmd);
}

LRESULT CCommitDlg::OnSVNStatusListCtrlCheckChanged(WPARAM, LPARAM)
{
    UpdateOKButton();
    return 0;
}

LRESULT CCommitDlg::OnSVNStatusListCtrlChangelistChanged(WPARAM count, LPARAM)
{
    DialogEnableWindow(IDC_KEEPLISTS, count > 0);
    return 0;
}

LRESULT CCommitDlg::OnCheck(WPARAM wnd, LPARAM)
{
    if (!m_bBlock)
    {
        HWND hwnd = reinterpret_cast<HWND>(wnd);
        if (hwnd == GetDlgItem(IDC_CHECKALL)->GetSafeHwnd())
            m_listCtrl.Check(SVNSLC_SHOWEVERYTHING, true);
        else if (hwnd == GetDlgItem(IDC_CHECKNONE)->GetSafeHwnd())
            m_listCtrl.Check(0, true);
        else if (hwnd == GetDlgItem(IDC_CHECKUNVERSIONED)->GetSafeHwnd())
            m_listCtrl.Check(SVNSLC_SHOWUNVERSIONED, false);
        else if (hwnd == GetDlgItem(IDC_CHECKVERSIONED)->GetSafeHwnd())
            m_listCtrl.Check(SVNSLC_SHOWVERSIONED, false);
        else if (hwnd == GetDlgItem(IDC_CHECKADDED)->GetSafeHwnd())
            m_listCtrl.Check(SVNSLC_SHOWADDED | SVNSLC_SHOWADDEDHISTORY, false);
        else if (hwnd == GetDlgItem(IDC_CHECKDELETED)->GetSafeHwnd())
            m_listCtrl.Check(SVNSLC_SHOWREMOVED | SVNSLC_SHOWMISSING, false);
        else if (hwnd == GetDlgItem(IDC_CHECKMODIFIED)->GetSafeHwnd())
            m_listCtrl.Check(SVNSLC_SHOWMODIFIED | SVNSLC_SHOWREPLACED, false);
        else if (hwnd == GetDlgItem(IDC_CHECKFILES)->GetSafeHwnd())
            m_listCtrl.Check(SVNSLC_SHOWFILES, false);
        else if (hwnd == GetDlgItem(IDC_CHECKDIRECTORIES)->GetSafeHwnd())
            m_listCtrl.Check(SVNSLC_SHOWFOLDERS, false);
    }

    return 0;
}

void CCommitDlg::UpdateOKButton()
{
    BOOL bValidLogSize = FALSE;

    if (m_cLogMessage.GetText().GetLength() >= m_projectProperties.nMinLogSize)
        bValidLogSize = TRUE;

    LONG nSelectedItems = m_listCtrl.GetSelected();
    if (!bValidLogSize)
        m_tooltips.AddTool(IDOK, IDS_COMMITDLG_MSGLENGTH);
    else if (nSelectedItems == 0)
        m_tooltips.AddTool(IDOK, IDS_COMMITDLG_NOTHINGSELECTED);
    else
        m_tooltips.DelTool(IDOK);

    DialogEnableWindow(IDOK, !m_bBlock && bValidLogSize && nSelectedItems > 0);
}

void CCommitDlg::OnComError(HRESULT hr) const
{
    COMError ce(hr);
    CString  sErr;
    sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, static_cast<LPCWSTR>(m_bugtraqAssociation.GetProviderName()), ce.GetMessageAndDescription().c_str());
    ::MessageBox(m_hWnd, sErr, L"TortoiseSVN", MB_ICONERROR);
}

LRESULT CCommitDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_NOTIFY:
            if (wParam == IDC_SPLITTER)
            {
                SpcNMHDR* pHdr = reinterpret_cast<SpcNMHDR*>(lParam);
                DoSize(pHdr->delta);
            }
            break;
    }

    return __super::DefWindowProc(message, wParam, lParam);
}

void CCommitDlg::SetSplitterRange()
{
    if ((m_listCtrl) && (m_cLogMessage))
    {
        CRect rcTop;
        m_cLogMessage.GetWindowRect(rcTop);
        ScreenToClient(rcTop);
        CRect rcMiddle;
        m_listCtrl.GetWindowRect(rcMiddle);
        ScreenToClient(rcMiddle);
        if (rcMiddle.Height() && rcMiddle.Width())
            m_wndSplitter.SetRange(rcTop.top + CDPIAware::Instance().Scale(GetSafeHwnd(), 60), rcMiddle.bottom - CDPIAware::Instance().Scale(GetSafeHwnd(), 80));
    }
}

void CCommitDlg::DoSize(int delta)
{
    RemoveAnchor(IDC_MESSAGEGROUP);
    RemoveAnchor(IDC_LOGMESSAGE);
    RemoveAnchor(IDC_SPLITTER);
    RemoveAnchor(IDC_LISTGROUP);
    RemoveAnchor(IDC_FILELIST);
    RemoveAnchor(IDC_SELECTLABEL);
    RemoveAnchor(IDC_CHECKALL);
    RemoveAnchor(IDC_CHECKNONE);
    RemoveAnchor(IDC_CHECKUNVERSIONED);
    RemoveAnchor(IDC_CHECKVERSIONED);
    RemoveAnchor(IDC_CHECKADDED);
    RemoveAnchor(IDC_CHECKDELETED);
    RemoveAnchor(IDC_CHECKMODIFIED);
    RemoveAnchor(IDC_CHECKFILES);
    RemoveAnchor(IDC_CHECKDIRECTORIES);
    auto hdwp = BeginDeferWindowPos(14);
    CSplitterControl::ChangeRect(hdwp, &m_cLogMessage, 0, 0, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_MESSAGEGROUP), 0, 0, 0, delta);
    CSplitterControl::ChangeRect(hdwp, &m_listCtrl, 0, delta, 0, 0);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_LISTGROUP), 0, delta, 0, 0);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_SELECTLABEL), 0, delta, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKALL), 0, delta, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKNONE), 0, delta, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKUNVERSIONED), 0, delta, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKVERSIONED), 0, delta, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKADDED), 0, delta, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKDELETED), 0, delta, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKMODIFIED), 0, delta, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKFILES), 0, delta, 0, delta);
    CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKDIRECTORIES), 0, delta, 0, delta);
    EndDeferWindowPos(hdwp);
    AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SELECTLABEL, TOP_LEFT);
    AddAnchor(IDC_CHECKALL, TOP_LEFT);
    AddAnchor(IDC_CHECKNONE, TOP_LEFT);
    AddAnchor(IDC_CHECKUNVERSIONED, TOP_LEFT);
    AddAnchor(IDC_CHECKVERSIONED, TOP_LEFT);
    AddAnchor(IDC_CHECKADDED, TOP_LEFT);
    AddAnchor(IDC_CHECKDELETED, TOP_LEFT);
    AddAnchor(IDC_CHECKMODIFIED, TOP_LEFT);
    AddAnchor(IDC_CHECKFILES, TOP_LEFT);
    AddAnchor(IDC_CHECKDIRECTORIES, TOP_LEFT);
    ArrangeLayout();
    // adjust the minimum size of the dialog to prevent the resizing from
    // moving the list control too far down.
    CRect rcLogMsg;
    m_cLogMessage.GetClientRect(rcLogMsg);
    SetMinTrackSize(CSize(m_dlgOrigRect.Width(), m_dlgOrigRect.Height() - m_logMsgOrigRect.Height() + rcLogMsg.Height()));

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

void CCommitDlg::OnBnClickedShowexternals()
{
    m_tooltips.Pop(); // hide the tooltips
    UpdateData();
    m_regShowExternals = m_bShowExternals;
    if (!m_bBlock)
    {
        DWORD dwShow = m_listCtrl.GetShowFlags();
        if (static_cast<DWORD>(m_regShowExternals))
            dwShow |= SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO | SVNSLC_SHOWEXTDISABLED;
        else
            dwShow &= ~(SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO | SVNSLC_SHOWEXTDISABLED);
        m_listCtrl.Show(dwShow, CTSVNPathList(), 0, true, true);
        UpdateCheckLinks();
    }
}

void CCommitDlg::UpdateCheckLinks()
{
    DialogEnableWindow(IDC_CHECKALL, true);
    DialogEnableWindow(IDC_CHECKNONE, true);
    DialogEnableWindow(IDC_CHECKUNVERSIONED, m_listCtrl.GetUnversionedCount() > 0);
    DialogEnableWindow(IDC_CHECKVERSIONED, m_listCtrl.GetItemCount() > m_listCtrl.GetUnversionedCount());
    DialogEnableWindow(IDC_CHECKADDED, m_listCtrl.GetAddedCount() > 0);
    DialogEnableWindow(IDC_CHECKDELETED, m_listCtrl.GetDeletedCount() > 0);
    DialogEnableWindow(IDC_CHECKMODIFIED, m_listCtrl.GetModifiedCount() > 0);
    DialogEnableWindow(IDC_CHECKFILES, m_listCtrl.GetFileCount() > 0);
    DialogEnableWindow(IDC_CHECKDIRECTORIES, m_listCtrl.GetFolderCount() > 0);
}

void CCommitDlg::VersionCheck()
{
    if (CRegDWORD(L"Software\\TortoiseSVN\\VersionCheck", TRUE) == FALSE)
        return;
    CRegString regVer(L"Software\\TortoiseSVN\\NewVersion");
    CString    verTemp = regVer;
    int        major   = _wtoi(verTemp);
    verTemp            = verTemp.Mid(verTemp.Find('.') + 1);
    int minor          = _wtoi(verTemp);
    verTemp            = verTemp.Mid(verTemp.Find('.') + 1);
    int micro          = _wtoi(verTemp);
    verTemp            = verTemp.Mid(verTemp.Find('.') + 1);
    int  build         = _wtoi(verTemp);
    BOOL bNewer        = FALSE;
    if (major > TSVN_VERMAJOR)
        bNewer = TRUE;
    else if ((minor > TSVN_VERMINOR) && (major == TSVN_VERMAJOR))
        bNewer = TRUE;
    else if ((micro > TSVN_VERMICRO) && (minor == TSVN_VERMINOR) && (major == TSVN_VERMAJOR))
        bNewer = TRUE;
    else if ((build > TSVN_VERBUILD) && (micro == TSVN_VERMICRO) && (minor == TSVN_VERMINOR) && (major == TSVN_VERMAJOR))
        bNewer = TRUE;

    if (bNewer)
    {
        CRegString regDownText(L"Software\\TortoiseSVN\\NewVersionText");
        CRegString regDownLink(L"Software\\TortoiseSVN\\NewVersionLink", L"https://tortoisesvn.net");

        if (CString(regDownText).IsEmpty())
        {
            CString temp;
            temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
            regDownText = temp;
        }
        m_cUpdateLink.SetURL(CString(regDownLink));
        m_cUpdateLink.SetWindowText(CString(regDownText));
        m_cUpdateLink.SetColors(RGB(255, 0, 0), RGB(150, 0, 0));

        m_cUpdateLink.ShowWindow(SW_SHOW);
    }
}

BOOL CCommitDlg::OnQueryEndSession()
{
    if (!__super::OnQueryEndSession())
        return FALSE;

    OnCancel();

    return TRUE;
}

void CCommitDlg::ReadPersistedDialogSettings()
{
    m_regAddBeforeCommit = CRegDWORD(L"Software\\TortoiseSVN\\AddBeforeCommit", TRUE);
    m_bShowUnversioned   = m_regAddBeforeCommit;

    m_regShowExternals = CRegDWORD(L"Software\\TortoiseSVN\\ShowExternals", TRUE);
    m_bShowExternals   = m_regShowExternals;

    m_history.SetMaxHistoryItems(static_cast<LONG>(CRegDWORD(L"Software\\TortoiseSVN\\MaxHistoryItems", 25)));

    m_regKeepChangelists = CRegDWORD(L"Software\\TortoiseSVN\\KeepChangeLists", FALSE);
    m_bKeepChangeList    = m_regKeepChangelists;

    if (CRegDWORD(L"Software\\TortoiseSVN\\AlwaysWarnIfNoIssue", FALSE))
        m_projectProperties.bWarnIfNoIssue = TRUE;

    m_bKeepLocks = SVNConfig::Instance().KeepLocks();
}

void CCommitDlg::SubclassControls()
{
    m_aeroControls.SubclassControl(this, IDC_KEEPLOCK);
    m_aeroControls.SubclassControl(this, IDC_KEEPLISTS);
    m_aeroControls.SubclassControl(this, IDC_LOG);
    m_aeroControls.SubclassControl(this, IDC_RUNHOOK);
    m_aeroControls.SubclassOkCancelHelp(this);
}

void CCommitDlg::InitializeListControl()
{
    m_listCtrl.SetRestorePaths(m_restorePaths);
    m_listCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS | SVNSLC_COLPROPSTATUS | SVNSLC_COLLOCK,
                    L"CommitDlg", SVNSLC_POPALL ^ SVNSLC_POPCOMMIT);
    m_listCtrl.SetStatLabel(GetDlgItem(IDC_STATISTICS));
    m_listCtrl.SetCancelBool(&m_bCancelled);
    m_listCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMIT);
    m_listCtrl.EnableFileDrop();
    m_listCtrl.SetBackgroundImage(IDI_COMMIT_BKG);
}

void CCommitDlg::InitializeLogMessageControl()
{
    m_cLogMessage.Init(m_projectProperties);
    m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
    m_cLogMessage.RegisterContextMenuHandler(this);
    std::map<int, UINT> icons;
    icons[AUTOCOMPLETE_SPELLING]    = IDI_SPELL;
    icons[AUTOCOMPLETE_FILENAME]    = IDI_FILE;
    icons[AUTOCOMPLETE_PROGRAMCODE] = IDI_CODE;
    icons[AUTOCOMPLETE_SNIPPET]     = IDI_SNIPPET;
    m_cLogMessage.SetIcon(icons);
}

void CCommitDlg::SetControlAccessibilityProperties()
{
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_TEXT);
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_HELP, CString(MAKEINTRESOURCE(IDS_INPUT_ENTERLOG)));
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+" + CString(CAppUtils::FindAcceleratorKey(this, IDC_INVISIBLE)));
}

void CCommitDlg::SetupToolTips()
{
    m_tooltips.AddTool(IDC_EXTERNALWARNING, IDS_COMMITDLG_EXTERNALS);
    m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);
}

void CCommitDlg::HideAndDisableBugTraqButton() const
{
    GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);
}

void CCommitDlg::SetupBugTraqControlsIfConfigured()
{
    CBugTraqAssociations bugtraqAssociations;
    bugtraqAssociations.Load(m_projectProperties.GetProviderUUID(), m_projectProperties.sProviderParams);

    bool bExtendUrlControl = true;
    if (bugtraqAssociations.FindProvider(m_pathList, &m_bugtraqAssociation))
    {
        CComPtr<IBugTraqProvider> pProvider;
        HRESULT                   hr = pProvider.CoCreateInstance(m_bugtraqAssociation.GetProviderClass());
        if (SUCCEEDED(hr))
        {
            m_bugTraqProvider = pProvider;
            ATL::CComBSTR temp;
            ATL::CComBSTR parameters;
            parameters.Attach(m_bugtraqAssociation.GetParameters().AllocSysString());
            hr = pProvider->GetLinkText(GetSafeHwnd(), parameters, &temp);
            if (SUCCEEDED(hr))
            {
                SetDlgItemText(IDC_BUGTRAQBUTTON, temp == 0 ? L"" : temp);
                GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_SHOW);
                bExtendUrlControl = false;
            }
        }

        GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
    }

    if (bExtendUrlControl)
        CAppUtils::ExtendControlOverHiddenControl(this, IDC_COMMIT_TO, IDC_BUGTRAQBUTTON);
}

void CCommitDlg::RestoreBugIdAndLabelFromProjectProperties()
{
    GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
    if (!m_projectProperties.sLabel.IsEmpty())
        SetDlgItemText(IDC_BUGIDLABEL, m_projectProperties.sLabel);
    GetDlgItem(IDC_BUGID)->SetFocus();
    CString sBugID = m_projectProperties.GetBugIDFromLog(m_sLogMessage);
    if (!sBugID.IsEmpty())
    {
        SetDlgItemText(IDC_BUGID, sBugID);
    }
}

void CCommitDlg::HideBugIdAndLabel() const
{
    GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
}

void CCommitDlg::ShowOrHideBugIdAndLabelControls()
{
    if (!m_projectProperties.sMessage.IsEmpty())
        RestoreBugIdAndLabelFromProjectProperties();
    else
        HideBugIdAndLabel();
}

void CCommitDlg::SetupLogMessageDefaultText()
{
    if (!m_sLogMessage.IsEmpty())
        m_cLogMessage.SetText(m_sLogMessage);
    else
        m_cLogMessage.SetText(m_projectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATECOMMIT));
    OnEnChangeLogmessage();
}

void CCommitDlg::SetCommitWindowTitleAndEnableStatus()
{
    GetWindowText(m_sWindowTitle);
    DialogEnableWindow(IDC_LOG, m_pathList.GetCount() > 0);
}

void CCommitDlg::AdjustControlSizes()
{
    AdjustControlSize(IDC_SHOWUNVERSIONED);
    AdjustControlSize(IDC_KEEPLOCK);
}

void CCommitDlg::LineupControlsAndAdjustSizes()
{
    // line up all controls and adjust their sizes.
#define LINKSPACING 9
    RECT rc = AdjustControlSize(IDC_SELECTLABEL);
    rc.right -= 15; // AdjustControlSize() adds 20 pixels for the checkbox/radio button bitmap, but this is a label...
    rc = AdjustStaticSize(IDC_CHECKALL, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKNONE, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKUNVERSIONED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKVERSIONED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKADDED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKDELETED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKMODIFIED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKFILES, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKDIRECTORIES, rc, LINKSPACING);
}

void CCommitDlg::AddAnchorsToFacilitateResizing()
{
    AddAnchor(IDC_COMMITLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
    AddAnchor(IDC_BUGID, TOP_RIGHT);
    AddAnchor(IDC_BUGTRAQBUTTON, TOP_RIGHT);
    AddAnchor(IDC_COMMIT_TO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_HISTORY, TOP_LEFT);
    AddAnchor(IDC_NEWVERSIONLINK, TOP_LEFT, TOP_RIGHT);

    AddAnchor(IDC_SELECTLABEL, TOP_LEFT);
    AddAnchor(IDC_CHECKALL, TOP_LEFT);
    AddAnchor(IDC_CHECKNONE, TOP_LEFT);
    AddAnchor(IDC_CHECKUNVERSIONED, TOP_LEFT);
    AddAnchor(IDC_CHECKVERSIONED, TOP_LEFT);
    AddAnchor(IDC_CHECKADDED, TOP_LEFT);
    AddAnchor(IDC_CHECKDELETED, TOP_LEFT);
    AddAnchor(IDC_CHECKMODIFIED, TOP_LEFT);
    AddAnchor(IDC_CHECKFILES, TOP_LEFT);
    AddAnchor(IDC_CHECKDIRECTORIES, TOP_LEFT);

    AddAnchor(IDC_INVISIBLE, TOP_RIGHT);
    AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);

    AddAnchor(IDC_INVISIBLE2, TOP_RIGHT);
    AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_DWM, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWEXTERNALS, BOTTOM_LEFT);
    AddAnchor(IDC_EXTERNALWARNING, BOTTOM_RIGHT);
    AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_KEEPLOCK, BOTTOM_LEFT);
    AddAnchor(IDC_KEEPLISTS, BOTTOM_LEFT);
    AddAnchor(IDC_RUNHOOK, BOTTOM_RIGHT);
    AddAnchor(IDC_LOG, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
}

void CCommitDlg::SaveDialogAndLogMessageControlRectangles()
{
    GetClientRect(m_dlgOrigRect);
    m_cLogMessage.GetClientRect(m_logMsgOrigRect);
}

void CCommitDlg::CenterWindowWhenLaunchedFromExplorer()
{
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
}

void CCommitDlg::AdjustDialogSizeAndPanes()
{
    EnableSaveRestore(L"CommitDlg");
    DWORD yPos = CRegDWORD(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\CommitDlgSizer");
    RECT  rcDlg, rcLogMsg, rcFileList;
    GetClientRect(&rcDlg);
    m_cLogMessage.GetWindowRect(&rcLogMsg);
    ScreenToClient(&rcLogMsg);
    m_listCtrl.GetWindowRect(&rcFileList);
    ScreenToClient(&rcFileList);
    if (yPos)
    {
        RECT rectSplitter;
        m_wndSplitter.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        int delta = yPos - rectSplitter.top;
        if ((rcLogMsg.bottom + delta > rcLogMsg.top) && (rcLogMsg.bottom + delta < rcFileList.bottom - CDPIAware::Instance().Scale(GetSafeHwnd(), 30)))
        {
            m_wndSplitter.SetWindowPos(nullptr, rectSplitter.left, yPos, 0, 0, SWP_NOSIZE);
            DoSize(delta);
            Invalidate();
        }
    }
}

void CCommitDlg::AddDirectoriesToPathWatcher()
{
    // add all directories to the watcher
    for (int i = 0; i < m_pathList.GetCount(); ++i)
    {
        if (m_pathList[i].IsDirectory())
            m_pathWatcher.AddPath(m_pathList[i]);
    }
    m_updatedPathList = m_pathList;
}

void CCommitDlg::ShowBalloonInCaseOfError() const
{
    CRegDWORD err         = CRegDWORD(L"Software\\TortoiseSVN\\ErrorOccurred", FALSE);
    CRegDWORD historyHint = CRegDWORD(L"Software\\TortoiseSVN\\HistoryHintShown", FALSE);
    if ((static_cast<DWORD>(err) != FALSE) && ((static_cast<DWORD>(historyHint) == FALSE)))
    {
        historyHint = TRUE;
        m_tooltips.ShowBalloon(IDC_HISTORY, IDS_COMMITDLG_HISTORYHINT_TT, IDS_WARN_NOTE, TTI_INFO);
    }
    err = FALSE;
}

void CCommitDlg::OnBnClickedRunhook()
{
    UpdateData();
    m_sLogMessage = m_cLogMessage.GetText();
    // create a list of all checked items
    CTSVNPathList checkedItems;
    m_listCtrl.WriteCheckedNamesToPathList(checkedItems);
    DWORD   exitCode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(checkedItems.GetCommonRoot(), m_projectProperties);
    if (CHooks::Instance().ManualPreCommit(m_hWnd, checkedItems, m_sLogMessage, exitCode, error))
    {
        if (exitCode)
        {
            ::MessageBox(m_hWnd, error, L"TortoiseSVN hook", MB_ICONERROR);
            // parse the error message and select all mentioned paths if there are any
            // the paths must be separated by newlines!
            CTSVNPathList errorpaths;
            error.Replace(L"\r\n", L"*");
            error.Replace(L"\n", L"*");
            errorpaths.LoadFromAsteriskSeparatedString(error);
            m_listCtrl.SetSelectionMark(-1);
            for (int i = 0; i < errorpaths.GetCount(); ++i)
            {
                CTSVNPath errorPath  = errorpaths[i];
                int       nListItems = m_listCtrl.GetItemCount();
                for (int j = 0; j < nListItems; j++)
                {
                    const CSVNStatusListCtrl::FileEntry* entry = m_listCtrl.GetConstListEntry(j);
                    if (entry->GetPath().IsEquivalentTo(errorPath))
                    {
                        m_listCtrl.SetItemState(j, TVIS_SELECTED, TVIS_SELECTED);
                        break;
                    }
                }
            }

            return;
        }
        m_tooltips.ShowBalloon(IDC_RUNHOOK, IDS_COMMITDLG_HOOKSUCCESSFUL_TT, IDS_WARN_NOTE, TTI_INFO);
    }
}
