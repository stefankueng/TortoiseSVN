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

#include "TSVNPath.h"
#include "SVNRev.h"
#include "Colors.h"
#include "DragDropImpl.h"

class SVNConfig;
class SVNStatus;
class CSVNStatusListCtrlDropTarget;

#define SVNSLC_COLEXT			0x000000002
#define SVNSLC_COLSTATUS		0x000000004
#define SVNSLC_COLREMOTESTATUS	0x000000008
#define SVNSLC_COLTEXTSTATUS	0x000000010
#define SVNSLC_COLPROPSTATUS	0x000000020
#define SVNSLC_COLREMOTETEXT	0x000000040
#define SVNSLC_COLREMOTEPROP	0x000000080
#define SVNSLC_COLURL			0x000000100
#define SVNSLC_COLLOCK			0x000000200
#define SVNSLC_COLLOCKCOMMENT	0x000000400
#define SVNSLC_COLAUTHOR		0x000000800
#define	SVNSLC_COLREVISION		0x000001000
#define	SVNSLC_COLDATE			0x000002000
#define SVNSLC_COLSVNNEEDSLOCK	0x000004000
#define SVNSLC_COLCOPYFROM		0x000008000
#define SVNSLC_NUMCOLUMNS		16

#define SVNSLC_SHOWUNVERSIONED	0x000000001
#define SVNSLC_SHOWNORMAL		0x000000002
#define SVNSLC_SHOWMODIFIED		0x000000004
#define SVNSLC_SHOWADDED		0x000000008
#define SVNSLC_SHOWREMOVED		0x000000010
#define SVNSLC_SHOWCONFLICTED	0x000000020
#define SVNSLC_SHOWMISSING		0x000000040
#define SVNSLC_SHOWREPLACED		0x000000080
#define SVNSLC_SHOWMERGED		0x000000100
#define SVNSLC_SHOWIGNORED		0x000000200
#define SVNSLC_SHOWOBSTRUCTED	0x000000400
#define SVNSLC_SHOWEXTERNAL		0x000000800
#define SVNSLC_SHOWINCOMPLETE	0x000001000
#define SVNSLC_SHOWINEXTERNALS	0x000002000
#define SVNSLC_SHOWREMOVEDANDPRESENT 0x000004000
#define SVNSLC_SHOWLOCKS		0x000008000
#define SVNSLC_SHOWDIRECTFILES	0x000010000
#define SVNSLC_SHOWDIRECTFOLDER 0x000020000
#define SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO 0x000040000
#define SVNSLC_SHOWSWITCHED		0x000080000

#define SVNSLC_SHOWDIRECTS		(SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWDIRECTFOLDER)


#define SVNSLC_SHOWVERSIONED (SVNSLC_SHOWNORMAL|SVNSLC_SHOWMODIFIED|\
SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWCONFLICTED|SVNSLC_SHOWMISSING|\
SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|SVNSLC_SHOWIGNORED|SVNSLC_SHOWOBSTRUCTED|\
SVNSLC_SHOWEXTERNAL|SVNSLC_SHOWINCOMPLETE|SVNSLC_SHOWINEXTERNALS|\
SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO)

#define SVNSLC_SHOWVERSIONEDBUTNORMAL (SVNSLC_SHOWMODIFIED|SVNSLC_SHOWADDED|\
SVNSLC_SHOWREMOVED|SVNSLC_SHOWCONFLICTED|SVNSLC_SHOWMISSING|\
SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|SVNSLC_SHOWIGNORED|SVNSLC_SHOWOBSTRUCTED|\
SVNSLC_SHOWEXTERNAL|SVNSLC_SHOWINCOMPLETE|SVNSLC_SHOWINEXTERNALS|\
SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO)

