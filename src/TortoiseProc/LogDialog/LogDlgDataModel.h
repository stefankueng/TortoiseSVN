// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2014-2015, 2017, 2019, 2021 - TortoiseSVN

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
#include "SVN.h"

using namespace LogCache;

/// forward declaration

class CLogDlgFilter;

/**
 * data structure to accommodate the change list.
 */
class CLogChangedPath
{
private:
    CDictionaryBasedPath path;
    CDictionaryBasedPath copyFromPath;
    svn_revnum_t         copyFromRev;

    /// since we may have north of 10 mio instances
    /// of this class, use bit stuffing to minimize
    /// the memory footprint

    struct
    {
        // note on bitfields: we have to use 'unsigned' and not the
        // real enum types, because the highest bit will be otherwise
        // treated as the sign bit, which then leads to wrong values
        unsigned nodeKind             : 3;
        DWORD    action               : 8;
        unsigned textModifies         : 2;
        unsigned propsModifies        : 2;
        int      relevantForStartPath : 1; // we can't use bool here
                                           // (takes an additional 4 bytes)
    } flags;

    /// true, if it affects the content of the path that
    /// the log was originally shown for

    // conversion utility

    static CString GetUIPath(const CDictionaryBasedPath& p);

public:
    /// construction

    CLogChangedPath(const CRevisionInfoContainer::CChangesIterator& iter, const CDictionaryBasedTempPath& logPath);

    /// r/o data access

    const CDictionaryBasedPath& GetCachedPath() const { return path; }
    const CDictionaryBasedPath& GetCachedCopyFromPath() const { return copyFromPath; }

    CString         GetPath() const;
    CString         GetCopyFromPath() const;
    svn_revnum_t    GetCopyFromRev() const { return copyFromRev; }
    svn_node_kind_t GetNodeKind() const { return static_cast<svn_node_kind_t>(flags.nodeKind); }
    DWORD           GetAction() const { return flags.action; }
    svn_tristate_t  GetTextModifies() const { return static_cast<svn_tristate_t>(flags.textModifies); }
    svn_tristate_t  GetPropsModifies() const { return static_cast<svn_tristate_t>(flags.propsModifies); }

    bool IsRelevantForStartPath() const { return flags.relevantForStartPath != 0; }

    /// returns the action as a string

    static const std::string& GetActionString(DWORD action);
    const std::string&        GetActionString() const;
};

/**
 * Factory and container for \ref CLogChangedPath objects.
 * Provides just enough methods to read them.
 */

class CLogChangedPathArray : private std::vector<CLogChangedPath>
{
private:
    /// \ref MarkRelevantChanges found that the log path
    /// has been copied in this revision

    bool copiedSelf;

    /// cached actions info

    mutable DWORD actions;

public:
    /// construction

    CLogChangedPathArray();

    /// modification

    void Add(CRevisionInfoContainer::CChangesIterator& first, const CRevisionInfoContainer::CChangesIterator& last, CDictionaryBasedTempPath& logPath);

    void Add(const CLogChangedPath& item);

    void RemoveAll();
    void RemoveIrrelevantPaths();

    void Sort(int column, bool ascending);

    /// data access

    size_t                 GetCount() const { return size(); }
    const CLogChangedPath& operator[](size_t index) const { return at(index); }
    bool                   ContainsSelfCopy() const { return copiedSelf; }

    /// derived information

    DWORD GetActions() const;
    bool  ContainsCopies() const;
};

/**
 * \ingroup TortoiseProc
 * Contains the data of one log entry, used in the log dialog
 */

class CLogEntryData
{
private:
    /// encapsulate data

    CLogChangedPathArray changedPaths;

    mutable std::string sDate;
    mutable std::string sBugIDs;

    std::string  sAuthor;
    std::string  sMessage;
    svn_revnum_t revision;
    __time64_t   tmDate;

    CLogEntryData* parent;

    ProjectProperties* projectProperties;

    struct
    {
        unsigned             childStackDepth  : 24;
        unsigned int         checked          : 1;
        unsigned int         hasParent        : 1;
        unsigned int         hasChildren      : 1;
        unsigned int         nonInheritable   : 1;
        unsigned int         subtractiveMerge : 1;
        unsigned int         unread           : 1;
        mutable unsigned int bugIDsPending    : 1;
    };

    /// no copy support

    CLogEntryData(const CLogEntryData&) = delete;
    CLogEntryData& operator=(const CLogEntryData&) = delete;

    /// initialization utility
    void InitDateStrings() const;
    void InitBugIDs() const;

public:
    /// initialization

