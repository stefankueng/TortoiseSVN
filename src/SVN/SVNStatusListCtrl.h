﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2017-2022 - TortoiseSVN

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
#pragma warning(push)
#include "svn_wc.h"
#pragma warning(pop)

#include "TSVNPath.h"
#include "SVNRev.h"
#include "Colors.h"
#include "DragDropImpl.h"
#include "ReaderWriterLock.h"
#include "SVNProperties.h"

class SVNConfig;
class SVNStatus;
class CSVNStatusListCtrlDropTarget;

// these defines must be in the order the columns are inserted!
#define SVNSLC_COLFILEPATH         0x000000001
#define SVNSLC_COLFILENAME         0x000000002
#define SVNSLC_COLEXT              0x000000004
#define SVNSLC_COLSTATUS           0x000000008
#define SVNSLC_COLREMOTESTATUS     0x000000010
#define SVNSLC_COLTEXTSTATUS       0x000000020
#define SVNSLC_COLPROPSTATUS       0x000000040
#define SVNSLC_COLREMOTETEXT       0x000000080
#define SVNSLC_COLREMOTEPROP       0x000000100
#define SVNSLC_COLDEPTH            0x000000200
#define SVNSLC_COLURL              0x000000400
#define SVNSLC_COLLOCK             0x000000800
#define SVNSLC_COLLOCKCOMMENT      0x000001000
#define SVNSLC_COLLOCKDATE         0x000002000
#define SVNSLC_COLAUTHOR           0x000004000
#define SVNSLC_COLREVISION         0x000008000
#define SVNSLC_COLREMOTEREVISION   0x000010000
#define SVNSLC_COLDATE             0x000020000
#define SVNSLC_COLMODIFICATIONDATE 0x000040000
#define SVNSLC_COLSIZE             0x000080000
#define SVNSLC_NUMCOLUMNS          20

#define SVNSLC_SHOWUNVERSIONED               0x00000001
#define SVNSLC_SHOWNORMAL                    0x00000002
#define SVNSLC_SHOWMODIFIED                  0x00000004
#define SVNSLC_SHOWADDED                     0x00000008
#define SVNSLC_SHOWREMOVED                   0x00000010
#define SVNSLC_SHOWCONFLICTED                0x00000020
#define SVNSLC_SHOWMISSING                   0x00000040
#define SVNSLC_SHOWREPLACED                  0x00000080
#define SVNSLC_SHOWMERGED                    0x00000100
#define SVNSLC_SHOWIGNORED                   0x00000200
#define SVNSLC_SHOWOBSTRUCTED                0x00000400
#define SVNSLC_SHOWEXTERNAL                  0x00000800
#define SVNSLC_SHOWINCOMPLETE                0x00001000
#define SVNSLC_SHOWINEXTERNALS               0x00002000
#define SVNSLC_SHOWREMOVEDANDPRESENT         0x00004000
#define SVNSLC_SHOWLOCKS                     0x00008000
#define SVNSLC_SHOWDIRECTFILES               0x00010000
#define SVNSLC_SHOWDIRECTFOLDER              0x00020000
#define SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO 0x00040000
#define SVNSLC_SHOWSWITCHED                  0x00080000
#define SVNSLC_SHOWINCHANGELIST              0x00100000
#define SVNSLC_SHOWEXTDISABLED               0x00200000
#define SVNSLC_SHOWNESTED                    0x00400000
#define SVNSLC_SHOWFILES                     0x00800000
#define SVNSLC_SHOWFOLDERS                   0x01000000
#define SVNSLC_SHOWADDEDHISTORY              0x02000000
#define SVNSLC_SHOWADDEDINADDED              0x04000000

#define SVNSLC_SHOWEVERYTHING 0xFFFFFFFF

#define SVNSLC_SHOWDIRECTS (SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWDIRECTFOLDER)

#define SVNSLC_SHOWVERSIONED (SVNSLC_SHOWNORMAL | SVNSLC_SHOWMODIFIED |                                                                      \
                              SVNSLC_SHOWADDED | SVNSLC_SHOWADDEDHISTORY | SVNSLC_SHOWREMOVED | SVNSLC_SHOWCONFLICTED | SVNSLC_SHOWMISSING | \
                              SVNSLC_SHOWREPLACED | SVNSLC_SHOWMERGED | SVNSLC_SHOWIGNORED | SVNSLC_SHOWOBSTRUCTED |                         \
                              SVNSLC_SHOWEXTERNAL | SVNSLC_SHOWINCOMPLETE | SVNSLC_SHOWINEXTERNALS |                                         \
                              SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO)

#define SVNSLC_SHOWVERSIONEDBUTNORMAL (SVNSLC_SHOWMODIFIED | SVNSLC_SHOWADDED |                                                    \
                                       SVNSLC_SHOWADDEDHISTORY | SVNSLC_SHOWREMOVED | SVNSLC_SHOWCONFLICTED | SVNSLC_SHOWMISSING | \
                                       SVNSLC_SHOWREPLACED | SVNSLC_SHOWMERGED | SVNSLC_SHOWIGNORED | SVNSLC_SHOWOBSTRUCTED |      \
                                       SVNSLC_SHOWEXTERNAL | SVNSLC_SHOWINCOMPLETE | SVNSLC_SHOWINEXTERNALS |                      \
                                       SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO)

#define SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS (SVNSLC_SHOWMODIFIED |                                                                                          \
                                                                     SVNSLC_SHOWADDED | SVNSLC_SHOWADDEDHISTORY | SVNSLC_SHOWREMOVED | SVNSLC_SHOWCONFLICTED | SVNSLC_SHOWMISSING | \
                                                                     SVNSLC_SHOWREPLACED | SVNSLC_SHOWMERGED | SVNSLC_SHOWIGNORED | SVNSLC_SHOWOBSTRUCTED |                         \
                                                                     SVNSLC_SHOWINCOMPLETE | SVNSLC_SHOWEXTERNAL | SVNSLC_SHOWINEXTERNALS)