#define SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS (SVNSLC_SHOWMODIFIED|\
SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWCONFLICTED|SVNSLC_SHOWMISSING|\
SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|SVNSLC_SHOWIGNORED|SVNSLC_SHOWOBSTRUCTED|\
SVNSLC_SHOWINCOMPLETE|SVNSLC_SHOWEXTERNAL|SVNSLC_SHOWINEXTERNALS)

#define SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS (SVNSLC_SHOWMODIFIED|\
	SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWCONFLICTED|SVNSLC_SHOWMISSING|\
	SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|SVNSLC_SHOWIGNORED|SVNSLC_SHOWOBSTRUCTED|\
	SVNSLC_SHOWINCOMPLETE)

#define SVNSLC_SHOWALL (SVNSLC_SHOWVERSIONED|SVNSLC_SHOWUNVERSIONED)

#define SVNSLC_POPALL					0xFFFFFFFF
#define SVNSLC_POPCOMPAREWITHBASE		0x00000001
#define SVNSLC_POPCOMPARE				0x00000002
#define SVNSLC_POPGNUDIFF				0x00000004
#define SVNSLC_POPREVERT				0x00000008
#define SVNSLC_POPUPDATE				0x00000010
#define SVNSLC_POPSHOWLOG				0x00000020
#define SVNSLC_POPOPEN					0x00000040
#define SVNSLC_POPDELETE				0x00000080
#define SVNSLC_POPADD					0x00000100
#define SVNSLC_POPIGNORE				0x00000200
#define SVNSLC_POPCONFLICT				0x00000400
#define SVNSLC_POPRESOLVE				0x00000800
#define SVNSLC_POPLOCK					0x00001000
#define SVNSLC_POPUNLOCK				0x00002000
#define SVNSLC_POPUNLOCKFORCE			0x00004000
#define SVNSLC_POPEXPLORE				0x00008000
#define SVNSLC_POPCOMMIT				0x00010000
#define SVNSLC_POPPROPERTIES			0x00020000
#define SVNSLC_POPREPAIRMOVE			0x00040000
#define SVNSLC_POPCHANGELISTS			0x00080000


typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);
typedef CComCritSecLock<CComCriticalSection> Locker;

/**
 * \ingroup SVN
 * A List control, based on the MFC CListCtrl which shows a list of
 * files with their Subversion status. The control also provides a context
 * menu to do some Subversion tasks on the selected files.
 *
 * This is the main control used in many dialogs to show a list of files to
 * work on.
 */
class CSVNStatusListCtrl : public CListCtrl
{
public:

	/**
	 * Sent to the parent window (using ::SendMessage) after a context menu
	 * command has finished if the item count has changed.
	 */
	static const UINT SVNSLNM_ITEMCOUNTCHANGED;
	/**
	 * Sent to the parent window (using ::SendMessage) when the control needs
	 * to be refreshed. Since this is done usually in the parent window using
	 * a thread, this message is used to tell the parent to do exactly that.
	 */
	static const UINT SVNSLNM_NEEDSREFRESH;

	/**
	 * Sent to the parent window (using ::SendMessage) when the user drops
	 * files on the control. The LPARAM is a pointer to a TCHAR string
	 * containing the dropped path.
	 */
	static const UINT SVNSLNM_ADDFILE;

	/**
	 * Sent to the parent window (using ::SendMessage) when the user checks/unchecks
	 * one or more items in the control. The WPARAM contains the number of
	 * checked items in the control.
	 */
	static const UINT SVNSLNM_CHECKCHANGED;

	CSVNStatusListCtrl();
	~CSVNStatusListCtrl();

