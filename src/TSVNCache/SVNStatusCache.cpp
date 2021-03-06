// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006,2008-2015, 2017, 2021 - TortoiseSVN

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

#include "stdafx.h"
#include "SVNStatus.h"
#include "SVNStatusCache.h"
#include "CacheInterface.h"
#include "UnicodeUtils.h"
#include "SVNConfig.h"
#include "SVNAdminDir.h"
#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <shlobj.h>
#pragma warning(pop)

//////////////////////////////////////////////////////////////////////////
#define BLOCK_PATH_DEFAULT_TIMEOUT 600  // 10 minutes
#define BLOCK_PATH_MAX_TIMEOUT     1200 // 20 minutes

#define CACHEDISKVERSION 2

#ifdef _WIN64
#    define STATUSCACHEFILENAME L"\\cache64"
#else
#    define STATUSCACHEFILENAME L"\\cache"
#endif

CSVNStatusCache* CSVNStatusCache::m_pInstance;

CSVNStatusCache& CSVNStatusCache::Instance()
{
    ATLASSERT(m_pInstance != nullptr);
    return *m_pInstance;
}

void CSVNStatusCache::Create()
{
    ATLASSERT(m_pInstance == nullptr);
    if (m_pInstance != nullptr)
        return;

    m_pInstance = new CSVNStatusCache;

    m_pInstance->watcher.SetFolderCrawler(&m_pInstance->m_folderCrawler);
#define LOADVALUEFROMFILE(x)                 \
    if (fread(&x, sizeof(x), 1, pFile) != 1) \
        goto exit;
#define LOADVALUEFROMFILE2(x)                \
    if (fread(&x, sizeof(x), 1, pFile) != 1) \
        goto error;
    unsigned int value = static_cast<unsigned>(-1);
    FILE*        pFile = nullptr;
    // find the location of the cache
    CString path = GetSpecialFolder(FOLDERID_LocalAppData);
    CString path2;
    if (!path.IsEmpty())
    {
        path += L"\\TSVNCache";
        if (!PathIsDirectory(path))
        {
            DeleteFile(path);
            if (CreateDirectory(path, nullptr) == 0)
                goto error;
        }
        path += STATUSCACHEFILENAME;
        // in case the cache file is corrupt, we could crash while
        // reading it! To prevent crashing every time once that happens,
        // we make a copy of the cache file and use that copy to read from.
        // if that copy is corrupt, the original file won't exist anymore
        // and the second time we start up and try to read the file,
        // it's not there anymore and we start from scratch without a crash.
        path2 = path;
        path2 += L"2";
        DeleteFile(path2);
        CopyFile(path, path2, FALSE);
        DeleteFile(path);
        pFile = _tfsopen(path2, L"rb", _SH_DENYNO);
        if (pFile)
        {
            try
            {
                LOADVALUEFROMFILE(value);
                if (value != CACHEDISKVERSION)
                {
                    goto error;
                }
                int mapSize = 0;
                LOADVALUEFROMFILE(mapSize);
                for (int i = 0; i < mapSize; ++i)
                {
                    LOADVALUEFROMFILE2(value);
                    if (value > MAX_PATH)
                        goto error;
                    if (value)
                    {
                        CString sKey;
                        if (fread(sKey.GetBuffer(value + 1), sizeof(wchar_t), value, pFile) != value)
                        {
                            sKey.ReleaseBuffer(0);
                            goto error;
                        }
                        sKey.ReleaseBuffer(value);
                        auto cachedDir = std::make_unique<CCachedDirectory>();
                        if (!cachedDir.get() || !cachedDir->LoadFromDisk(pFile))
                        {
                            cachedDir.reset();
                            goto error;
                        }
                        CTSVNPath keyPath = CTSVNPath(sKey);
                        if (m_pInstance->IsPathAllowed(keyPath))
                        {
                            // only add the path to the watch list if it is versioned
                            if ((cachedDir->GetCurrentFullStatus() != svn_wc_status_unversioned) && (cachedDir->GetCurrentFullStatus() != svn_wc_status_none))
                                m_pInstance->watcher.AddPath(keyPath, false);

                            m_pInstance->m_directoryCache[keyPath] = cachedDir.release();

                            // do *not* add the paths for crawling!
                            // because crawled paths will trigger a shell
                            // notification, which makes the desktop flash constantly
                            // until the whole first time crawling is over
                            // m_pInstance->AddFolderForCrawling(KeyPath);
                        }
                    }
                }
            }
            catch (CAtlException)
            {
                goto error;
            }
        }
    }
exit:
    if (pFile)
        fclose(pFile);
    if (!path2.IsEmpty())
        DeleteFile(path2);
    m_pInstance->watcher.ClearInfoMap();
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": cache loaded from disk successfully!\n");
    return;