    CLogEntryData(CLogEntryData* parent, svn_revnum_t revision, __time64_t tmDate,
                  const std::string& sAuthor, const std::string& sMessage,
                  ProjectProperties* projectProperties, const MergeInfo* mergeInfo);

    /// destruction

    ~CLogEntryData();

    /// modification

    void SetAuthor(const std::string& author);
    void SetMessage(const std::string& message);
    void SetChecked(bool newState);

    /// finalization (call this once the cache is available)

    void Finalize(const CCachedLogInfo* cache, CDictionaryBasedTempPath& logPath);

    /// r/o access to the data

    CLogEntryData*       GetParent() { return parent; }
    const CLogEntryData* GetParent() const { return parent; }
    bool                 HasParent() const { return hasParent != FALSE; }
    bool                 HasChildren() const { return hasChildren != FALSE; }
    bool                 IsNonInheritable() const { return nonInheritable != FALSE; }
    bool                 IsSubtractiveMerge() const { return subtractiveMerge != FALSE; }
    DWORD                GetDepth() const { return childStackDepth; }

    svn_revnum_t GetRevision() const { return revision; }
    __time64_t   GetDate() const { return tmDate; }

    const std::string& GetDateString() const;
    const std::string& GetAuthor() const { return sAuthor; }
    const std::string& GetMessage() const { return sMessage; }
    const std::string& GetBugIDs() const;
    CString            GetShortMessageUTF16() const;

    const CLogChangedPathArray& GetChangedPaths() const { return changedPaths; }

    bool GetChecked() const { return checked != FALSE; }

    bool GetUnread() const { return unread != FALSE; }
    void SetUnread(bool ur) { unread = ur; }
};

typedef CLogEntryData LOGENTRYDATA, *PLOGENTRYDATA;

/**
 * \ingroup TortoiseProc
 * Helper class for the log dialog, handles all the log entries, including
 * sorting.
 */
class CLogDataVector : private std::vector<PLOGENTRYDATA>
{
private:
    /// indices of visible entries

    std::vector<size_t> visible;

    /// max of LogEntryData::GetDepth

    DWORD maxDepth;

    __time64_t minDate;
    __time64_t maxDate;

    svn_revnum_t minRevision;
    svn_revnum_t maxRevision;

    /// used temporarily when fetching logs with merge info

    std::vector<PLOGENTRYDATA> logParents;

    /// the query used to fill this container

    std::unique_ptr<const CCacheLogQuery> query;

    /// filter utility method

    std::vector<size_t> FilterRange(const CLogDlgFilter* filter, size_t first, size_t last);

    /// returns true if the result vector already contains a parent entry

    template <class T>
    bool contains(std::vector<T> const& v, T const& x)
    {
        return std::find(v.begin(), v.end(), x) != v.end();
    }

public:
    /// construction

    CLogDataVector();

    /// De-allocates log items.

    void ClearAll();

    /// add / remove items

    void Add(svn_revnum_t revision, __time64_t tmDate,
             const std::string& author, const std::string& message,
             ProjectProperties* projectProperties, const MergeInfo* mergeInfo);

    void AddSorted(PLOGENTRYDATA item, ProjectProperties* projectProperties);
    void RemoveLast();

    /// finalization (call this after receiving all log entries)

    void Finalize(std::unique_ptr<const CCacheLogQuery> aQuery, const CString& startLogPath, bool bMerge);

    /// access to unfiltered info

    size_t        size() const { return __super::size(); }
    PLOGENTRYDATA operator[](size_t index) const { return at(index); }

    DWORD        GetMaxDepth() const { return maxDepth; }
    __time64_t   GetMinDate() const { return minDate; }
    __time64_t   GetMaxDate() const { return maxDate; }
    svn_revnum_t GetMinRevision() const { return minRevision; }
    svn_revnum_t GetMaxRevision() const { return maxRevision; }

    /// access to the filtered info

    size_t        GetVisibleCount() const;
    PLOGENTRYDATA GetVisible(size_t index) const;

    /// encapsulate sorting

    enum SortColumn
    {
        RevisionCol = 0,
        ActionCol,
        AuthorCol,
        DateCol,
        BugTraqCol,
        MessageCol
    };

    void Sort(SortColumn column, bool ascending);

    /// filter support

    void Filter(const CLogDlgFilter& filter);
    void Filter(__time64_t from, __time64_t to, bool includeMergedRevs, std::set<svn_revnum_t>* mergedRevs, svn_revnum_t minRev);

    void ClearFilter(bool includeMergedRevs, std::set<svn_revnum_t>* mergedRevs, svn_revnum_t minRev);
};
