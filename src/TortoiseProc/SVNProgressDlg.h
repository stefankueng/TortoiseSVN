﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2017, 2020-2021 - TortoiseSVN

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
#include "TSVNPath.h"
#include "ProjectProperties.h"
#include "SVN.h"
#include "Colors.h"
#include "SVNExternals.h"
#include "../IBugTraqProvider/IBugTraqProvider_h.h"
#include "LinkControl.h"
#include "Hooks.h"
#include "LogDialog/LogDlgDataModel.h"

class CCmdLineParser;

using GENERICCOMPAREFN = int(__cdecl*)(const void* elem1, const void* elem2);

/**
 * \ingroup TortoiseProc
 * Options which can be used to configure the way the dialog box works
 */
enum ProgressOptions
{
    ProgOptNone              = 0,
    ProgOptRecursive         = 0x01,
    ProgOptNonRecursive      = 0x00,
    ProgOptDryRun            = 0x04,
    ProgOptIgnoreExternals   = 0x08,
    ProgOptKeeplocks         = 0x10,
    ProgOptForce             = 0x20, ///< for locking this means steal the lock, for unlocking it means breaking the lock
    ProgOptSwitchAfterCopy   = 0x40,
    ProgOptIncludeIgnored    = 0x80,
    ProgOptIgnoreAncestry    = 0x100,
    ProgOptEolDefault        = 0x200,
    ProgOptEolCRLF           = 0x400,
    ProgOptEolLF             = 0x800,
    ProgOptEolCR             = 0x1000,
    ProgOptSkipConflictCheck = 0x2000,
    ProgOptRecordOnly        = 0x4000,
    ProgOptStickyDepth       = 0x8000,
    ProgOptUseAutoprops      = 0x10000,
    ProgOptIgnoreKeywords    = 0x20000,
    ProgOptMakeParents       = 0x40000,
    ProgOptAllowMixedRev     = 0x80000,
    ProgOptSkipPreChecks     = 0x100000,
    ProgOptClearChangeLists  = 0x200000,
};

enum ProgressCloseOptions
{
    CLOSE_MANUAL = 0,
    CLOSE_NOERRORS,
    CLOSE_NOCONFLICTS,
    CLOSE_NOMERGES,
    CLOSE_LOCAL
};

/**
 * \ingroup TortoiseProc
 * Handles different Subversion commands and shows the notify messages
 * in a listbox. Since several Subversion commands have similar notify
 * messages they are grouped together in this single class.
 */
