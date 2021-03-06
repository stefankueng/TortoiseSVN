// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2012, 2014, 2021 - TortoiseSVN

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

#pragma once

#include "SVNStatus.h"
#include "TSVNPath.h"
#include "SmartHandle.h"

/**
 * \ingroup TortoiseShell
 * a simple utility class:
 * stores unique copies of given string values,
 * i.e. for a given value, always the same const char*
 * will be returned.
 *
 * The strings returned are owned by the pool!
 */
class StringPool
{
public:
    StringPool() { emptyString[0] = 0; }
    ~StringPool() { clear(); }

    /**
     * Return a string equal to value from the internal pool.
     * If no such string is available, a new one is allocated.
     * NULL is valid for value.
     */
    const char* GetString(const char* value);

    /**
     * invalidates all strings returned by Gestd::wstring()
     * frees all internal data
     */
    void clear();

private:
    // comparator: compare C-style strings

    struct LessString
    {
        bool operator()(const char* lhs, const char* rhs) const
        {
            return strcmp(lhs, rhs) < 0;
        }
    };

    // store the strings in a map
    // caution: modifying the map must not modify the string pointers

    std::set<const char*, LessString> pool;
    char                              emptyString[1];
};

#define SVNFOLDERSTATUS_CACHETIMES            10
#define SVNFOLDERSTATUS_CACHETIMEOUT          2000
#define SVNFOLDERSTATUS_RECURSIVECACHETIMEOUT 4000
#define SVNFOLDERSTATUS_FOLDER                500

class FileStatusCacheEntry
{
public:
    FileStatusCacheEntry()
        : status(svn_wc_status_none)
        , author("")
        , url("")
        , owner("")
        , needsLock(false)
        , rev(-1)
        , askedCounter(SVNFOLDERSTATUS_CACHETIMES)
        , lock(nullptr)
        , treeConflict(false)
    {
    }
    svn_wc_status_kind status;
    const char*        author; ///< points to a (possibly) shared value
    const char*        url;    ///< points to a (possibly) shared value
    const char*        owner;  ///< points to a (possible) lock owner
    bool               needsLock;
    svn_revnum_t       rev;
    int                askedCounter;
    const svn_lock_t*  lock;
    bool               treeConflict;
};

/**
 * \ingroup TortoiseShell
 * This class represents a caching mechanism for the
 * subversion statuses. Once a status for a versioned
 * file is requested (GetFileStatus()) first its checked
 * if that status is already in the cache. If it is not
 * then the subversion statuses for ALL files in the same
 * directory is fetched and cached. This is because subversion
 * needs almost the same time to get one or all status (in
 * the same directory).
 * To prevent a cache flush for the explorer folder view
 * the cache is only fetched for versioned files and
 * not for folders.
 */
class SVNFolderStatus
{
public:
    SVNFolderStatus();
    ~SVNFolderStatus();
    const FileStatusCacheEntry* GetFullStatus(const CTSVNPath& filePath, BOOL bIsFolder, BOOL bColumnProvider = FALSE);
    const FileStatusCacheEntry* GetCachedItem(const CTSVNPath& filepath);

    FileStatusCacheEntry invalidStatus;

private:
    const FileStatusCacheEntry* BuildCache(const CTSVNPath& filePath, BOOL bIsFolder, BOOL bDirectFolder = FALSE);
    ULONGLONG                   GetTimeoutValue() const;
    static svn_error_t*         fillStatusMap(void* baton, const char* path, const svn_client_status_t* status, apr_pool_t* pool);
    static svn_error_t*         findFolderStatus(void* baton, const char* path, const svn_client_status_t* status, apr_pool_t* pool);
    static CTSVNPath            folderPath;
    void                        ClearCache();

    int m_nCounter;
    using FileStatusMap = std::map<std::wstring, FileStatusCacheEntry>;
    FileStatusMap              m_cache;
    ULONGLONG                  m_timeStamp;
    FileStatusCacheEntry       dirStat;
    const svn_client_status_t* dirStatus;
    apr_pool_t*                rootPool;

    // merging these pools won't save memory
    // but access will become slower

    StringPool authors;
    StringPool urls;
    StringPool owners;
    char       emptyString[1];

    std::wstring sCacheKey;

    CAutoGeneralHandle m_hInvalidationEvent;

    // The item we most recently supplied status for
    CTSVNPath                   m_mostRecentPath;
    const FileStatusCacheEntry* m_mostRecentStatus;
};