	/**
	 * \ingroup TortoiseProc
	 * Helper class for CSVNStatusListCtrl which represents
	 * the data for each file shown.
	 */
	class FileEntry
	{
	public:
		FileEntry() : status(svn_wc_status_unversioned)
			, copyfrom_rev(0)
			, last_commit_date(0)
			, last_commit_rev(0)
			, textstatus(svn_wc_status_unversioned)
			, propstatus(svn_wc_status_unversioned)
			, remotestatus(svn_wc_status_unversioned)
			, remotetextstatus(svn_wc_status_unversioned)
			, remotepropstatus(svn_wc_status_unversioned)
			, copied(false)
			, switched(false)
			, checked(false)
			, inunversionedfolder(false)
			, inexternal(false)
			, differentrepo(false)
			, direct(false)
			, isfolder(false)
			, isNested(false)
			, Revision(0)
			, isConflicted(false)
			, present_props(_T(""))
		{
		}
		const CTSVNPath& GetPath() const
		{
			return path;
		}
		const bool IsChecked() const
		{
			return checked;
		}
		CString GetRelativeSVNPath() const
		{
			if (path.IsEquivalentTo(basepath))
				return path.GetSVNPathString();
			return path.GetSVNPathString().Mid(basepath.GetSVNPathString().GetLength()+1);
		}
		const bool IsLocked() const
		{
			return !(lock_token.IsEmpty() && lock_remotetoken.IsEmpty());
		}
		const bool IsFolder() const
		{
			return isfolder;
		}
		const bool IsInExternal() const
		{
			return inexternal;
		}
		const bool IsFromDifferentRepository() const
		{
			return differentrepo;
		}
		CString GetDisplayName() const
		{
			CString const& chopped = path.GetDisplayString(&basepath);
			if (!chopped.IsEmpty())
			{
				return chopped;
			}
			else
			{
				// "Display name" must not be empty.
				return path.GetFileOrDirectoryName();
			}
		}
		CString GetChangeList() const
		{
			return changelist;
		}
	public:
		svn_wc_status_kind		status;					///< local status
	private:
		CTSVNPath				path;					///< full path of the file
		CTSVNPath				basepath;				///< common ancestor path of all files
		CString					url;					///< Subversion URL of the file
		CString					lock_token;				///< universally unique URI representing lock
		CString					lock_owner;				///< the username which owns the lock
		CString					lock_remoteowner;		///< the username which owns the lock in the repository
		CString					lock_remotetoken;		///< the unique URI in the repository of the lock
		CString					lock_comment;			///< the message for the lock
		CString					changelist;				///< the name of the changelist the item belongs to
		CString					copyfrom_url;			///< the copied-from URL (if available, i.e. \a copied is true)
		svn_revnum_t			copyfrom_rev;			///< the copied-from revision
		CString					last_commit_author;		///< the author which last committed this item
		apr_time_t				last_commit_date;		///< the date when this item was last committed
		svn_revnum_t			last_commit_rev;		///< the revision where this item was last committed
		svn_wc_status_kind		textstatus;				///< local text status
		svn_wc_status_kind		propstatus;				///< local property status
		svn_wc_status_kind		remotestatus;			///< remote status
		svn_wc_status_kind		remotetextstatus;		///< remote text status
		svn_wc_status_kind		remotepropstatus;		///< remote property status
		bool					copied;					///< if the file/folder is added-with-history
		bool					switched;				///< if the file/folder is switched to another url
		bool					checked;				///< if the file is checked in the list control
		bool					inunversionedfolder;	///< if the file is inside an unversioned folder
		bool					inexternal;				///< if the item is in an external folder
		bool					differentrepo;			///< if the item is from a different repository than the rest
		bool					direct;					///< directly included (TRUE) or just a child of a folder
		bool					isfolder;				///< TRUE if entry refers to a folder
		bool					isNested;				///< TRUE if the folder from a different repository and/or path
		bool					isConflicted;			///> TRUE if a file entry is conflicted, i.e. if it has the conflicted paths set
		svn_revnum_t			Revision;				///< the base revision
		CString					present_props;			///< cacheable properties present in BASE
		friend class CSVNStatusListCtrl;
	};