#define SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS (SVNSLC_SHOWMODIFIED |                                                                                          \
                                                   SVNSLC_SHOWADDED | SVNSLC_SHOWADDEDHISTORY | SVNSLC_SHOWREMOVED | SVNSLC_SHOWCONFLICTED | SVNSLC_SHOWMISSING | \
                                                   SVNSLC_SHOWREPLACED | SVNSLC_SHOWMERGED | SVNSLC_SHOWIGNORED | SVNSLC_SHOWOBSTRUCTED |                         \
                                                   SVNSLC_SHOWINCOMPLETE)

#define SVNSLC_SHOWALL (SVNSLC_SHOWVERSIONED | SVNSLC_SHOWUNVERSIONED)

#define SVNSLC_POPALL             0xFFFFFFFF
#define SVNSLC_POPCOMPAREWITHBASE 0x00000001
#define SVNSLC_POPCOMPARE         0x00000002
#define SVNSLC_POPGNUDIFF         0x00000004
#define SVNSLC_POPREVERT          0x00000008
#define SVNSLC_POPUPDATE          0x00000010
#define SVNSLC_POPSHOWLOG         0x00000020
#define SVNSLC_POPOPEN            0x00000040
#define SVNSLC_POPDELETE          0x00000080
#define SVNSLC_POPADD             0x00000100
#define SVNSLC_POPIGNORE          0x00000200
#define SVNSLC_POPCONFLICT        0x00000400
#define SVNSLC_POPRESOLVE         0x00000800
#define SVNSLC_POPLOCK            0x00001000
#define SVNSLC_POPUNLOCK          0x00002000
#define SVNSLC_POPUNLOCKFORCE     0x00004000
#define SVNSLC_POPEXPLORE         0x00008000
#define SVNSLC_POPCOMMIT          0x00010000
#define SVNSLC_POPPROPERTIES      0x00020000
#define SVNSLC_POPREPAIRMOVE      0x00040000
#define SVNSLC_POPCHANGELISTS     0x00080000
#define SVNSLC_POPBLAME           0x00100000
#define SVNSLC_POPCREATEPATCH     0x00200000
#define SVNSLC_POPCHECKFORMODS    0x00400000
#define SVNSLC_POPREPAIRCOPY      0x00800000
#define SVNSLC_POPSWITCH          0x01000000
#define SVNSLC_POPCOMPARETWO      0x02000000
#define SVNSLC_POPRESTORE         0x04000000
#define SVNSLC_POPEXPORT          0x08000000

#define SVNSLC_IGNORECHANGELIST L"ignore-on-commit"

// This gives up to 64 standard properties and menu entries
// plus 192 user-defined properties (should be plenty).
// User-defined properties will start at column SVNSLC_NUMCOLUMNS+1
// but in the registry, we will record them starting at SVNSLC_USERPROPCOLOFFSET.

#define SVNSLC_USERPROPCOLOFFSET 0x40
#define SVNSLC_USERPROPCOLLIMIT  0xff
#define SVNSLC_MAXCOLUMNCOUNT    0xff

// Supporting extremely long user props makes no sense here --
// especially for binary properties. CString uses a pool allocator
// that works for up to 256 chars. Make sure we are well below that.

#define SVNSLC_MAXUSERPROPLENGTH 0x70

using GENERICCOMPAREFN = int(__cdecl* )(const void* elem1, const void* elem2);
using Locker = CComCritSecLock<CComCriticalSection>;

