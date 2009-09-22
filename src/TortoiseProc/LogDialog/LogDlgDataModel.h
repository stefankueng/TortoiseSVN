// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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

/**
 * \ingroup TortoiseProc
 * Contains the data of one log entry, used in the log dialog
 */

class LogEntryData
{   
private:

    /// encapsulate data

    LogEntryData* parent;
	bool hasChildren;
	DWORD childStackDepth;

    svn_revnum_t Rev;
	__time64_t tmDate;
	CString sDate;
	CString sAuthor;
	CString sMessage;
	CString sShortMessage;
	CString sBugIDs;

	const LogChangedPathArray* changedPaths;

	bool checked;

    /// no copy support

    LogEntryData (const LogEntryData&);
    LogEntryData& operator=(const LogEntryData&);

public:

    /// initialization

    LogEntryData ( LogEntryData* parent
                 , svn_revnum_t Rev
                 , __time64_t tmDate
                 , const CString& sDate
                 , const CString& sAuthor
                 , const CString& sMessage
                 , ProjectProperties* projectProperties
                 , LogChangedPathArray* changedPaths
                 , CString& selfRelativeURL);

    /// destruction

    ~LogEntryData();

    /// modification

    void SetAuthor 
        ( const CString& author);
    void SetMessage 
        ( const CString& message
        , ProjectProperties* projectProperties);
    void SetChecked
        ( bool newState);

    /// r/o access to the data

    LogEntryData* GetParent() {return parent;}
    const LogEntryData* GetParent() const {return parent;}
    bool HasChildren() const {return hasChildren;}
    DWORD GetChildStackDepth() const {return childStackDepth;}

    svn_revnum_t GetRevision() const {return Rev;}
    __time64_t GetDate() const {return tmDate;}

    const CString& GetDateString() const {return sDate;}
	const CString& GetAuthor() const {return sAuthor;}
	const CString& GetMessage() const {return sMessage;}
	const CString& GetShortMessage() const {return sShortMessage;}
	const CString& GetBugIDs() const {return sBugIDs;}

    const LogChangedPathArray& GetChangedPaths() const {return *changedPaths;}

    bool GetChecked() const {return checked;}
};

typedef LogEntryData LOGENTRYDATA, *PLOGENTRYDATA;

/**
 * \ingroup TortoiseProc
 * Helper class for the log dialog, handles all the log entries, including
 * sorting.
 */
class CLogDataVector : private std::vector<PLOGENTRYDATA>
{
private:

    typedef std::vector<PLOGENTRYDATA> inherited;

    /// indices of visible entries

    std::vector<size_t> visible;

    /// filter utiltiy method

    bool MatchText(const vector<tr1::wregex>& patterns, const wstring& text);

public:
	/// De-allocates log items.
	void ClearAll();

    /// add / remove items

    void Add (PLOGENTRYDATA item);
    void AddSorted (PLOGENTRYDATA item);
    void RemoveLast();

    /// access to unfilered info

    size_t size() const {return inherited::size();}
    PLOGENTRYDATA operator[](size_t index) const {return at (index);}

    /// access to the filtered info

    size_t GetVisibleCount() const;
    PLOGENTRYDATA GetVisible (size_t index) const;

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

    void Sort (SortColumn column, bool ascending);

    /// filter support

    void Filter 
        ( const CString& filter
        , bool filterWithRegex
        , int selectedFilter
        , __time64_t from
        , __time64_t to
        , bool scanRelevantPathsOnly
        , svn_revnum_t revToKeep);

    void Filter 
        ( __time64_t from
        , __time64_t to);

    void ClearFilter();

    static bool ValidateRegexp 
        ( LPCTSTR regexp_str
        , vector<tr1::wregex>& patterns);
};