	/**
	 * Initializes the control, sets up the columns.
	 * \param dwColumns mask of columns to show. Use the SVNSLC_COLxxx defines.
	 * \param sColumnInfoContainer Name of a registry key
	 *                             where the position and visibility of each column
	 *                             is saved and used from. If the registry key
	 *                             doesn't exist, the default order is used
	 *                             and dwColumns tells which columns are visible.
	 * \param dwContextMenus mask of context menus to be active, not all make sense for every use of this control.
	 *                       Use the SVNSLC_POPxxx defines.
	 * \param bHasCheckboxes TRUE if the control should show checkboxes on the left of each file entry.
	 */
	void Init(DWORD dwColumns, const CString& sColumnInfoContainer, DWORD dwContextMenus = (SVNSLC_POPALL ^ SVNSLC_POPCOMMIT), bool bHasCheckboxes = true);
	/**
	 * Sets a background image for the list control.
	 * The image is shown in the right bottom corner.
	 * \param nID the resource ID of the bitmap to use as the background
	 */
	bool SetBackgroundImage(UINT nID);
	/**
	 * Makes the 'ignore' context menu only ignore the files and not add the
	 * folder which gets the svn:ignore property changed to the list.
	 * This is needed e.g. for the Add-dialog, where the modified folder
	 * showing up would break the resulting "add" command.
	 */
	void SetIgnoreRemoveOnly(bool bRemoveOnly = true) {m_bIgnoreRemoveOnly = bRemoveOnly;}
	/**
	 * The unversioned items are by default shown after all other files in the list.
	 * If that behavior should be changed, set this value to false.
	 */
	void PutUnversionedLast(bool bLast) {m_bUnversionedLast = bLast;}
	/**
	 * Fetches the Subversion status of all files and stores the information
	 * about them in an internal array.
	 * \param sFilePath path to a file which contains a list of files and/or folders for which to
	 *                  fetch the status, separated by newlines.
	 * \param bUpdate TRUE if the remote status is requested too.
	 * \return TRUE on success.
	 */
	BOOL GetStatus(const CTSVNPathList& pathList, bool bUpdate = false, bool bShowIgnores = false);

	/**
	 * Populates the list control with the previously (with GetStatus) gathered status information.
	 * \param dwShow mask of file types to show. Use the SVNSLC_SHOWxxx defines.
	 * \param dwCheck mask of file types to check. Use SVNLC_SHOWxxx defines. Default (0) means 'use the entry's stored check status'
	 */
	void Show(DWORD dwShow, DWORD dwCheck = 0, bool bShowFolders = true);
	void Show(DWORD dwShow, const CTSVNPathList& checkedList, bool bShowFolders = true);

	/**
	 * Copies the selected entries in the control to the clipboard. The entries
	 * are separated by newlines.
	 * \param dwCols the columns to copy. Each column is separated by a tab.
	 */
	bool CopySelectedEntriesToClipboard(DWORD dwCols);

	/**
	 * If during the call to GetStatus() some svn:externals are found from different
	 * repositories than the first one checked, then this method returns TRUE.
	 */
	BOOL HasExternalsFromDifferentRepos() const {return m_bHasExternalsFromDifferentRepos;}

	/**
	 * If during the call to GetStatus() some svn:externals are found then this method returns TRUE.
	 */
	BOOL HasExternals() const {return m_bHasExternals;}

	/**
	 * If unversioned files are found (but not necessarily shown) TRUE is returned.
	 */
	BOOL HasUnversionedItems() {return m_bHasUnversionedItems;}

	/**
	 * If there are any locks in the working copy, TRUE is returned
	 */
	BOOL HasLocks() const {return m_bHasLocks;}

	/**
	 * If there are any changelists defined in the working copy, TRUE is returned
	 */
	BOOL HasChangeLists() const {return m_bHasChangeLists;}

	/**
	 * Returns the file entry data for the list control index.
	 */
	CSVNStatusListCtrl::FileEntry * GetListEntry(int index);

	/**
	 * Returns a String containing some statistics like number of modified, normal, deleted,...
	 * files.
	 */
	CString GetStatisticsString();