error:
    if (pFile)
        fclose(pFile);
    if (!path2.IsEmpty())
        DeleteFile(path2);
    m_pInstance->watcher.ClearInfoMap();
    Destroy();
    m_pInstance = new CSVNStatusCache;
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": cache not loaded from disk\n");
}

bool CSVNStatusCache::SaveCache()
{
#define WRITEVALUETOFILE(x)                   \
    if (fwrite(&x, sizeof(x), 1, pFile) != 1) \
        goto error;
    unsigned int value = 0;
    // save the cache to disk
    FILE* pFile = nullptr;
    // find a location to write the cache to
    CString path = GetSpecialFolder(FOLDERID_LocalAppData);
    if (!path.IsEmpty())
    {
        path += L"\\TSVNCache";
        if (!PathIsDirectory(path))
            CreateDirectory(path, nullptr);
        path += STATUSCACHEFILENAME;
        _tfopen_s(&pFile, path, L"wb");
        if (pFile)
        {
            value = CACHEDISKVERSION; // 'version'
            WRITEVALUETOFILE(value);
            value = static_cast<int>(m_pInstance->m_directoryCache.size());
            WRITEVALUETOFILE(value);
            for (CCachedDirectory::CachedDirMap::iterator I = m_pInstance->m_directoryCache.begin(); I != m_pInstance->m_directoryCache.end(); ++I)
            {
                if (I->second == nullptr)
                {
                    value = 0;
                    WRITEVALUETOFILE(value);
                    continue;
                }
                const CString& key = I->first.GetWinPathString();
                value              = key.GetLength();
                WRITEVALUETOFILE(value);
                if (value)
                {
                    if (fwrite(static_cast<LPCWSTR>(key), sizeof(wchar_t), value, pFile) != value)
                        goto error;
                    if (!I->second->SaveToDisk(pFile))
                        goto error;
                }
            }
            fclose(pFile);
        }
    }
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": cache saved to disk at %s\n", static_cast<LPCWSTR>(path));
    return true;
error:
    fclose(pFile);
    Destroy();
    DeleteFile(path);
    return false;
}

void CSVNStatusCache::Destroy()
{
    if (m_pInstance)
    {
        m_pInstance->Stop();
        Sleep(100);
        delete m_pInstance;
        m_pInstance = nullptr;
    }
}

void CSVNStatusCache::Stop()
{
    m_svnHelp.Cancel(true);
    watcher.Stop();
    m_folderCrawler.Stop();
    m_shellUpdater.Stop();
}

void CSVNStatusCache::Init()
{
    m_folderCrawler.Initialise();
    m_shellUpdater.Initialise();
}

CSVNStatusCache::CSVNStatusCache()
{
    constexpr DWORD forever = static_cast<DWORD>(-1);
    AutoLocker      lock(m_noWatchPathCritSec);
    m_noWatchPaths[CTSVNPath(GetSpecialFolder(FOLDERID_Cookies))]          = forever;
    m_noWatchPaths[CTSVNPath(GetSpecialFolder(FOLDERID_History))]          = forever;
    m_noWatchPaths[CTSVNPath(GetSpecialFolder(FOLDERID_InternetCache))]    = forever;
    m_noWatchPaths[CTSVNPath(GetSpecialFolder(FOLDERID_Windows))]          = forever;
    m_noWatchPaths[CTSVNPath(GetSpecialFolder(FOLDERID_CDBurning))]        = forever;
    m_noWatchPaths[CTSVNPath(GetSpecialFolder(FOLDERID_Fonts))]            = forever;
    m_noWatchPaths[CTSVNPath(GetSpecialFolder(FOLDERID_RecycleBinFolder))] = forever;
    m_noWatchPaths[CTSVNPath(GetSpecialFolder(FOLDERID_SearchHistory))]    = forever;
    m_bClearMemory                                                         = false;
    m_mostRecentExpiresAt                                                  = 0;
}

CSVNStatusCache::~CSVNStatusCache()
{
    ClearCache();
}

void CSVNStatusCache::Refresh()
{
    m_shellCache.RefreshIfNeeded();
    SVNConfig::Instance().Refresh();
    if (!m_pInstance->m_directoryCache.empty())
    {
        for (auto I = m_pInstance->m_directoryCache.begin(); I != m_pInstance->m_directoryCache.end(); ++I)
        {
            if (m_shellCache.IsPathAllowed(I->first.GetWinPath()))
                I->second->RefreshMostImportant();
            else
            {
                CSVNStatusCache::Instance().RemoveCacheForPath(I->first);
                I = m_pInstance->m_directoryCache.begin();
                if (I == m_pInstance->m_directoryCache.end())
                    break;
            }
        }
    }
}

