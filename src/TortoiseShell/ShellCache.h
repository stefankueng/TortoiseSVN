// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2013-2014 - TortoiseSVN

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
#include "registry.h"
#include "Globals.h"
#include "SVNAdminDir.h"
#include <shlobj.h>

#define REGISTRYTIMEOUT 2000
#define EXCLUDELISTTIMEOUT 5000
#define ADMINDIRTIMEOUT 10000
#define DRIVETYPETIMEOUT 300000     // 5 min
#define NUMBERFMTTIMEOUT 300000
#define MENUTIMEOUT 100

typedef CComCritSecLock<CComCriticalSection> Locker;

struct BoolTimeout
{
    bool        bBool;
    ULONGLONG   timeout;
};

/**
 * \ingroup TortoiseShell
 * Helper class which caches access to the registry. Also provides helper methods
 * for checks against the settings stored in the registry.
 */
class ShellCache
{
public:
    enum CacheType
    {
        none,
        exe,
        dll
    };
    ShellCache();
    ~ShellCache() {}

    void ForceRefresh();
    CacheType GetCacheType();
    DWORD BlockStatus();
    unsigned __int64 GetMenuLayout();
    unsigned __int64 GetMenuMask();

    BOOL IsRecursive();
    BOOL IsFolderOverlay();
    BOOL HasShellMenuAccelerators();
    BOOL IsUnversionedAsModified();
    BOOL IsIgnoreOnCommitIgnored();
    BOOL IsGetLockTop();
    BOOL ShowExcludedAsNormal();
    BOOL AlwaysExtended();
    BOOL HideMenusForUnversionedItems();

    BOOL IsRemote();
    BOOL IsFixed();
    BOOL IsCDRom();
    BOOL IsRemovable();
    BOOL IsRAM();
    BOOL IsUnknown();

    BOOL IsContextPathAllowed(LPCTSTR path);
    BOOL IsPathAllowed(LPCTSTR path);
    DWORD GetLangID();
    NUMBERFMT * GetNumberFmt();
    BOOL IsVersioned(LPCTSTR path, bool bIsDir, bool mustbeok);
    bool IsColumnsEveryWhere();

private:

    void DriveValid();
    void ExcludeContextValid();
    void ValidatePathFilter();

    class CPathFilter
    {
    public:

        /// node in the lookup tree

        struct SEntry
        {
            tstring path;

            /// default (path spec did not end a '?').
            /// if this is not set, the default for all
            /// sub-paths is !included.
            /// This is a temporary setting an be invalid
            /// after @ref PostProcessData

            bool recursive;

            /// this is an "include" specification

            svn_tristate_t included;

            /// if @ref recursive is not set, this is
            /// the parent path status being passed down
            /// combined with the information of other
            /// entries for the same @ref path.

            svn_tristate_t subPathIncluded;

            /// do entries for sub-paths exist?

            bool hasSubFolderEntries;

            /// STL support
            /// For efficient folding, it is imperative that
            /// "recursive" entries are first

            bool operator<(const SEntry& rhs) const
            {
                int diff = _wcsicmp (path.c_str(), rhs.path.c_str());
                return (diff < 0)
                    || ((diff == 0) && recursive && !rhs.recursive);
            }

            friend bool operator<
                ( const SEntry& rhs
                , const std::pair<LPCTSTR, size_t>& lhs);
            friend bool operator<
                ( const std::pair<LPCTSTR
                , size_t>& lhs, const SEntry& rhs);
        };

    private:

        /// lookup by path (all entries sorted by path)

        typedef std::vector<SEntry> TData;
        TData data;

        /// registry keys plus cached last content

        CRegStdString excludelist;
        tstring excludeliststr;

        CRegStdString includelist;
        tstring includeliststr;

        /// construct \ref data content

        void AddEntry (const tstring& s, bool include);
        void AddEntries (const tstring& s, bool include);

        /// for all paths, have at least one entry in data

        void PostProcessData();