	/**
	 * Set a static control which will be updated automatically with
	 * the number of selected and total files shown in the list control.
	 */
	void SetStatLabel(CWnd * pStatLabel){m_pStatLabel = pStatLabel;};

	/**
	 * Set a tri-state checkbox which is updated automatically if the
	 * user checks/unchecks file entries in the list control to indicate
	 * if all files are checked, none are checked or some are checked.
	 */
	void SetSelectButton(CButton * pButton) {m_pSelectButton = pButton;}

	/**
	 * Set a button which is de-/activated automatically. The button is
	 * only set active if at least one item is selected.
	 */
	void SetConfirmButton(CButton * pButton) {m_pConfirmButton = pButton;}

	/**
	 * Select/unselect all entries in the list control.
	 * \param bSelect TRUE to check, FALSE to uncheck.
	 */
	void SelectAll(bool bSelect);

	/** Set a checkbox on an entry in the listbox
	 * Keeps the listctrl checked state and the FileEntry's checked flag in sync
	 */
	void SetEntryCheck(FileEntry* pEntry, int listboxIndex, bool bCheck);

	/** Write a list of the checked items' paths into a path list
	 */
	void WriteCheckedNamesToPathList(CTSVNPathList& pathList);

	/** fills in \a lMin and \a lMax with the lowest/highest revision of all
	 * files/folders in the working copy.
	 * \param bShownOnly if true, the min/max revisions are calculated only for shown items
	 * \param bCheckedOnly if true, the min/max revisions are calculated only for items 
	 *                   which are checked.
	 * \remark Since an item can only be checked if it is visible/shown in the list control
	 *         bShownOnly is automatically set to true if bCheckedOnly is true
	 */
	void GetMinMaxRevisions(svn_revnum_t& rMin, svn_revnum_t& rMax, bool bShownOnly, bool bCheckedOnly);

	/**
	 * Returns the parent directory of all entries in the control.
	 * if \a bStrict is set to false, then the paths passed to the control
	 * to fetch the status (in GetStatus()) are used if possible.
	 */
	CTSVNPath GetCommonDirectory(bool bStrict);

	/**
	 * Sets a pointer to a boolean variable which is checked periodically
	 * during the status fetching. As soon as the variable changes to true,
	 * the operations stops.
	 */
	void SetCancelBool(bool * pbCanceled) {m_pbCanceled = pbCanceled;}

	/**
	 * Sets the string shown in the control while the status is fetched.
	 * If not set, it defaults to "please wait..."
	 */
	void SetBusyString(const CString& str) {m_sBusy = str;}
	void SetBusyString(UINT id) {m_sBusy.LoadString(id);}

	/**
	 * Sets the string shown in the control if no items are shown. This
	 * can happen for example if there's nothing modified and the unversioned
	 * files aren't shown either, so there's nothing to commit.
	 * If not set, it defaults to "file list is empty".
	 */
	void SetEmptyString(const CString& str) {m_sEmpty = str;}
	void SetEmptyString(UINT id) {m_sEmpty.LoadString(id);}

	/**
	 * Determines if the control should recurse into unversioned folders
	 * when fetching the status. The default behavior is defined by the
	 * registry key HKCU\Software\TortoiseSVN\UnversionedRecurse, which
	 * is read in the Init() method.
	 * If you want to change the behavior, call this method *after*
	 * calling Init().
	 */
	void SetUnversionedRecurse(bool bUnversionedRecurse) {m_bUnversionedRecurse = bUnversionedRecurse;}

	/**
	 * Returns the number of selected items
	 */
	LONG GetSelected(){return m_nSelected;};

	/**
	 * Enables dropping of files on the control.
	 */
	bool EnableFileDrop();

	/**
	 * Checks if the path already exists in the list.
	 */
	bool HasPath(CTSVNPath path);