bool CSVNStatusCache::IsPathGood(const CTSVNPath& path)
{
    AutoLocker lock(m_noWatchPathCritSec);
    for (std::map<CTSVNPath, ULONGLONG>::const_iterator it = m_noWatchPaths.begin(); it != m_noWatchPaths.end(); ++it)
    {
        if (it->first.IsAncestorOf(path))
        {
            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": path not good: %s\n", path.GetWinPath());
            return false;
        }
    }
    return true;
}

bool CSVNStatusCache::BlockPath(const CTSVNPath& path, bool specific, ULONGLONG timeout /* = 0 */)
{
    if (timeout == 0)
        timeout = BLOCK_PATH_DEFAULT_TIMEOUT;

    if (timeout > BLOCK_PATH_MAX_TIMEOUT)
        timeout = BLOCK_PATH_MAX_TIMEOUT;

    timeout = GetTickCount64() + (timeout * 1000); // timeout is in seconds, but we need the milliseconds

    if (!specific)
    {
        CTSVNPath p(path);
        do
        {
            CTSVNPath dbPath(p);
            dbPath.AppendPathString(g_SVNAdminDir.GetAdminDirName() + L"\\wc.db");
            if (!dbPath.Exists())
                p = p.GetContainingDirectory();
            else
            {
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": block path %s\n", p.GetWinPath());
                AutoLocker lock(m_noWatchPathCritSec);
                m_noWatchPaths[p] = timeout;
                return true;
            }
        } while (!p.IsEmpty());
    }

    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": block path %s\n", path.GetWinPath());
    AutoLocker lock(m_noWatchPathCritSec);
    m_noWatchPaths[path.GetDirectory()] = timeout;

    return true;
}

bool CSVNStatusCache::UnBlockPath(const CTSVNPath& path)
{
    bool ret = false;

    CTSVNPath p(path);
    do
    {
        CTSVNPath dbPath(p);
        dbPath.AppendPathString(g_SVNAdminDir.GetAdminDirName() + L"\\wc.db");
        if (!dbPath.Exists())
            p = p.GetContainingDirectory();
        else
        {
            AutoLocker                               lock(m_noWatchPathCritSec);
            std::map<CTSVNPath, ULONGLONG>::iterator it = m_noWatchPaths.find(p);
            if (it != m_noWatchPaths.end())
            {
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": path removed from no good: %s\n", it->first.GetWinPath());
                m_noWatchPaths.erase(it);
                ret = true;
            }
            break;
        }
    } while (!p.IsEmpty());

    AutoLocker                               lock(m_noWatchPathCritSec);
    std::map<CTSVNPath, ULONGLONG>::iterator it = m_noWatchPaths.find(path.GetDirectory());
    if (it != m_noWatchPaths.end())
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": path removed from no good: %s\n", it->first.GetWinPath());
        m_noWatchPaths.erase(it);
        ret = true;
    }
    AddFolderForCrawling(path.GetDirectory());

    return ret;
}

bool CSVNStatusCache::RemoveTimedoutBlocks()
{
    const ULONGLONG        currentTicks = GetTickCount64();
    AutoLocker             lock(m_noWatchPathCritSec);
    std::vector<CTSVNPath> toRemove;
    for (std::map<CTSVNPath, ULONGLONG>::const_iterator it = m_noWatchPaths.begin(); it != m_noWatchPaths.end(); ++it)
    {
        if (currentTicks > it->second)
        {
            toRemove.push_back(it->first);
        }
    }
    if (toRemove.empty())
        return false;

    for (std::vector<CTSVNPath>::const_iterator it = toRemove.begin(); it != toRemove.end(); ++it)
    {
        if (UnBlockPath(*it))
            return true;
    }
    return false;
}

void CSVNStatusCache::UpdateShell(const CTSVNPath& path)
{
    m_shellUpdater.AddPathForUpdate(path);
}

void CSVNStatusCache::ClearCache()
{
    CAutoWriteLock writeLock(m_guard);
    for (CCachedDirectory::CachedDirMap::iterator I = m_directoryCache.begin(); I != m_directoryCache.end(); ++I)
    {
        delete I->second;
        I->second = nullptr;
    }
    m_directoryCache.clear();
}