class CSVNProgressDlg : public CResizableStandAloneDialog
    , public SVN
{
public:
    enum Command
    {
        SVNProgress_None,
        SVNProgress_Add,
        SVNProgress_Checkout,
        SVNProgress_SparseCheckout,
        SVNProgress_SingleFileCheckout,
        SVNProgress_Commit,
        SVNProgress_Copy,
        SVNProgress_Export,
        SVNProgress_Import,
        SVNProgress_Lock,
        SVNProgress_Merge,
        SVNProgress_MergeReintegrate,
        SVNProgress_MergeReintegrateOldStyle,
        SVNProgress_MergeAll,
        SVNProgress_Rename,
        SVNProgress_Resolve,
        SVNProgress_Revert,
        SVNProgress_Switch,
        SVNProgress_SwitchBackToParent,
        SVNProgress_Unlock,
        SVNProgress_Update,
    };

    DECLARE_DYNAMIC(CSVNProgressDlg)

public:
    CSVNProgressDlg(CWnd* pParent = nullptr);
    ~CSVNProgressDlg() override;

    void SetCommand(Command cmd) { m_command = cmd; }
    void SetAutoClose(const CCmdLineParser& parser);
    void SetAutoClose(DWORD ac) { m_dwCloseOnEnd = ac; }
    void SetAutoCloseLocal(BOOL bCloseLocal) { m_bCloseLocalOnEnd = bCloseLocal; }
    void SetHidden(bool hidden) { m_hidden = hidden; }
    void SetOptions(DWORD opts) { m_options = opts; }
    void SetPathList(const CTSVNPathList& pathList) { m_targetPathList = pathList; }
    void SetOrigPathList(const CTSVNPathList& oritList) { m_origPathList = oritList; }
    void SetUrl(const CString& url) const { m_url.SetFromUnknown(url); }
    void SetSecondUrl(const CString& url) const { m_url2.SetFromUnknown(url); }
    void SetCommitMessage(const CString& msg) { m_sMessage = msg; }
    void SetExternals(const SVNExternals& ext) { m_externals = ext; }
    void SetPathDepths(const std::map<CString, svn_depth_t>& pathmap) { m_pathDepths = pathmap; }
    void SetRevision(const SVNRev& rev) { m_revision = rev; }
    void SetRevisionEnd(const SVNRev& rev) { m_revisionEnd = rev; }

    void SetDiffOptions(const CString& opts) { m_diffOptions = opts; }
    void SetDepth(svn_depth_t depth = svn_depth_unknown) { m_depth = depth; }
    void SetPegRevision(const SVNRev& pegrev = SVNRev()) { m_pegRev = pegrev; }
    void SetProjectProperties(const ProjectProperties& props) { m_projectProperties = props; }
    void SetChangeList(const CString& changelist, bool keepchangelist)
    {
        m_changeList     = changelist;
        m_keepChangeList = keepchangelist;
    }
    void                                            SetSelectedList(const CTSVNPathList& selPaths);
    void                                            SetRevisionRanges(const SVNRevRangeArray& revArray) { m_revisionArray = revArray; }
    void                                            SetBugTraqProvider(const CComPtr<IBugTraqProvider>& pBugtraqProvider) { m_bugTraqProvider = pBugtraqProvider; }
    void                                            SetRevisionProperties(const RevPropHash& revProps) { m_revProps = revProps; }
    void                                            SetRestorePaths(const std::map<CString, std::tuple<CString, CString>>& restorepaths) { m_restorePaths = restorepaths; }
    std::map<CString, std::tuple<CString, CString>> GetRestorePaths() const { return m_restorePaths; }
    /**
     * If the number of items for which the operation is done on is known
     * beforehand, that number can be set here. It is then used to show a more
     * accurate progress bar during the operation.
     */
    void SetItemCount(INT_PTR count)
    {
        if (count)
            m_itemCountTotal = count;
    }

    bool SetBackgroundImage(UINT nID) const;

    bool DidErrorsOccur() const { return m_bErrorsOccurred; }
    bool DidConflictsOccur() const { return m_nConflicts > 0; }

    enum
    {
        IDD = IDD_SVNPROGRESS
    };

private:
    class NotificationData
    {
    public:
        NotificationData()
            : action(static_cast<svn_wc_notify_action_t>(-1))
            , kind(svn_node_none)
            , contentState(svn_wc_notify_state_inapplicable)
            , propState(svn_wc_notify_state_inapplicable)
            , lockState(svn_wc_notify_lock_state_unchanged)
            , rev(0)
            , color(::GetSysColor(COLOR_WINDOWTEXT))
            , colorIsDirect(false)
            , bConflictedActionItem(false)
            , bTreeConflict(false)
            , bAuxItem(false)
            , bConflictSummary(false)
            , bBold(false)
            , indent(0)
            , id(0)
        {
            mergeRange.end         = 0;
            mergeRange.start       = 0;
            mergeRange.inheritable = false;
        }

    public:
        // The text we put into the first column (the SVN action for normal items, just text for aux items)
        CString   sActionColumnText;
        CTSVNPath path;
        CTSVNPath basePath;
        CTSVNPath url;
        CString   changeListName;
        CString   propertyName;

        svn_wc_notify_action_t     action;
        svn_node_kind_t            kind;
        CString                    mimeType;
        svn_wc_notify_state_t      contentState;
        svn_wc_notify_state_t      propState;
        svn_wc_notify_lock_state_t lockState;
        svn_merge_range_t          mergeRange;
        svn_revnum_t               rev;
        COLORREF                   color;
        bool                       colorIsDirect;
        CString                    owner;                 ///< lock owner
        bool                       bConflictedActionItem; ///< Is this item a conflict?
        bool                       bTreeConflict;         ///< item is tree conflict
        bool                       bAuxItem;              ///< Set if this item is not a true 'SVN action'
        bool                       bConflictSummary;      ///< if true, the entry is "one or more items are in a conflicted state"
        bool                       bBold;                 ///< if true, the line is shown with a bold font
        CString                    sPathColumnText;
        int                        indent; ///< indentation
        long                       id;     ///< used to identify an entry even after sorting
    };

protected:
    //implement the virtual methods from SVN base class
    BOOL Notify(const CTSVNPath& path, const CTSVNPath& url, svn_wc_notify_action_t action,
                svn_node_kind_t kind, const CString& mimeType,
                svn_wc_notify_state_t contentState,
                svn_wc_notify_state_t propState, LONG rev,
                const svn_lock_t* lock, svn_wc_notify_lock_state_t lockState,
                const CString&     changeListName,
                const CString&     propertyName,
                svn_merge_range_t* range,
                svn_error_t* err, apr_pool_t* pool) override;
    BOOL Cancel() override;

    BOOL OnInitDialog() override;
    void OnCancel() override;
    BOOL PreTranslateMessage(MSG* pMsg) override;
    void DoDataExchange(CDataExchange* pDX) override;

    afx_msg void    OnNMCustomdrawSvnprogress(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnGetdispinfoSvnprogress(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMDblclkSvnprogress(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnBnClickedLogbutton();
    afx_msg void    OnHdnItemclickSvnprogress(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void    OnClose();
    afx_msg void    OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg LRESULT OnSVNProgress(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnLvnBegindragSvnprogress(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnSize(UINT nType, int cx, int cy);
    afx_msg LRESULT OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCloseOnEnd(WPARAM /*wParam*/, LPARAM /*lParam*/);
    afx_msg void    OnBnClickedRetrynohooks();
    afx_msg void    OnBnClickedRetryMerge();
    afx_msg void    OnBnClickedRetryDifferentUser();
    afx_msg LRESULT OnCheck(WPARAM wnd, LPARAM);
    afx_msg LRESULT OnResolveMsg(WPARAM, LPARAM);
    afx_msg void    OnSysColorChange();

    DECLARE_MESSAGE_MAP()

    void        Sort();
    static bool SortCompare(const NotificationData* pElem1, const NotificationData* pElem2);

    static BOOL m_bAscending;
    static int  m_nSortedColumn;
    CStringList m_extStack;
    bool        m_bExtDataAdded;
    bool        m_bHideExternalInfo;
    bool        m_bExternalStartInfoShown;

private:
    static UINT   ProgressThreadEntry(LPVOID pVoid);
    UINT          ProgressThread();
    void          OnOK() override;
    void          ReportSVNError();
    void          ReportError(const CString& sError);
    void          ReportHookFailed(HookType t, const CTSVNPathList& pathList, const CString& error);
    void          ReportWarning(const CString& sWarning);
    void          ReportNotification(const CString& sNotification);
    void          ReportCmd(const CString& sCmd);
    void          ReportString(CString sMessage, const CString& sMsgKind, bool colorIsDirect, COLORREF color = ::GetSysColor(COLOR_WINDOWTEXT));
    void          AddItemToList(NotificationData* data);
    void          RemoveItemFromList(size_t index);
    CString       BuildInfoString();
    CString       GetPathFromColumnText(const CString& sColumnText) const;
    bool          IsCommittingToTag(CString& url);
    void          OnCommitFinished();
    bool          CheckUpdateAndRetry();
    void          ResetVars();
    void          MergeAfterCommit();
    void          GenerateMergeLogMessage();
    static bool   IsRevisionRelatedToMerge(const CDictionaryBasedTempPath& basePath, PLOGENTRYDATA pLogItem);
    void          CompareWithWC(NotificationData* data);
    CTSVNPathList GetPathsForUpdateHook(const CTSVNPathList& pathList);
    void          ResolvePostOperationConflicts();

    /**
     * Resizes the columns of the progress list so that the headings are visible.
     */
    void ResizeColumns();

    /// Predicate function to tell us if a notification data item is auxiliary or not
    static bool NotificationDataIsAux(const NotificationData* pData);

    // the commands to execute
    bool CmdAdd(CString& sWindowTitle, bool& localOperation);
    bool CmdCheckout(CString& sWindowTitle, bool& localoperation);
    bool CmdSparseCheckout(CString& sWindowTitle, bool& localoperation);
    bool CmdSingleFileCheckout(CString& sWindowTitle, bool& localOperation);
    bool CmdCommit(CString& sWindowTitle, bool& localoperation);
    bool CmdCopy(CString& sWindowTitle, bool& localoperation);
    bool CmdExport(CString& sWindowTitle, bool& localoperation);
    bool CmdImport(CString& sWindowTitle, bool& localoperation);
    bool CmdLock(CString& sWindowTitle, bool& localoperation);
    bool CmdMerge(CString& sWindowTitle, bool& localoperation);
    bool CmdMergeAll(CString& sWindowTitle, bool& localoperation);
    bool CmdMergeReintegrate(CString& sWindowTitle, bool& localoperation);
    bool CmdMergeReintegrateOldStyle(CString& sWindowTitle, bool& localoperation);
    bool CmdRename(CString& sWindowTitle, bool& localoperation);
    bool CmdResolve(CString& sWindowTitle, bool& localoperation);
    bool CmdRevert(CString& sWindowTitle, bool& localoperation);
    bool CmdSwitch(CString& sWindowTitle, bool& localoperation);
    bool CmdSwitchBackToParent(CString& sWindowTitle, bool& localoperation);
    bool CmdUnlock(CString& sWindowTitle, bool& localoperation);
    bool CmdUpdate(CString& sWindowTitle, bool& localoperation);

private:
    using StringRevMap         = std::map<CStringA, svn_revnum_t>;
    using StringWRevMap        = std::map<CString, svn_revnum_t>;
    using NotificationDataDeck = std::deque<NotificationData*>;

    CString              m_mergedfile;
    NotificationDataDeck m_arData;

    CWinThread*   m_pThread;
    volatile LONG m_bThreadRunning;

    ProjectProperties                               m_projectProperties;
    CListCtrl                                       m_progList;
    Command                                         m_command;
    int                                             m_options; // Use values from the ProgressOptions enum
    svn_depth_t                                     m_depth;
    CTSVNPathList                                   m_targetPathList;
    CTSVNPathList                                   m_selectedPaths;
    CTSVNPathList                                   m_origPathList;
    CTSVNPath                                       m_url;
    CTSVNPath                                       m_url2;
    CString                                         m_sMessage;
    CString                                         m_diffOptions;
    SVNRev                                          m_revision;
    SVNRev                                          m_revisionEnd;
    SVNRev                                          m_pegRev;
    SVNRevRangeArray                                m_revisionArray;
    SVNRevRangeArray                                m_mergedRevisions;
    CString                                         m_changeList;
    bool                                            m_keepChangeList;
    RevPropHash                                     m_revProps;
    SVNExternals                                    m_externals;
    std::map<CString, svn_depth_t>                  m_pathDepths;
    std::map<CString, std::tuple<CString, CString>> m_restorePaths;

    DWORD m_dwCloseOnEnd;
    DWORD m_bCloseLocalOnEnd;
    bool  m_hidden;
    bool  m_bRetryDone;

    CTSVNPath    m_basePath;
    StringRevMap m_updateStartRevMap;
    StringRevMap m_finishedRevMap;

    wchar_t m_columnBuf[MAX_PATH];

    BOOL m_bCancelled;
    int  m_nConflicts;
    int  m_nTotalConflicts;
    bool m_bConflictWarningShown;
    bool m_bWarningShown;
    bool m_bErrorsOccurred;
    bool m_bMergesAddsDeletesOccurred;
    bool m_bHookError;
    bool m_bNoHooks;
    bool m_bHooksAreOptional;
    bool m_bAuthorizationError;

    int  iFirstResized;
    BOOL bSecondResized;
    int  nEnsureVisibleCount;

    CString      m_sTotalBytesTransferred;
    CLinkControl m_jumpConflictControl;

    CColors m_colors;
    CFont   m_boldFont;

    bool m_bLockWarning;
    bool m_bLockExists;
    bool m_bFinishedItemAdded;
    bool m_bLastVisible;

    INT_PTR m_itemCount;
    INT_PTR m_itemCountTotal;

    CComPtr<IBugTraqProvider> m_bugTraqProvider;
    CComPtr<ITaskbarList3>    m_pTaskbarList;

    // some strings different methods can use
    CString sIgnoredIncluded;
    CString sExtExcluded;
    CString sExtIncluded;
    CString sIgnoreAncestry;
    CString sRespectAncestry;
    CString sDryRun;
    CString sRecordOnly;
    CString sForce;
};

static UINT WM_TASKBARBTNCREATED       = RegisterWindowMessage(L"TaskbarButtonCreated");
static UINT TORTOISESVN_CLOSEONEND_MSG = RegisterWindowMessage(L"TORTOISESVN_CLOSEONEND_MSG");