	/**
	 * Forces the children to be checked when the parent folder is checked,
	 * and the parent folder to be unchecked if one of its children is unchecked.
	 */
	void CheckChildrenWithParent(bool bCheck) {m_bCheckChildrenWithParent = bCheck;}
public:
	CString GetLastErrorMessage() {return m_sLastError;}

	void Block(BOOL block) {m_bBlock = block;}

	LONG						m_nTargetCount;		///< number of targets in the file passed to GetStatus()

	CString						m_sURL;				///< the URL of the target or "(multiple targets)"

	SVNRev						m_HeadRev;			///< the HEAD revision of the repository if bUpdate was TRUE

	CString						m_sUUID;			///< the UUID of the associated repository

	DECLARE_MESSAGE_MAP()

private:
	void SaveColumnWidths(bool bSaveToRegistry = false);
	void Sort();	///< Sorts the control by columns
	void AddEntry(FileEntry * entry, WORD langID, int listIndex);	///< add an entry to the control
	void RemoveListEntry(int index);	///< removes an entry from the listcontrol and both arrays
	bool BuildStatistics();	///< build the statistics and correct the case of files/folders
	void StartDiff(int fileindex);	///< start the external diff program
	static bool CSVNStatusListCtrl::SortCompare(const FileEntry* entry1, const FileEntry* entry2);

	/// Process one line of the command file supplied to GetStatus
	bool FetchStatusForSingleTarget(SVNConfig& config, SVNStatus& status, const CTSVNPath& target, bool bFetchStatusFromRepository, CStringA& strCurrentRepositoryUUID, CTSVNPathList& arExtPaths, bool bAllDirect, bool recurse = true, bool bShowIgnores = false);

	/// Create 'status' data for each item in an unversioned folder
	void AddUnversionedFolder(const CTSVNPath& strFolderName, const CTSVNPath& strBasePath, SVNConfig * config);

	/// Read the all the other status items which result from a single GetFirstStatus call
	void ReadRemainingItemsStatus(SVNStatus& status, const CTSVNPath& strBasePath, CStringA& strCurrentRepositoryUUID, CTSVNPathList& arExtPaths, SVNConfig * config, bool bAllDirect);

	/// Clear the status vector (contains custodial pointers)
	void ClearStatusArray();

	/// Sort predicate function - Compare the paths of two entries without regard to case
	static bool EntryPathCompareNoCase(const FileEntry* pEntry1, const FileEntry* pEntry2);

	/// Predicate used to build a list of only the versioned entries of the FileEntry array
	static bool IsEntryVersioned(const FileEntry* pEntry1);

	/// Look up the relevant show flags for a particular SVN status value
	DWORD GetShowFlagsFromSVNStatus(svn_wc_status_kind status);

	/// Build a FileEntry item and add it to the FileEntry array
	const FileEntry* AddNewFileEntry(
		const svn_wc_status2_t* pSVNStatus,  // The return from the SVN GetStatus functions
		const CTSVNPath& path,				// The path of the item we're adding
		const CTSVNPath& basePath,			// The base directory for this status build
		bool bDirectItem,					// Was this item the first found by GetFirstFileStatus or by a subsequent GetNextFileStatus call
		bool bInExternal,					// Are we in an 'external' folder
		bool bEntryfromDifferentRepo		// if the entry is from a different repository
		);

	/// Adjust the checkbox-state on all descendents of a specific item
	void SetCheckOnAllDescendentsOf(const FileEntry* parentEntry, bool bCheck);

	/// Build a path list of all the selected items in the list (NOTE - SELECTED, not CHECKED)
	void FillListOfSelectedItemPaths(CTSVNPathList& pathList, bool bNoIgnored = false);

	/// Enables/Disables group view and adds all groups to the list control.
	/// If bForce is true, then groupview is enabled and the 'null' group is added.
	bool PrepareGroups(bool bForce = false);
	/// Returns the group number to which the group header belongs
	/// If the point is not over a group header, -1 is returned
	int GetGroupFromPoint(POINT * ppt);
	/// Returns the number of changelists the selection has
	int GetNumberOfChangelistsInSelection();

