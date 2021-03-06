// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2005-2006, 2013-2014, 2021 - TortoiseSVN

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
#include "SVNHelpers.h"
#include "StatusCacheEntry.h"
#include "CachedDirectory.h"
#include "FolderCrawler.h"
#include "DirectoryWatcher.h"
#include "ShellUpdater.h"
#include "WCRoots.h"
#include "ReaderWriterLock.h"
#include <atlcoll.h>

//////////////////////////////////////////////////////////////////////////

/**
 * \ingroup TSVNCache
 * The main class handling the status cache.
 * Provides access to a global object of itself which handles all
 * the requests for status.
 */
class CSVNStatusCache
{
private:
    CSVNStatusCache();
    ~CSVNStatusCache();

public:
    static CSVNStatusCache& Instance();
    static void             Create();
    static void             Destroy();
    static bool             SaveCache();

public:
    /// Refreshes the cache.
    void Refresh();

    /// Get the status for a single path (main entry point, called from named-pipe code
    CStatusCacheEntry GetStatusForPath(const CTSVNPath& path, DWORD flags, bool bFetch = true);

    /// Find a directory in the cache (a new entry will be created if there isn't an existing entry)
    CCachedDirectory* GetDirectoryCacheEntry(const CTSVNPath& path);
    CCachedDirectory* GetDirectoryCacheEntryNoCreate(const CTSVNPath& path);

    /// Add a folder to the background crawler's work list
    void AddFolderForCrawling(const CTSVNPath& path);

    /// Removes the cache for a specific path, e.g. if a folder got deleted/renamed
    void RemoveCacheForPath(const CTSVNPath& path);

    /// Removes all items from the cache
    void ClearCache();

    /// Notifies the shell about file/folder status changes.
    /// A notification is only sent for paths which aren't currently
    /// in the list of handled shell requests to avoid deadlocks.
    void UpdateShell(const CTSVNPath& path);

    size_t GetCacheSize() const { return m_directoryCache.size(); }
    int    GetNumberOfWatchedPaths() const { return watcher.GetNumberOfWatchedPaths(); }

    void Init();
    void Stop();

    void CloseWatcherHandles(HANDLE hFile);
    void CloseWatcherHandles(const CTSVNPath& path);

    bool IsPathAllowed(const CTSVNPath& path) { return !!m_shellCache.IsPathAllowed(path.GetWinPath()); }
    bool IsUnversionedAsModified() { return !!m_shellCache.IsUnversionedAsModified(); }
    bool IsIgnoreOnCommitIgnored() { return !!m_shellCache.IsIgnoreOnCommitIgnored(); }
    bool IsPathGood(const CTSVNPath& path);
    bool IsPathWatched(const CTSVNPath& path) { return watcher.IsPathWatched(path); }
    bool AddPathToWatch(const CTSVNPath& path) { return watcher.AddPath(path); }
    bool BlockPath(const CTSVNPath& path, bool specific, ULONGLONG timeout = 0);
    bool UnBlockPath(const CTSVNPath& path);
    bool RemoveTimedoutBlocks();

    CWCRoots* WCRoots() { return &m_wcRoots; }

    CReaderWriterLock& GetGuard() { return m_guard; }
    bool               m_bClearMemory;

private:
    bool                           RemoveCacheForDirectory(CCachedDirectory* cdir);
    static CString                 GetSpecialFolder(REFKNOWNFOLDERID rfid);
    CReaderWriterLock              m_guard;
    CAtlList<CString>              m_askedList;
    CCachedDirectory::CachedDirMap m_directoryCache;

    CComAutoCriticalSection        m_noWatchPathCritSec;
    std::map<CTSVNPath, ULONGLONG> m_noWatchPaths; ///< paths to block from getting crawled, and the time in ms until they're unblocked

    SVNHelper  m_svnHelp;
    ShellCache m_shellCache;

    static CSVNStatusCache* m_pInstance;

    CFolderCrawler m_folderCrawler;
    CShellUpdater  m_shellUpdater;
    CWCRoots       m_wcRoots;

    CComAutoCriticalSection m_critSec;
    CTSVNPath               m_mostRecentAskedPath;
    CStatusCacheEntry       m_mostRecentStatus;
    LONGLONG                m_mostRecentExpiresAt;

    CDirectoryWatcher watcher;

    friend class CCachedDirectory; // Needed for access to the SVN helpers
};