        /// lookup. default result is "unknown".
        /// We must look for *every* parent path because of situations like:
        /// excluded: C:, C:\some\deep\path
        /// include: C:\some
        /// lookup for C:\some\deeper\path

        svn_tristate_t IsPathAllowed
            ( LPCTSTR path
            , TData::const_iterator begin
            , TData::const_iterator end) const;

    public:

        /// construction

        CPathFilter();

        /// notify of (potential) registry settings

        void Refresh();

        /// data access

        svn_tristate_t IsPathAllowed (LPCTSTR path) const;
    };

    friend bool operator< (const CPathFilter::SEntry& rhs, const std::pair<LPCTSTR, size_t>& lhs);
    friend bool operator< (const std::pair<LPCTSTR, size_t>& lhs, const CPathFilter::SEntry& rhs);
    CRegStdDWORD cachetype;
    CRegStdDWORD blockstatus;
    CRegStdDWORD langid;
    CRegStdDWORD showrecursive;
    CRegStdDWORD folderoverlay;
    CRegStdDWORD getlocktop;
    CRegStdDWORD driveremote;
    CRegStdDWORD drivefixed;
    CRegStdDWORD drivecdrom;
    CRegStdDWORD driveremove;
    CRegStdDWORD drivefloppy;
    CRegStdDWORD driveram;
    CRegStdDWORD driveunknown;
    CRegStdDWORD menulayoutlow;
    CRegStdDWORD menulayouthigh;
    CRegStdDWORD shellmenuaccelerators;
    CRegStdDWORD menumasklow_lm;
    CRegStdDWORD menumaskhigh_lm;
    CRegStdDWORD menumasklow_cu;
    CRegStdDWORD menumaskhigh_cu;
    CRegStdDWORD unversionedasmodified;
    CRegStdDWORD ignoreoncommitignored;
    CRegStdDWORD excludedasnormal;
    CRegStdDWORD alwaysextended;
    CRegStdDWORD hidemenusforunversioneditems;
    CRegStdDWORD columnseverywhere;

    CPathFilter pathFilter;

    ULONGLONG cachetypeticker;
    ULONGLONG recursiveticker;
    ULONGLONG folderoverlayticker;
    ULONGLONG getlocktopticker;
    ULONGLONG driveticker;
    ULONGLONG drivetypeticker;
    ULONGLONG layoutticker;
    ULONGLONG menumaskticker;
    ULONGLONG langticker;
    ULONGLONG blockstatusticker;
    ULONGLONG columnrevformatticker;
    ULONGLONG pathfilterticker;
    ULONGLONG shellmenuacceleratorsticker;
    ULONGLONG unversionedasmodifiedticker;
    ULONGLONG ignoreoncommitignoredticker;
    ULONGLONG excludedasnormalticker;
    ULONGLONG alwaysextendedticker;
    ULONGLONG hidemenusforunversioneditemsticker;
    ULONGLONG columnseverywhereticker;
    UINT  drivetypecache[27];
    TCHAR drivetypepathcache[MAX_PATH];     // MAX_PATH ok.
    NUMBERFMT columnrevformat;
    TCHAR szDecSep[5];
    TCHAR szThousandsSep[5];
    std::map<tstring, BoolTimeout> admindircache;
    CRegStdString nocontextpaths;
    tstring excludecontextstr;
    std::vector<tstring> excontextvector;
    ULONGLONG excontextticker;
    CComCriticalSection m_critSec;
};

inline bool operator<
    ( const ShellCache::CPathFilter::SEntry& lhs
    , const std::pair<LPCTSTR, size_t>& rhs)
{
    return _wcsnicmp (lhs.path.c_str(), rhs.first, rhs.second) < 0;
}

inline bool operator<
    ( const std::pair<LPCTSTR, size_t>& lhs
    , const ShellCache::CPathFilter::SEntry& rhs)
{
    return _wcsnicmp (lhs.first, rhs.path.c_str(), lhs.second) < 0;
}
