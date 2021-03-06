// TortoiseSVN - a Windows shell extension for easy version control

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
#pragma once

#include "TSVNPath.h"
#include "RepositoryBar.h"
#include "StandAloneDlg.h"
#include "ProjectProperties.h"
#include "LogDialog/LogDlg.h"
#include "HintCtrl.h"
#include "RepositoryLister.h"
#include "ReaderWriterLock.h"

#include <list>

#define REPOBROWSER_CTRL_MIN_WIDTH 20
#define REPOBROWSER_FETCHTIMER     101

class CInputLogDlg;
class CTreeDropTarget;
class CListDropTarget;
class CRepositoryBrowserSelection;
class EditFileCommand;

/**
 * \ingroup TortoiseProc
 * helper class which holds the information for a tree item
 * in the repository browser.
 */
class CTreeItem
{
public:
    CTreeItem()
        : m_isExternal(false)
        , m_childrenFetched(false)
        , m_checkboxToggled(false)
        , m_hasChildFolders(false)
        , m_unversioned(false)
        , m_kind(svn_node_unknown)
        , m_svnParentPathRoot(false)
        , m_bookmark(false)
        , m_dummy(false)
    {
    }

    CString           m_unescapedName;
    SRepositoryInfo   m_repository;
    CString           m_url;             ///< escaped URL
    CString           m_logicalPath;     ///< concatenated unescapedname values
    bool              m_isExternal;      ///< if set, several operations may not be available
    bool              m_childrenFetched; ///< whether the contents of the folder are known/fetched or not
    bool              m_checkboxToggled; ///< whether the checkbox has been modified by the user
    bool              m_hasChildFolders;
    bool              m_unversioned;
    std::deque<CItem> m_children;
    CString           m_error;
    svn_node_kind_t   m_kind;
    bool              m_svnParentPathRoot;
    bool              m_bookmark;
    bool              m_dummy;
    SVNRev            m_revision;
};

/**
 * \ingroup TortoiseProc
 * Dialog to browse a repository.
 */
