// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2012 - TortoiseSVN

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
#include "svn_opt.h"
#pragma warning(pop)
#include <vector>


/**
 * \ingroup SVN
 * SVNRev represents a subversion revision. A subversion revision can
 * be either a simple revision number, a date or a string with one
 * of the following keywords:
 * - BASE
 * - COMMITTED
 * - PREV
 * - HEAD
 *
 * For convenience, this class also accepts the string "WC" as a
 * keyword for the working copy revision.
 */
class SVNRev
{
public:
    SVNRev(LONG nRev);
    SVNRev(const CString& sRev);
    SVNRev(svn_opt_revision_t revision) {rev = revision;m_bIsValid = (rev.kind != svn_opt_revision_unspecified);}
    SVNRev(){rev.kind = svn_opt_revision_unspecified;m_bIsValid = FALSE;}
    ~SVNRev();

    /// returns TRUE if the revision is valid (i.e. not unspecified)
    BOOL IsValid() const {return m_bIsValid;}
    /// returns TRUE if the revision is HEAD
    BOOL IsHead() const {return (rev.kind == svn_opt_revision_head);}
    /// returns TRUE if the revision is BASE
    BOOL IsBase() const {return (rev.kind == svn_opt_revision_base);}
    /// returns TRUE if the revision is WORKING
    BOOL IsWorking() const {return (rev.kind == svn_opt_revision_working);}
    /// returns TRUE if the revision is PREV
    BOOL IsPrev() const {return (rev.kind == svn_opt_revision_previous);}
    /// returns TRUE if the revision is COMMITTED
    BOOL IsCommitted() const {return (rev.kind == svn_opt_revision_committed);}
    /// returns TRUE if the revision is a date
    BOOL IsDate() const {return (rev.kind == svn_opt_revision_date);}
    /// returns TRUE if the revision is a number
    BOOL IsNumber() const {return (rev.kind == svn_opt_revision_number);}

    // Returns the kind of revision representation (number, HEAD, BASE, etc.)
    svn_opt_revision_kind GetKind() const {return rev.kind;}

    /// Returns a string representing the date of a DATE revision, otherwise an empty string.
    CString GetDateString() const {return sDate;}
    /// Returns the date if the revision is of type svn_opt_revision_date
    apr_time_t GetDate() const {ATLASSERT(IsDate()); return rev.value.date;}
    /// Converts the revision into a string representation.
    CString ToString() const;
    /// checks whether two SVNRev objects are the same
    bool IsEqual(const SVNRev& revision) const;

    operator LONG () const;
    operator const svn_opt_revision_t * () const;
    enum
    {
        REV_HEAD = -1,          ///< head revision
        REV_BASE = -2,          ///< base revision
        REV_WC = -3,            ///< revision of the working copy
        REV_DATE = -4,          ///< a date revision
        REV_UNSPECIFIED = -5,   ///< unspecified revision
    };
protected:
    void Create(svn_revnum_t nRev);
    void Create(CString sRev);
private:
    svn_opt_revision_t rev;
    BOOL m_bIsValid;
    CString sDate;
};

/**
 * \ingroup SVN
 * represents a revision range.
 */
class SVNRevRange
{
public:
    SVNRevRange()
    {
        SecureZeroMemory(&revrange, sizeof(svn_opt_revision_range_t));
    }
    SVNRevRange(const SVNRev& start, const SVNRev& end)
    {
        SecureZeroMemory(&revrange, sizeof(svn_opt_revision_range_t));
        revrange.start = *(const svn_opt_revision_t*)start;
        revrange.end = *(const svn_opt_revision_t*)end;
    }

    void        SetRange(const SVNRev& rev1, const SVNRev& rev2) {revrange.start = *(const svn_opt_revision_t*)rev1; revrange.end = *(const svn_opt_revision_t*)rev2;}
    SVNRev      GetStartRevision() const {return SVNRev(revrange.start);}
    SVNRev      GetEndRevision() const {return SVNRev(revrange.end);}

    operator const svn_opt_revision_range_t * () const {return &revrange;}

private:
    svn_opt_revision_range_t revrange;
};


class SVNRevRangeArray
{
public:
    SVNRevRangeArray() {}
    ~SVNRevRangeArray() {}

    int                 AddRevision(const SVNRev& revision, bool reverse);
    int                 AddRevRange(const SVNRevRange& revrange);
    int                 AddRevRange(const SVNRev& start, const SVNRev& end);
    void                AddRevisions(const std::vector<svn_revnum_t>& revisions);
    int                 GetCount() const;
    void                Clear();
    void                AdjustForMerge(bool bReverse = false);

    const apr_array_header_t* GetAprArray(apr_pool_t * pool) const;

    const SVNRevRange&  operator[](int index) const;
    SVNRev              GetHighestRevision() const;
    SVNRev              GetLowestRevision() const;

    bool                FromListString(const CString& string);
    CString             ToListString(bool bReverse = false) const;

    struct AscSort
    {
        bool operator()(SVNRevRange& pStart, SVNRevRange& pEnd)
        {
            return svn_revnum_t(pStart.GetStartRevision()) < svn_revnum_t(pEnd.GetStartRevision());
        }
    };

    struct DescSort
    {
        bool operator()(SVNRevRange& pStart, SVNRevRange& pEnd)
        {
            return svn_revnum_t(pStart.GetStartRevision()) > svn_revnum_t(pEnd.GetStartRevision());
        }
    };

private:

    std::vector<SVNRevRange>    m_array;
};