#define OVL_EXTERNAL        1
#define OVL_NESTED          2
#define OVL_DEPTHFILES      3
#define OVL_DEPTHIMMEDIATES 4
#define OVL_DEPTHEMPTY      5
#define OVL_RESTORE         6
#define OVL_MERGEINFO       7
#define OVL_EXTERNALPEGGED  8

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
     * files on the control. The LPARAM is a pointer to a wchar_t string
     * containing the dropped path.
     */
    static const UINT SVNSLNM_ADDFILE;

    /**
     * Sent to the parent window (using ::SendMessage) when the user checks/unchecks
     * one or more items in the control. The WPARAM contains the number of
     * checked items in the control.
     */
    static const UINT SVNSLNM_CHECKCHANGED;

    /**
     * Sent to the parent window (using ::SendMessage) when the number of
     * changelists change. The WPARAM contains the number of changelists.
     */
    static const UINT SVNSLNM_CHANGELISTCHANGED;

    CSVNStatusListCtrl();
    ~CSVNStatusListCtrl() override;

    /**
     * \ingroup TortoiseProc
     * Helper class for CSVNStatusListCtrl which represents
     * the user properties and their respective values for each file shown.
     */
    class PropertyList
    {
    public:
        /// only default construction / destruction

        PropertyList(){};
        ~PropertyList(){};

        /// assign property list

        PropertyList& operator=(const char* rhs);

        /// collect property names in a set

        void GetPropertyNames(std::set<CString>& names) const;

        /// get a property value.
        /// Returns an empty string if there is no such property.

        const CString& operator[](const CString& name) const;

        /// set a property value.
        /// The function assert()s that the name is a key in the
        /// properties map. If it is not, a dummy will be returned.

        CString& operator[](const CString& name);

        /// check whether that property has been set on this item.

        bool HasProperty(const CString& name) const;

        /// due to frequent use: special check for svn:needs-lock

        bool IsNeedsLockSet() const;

        /// remove all entries

        void Clear();

        /// number of properties

        size_t Count() const;

    private:
        /// all the data we have. map: propname -> value

        std::map<CString, CString> properties;
    };

    /**
     * \ingroup TortoiseProc
     * Helper class for CSVNStatusListCtrl which represents
     * the data for each file shown.
     */
    class FileEntry
    {
    public:
        FileEntry()
            : status(svn_wc_status_unversioned)
            , textStatus(svn_wc_status_unversioned)
            , propStatus(svn_wc_status_unversioned)
            , remoteStatus(svn_wc_status_unversioned)
            , remoteTextStatus(svn_wc_status_unversioned)
            , remotePropStatus(svn_wc_status_unversioned)
            , lockDate(0)
            , lastCommitDate(0)
            , lastCommitRev(0)
            , remoteRev(0)
            , copied(false)
            , switched(false)
            , checked(false)
            , inUnversionedFolder(false)
            , inExternal(false)
            , peggedExternal(false)
            , differentRepo(false)
            , direct(false)
            , isFolder(false)
            , kind(svn_node_unknown)
            , isNested(false)
            , isConflicted(false)
            , onlyMergeInfoMods(false)
            , onlyMergeInfoModsKnown(false)
            , revision(0)
            , workingSize(SVN_WC_ENTRY_WORKING_SIZE_UNKNOWN)
            , depth(svn_depth_unknown)
            , fileExternal(false)
            , id(0)
        {
        }
        const CTSVNPath& GetPath() const
        {
            return path;
        }
        bool IsChecked() const
        {
            return checked;
        }
        CString GetRelativeSVNPath(bool allowEmpty) const
        {
            if ((!allowEmpty) && (path.IsEquivalentTo(basePath)))
                return path.GetSVNPathString();
            if (basePath.GetSVNPathString().Right(2) == L":/")
                return path.GetSVNPathString().Mid(basePath.GetSVNPathString().GetLength());
            return path.GetSVNPathString().Mid(basePath.GetSVNPathString().GetLength() + 1);
        }
        bool IsLocked() const
        {
            return !(lockToken.IsEmpty() && lockRemoteToken.IsEmpty());
        }
        bool IsFolder() const
        {
            return isFolder;
        }
        bool IsInExternal() const
        {
            return inExternal;
        }
        bool IsPeggedExternal() const
        {
            return peggedExternal;
        }
        bool IsNested() const
        {
            return isNested;
        }
        bool IsFromDifferentRepository() const
        {
            return differentRepo;
        }
        bool IsVersioned() const
        {
            return ((status != svn_wc_status_none) && (status != svn_wc_status_unversioned));
        }
        bool IsCopied() const
        {
            return copied;
        }
        LPCWSTR GetDisplayName() const
        {
            LPCWSTR chopped = path.GetDisplayString(&basePath);
            if (*chopped != 0)
            {
                return chopped;
            }
            else
            {
                // "Display name" must not be empty.
                const CString& winPath = path.GetWinPathString();
                return static_cast<LPCWSTR>(winPath) + winPath.ReverseFind('\\') + 1;
            }
        }
        CString GetChangeList() const
        {
            return changelist;
        }
        CString GetURL() const
        {
            return url;
        }
        CString GetRestorePath() const
        {
            return restorePath;
        }

    public:
        svn_wc_status_kind status;           ///< local status
        svn_wc_status_kind textStatus;       ///< local text status
        svn_wc_status_kind propStatus;       ///< local property status
        svn_wc_status_kind remoteStatus;     ///< remote status
        svn_wc_status_kind remoteTextStatus; ///< remote text status
        svn_wc_status_kind remotePropStatus; ///< remote property status
    private:
        CTSVNPath       path;                   ///< full path of the file
        CTSVNPath       basePath;               ///< common ancestor path of all files
        CString         url;                    ///< Subversion URL of the file
        CString         lockToken;              ///< universally unique URI representing lock
        CString         lockOwner;              ///< the username which owns the lock
        CString         lockRemoteOwner;        ///< the username which owns the lock in the repository
        CString         lockRemoteToken;        ///< the unique URI in the repository of the lock
        CString         lockComment;            ///< the message for the lock
        CString         restorePath;            ///< path to a copy of the file, to be restored after a commit
        apr_time_t      lockDate;               ///< the date when this item was locked
        CString         changelist;             ///< the name of the changelist the item belongs to
        CString         lastCommitAuthor;       ///< the author which last committed this item
        apr_time_t      lastCommitDate;         ///< the date when this item was last committed
        svn_revnum_t    lastCommitRev;          ///< the revision where this item was last committed
        svn_revnum_t    remoteRev;              ///< the revision in HEAD of the repository
        bool            copied;                 ///< if the file/folder is added-with-history
        bool            switched;               ///< if the file/folder is switched to another url
        bool            checked;                ///< if the file is checked in the list control
        bool            inUnversionedFolder;    ///< if the file is inside an unversioned folder
        bool            inExternal;             ///< if the item is in an external folder
        bool            peggedExternal;         ///< if inExternal is true, then this is true if the external the item refers to is pegged to a specific revision, not HEAD
        bool            differentRepo;          ///< if the item is from a different repository than the rest
        bool            direct;                 ///< directly included (TRUE) or just a child of a folder
        bool            isFolder;               ///< TRUE if entry refers to a folder
        svn_node_kind_t kind;                   ///< file/folder kind, used to know the 'unspecified' type
        bool            isNested;               ///< TRUE if the folder from a different repository and/or path
        bool            isConflicted;           ///< TRUE if a file entry is conflicted, i.e. if it has the conflicted paths set
        bool            onlyMergeInfoMods;      ///< TRUE if only the svn:mergeinfo property has mods, no other properties
        bool            onlyMergeInfoModsKnown; ///< TRUE if onlyMergeInfoMods has been calculated
        svn_revnum_t    revision;               ///< the base revision
        svn_filesize_t  workingSize;            ///< Size of the file after being translated into local representation or SVN_WC_ENTRY_WORKING_SIZE_UNKNOWN
        svn_depth_t     depth;                  ///< the depth of this entry
        bool            fileExternal;           ///< if the item is a file that was added to the working copy with an svn:externals; if fileExternal is TRUE, then switched is always FALSE.
        CString         copyFromUrlString;      ///< contains the url which this item was copied from. Note: this is not filled in by the status call but only
                                                ///< filled in when needed. This member is only here as a cache.
        CString movedFromAbspath;               ///< Set to the local absolute path that this node was moved from
        CString movedToAbspath;                 ///< Set to the local absolute path that this node was moved to
        _int64  id;                             ///< id/index of the entry, stays the same even after resorting
        friend class CSVNStatusListCtrl;
        friend class CSVNStatusListCtrlDropTarget;
        friend class CSorter;
    };

    /**
     * \ingroup TortoiseProc
     * Helper class for CSVNStatusListCtrl that represents
     * the columns visible and their order as well as
     * persisting that data in the registry.
     *
     * It assigns logical index values to the (potential) columns:
     * 0 .. SVNSLC_NUMCOLUMNS-1 contain the standard attributes
     * SVNSLC_USERPROPCOLOFFSET .. SVNSLC_MAXCOLUMNCOUNT are user props.
     *
     * The column vector contains the columns that are actually
     * available in the control.
     *
     * Since the set of userprops may change from one WC to another,
     * we also store the settings (width and order) for those
     * userprops that are not used in this WC.
     *
     * A userprop is considered "in use", if the respective column
     * is not hidden or if at least one item has this property set.
     */
    class ColumnManager
    {
    public:
        /// construction / destruction

        explicit ColumnManager(CListCtrl* control)
            : control(control){};
        ~ColumnManager(){};

        /// registry access

        void ReadSettings(DWORD defaultColumns, const CString& containerName);
        void WriteSettings() const;

        /// read column definitions

        int            GetColumnCount() const; ///< total number of columns
        bool           IsVisible(int column) const;
        int            GetInvisibleCount() const;
        bool           IsRelevant(int column) const;
        bool           IsUserProp(int column) const;
        const CString& GetName(int column) const;
        int            GetWidth(int column, bool useDefaults = false) const;
        int            GetVisibleWidth(int column, bool useDefaults) const;

        /// switch columns on and off

        void SetVisible(int column, bool visible);

        /// tracking column modifications

        void ColumnMoved(int column, int position);
        void ColumnResized(int column);

        /// call these to update the user-prop list
        /// (will also auto-insert /-remove new list columns)

        void UpdateUserPropList(const std::map<CTSVNPath, PropertyList>& propertyMap);
        void UpdateRelevance(const std::vector<FileEntry*>& files, const std::vector<size_t>& visibleFiles, const std::map<CTSVNPath, PropertyList>& propertyMap);

        /// don't clutter the context menu with irrelevant prop info

        bool AnyUnusedProperties() const;
        void RemoveUnusedProps();

        /// bring everything back to its "natural" order

        void ResetColumns(DWORD defaultColumns);

    private:
        /// initialization utilities

        void ParseUserPropSettings(const CString& userPropList, const CString& shownUserProps);
        void ParseWidths(const CString& widths);
        void SetStandardColumnVisibility(DWORD visibility);
        void ParseColumnOrder(const CString& widths);

        /// map internal column order onto visible column order
        /// (all invisibles in front)

        std::vector<int> GetGridColumnOrder() const;
        void             ApplyColumnOrder() const;

        /// utilities used when writing data to the registry

        DWORD   GetSelectedStandardColumns() const;
        CString GetUserPropList() const;
        CString GetShownUserProps() const;
        CString GetWidthString() const;
        CString GetColumnOrderString() const;

        /// our parent control and its data

        CListCtrl* control;

        /// where to store in the registry

        CString registryPrefix;

        /// all columns in their "natural" order

        struct ColumnInfo
        {
            int  index; ///< is a user prop when < SVNSLC_USERPROPCOLOFFSET
            int  width;
            bool visible;
            bool relevant; ///< set to @a visible, if no *shown* item has that property
        };

        std::vector<ColumnInfo> columns;

        /// user-defined properties

        struct UserProp
        {
            CString name; ///< is a user prop when < SVNSLC_USERPROPCOLOFFSET
            int     width;
        };

        std::vector<UserProp> userProps;

        /// stored result from last UpdateUserPropList() call

        std::set<CString> itemProps;

        /// global column ordering including unused user props

        std::vector<int> columnOrder;
    };

    /**
     * \ingroup TortoiseProc
     * Simple utility class that defines the sort column order.
     */
    class CSorter
    {
    public:
        CSorter(ColumnManager* columnManager, CSVNStatusListCtrl* listControl, int sortedColumn, bool ascending);

        bool operator()(const FileEntry* entry1, const FileEntry* entry2) const;

    private:
        ColumnManager*      columnManager;
        CSVNStatusListCtrl* control;
        int                 sortedColumn;
        bool                ascending;
        bool                m_bSortLogical;
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
     * \param bHasCheckboxes TRUE if the control should show check boxes on the left of each file entry.
     */
    void Init(DWORD dwColumns, const CString& sColumnInfoContainer, DWORD dwContextMenus = ((SVNSLC_POPALL ^ SVNSLC_POPCOMMIT) ^ SVNSLC_POPRESTORE), bool bHasCheckboxes = true);
    /**
     * Sets a background image for the list control.
     * The image is shown in the right bottom corner.
     * \param nID the resource ID of the bitmap to use as the background
     */
    bool SetBackgroundImage(UINT nID) const;
    /**
     * Makes the 'ignore' context menu only ignore the files and not add the
     * folder which gets the svn:ignore property changed to the list.
     * This is needed e.g. for the Add-dialog, where the modified folder
     * showing up would break the resulting "add" command.
     */
    void SetIgnoreRemoveOnly(bool bRemoveOnly = true) { m_bIgnoreRemoveOnly = bRemoveOnly; }
    /**
     * The unversioned items are by default shown after all other files in the list.
     * If that behavior should be changed, set this value to false.
     */
    void PutUnversionedLast(bool bLast) { m_bUnversionedLast = bLast; }
    /**
     * Fetches the Subversion status of all files and stores the information
     * about them in an internal array.
     * \param pathList list of paths to fetch the status for.
     * \param bUpdate TRUE if the remote status is requested too.
     * \param bShowIgnores TRUE if ignored files and folders are requested too.
     * \param bShowUserProps TRUE if user-defined props shall be read too.
     * \return TRUE on success.
     */
    BOOL GetStatus(const CTSVNPathList& pathList, bool bUpdate = false, bool bShowIgnores = false, bool bShowUserProps = false);

    /**
     * Fetches / removed user properties for all items in the internal array.
     * \param bShowUserProps TRUE if user-defined props shall be read too.
     */
    void GetUserProps(bool bShowUserProps);

    /**
     * Populates the list control with the previously (with GetStatus) gathered status information.
     * \param dwShow mask of file types to show. Use the SVNSLC_SHOWxxx defines.
     * \param checkedList
     * \param dwCheck mask of file types to check. Use SVNLC_SHOWxxx defines. Default (0) means 'use the entry's stored check status'
     * \param bShowFolders
     * \param bShowFiles
     */
    void Show(DWORD dwShow, const CTSVNPathList& checkedList, DWORD dwCheck, bool bShowFolders, bool bShowFiles);

    /**
     * Copies the selected entries in the control to the clipboard. The entries
     * are separated by newlines.
     * \param dwCols the columns to copy. Each column is separated by a tab.
     */
    bool CopySelectedEntriesToClipboard(DWORD dwCols, int cmd);

    /**
     * If during the call to GetStatus() some svn:externals are found from different
     * repositories than the first one checked, then this method returns TRUE.
     */
    BOOL HasExternalsFromDifferentRepos() const { return m_bHasExternalsFromDifferentRepos; }

    /**
     * If during the call to GetStatus() some svn:externals are found then this method returns TRUE.
     */
    BOOL HasExternals() const { return m_bHasExternals; }

    /**
     * If unversioned files are found (but not necessarily shown) TRUE is returned.
     */
    BOOL HasUnversionedItems() const { return m_bHasUnversionedItems; }

    /**
     * If there are any locks in the working copy, TRUE is returned
     */
    BOOL HasLocks() const { return m_bHasLocks; }

    /**
     * If there are any change lists defined in the working copy, TRUE is returned
     */
    BOOL HasChangeLists() const { return m_bHasChangeLists; }

    /**
     * Returns the file entry data for the list control index.
     */
    const CSVNStatusListCtrl::FileEntry* GetConstListEntry(UINT_PTR index) const;

    /**
     * Returns the file entry data for the specified path.
     * \note The entry might not be shown in the list control.
     */
    const CSVNStatusListCtrl::FileEntry* GetConstListEntry(const CTSVNPath& path) const;

    /**
     * Returns the index of the list control entry with the specified path,
     * or -1 if the path is not in the list control.
     */
    int GetIndex(const CTSVNPath& path) const;

    /**
     * Returns a String containing some statistics like number of modified, normal, deleted,...
     * files.
     */
    CString GetStatisticsString() const;

    /**
     * Set a static control which will be updated automatically with
     * the number of selected and total files shown in the list control.
     */
    void SetStatLabel(CWnd* pStatLabel) { m_pStatLabel = pStatLabel; };

    /**
     * Set a tri-state checkbox which is updated automatically if the
     * user checks/unchecks file entries in the list control to indicate
     * if all files are checked, none are checked or some are checked.
     */
    void SetSelectButton(CButton* pButton) { m_pSelectButton = pButton; }

    /**
     * Set a button which is de-/activated automatically. The button is
     * only set active if at least one item is selected.
     */
    void SetConfirmButton(CButton* pButton) { m_pConfirmButton = pButton; }

    /**
     * Select/unselect all entries in the list control.
     * \param bSelect TRUE to check, FALSE to uncheck.
     * \param bIncludeNoCommits
     */
    void SelectAll(bool bSelect, bool bIncludeNoCommits = false);

    /**
     * Checks all specified items, removes the checks from the ones not specified
     * \param dwCheck SVNLC_SHOWxxx defines
     * \param uncheckNonMatches
     */
    void Check(DWORD dwCheck, bool uncheckNonMatches);

    /** Set a checkbox on an entry in the listbox
     * Keeps the listctrl checked state and the FileEntry's checked flag in sync
     * \return the previous check value of the item
     */
    bool SetEntryCheck(int listboxIndex, bool bCheck);

    /** Write a list of the checked items' paths into a path list
     */
    void WriteCheckedNamesToPathList(CTSVNPathList& pathList) const;

    /** fills in \a lMin and \a lMax with the lowest/highest revision of all
     * files/folders in the working copy.
     * \param rMin min revision found
     * \param rMax max revision found
     * \param bShownOnly if true, the min/max revisions are calculated only for shown items
     * \param bCheckedOnly if true, the min/max revisions are calculated only for items
     *                   which are checked.
     * \remark Since an item can only be checked if it is visible/shown in the list control
     *         bShownOnly is automatically set to true if bCheckedOnly is true
     */
    void GetMinMaxRevisions(svn_revnum_t& rMin, svn_revnum_t& rMax, bool bShownOnly, bool bCheckedOnly) const;

    /**
     * Returns the parent directory of all entries in the control.
     * if \a bStrict is set to false, then the paths passed to the control
     * to fetch the status (in GetStatus()) are used if possible.
     */
    CTSVNPath GetCommonDirectory(bool bStrict) const;

    /**
     * Returns the parent url of all entries in the control.
     * if \a bStrict is set to false, then the paths passed to the control
     * to fetch the status (in GetStatus()) are used if possible.
     */
    CTSVNPath GetCommonURL(bool bStrict) const;

    /**
     * Sets a pointer to a boolean variable which is checked periodically
     * during the status fetching. As soon as the variable changes to true,
     * the operations stops.
     */
    void SetCancelBool(bool* pbCanceled) { m_pbCanceled = pbCanceled; }

    /**
     * Sets the string shown in the control while the status is fetched.
     * If not set, it defaults to "please wait..."
     */
    void SetBusyString(const CString& str) { m_sBusy = str; }
    void SetBusyString(UINT id) { m_sBusy.LoadString(id); }

    /**
     * Sets the string shown in the control if no items are shown. This
     * can happen for example if there's nothing modified and the unversioned
     * files aren't shown either, so there's nothing to commit.
     * If not set, it defaults to "file list is empty".
     */
    void SetEmptyString(const CString& str) { m_sEmpty = str; }
    void SetEmptyString(UINT id) { m_sEmpty.LoadString(id); }

    /**
     * Determines if the control should recurse into unversioned folders
     * when fetching the status. The default behavior is defined by the
     * registry key HKCU\Software\TortoiseSVN\UnversionedRecurse, which
     * is read in the Init() method.
     * If you want to change the behavior, call this method *after*
     * calling Init().
     */
    void SetUnversionedRecurse(bool bUnversionedRecurse) { m_bUnversionedRecurse = bUnversionedRecurse; }

    /**
     * If set to \c true, fetch the status with depth svn_depth_infinity, otherwise
     * (the default) fetch the status with the depth of the working copy.
     */
    void SetDepthInfinity(bool bInfinity) { m_bDepthInfinity = bInfinity; }

    /**
     * Returns the number of selected items
     */
    LONG GetSelected() const { return m_nSelected; };

    /**
     * Enables dropping of files on the control.
     */
    bool EnableFileDrop();

    /**
     * Checks if the path already exists in the list.
     */
    bool HasPath(const CTSVNPath& path) const;
    /**
     * Checks if the path is shown/visible in the list control.
     */
    bool IsPathShown(const CTSVNPath& path) const;
    /**
     * Forces the children to be checked when the parent folder is checked,
     * and the parent folder to be unchecked if one of its children is unchecked.
     */
    void CheckChildrenWithParent(bool bCheck) { m_bCheckChildrenWithParent = bCheck; }

    void SetRevertMode(bool bRevert) { m_bRevertMode = bRevert; }
    /**
     * Allows checking the items if change lists are present. If set to false,
     * items are not checked if at least one changelist is available.
     */
    void CheckIfChangelistsArePresent(bool bCheck) { m_bCheckIfGroupsExist = bCheck; }
    /**
     * Returns the currently used show flags passed to the Show() method.
     */
    DWORD GetShowFlags() const { return m_dwShow; }

    /**
     * Sets restore paths from a previous run
     */
    void                                                   SetRestorePaths(const std::map<CString, std::tuple<CString, CString>>& restorepaths) { m_restorePaths = restorepaths; }
    const std::map<CString, std::tuple<CString, CString>>& GetRestorePaths() const { return m_restorePaths; }

    CString GetLastErrorMessage() const { return m_sLastError; }

    void BusyCursor(bool bBusy) { m_bWaitCursor = bBusy; }

    LONG GetUnversionedCount() const { return m_nShownUnversioned; }
    LONG GetNormalCount() const { return m_nShownNormal; }
    LONG GetModifiedCount() const { return m_nShownModified; }
    LONG GetAddedCount() const { return m_nShownAdded; }
    LONG GetDeletedCount() const { return m_nShownDeleted; }
    LONG GetConflictedCount() const { return m_nShownConflicted; }
    LONG GetFileCount() const { return m_nShownFiles; }
    LONG GetFolderCount() const { return m_nShownFolders; }

    void AcquireReaderLock() const { m_guard.AcquireReaderLock(); }
    void ReleaseReaderLock() const { m_guard.ReleaseReaderLock(); }
    void AcquireWriterLock() const { m_guard.AcquireWriterLock(); }
    void ReleaseWriterLock() const { m_guard.ReleaseWriterLock(); }

    LONG m_nTargetCount; ///< number of targets in the file passed to GetStatus()

    CString m_sURL; ///< the URL of the target or "(multiple targets)"

    SVNRev m_headRev; ///< the HEAD revision of the repository if bUpdate was TRUE

    CString m_sRepositoryRoot; ///< The repository root of the first item which has one, or an empty string
    DECLARE_MESSAGE_MAP()

    using FileEntryVector = std::vector<FileEntry*>;

private:
    void        SaveColumnWidths(bool bSaveToRegistry = false);
    void        Sort();                                           ///< Sorts the control by columns
    CString     GetCellText(int listIndex, int column);           ///< get the text for a certain grid cell
    void        AddEntry(FileEntry* entry, int listIndex);        ///< add an entry to the control
    void        RemoveListEntry(int index);                       ///< removes an entry from the listcontrol and both arrays
    bool        BuildStatistics(bool repairCaseRenames);          ///< build the statistics and correct the case of files/folders
    void        StartDiff(int fileIndex, bool ignoreProps) const; ///< start the external diff program
    void        StartDiff(FileEntry* entry, bool ignoreProps) const;
    void        StartDiffOrResolve(int fileIndex) const;
    void        StartConflictEditor(const CTSVNPath& filePath, __int64 id) const;
    static void AddPropsPath(const CTSVNPath& filePath, CString& command);

    /// fetch all user properties for all items
    void FetchUserProperties(size_t first, size_t last);
    void FetchUserProperties();

    /// Process one line of the command file supplied to GetStatus
    bool FetchStatusForSingleTarget(SVNStatus& status, const CTSVNPath& target, const CTSVNPath& basePath, bool bFetchStatusFromRepository, CStringA& strCurrentRepositoryUuid, CTSVNPathList& arExtPaths, bool bAllDirect, svn_depth_t depth = svn_depth_infinity, bool bShowIgnores = false);

    /// Create 'status' data for each item in an unversioned folder
    void AddUnversionedFolder(const CTSVNPath& strFolderName, const CTSVNPath& strBasePath, bool inExternal);

    /// Add unversioned / ignored folder
    void PostProcessEntry(const FileEntry* entry, svn_wc_status_kind wcFileStatus);

    /// Read the all the other status items which result from a single GetFirstStatus call
    void ReadRemainingItemsStatus(SVNStatus& status, const CTSVNPath& strBasePath, CStringA& strCurrentRepositoryUuid, CTSVNPathList& arExtPaths, bool bAllDirect);

    /// Clear the status vector (contains custodial pointers)
    void ClearStatusArray();

    /// Sort predicate function - Compare the paths of two entries without regard to case
    static bool EntryPathCompareNoCase(const FileEntry* pEntry1, const FileEntry* pEntry2);

    /// Predicate used to build a list of only the versioned entries of the FileEntry array
    static bool IsEntryVersioned(const FileEntry* pEntry1);

    /// Look up the relevant show flags for a particular SVN status entry
    DWORD GetShowFlagsFromFileEntry(const FileEntry* entry) const;

    /// Build a FileEntry item and add it to the FileEntry array
    FileEntry* AddNewFileEntry(
        const svn_client_status_t* pSVNStatus,             // The return from the SVN GetStatus functions
        const CTSVNPath&           path,                   // The path of the item we're adding
        const CTSVNPath&           basePath,               // The base directory for this status build
        bool                       bDirectItem,            // Was this item the first found by GetFirstFileStatus or by a subsequent GetNextFileStatus call
        bool                       bInExternal,            // Are we in an 'external' folder
        bool                       bEntryfromDifferentRepo // if the entry is from a different repository
    );

    /// Returns the file entry data for the list control index.
    CSVNStatusListCtrl::FileEntry* GetListEntry(UINT_PTR index) const;

    /// Returns the file entry data for the specified path.
    CSVNStatusListCtrl::FileEntry* GetListEntry(const CTSVNPath& path) const;

    bool SetEntryCheck(FileEntry* pEntry, int listboxIndex, bool bCheck);

    /// Adjust the checkbox-state on all descendants of a specific item
    void SetCheckOnAllDescendentsOf(const FileEntry* parentEntry, bool bCheck);

    /// Build a path list of all the selected items in the list (NOTE - SELECTED, not CHECKED)
    void FillListOfSelectedItemPaths(CTSVNPathList& pathList, bool bNoIgnored = false, bool bNoUnversioned = false) const;

    /// Enables/Disables group view and adds all groups to the list control.
    /// If bForce is true, then group view is enabled and the 'null' group is added.
    bool PrepareGroups(bool bForce = false);
    /// Returns the group number to which the group header belongs
    /// If the point is not over a group header, -1 is returned
    int GetGroupFromPoint(POINT* ppt, bool bHeader = true) const;
    /// Returns the number of change lists the selection has
    size_t GetNumberOfChangelistsInSelection() const;

    /// Puts the item to the corresponding group
    bool SetItemGroup(int item, int groupIndex);

    void CheckEntry(int index, int nListItems);
    void UncheckEntry(int index, int nListItems);

    /// remove the \c filepath item (svn remove)
    void Remove(const CTSVNPath& filepath, bool bKeepLocal);
    /// delete the \c filepath item (move to trashbin, windows delete)
    void Delete(const CTSVNPath& filepath, int selIndex);
    /// revert the \c filepath item
    void Revert(const CTSVNPath& filepath);

    /// sends an SVNSLNM_CHECKCHANGED notification to the parent
    void NotifyCheck() const;

    int CellRectFromPoint(CPoint& point, RECT* cellRect, int* col) const;

    void OnContextMenuList(CWnd* pWnd, CPoint point);
    void OnContextMenuGroup(CWnd* pWnd, CPoint point);
    void OnContextMenuHeader(CWnd* pWnd, CPoint point);

    void           ClearSortsFromHeaders() const;
    void           CreateChangeList(const CString& name);
    void           ShowErrorMessage();
    LRESULT        DoInsertGroup(LPWSTR groupName, int groupId, int index);
    LRESULT        DoInsertGroup(LPWSTR groupName, int groupId);
    int            GetGroupId(int itemIndex) const;
    void           RemoveListEntries(const std::vector<CString>& paths);
    void           RemoveListEntries(const std::vector<int>& indices);
    static CString BuildIgnoreList(const CString& fileOrDirectoryName, SVNProperties& properties, bool bRecursive);
    void           OnIgnoreMask(const CTSVNPath& path, bool bRecursive);
    void           OnIgnore(const CTSVNPath& path, bool bRecursive);
    void           OnResolve(svn_wc_conflict_choice_t resolveStrategy);
    void           AddEntryOnIgnore(const CTSVNPath& parentFolder, const CTSVNPath& basePath);
    void           OnUnlock(bool bForce);
    void           OnRepairMove();
    void           OnRepairCopy();
    void           OnRemoveFromCS(const CTSVNPath& filepath);
    void           OnContextMenuListDefault(FileEntry* entry, int command, const CTSVNPath& path);
    void           SendNeedsRefresh();
    void           Open(const CTSVNPath& filepath, FileEntry* entry, bool bOpenWith) const;
    bool           CheckMultipleDiffs() const;

    void            PreSubclassWindow() override;
    BOOL            PreTranslateMessage(MSG* pMsg) override;
    ULONG           GetGestureStatus(CPoint ptTouch) override;
    INT_PTR         OnToolHitTest(CPoint point, TOOLINFO* pTi) const override;
    BOOL            OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
    afx_msg void    OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL    OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnHdnItemclick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnItemchanging(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL    OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnColumnResized(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnColumnMoved(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void    OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg UINT    OnGetDlgCode();
    afx_msg void    OnNMReturn(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void    OnPaint();
    afx_msg void    OnHdnBegintrack(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnHdnItemchanging(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnDestroy();
    afx_msg void    OnSysColorChange();
    afx_msg LRESULT OnResolveMsg(WPARAM, LPARAM);
    afx_msg LRESULT OnRefreshStatusMsg(WPARAM wParam, LPARAM);

private:
    bool*                             m_pbCanceled;
    bool                              m_bAscending;    ///< sort direction
    int                               m_nSortedColumn; ///< which column to sort
    bool                              m_bHasCheckboxes;
    bool                              m_bUnversionedLast;
    bool                              m_bHasExternalsFromDifferentRepos;
    bool                              m_bHasExternals;
    BOOL                              m_bHasUnversionedItems;
    bool                              m_bHasLocks;
    bool                              m_bHasChangeLists;
    bool                              m_bExternalsGroups;
    FileEntryVector                   m_arStatusArray;
    CReaderWriterLock                 m_propertyMapGuard;
    std::map<CTSVNPath, PropertyList> m_propertyMap;
    std::vector<size_t>               m_arListArray;
    std::map<CString, int>            m_changelists;
    bool                              m_bHasIgnoreGroup;
    CTSVNPathList                     m_conflictFileList;
    CTSVNPathList                     m_statusFileList;
    CTSVNPathList                     m_statusUrlList;
    CTSVNPathList                     m_targetPathList;
    CString                           m_sLastError;

    LONG m_nUnversioned;
    LONG m_nNormal;
    LONG m_nModified;
    LONG m_nAdded;
    LONG m_nDeleted;
    LONG m_nConflicted;
    LONG m_nTotal;
    LONG m_nSelected;
    LONG m_nSwitched;

    LONG m_nShownUnversioned;
    LONG m_nShownNormal;
    LONG m_nShownModified;
    LONG m_nShownAdded;
    LONG m_nShownDeleted;
    LONG m_nShownConflicted;
    LONG m_nShownFiles;
    LONG m_nShownFolders;

    DWORD m_dwDefaultColumns;
    DWORD m_dwShow;
    bool  m_bShowFolders;
    bool  m_bShowFiles;
    bool  m_bShowIgnores;
    bool  m_bUpdate;
    DWORD m_dwContextMenus;
    bool  m_bBusy;
    bool  m_bWaitCursor;
    bool  m_bEmpty;
    bool  m_bIgnoreRemoveOnly;
    bool  m_bCheckIfGroupsExist;
    bool  m_bFileDropsEnabled;
    bool  m_bOwnDrag;
    bool  m_bDepthInfinity;
    bool  m_bResortAfterShow;
    bool  m_bAllowPeggedExternals;

    int m_nIconFolder;

    CWnd*    m_pStatLabel;
    CButton* m_pSelectButton;
    CButton* m_pConfirmButton;
    CColors  m_colors;

    CString m_sEmpty;
    CString m_sBusy;
    CString m_sNoPropValueText;

    bool m_bUnversionedRecurse;
    bool m_bFixCaseRenames;

    bool                                          m_bCheckChildrenWithParent;
    bool                                          m_bRevertMode;
    std::unique_ptr<CSVNStatusListCtrlDropTarget> m_pDropTarget;

    ColumnManager m_columnManager;

    WCHAR m_tooltipBuf[4096];

    std::map<CString, bool>                         m_mapFilenameToChecked; ///< Remember manually de-/selected items
    int                                             m_nBlockItemChangeHandler;
    std::set<CTSVNPath>                             m_externalSet;
    std::map<CString, std::tuple<CString, CString>> m_restorePaths;
    mutable CReaderWriterLock                       m_guard;

    HMENU         m_hShellMenu;
    LPCONTEXTMENU m_pContextMenu;

    HFONT m_uiFont;

    friend class CSVNStatusListCtrlDropTarget;
};

class CSVNStatusListCtrlDropTarget : public CIDropTarget
{
public:
    CSVNStatusListCtrlDropTarget(CSVNStatusListCtrl* pSVNStatusListCtrl)
        : CIDropTarget(pSVNStatusListCtrl->m_hWnd)
    {
        m_pSVNStatusListCtrl = pSVNStatusListCtrl;
    }

    bool                      OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* /*pdwEffect*/, POINTL pt) override;
    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect) override;

private:
    CSVNStatusListCtrl* m_pSVNStatusListCtrl;

    void    OnDrop(HDROP hDrop, POINTL pt) const;
    void    SendAddFile(LPCWSTR filename) const;
    CString GetChangelistName(LONG_PTR nGroup) const;
    void    DoDragOver(POINTL pt, DWORD __RPC_FAR* pdwEffect);
    void    SetDropDescriptionCopy();
};