class CRepositoryBrowser : public CResizableStandAloneDialog
    , public SVN
    , public IRepo
{
    DECLARE_DYNAMIC(CRepositoryBrowser)
    friend class CBaseDropTarget;
    friend class CTreeDropTarget;
    friend class CListDropTarget;

public:
    CRepositoryBrowser(const CString& url, const SVNRev& rev);                ///< standalone repository browser
    CRepositoryBrowser(const CString& url, const SVNRev& rev, CWnd* pParent); ///< dependent repository browser
    ~CRepositoryBrowser() override;

    /// Returns the currently displayed revision only (for convenience)
    SVNRev GetRevision() const;
    /// Returns the currently displayed URL's path only (for convenience)
    CString GetPath() const;
    /// Returns the paths currently selected in the tree view / list view (for convenience)
    const CString& GetSelectedURLs() const;

    /// switches to the \c url at \c rev. If the url is valid and exists,
    /// the repository browser will show the content of that url.
    bool ChangeToUrl(CString& url, SVNRev& rev, bool bAlreadyChecked) override;

    void                           SetPegRev(SVNRev& pegRev);
    CString                        GetRepoRoot() override { return m_repository.root; }
    std::map<CString, svn_depth_t> GetCheckoutDepths() const { return m_checkoutDepths; }
    std::map<CString, svn_depth_t> GetUpdateDepths() const { return m_updateDepths; }

    void OnCbenDragbeginUrlcombo(NMHDR* pNMHDR, LRESULT* pResult) override;

    HWND   GetHWND() const override { return GetSafeHwnd(); }
    size_t GetHistoryForwardCount() const override { return m_urlHistoryForward.size(); }
    size_t GetHistoryBackwardCount() const override { return m_urlHistory.size(); }
    bool   IsThreadRunning() const override { return m_bThreadRunning; }
    void   SetSparseCheckoutMode(const CTSVNPath& path)
    {
        m_bSparseCheckoutMode = true;
        m_bStandAlone         = false;
        m_wcPath              = path;
    }
    void SetStandaloneMode(bool bStandAlone) { m_bStandAlone = bStandAlone; }
    /// overwrite SVN callbacks
    BOOL Cancel() override;

    enum
    {
        IDD           = IDD_REPOSITORY_BROWSER,
        WM_REFRESHURL = WM_USER + 10
    };

    /// the project properties if the repository browser was started from a working copy
    ProjectProperties m_projectProperties;
    /// the local path of the working copy
    CTSVNPath m_path;

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnOK() override;
    void OnCancel() override;
    BOOL PreTranslateMessage(MSG* pMsg) override;

    afx_msg void    OnBnClickedHelp();
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void    OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void    OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void    OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void    OnTvnSelchangedRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnTvnItemexpandingRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnTvnItemChangedRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMDblclkRepolist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnHdnItemclickRepolist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnItemchangedRepolist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMCustomdrawRepolist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMCustomdrawRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnBegindragRepolist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnBeginrdragRepolist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnTvnBegindragRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnTvnBeginrdragRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg void    OnLvnBeginlabeleditRepolist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnEndlabeleditRepolist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnTvnBeginlabeleditRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnTvnEndlabeleditRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnCaptureChanged(CWnd* pWnd);
    afx_msg void    OnTvnItemChangingRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMSetCursorRepotree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnAuthCancelled(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnSysColorChange();

    afx_msg void OnUrlFocus();
    afx_msg void OnCopy();
    afx_msg void OnInlineedit();
    afx_msg void OnRefresh();
    afx_msg void OnDelete();
    afx_msg void OnGoUp();
    afx_msg void OnUrlHistoryBack();
    afx_msg void OnUrlHistoryForward();

    DECLARE_MESSAGE_MAP()

    /// initializes variables for the constructors
    void ConstructorInit(const SVNRev& rev);
    /// called after the init thread has finished
    LRESULT OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/);
    /// called to update the tree node for a specific URL
    LRESULT OnRefreshURL(WPARAM /*wParam*/, LPARAM lParam);

    /// recursively removes all items from \c hItem on downwards.
    void RecursiveRemove(HTREEITEM hItem, bool bChildrenOnly = false);
    /// remove all tree nodes and empty the list view
    void ClearUI();
    /// searches the tree item for the specified \c fullurl.
    HTREEITEM FindUrl(const CString& fullUrl);
    /// searches the tree item for the specified \c fullurl.
    HTREEITEM FindUrl(const CString& fullUrl, const CString& url, HTREEITEM hItem = TVI_ROOT);

    /// read list of children; including externals
    void FetchChildren(HTREEITEM node);

    /// Find and return the node that corresponds to the specified
    /// logical \ref path. Add such node (including parents) if it
    /// does not exist, yet.
    HTREEITEM AutoInsert(const CString& path);
    /// Find and return the sub-node to \ref hParent that follows
    /// the spec in \ref item. Add such sub-node if it does not
    /// exist, yet.
    HTREEITEM AutoInsert(HTREEITEM hParent, const CItem& item);
    /// Like the previous version but inserts more than one item.
    void AutoInsert(HTREEITEM hParent, const std::deque<CItem>& items);
    /// Actual sub-node creation
    HTREEITEM Insert(HTREEITEM hParent, CTreeItem* parentTreeItem, const CItem& item);
    /// Sort tree sub-nodes
    void Sort(HTREEITEM parent);

    void RefreshChildren(HTREEITEM node);
    /**
     * Refetches the information for \c hNode. If \c force is true, then the list
     * control is refilled again.
     */
    bool RefreshNode(HTREEITEM hNode, bool force = false);
    /// fetches the status of the associated working copy, used to fill
    /// the check states in sparse checkout mode
    void GetStatus();
    /// Fills the list control with all the children of \c pTreeItem.
    void FillList(CTreeItem* pTreeItem);
    /// Open / enter folder for entry number \ref item
    void OpenFromList(int item);
    /// Open the file in the default application
    void OpenFile(const CTSVNPath& url, const CTSVNPath& urlEscaped, bool bOpenWith);
    /// C/o & lock the file and open it in the default application for modification
    void EditFile(CTSVNPath url, CTSVNPath urlEscaped);
    /// Sets the sort arrow in the list view header according to the currently used sorting.
    void SetSortArrow() const;
    /// compares strings for sorting
    static int SortStrCmp(PCWSTR str1, PCWSTR str2);
    /// called when a drag-n-drop operation starts
    void OnBeginDrag(NMHDR* pNMHDR);
    void OnBeginDragTree(NMHDR* pNMHDR);
    /// called when a drag-n-drop operation ends and the user dropped something on us.
    bool OnDrop(const CTSVNPath& target, const CString& root, const CTSVNPathList& pathlist, const SVNRev& srcRev, DWORD dwEffect, POINTL pt);
    /**
     * Since all urls we store and use are not properly escaped but "UI friendly", this
     * method converts those urls to a properly escaped url which we can use in
     * Subversion API calls.
     */
    static CString EscapeUrl(const CTSVNPath& url);
    /// Initializes the repository browser with a new root url
    void InitRepo();
    /// Helper function to show the "File Save" dialog
    bool AskForSavePath(const CRepositoryBrowserSelection& selection, CTSVNPath& tempFile, bool bFolder) const;

    /// Saves the column widths
    void SaveColumnWidths(bool bSaveToRegistry = false);
    /// converts a string to an array of column widths
    static bool StringToWidthArray(const CString& widthString, int widthArray[]);
    /// converts an array of column widths to a string
    CString WidthArrayToString(int widthArray[]) const;
    void    SetRightDrag(bool bRightDrag) { m_bRightDrag = bRightDrag; }

    /// remove items for the associated URL sub-tree
    /// from the \ref m_lister cache.
    void InvalidateData(HTREEITEM node);
    void InvalidateData(HTREEITEM node, const SVNRev& revision);

    /// assume that the selected urls may have become invalid
    /// and reset the the cache accordingly
    void InvalidateDataParents(const CRepositoryBrowserSelection& selection);

    static UINT InitThreadEntry(LPVOID pVoid);
    UINT        InitThread();

    static int CALLBACK TreeSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3);
    static int CALLBACK ListSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3);

    void BeginDrag(const CWnd& window, CRepositoryBrowserSelection& selection,
                   POINT& point, bool setAsyncMode) const;

    /// extract info from controls before they get destroyed
    void StoreSelectedURLs();

    /// tries to fetch the html page returned by an apache server
    /// set up with the SVNParentPath directive, parse all the listed
    /// repositories and fill them in to the repo browser
    bool TrySVNParentPath();

    /// resizes the control so that the divider is at position 'point'
    void        HandleDividerMove(CPoint point);
    bool        CheckoutDepthForItem(HTREEITEM hItem);
    void        CheckTreeItem(HTREEITEM hItem, bool bCheck);
    void        CheckTreeItemRecursive(HTREEITEM hItem, bool bCheck);
    bool        HaveAllChildrenSameCheckState(HTREEITEM hItem, bool bChecked = false) const;
    void        CheckParentsOfTreeItem(HTREEITEM hItem);
    static bool CheckAndConfirmPath(const CTSVNPath& path);
    void        SaveDividerPosition() const;

    void        ShowText(const CString& sText, bool forceupdate = false);
    static void FilterInfinityDepthItems(std::map<CString, svn_depth_t>& depths);
    void        FilterUnknownDepthItems(std::map<CString, svn_depth_t>& depths);
    void        SetListItemInfo(int index, const CItem* it);

    bool      RunStartCommit(const CTSVNPathList& pathlist, CString& sLogMsg) const;
    bool      RunPreCommit(const CTSVNPathList& pathlist, svn_depth_t depth, CString& sMsg) const;
    bool      RunPostCommit(const CTSVNPathList& pathlist, svn_depth_t depth, svn_revnum_t revEnd, const CString& sMsg) const;
    void      LoadBookmarks();
    void      SaveBookmarks();
    HTREEITEM FindBookmarkRoot() const;
    void      RefreshBookmarks();
    CString   GetUrlWebViewerRev(CRepositoryBrowserSelection& selection) const;
    CString   GetUrlWebViewerPathRev(CRepositoryBrowserSelection& selection) const;