bool CSVNStatusCache::RemoveCacheForDirectory(CCachedDirectory* cdir)
{
    if (cdir == nullptr)
        return false;
    CAutoWriteLock writeLock(m_guard);
    if (!cdir->m_childDirectories.empty())
    {
        auto it = cdir->m_childDirectories.begin();
        for (; it != cdir->m_childDirectories.end();)
        {
            CTSVNPath path;
            CString   winPath = CUnicodeUtils::GetUnicode(it->first);
            path.SetFromWin(winPath, true);

            CCachedDirectory* childDir = CSVNStatusCache::Instance().GetDirectoryCacheEntryNoCreate(path);
            if ((childDir) && (!cdir->m_directoryPath.IsEquivalentTo(childDir->m_directoryPath)) && (cdir->m_directoryPath.GetFileOrDirectoryName() != L".."))
                RemoveCacheForDirectory(childDir);
            cdir->m_childDirectories.erase(it->first);
            it = cdir->m_childDirectories.begin();
        }
    }
    cdir->m_childDirectories.clear();
    m_directoryCache.erase(cdir->m_directoryPath);

    // we could have entries versioned and/or stored in our cache which are
    // children of the specified directory, but not in the m_childDirectories
    // member: this can happen for nested layouts or if we fetched the status
    // while e.g., an update/checkout was in progress
    auto itMap = m_directoryCache.lower_bound(cdir->m_directoryPath);
    do
    {
        if (itMap != m_directoryCache.end())
        {
            if (cdir->m_directoryPath.IsAncestorOf(itMap->first))
            {
                // just in case (see issue #255)
                if (itMap->second == cdir)
                {
                    m_directoryCache.erase(itMap);
                }
                else
                    RemoveCacheForDirectory(itMap->second);
            }
        }
        itMap = m_directoryCache.lower_bound(cdir->m_directoryPath);
    } while (itMap != m_directoryCache.end() && cdir->m_directoryPath.IsAncestorOf(itMap->first));

    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": removed from cache %s\n", cdir->m_directoryPath.GetWinPath());
    delete cdir;
    return true;
}

void CSVNStatusCache::RemoveCacheForPath(const CTSVNPath& path)
{
    // Stop the crawler starting on a new folder
    CCrawlInhibitor crawlInhibit(&m_folderCrawler);
    auto            itMap = m_directoryCache.find(path);

    CCachedDirectory* dirToRemove = nullptr;
    if ((itMap != m_directoryCache.end()) && (itMap->second))
        dirToRemove = itMap->second;
    if (dirToRemove == nullptr)
        return;
    ATLASSERT(path.IsEquivalentToWithoutCase(dirToRemove->m_directoryPath));
    RemoveCacheForDirectory(dirToRemove);
}

CCachedDirectory* CSVNStatusCache::GetDirectoryCacheEntry(const CTSVNPath& path)
{
    auto itMap = m_directoryCache.find(path);
    if ((itMap != m_directoryCache.end()) && (itMap->second))
    {
        // We've found this directory in the cache
        return itMap->second;
    }
    else
    {
        // if the CCachedDirectory is NULL but the path is in our cache,
        // that means that path got invalidated and needs to be treated
        // as if it never was in our cache. So we remove the last remains
        // from the cache and start from scratch.
        CAutoWriteLock writeLock(m_guard);

        // Since above there's a small chance that before we can upgrade to
        // writer state some other thread gained writer state and changed
        // the data, we have to recreate the iterator here again.
        itMap = m_directoryCache.find(path);
        if (itMap != m_directoryCache.end())
        {
            delete itMap->second;
            m_directoryCache.erase(itMap);
        }
        // We don't know anything about this directory yet - lets add it to our cache
        // but only if it exists!
        if (path.Exists() && m_shellCache.IsPathAllowed(path.GetWinPath()) && !g_SVNAdminDir.IsAdminDirPath(path.GetWinPath()))
        {
            // some notifications are for files which got removed/moved around.
            // In such cases, the CTSVNPath::IsDirectory() will return true (it assumes a directory if
            // the path doesn't exist). Which means we can get here with a path to a file
            // instead of a directory.
            // Since we're here most likely called from the crawler thread, the file could exist
            // again. If that's the case, just do nothing
            if (path.IsDirectory() || (!path.Exists()))
            {
                auto newCachedDir = std::make_unique<CCachedDirectory>(path);
                if (newCachedDir.get())
                {
                    itMap = m_directoryCache.lower_bound(path);
                    ASSERT((itMap == m_directoryCache.end()) || (itMap->path != path));

                    itMap = m_directoryCache.insert(itMap, std::make_pair(path, newCachedDir.release()));
                    if (!path.IsEmpty())
                        CSVNStatusCache::Instance().AddFolderForCrawling(path);

                    return itMap->second;
                }
                m_bClearMemory = true;
            }
        }
        return nullptr;
    }
}