	/// Puts the item to the corresponding group
	bool SetItemGroup(int item, int groupindex);

	void CheckEntry(int index, int nListItems);
	void UncheckEntry(int index, int nListItems);

	/// sends an SVNSLNM_CHECKCHANGED notification to the parent
	void NotifyCheck();

	int CellRectFromPoint(CPoint& point, RECT *cellrect, int *col) const;

	bool StringToOrderArray(const CString& OrderString, int OrderArray[]);
	CString OrderArrayToString(int OrderArray[]);
	bool StringToWidthArray(const CString& WidthString, int WidthArray[]);
	CString WidthArrayToString(int WidthArray[]);

	void HideColumn(int col);
	void ShowColumn(int col);

	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnNMReturn(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg void OnHdnBegintrack(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnItemchanging(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDestroy();

private:
	bool *						m_pbCanceled;
	static bool					m_bAscending;		///< sort direction
	static int					m_nSortedColumn;	///< which column to sort
	bool						m_bUnversionedLast;
	bool						m_bHasExternalsFromDifferentRepos;
	bool						m_bHasExternals;
	BOOL						m_bHasUnversionedItems;
	bool						m_bHasLocks;
	bool						m_bHasChangeLists;
	typedef std::vector<FileEntry*> FileEntryVector;
	FileEntryVector				m_arStatusArray;
	std::vector<DWORD>			m_arListArray;
	std::map<CString, int>		m_changelists;
	CTSVNPathList				m_ConflictFileList;
	CTSVNPathList				m_StatusFileList;
	CString						m_sLastError;

	LONG						m_nUnversioned;
	LONG						m_nNormal;
	LONG						m_nModified;
	LONG						m_nAdded;
	LONG						m_nDeleted;
	LONG						m_nConflicted;
	LONG						m_nTotal;
	LONG						m_nSelected;

	DWORD						m_dwColumns;
	DWORD						m_dwShow;
	bool						m_bShowFolders;
	bool						m_bShowIgnores;
	bool						m_bUpdate;
	DWORD						m_dwContextMenus;
	BOOL						m_bBlock;
	bool						m_bBusy;
	bool						m_bEmpty;
	bool						m_bIgnoreRemoveOnly;

	int							m_nIconFolder;

	CWnd *						m_pStatLabel;
	CButton *					m_pSelectButton;
	CButton *					m_pConfirmButton;
	CColors						m_Colors;

	CString						m_sEmpty;
	CString						m_sBusy;

	bool						m_bUnversionedRecurse;
	DWORD						m_ColumnShown[SVNSLC_NUMCOLUMNS];
	CString						m_sColumnInfoContainer;
	int							m_arColumnWidths[SVNSLC_NUMCOLUMNS];

	bool						m_bCheckChildrenWithParent;
	CSVNStatusListCtrlDropTarget * m_pDropTarget;

	std::map<CString,bool>		m_mapFilenameToChecked; ///< Remember manually de-/selected items
	CComCriticalSection			m_critSec;
};

class CSVNStatusListCtrlDropTarget : public CIDropTarget
{
public:
	CSVNStatusListCtrlDropTarget(HWND hTargetWnd):CIDropTarget(hTargetWnd){}
	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD * /*pdwEffect*/)
	{
		if(pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
		{
			HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
			if(hDrop != NULL)
			{
				TCHAR szFileName[MAX_PATH];

				UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
				for(UINT i = 0; i < cFiles; ++i)
				{
					DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));
					HWND hParentWnd = GetParent(m_hTargetWnd);
					if (hParentWnd != NULL)
						::SendMessage(hParentWnd, CSVNStatusListCtrl::SVNSLNM_ADDFILE, 0, (LPARAM)szFileName);
				}
			}
			GlobalUnlock(medium.hGlobal);
		}
		return true; //let base free the medium
	}

};