protected:
    bool              m_bInitDone;
    CRepositoryBar    m_barRepository;
    CRepositoryBarCnr m_cnrRepositoryBar;

    CHintCtrl<CTreeCtrl> m_repoTree;
    CHintCtrl<CListCtrl> m_repoList;

    SRepositoryInfo m_repository;

    HACCEL m_hAccel;

private:
    bool m_cancelled;

    bool              m_bStandAlone;
    bool              m_bSparseCheckoutMode;
    CString           m_initialUrl;
    CTSVNPath         m_redirectedUrl;
    CTSVNPath         m_wcPath;
    CString           m_selectedUrLs; ///< only valid after <OK>
    CString           m_initialFilename;
    volatile LONG     m_bThreadRunning;
    static const UINT AFTER_INIT_MESSAGE;
    bool              m_bFetchChildren;
    bool              m_bShowExternals;
    bool              m_bShowLocks;
    bool              m_bTrySVNParentPath;
    CTreeItem*        m_pListCtrlTreeItem;

    int m_nBookmarksIcon;
    int m_nIconFolder;
    int m_nOpenIconFolder;
    int m_nExternalOvl;
    int m_nSVNParentPath;

    volatile int m_blockEvents;

    static bool m_bSortLogical;
    bool        m_bSortAscending;
    int         m_nSortedColumn;
    int         m_arColumnWidths[7];
    int         m_arColumnAutoWidths[7];

    CTreeDropTarget* m_pTreeDropTarget;
    CListDropTarget* m_pListDropTarget;
    bool             m_bRightDrag;

    int  m_oldY, m_oldX;
    bool bDragMode;

    svn_node_kind_t m_diffKind;
    CTSVNPath       m_diffURL;

    CString m_origDlgTitle;

    CRepositoryLister              m_lister;
    std::map<CString, svn_depth_t> m_checkoutDepths;
    std::map<CString, svn_depth_t> m_updateDepths;
    std::list<CString>             m_urlHistory;
    std::list<CString>             m_urlHistoryForward;
    std::map<CString, svn_depth_t> m_wcDepths;

    std::set<std::wstring>           m_bookmarks;
    std::unique_ptr<EditFileCommand> m_editFileCommand;

    /// used to execute user ops (e.g. context menu actions) in the background
    async::CJobScheduler m_backgroundJobs;
    CReaderWriterLock    m_guard;
};

static UINT WM_AFTERINIT = RegisterWindowMessage(L"TORTOISESVN_AFTERINIT_MSG");