CCachedDirectory* CSVNStatusCache::GetDirectoryCacheEntryNoCreate(const CTSVNPath& path)
{
    auto itMap = m_directoryCache.find(path);
    if (itMap != m_directoryCache.end())
    {
        // We've found this directory in the cache
        return itMap->second;
    }
    return nullptr;
}

CStatusCacheEntry CSVNStatusCache::GetStatusForPath(const CTSVNPath& path, DWORD flags, bool bFetch /* = true */)
{
    bool bRecursive = !!(flags & TSVNCACHE_FLAGS_RECUSIVE_STATUS);

    // Check a very short-lived 'mini-cache' of the last thing we were asked for.
    auto now = static_cast<LONGLONG>(GetTickCount64());
    if (now - m_mostRecentExpiresAt < 0)
    {
        if (path.IsEquivalentToWithoutCase(m_mostRecentAskedPath))
        {
            return m_mostRecentStatus;
        }
    }
    {
        AutoLocker lock(m_critSec);
        m_mostRecentAskedPath = path;
        m_mostRecentExpiresAt = now + 1000;
    }

    if (m_shellCache.IsPathAllowed(path.GetWinPath()))
    {
        if (IsPathGood(path))
        {
            // Stop the crawler starting on a new folder while we're doing this much more important task...
            // Please note, that this may be a second "lock" used concurrently to the one in RemoveCacheForPath().
            CCrawlInhibitor crawlInhibit(&m_folderCrawler);

            CTSVNPath dirPath = path.GetContainingDirectory();
            if (dirPath.IsEmpty())
                dirPath = path.GetDirectory();
            CCachedDirectory* cachedDir = GetDirectoryCacheEntry(dirPath);
            if (cachedDir != nullptr)
            {
                CStatusCacheEntry entry = cachedDir->GetStatusForMember(path, bRecursive, bFetch);
                {
                    AutoLocker lock(m_critSec);
                    m_mostRecentStatus = entry;
                    return m_mostRecentStatus;
                }
            }
            cachedDir = GetDirectoryCacheEntry(path.GetDirectory());
            if (cachedDir != nullptr)
            {
                CStatusCacheEntry entry = cachedDir->GetStatusForMember(path, bRecursive, bFetch);
                {
                    AutoLocker lock(m_critSec);
                    m_mostRecentStatus = entry;
                    return m_mostRecentStatus;
                }
            }
        }
        else
        {
            // path is blocked for some reason: return the cached status if we have one
            // we do here only a cache search, absolutely no disk access is allowed!
            auto itMap = m_directoryCache.find(path);
            if ((itMap != m_directoryCache.end()) && (itMap->second))
            {
                // We've found this directory in the cache
                CCachedDirectory* cachedDir = itMap->second;
                CStatusCacheEntry entry     = cachedDir->GetCacheStatusForMember(path);
                {
                    AutoLocker lock(m_critSec);
                    m_mostRecentStatus = entry;
                    return m_mostRecentStatus;
                }
            }
        }
    }
    AutoLocker lock(m_critSec);
    m_mostRecentStatus = CStatusCacheEntry();
    if (m_shellCache.ShowExcludedAsNormal() && path.IsDirectory() && m_shellCache.IsVersioned(path.GetWinPath(), true, true))
    {
        m_mostRecentStatus.ForceStatus(svn_wc_status_normal);
    }
    return m_mostRecentStatus;
}

void CSVNStatusCache::AddFolderForCrawling(const CTSVNPath& path)
{
    m_folderCrawler.AddDirectoryForUpdate(path);
}

void CSVNStatusCache::CloseWatcherHandles(HANDLE hFile)
{
    if (!hFile)
        watcher.ClearInfoMap();
    else
    {
        CTSVNPath path = watcher.CloseInfoMap(hFile);
        m_folderCrawler.BlockPath(path);
    }
}

void CSVNStatusCache::CloseWatcherHandles(const CTSVNPath& path)
{
    watcher.CloseHandlesForPath(path);
    m_folderCrawler.BlockPath(path);
}

CString CSVNStatusCache::GetSpecialFolder(REFKNOWNFOLDERID rfid)
{
    PWSTR pszPath = nullptr;
    if (SHGetKnownFolderPath(rfid, KF_FLAG_CREATE, nullptr, &pszPath) != S_OK)
        return CString();

    CString path = pszPath;
    CoTaskMemFree(pszPath);
    return path;
}
