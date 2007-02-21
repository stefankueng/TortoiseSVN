// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006 - Will Dean, Stefan Kueng

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

#include "StdAfx.h"
#include "SVNStatus.h"
#include "Svnstatuscache.h"
#include "CacheInterface.h"
#include "shlobj.h"

//////////////////////////////////////////////////////////////////////////

CSVNStatusCache* CSVNStatusCache::m_pInstance;

CSVNStatusCache& CSVNStatusCache::Instance()
{
	ATLASSERT(m_pInstance != NULL);
	return *m_pInstance;
}

void CSVNStatusCache::Create()
{
	ATLASSERT(m_pInstance == NULL);
	m_pInstance = new CSVNStatusCache;

	m_pInstance->watcher.SetFolderCrawler(&m_pInstance->m_folderCrawler);
#define LOADVALUEFROMFILE(x) if (fread(&x, sizeof(x), 1, pFile)!=1) goto exit;
#define LOADVALUEFROMFILE2(x) if (fread(&x, sizeof(x), 1, pFile)!=1) goto error;
	unsigned int value = (unsigned int)-1;
	FILE * pFile = NULL;
	// find the location of the cache
	TCHAR path[MAX_PATH];		//MAX_PATH ok here.
	if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path)==S_OK)
	{
		_tcscat_s(path, MAX_PATH, _T("\\TSVNCache"));
		if (!PathIsDirectory(path))
		{
			if (CreateDirectory(path, NULL)==0)
				goto error;
		}
		_tcscat_s(path, MAX_PATH, _T("\\cache"));
		_tfopen_s(&pFile, path, _T("rb"));
		if (pFile)
		{
			LOADVALUEFROMFILE(value);
			if (value != 2)
			{
				goto error;
			}
			int mapsize = 0;
			LOADVALUEFROMFILE(mapsize);
			for (int i=0; i<mapsize; ++i)
			{
				LOADVALUEFROMFILE2(value);	
				if (value > MAX_PATH)
					goto error;
				if (value)
				{
					CString sKey;
					if (fread(sKey.GetBuffer(value+1), sizeof(TCHAR), value, pFile)!=value)
					{
						sKey.ReleaseBuffer(0);
						goto error;
					}
					sKey.ReleaseBuffer(value);
					CCachedDirectory * cacheddir = new CCachedDirectory();
					if (cacheddir == NULL)
						goto error;
					if (!cacheddir->LoadFromDisk(pFile))
						goto error;
					CTSVNPath KeyPath = CTSVNPath(sKey);
					if (m_pInstance->IsPathAllowed(KeyPath))
					{
						m_pInstance->m_directoryCache[KeyPath] = cacheddir;
						m_pInstance->watcher.AddPath(KeyPath);
						// do *not* add the paths for crawling!
						// because crawled paths will trigger a shell
						// notification, which makes the desktop flash constantly
						// until the whole first time crawling is over
						// m_pInstance->AddFolderForCrawling(KeyPath);
					}
				}
			}
		}
	}
exit:
	if (pFile)
		fclose(pFile);
	DeleteFile(path);
	ATLTRACE("cache loaded from disk successfully!\n");
	return;
error:
	fclose(pFile);
	DeleteFile(path);
	if (m_pInstance)
	{
		m_pInstance->Stop();
		Sleep(100);
	}
	delete m_pInstance;
	m_pInstance = new CSVNStatusCache;
	ATLTRACE("cache not loaded from disk\n");
}

bool CSVNStatusCache::SaveCache()
{
#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) goto error;
	unsigned int value = 0;
	// save the cache to disk
	FILE * pFile = NULL;
	// find a location to write the cache to
	TCHAR path[MAX_PATH];		//MAX_PATH ok here.
	if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path)==S_OK)
	{
		_tcscat_s(path, MAX_PATH, _T("\\TSVNCache"));
		if (!PathIsDirectory(path))
			CreateDirectory(path, NULL);
		_tcscat_s(path, MAX_PATH, _T("\\cache"));
		_tfopen_s(&pFile, path, _T("wb"));
		if (pFile)
		{
			value = 2;		// 'version'
			WRITEVALUETOFILE(value);
			value = (int)m_pInstance->m_directoryCache.size();
			WRITEVALUETOFILE(value);
			for (CCachedDirectory::CachedDirMap::iterator I = m_pInstance->m_directoryCache.begin(); I != m_pInstance->m_directoryCache.end(); ++I)
			{
				if (I->second == NULL)
				{
					value = 0;
					WRITEVALUETOFILE(value);
					continue;
				}
				const CString& key = I->first.GetWinPathString();
				value = key.GetLength();
				WRITEVALUETOFILE(value);
				if (value)
				{
					if (fwrite((LPCTSTR)key, sizeof(TCHAR), value, pFile)!=value)
						goto error;
					if (!I->second->SaveToDisk(pFile))
						goto error;
				}
			}
			fclose(pFile);
		}
	}
	ATLTRACE("cache saved to disk at %ws\n", path);
	return true;
error:
	fclose(pFile);
	if (m_pInstance)
	{
		m_pInstance->Stop();
		Sleep(100);
	}
	delete m_pInstance;
	m_pInstance = NULL;
	DeleteFile(path);
	return false;
}

void CSVNStatusCache::Destroy()
{
	if (m_pInstance)
	{
		m_pInstance->Stop();
		Sleep(100);
	}
	delete m_pInstance;
	m_pInstance = NULL;
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

CSVNStatusCache::CSVNStatusCache(void)
{
	TCHAR path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_COOKIES, NULL, 0, path);
	m_NoWatchPaths.insert(CTSVNPath(CString(path)));
	SHGetFolderPath(NULL, CSIDL_HISTORY, NULL, 0, path);
	m_NoWatchPaths.insert(CTSVNPath(CString(path)));
	SHGetFolderPath(NULL, CSIDL_INTERNET_CACHE, NULL, 0, path);
	m_NoWatchPaths.insert(CTSVNPath(CString(path)));
	SHGetFolderPath(NULL, CSIDL_SYSTEM, NULL, 0, path);
	m_NoWatchPaths.insert(CTSVNPath(CString(path)));
	SHGetFolderPath(NULL, CSIDL_WINDOWS, NULL, 0, path);
	m_NoWatchPaths.insert(CTSVNPath(CString(path)));
	m_bClearMemory = false;
}

CSVNStatusCache::~CSVNStatusCache(void)
{
	for (CCachedDirectory::CachedDirMap::iterator I = m_pInstance->m_directoryCache.begin(); I != m_pInstance->m_directoryCache.end(); ++I)
	{
		delete I->second;
		I->second = NULL;
	}
}

void CSVNStatusCache::Refresh()
{
	m_shellCache.ForceRefresh();
	if (m_pInstance->m_directoryCache.size())
	{
		CCachedDirectory::CachedDirMap::iterator I = m_pInstance->m_directoryCache.begin();
		for (/* no init */; I != m_pInstance->m_directoryCache.end(); ++I)
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

bool CSVNStatusCache::IsPathGood(CTSVNPath path)
{
	for (std::set<CTSVNPath>::iterator it = m_NoWatchPaths.begin(); it != m_NoWatchPaths.end(); ++it)
	{
		if (it->IsAncestorOf(path))
			return false;
	}
	return true;
}

void CSVNStatusCache::UpdateShell(const CTSVNPath& path)
{
	m_shellUpdater.AddPathForUpdate(path);
}

void CSVNStatusCache::ClearCache()
{
	for (CCachedDirectory::CachedDirMap::iterator I = m_directoryCache.begin(); I != m_directoryCache.end(); ++I)
	{
		delete I->second;
		I->second = NULL;
	}
	m_directoryCache.clear();
}

bool CSVNStatusCache::RemoveCacheForDirectory(CCachedDirectory * cdir)
{
	if (cdir == NULL)
		return false;
	AssertWriting();
	typedef std::map<CTSVNPath, svn_wc_status_kind>  ChildDirStatus;
	if (cdir->m_childDirectories.size())
	{
		ChildDirStatus::iterator it = cdir->m_childDirectories.begin();
		for (; it != cdir->m_childDirectories.end(); )
		{
			CCachedDirectory * childdir = CSVNStatusCache::Instance().GetDirectoryCacheEntryNoCreate(it->first);
			if ((childdir)&&(!cdir->m_directoryPath.IsEquivalentTo(childdir->m_directoryPath)))
				RemoveCacheForDirectory(childdir);
			cdir->m_childDirectories.erase(it->first);
			it = cdir->m_childDirectories.begin();
		}
	}
	cdir->m_childDirectories.clear();
	m_directoryCache.erase(cdir->m_directoryPath);
	ATLTRACE("removed path %ws from cache\n", cdir->m_directoryPath);
	delete cdir;
	cdir = NULL;
	return true;
}

void CSVNStatusCache::RemoveCacheForPath(const CTSVNPath& path)
{
	// Stop the crawler starting on a new folder
	CCrawlInhibitor crawlInhibit(&m_folderCrawler);
	CCachedDirectory::ItDir itMap;
	CCachedDirectory * dirtoremove = NULL;

	AssertWriting();
	itMap = m_directoryCache.find(path);
	if ((itMap != m_directoryCache.end())&&(itMap->second))
		dirtoremove = itMap->second;
	if (dirtoremove == NULL)
		return;
	ATLASSERT(path.IsEquivalentToWithoutCase(dirtoremove->m_directoryPath));
	RemoveCacheForDirectory(dirtoremove);
}

CCachedDirectory * CSVNStatusCache::GetDirectoryCacheEntry(const CTSVNPath& path)
{
	ATLASSERT(path.IsDirectory() || !PathFileExists(path.GetWinPath()));


	CCachedDirectory::ItDir itMap;
	itMap = m_directoryCache.find(path);
	if ((itMap != m_directoryCache.end())&&(itMap->second))
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
		if (!IsWriter())
		{
			// upgrading our state to writer
			ATLTRACE("trying to upgrade the state to \"Writer\"\n");
			Done();
			ATLTRACE("Returned \"Reader\" state\n");
			WaitToWrite();
			ATLTRACE("Got \"Writer\" state now\n");
		}
		if (itMap!=m_directoryCache.end())
			m_directoryCache.erase(itMap);
		// We don't know anything about this directory yet - lets add it to our cache
		// but only if it exists!
		if (path.Exists() && m_shellCache.IsPathAllowed(path.GetWinPath()) && !g_SVNAdminDir.IsAdminDirPath(path.GetWinPath()))
		{
			ATLTRACE("adding %ws to our cache\n", path.GetWinPath());
			ATLASSERT(path.IsDirectory()||(!path.Exists()));
			CCachedDirectory * newcdir = new CCachedDirectory(path);
			if (newcdir)
			{
				CCachedDirectory * cdir = m_directoryCache.insert(m_directoryCache.lower_bound(path), std::make_pair(path, newcdir))->second;
				if (!path.IsEmpty())
					watcher.AddPath(path);
				return cdir;		
			}
			m_bClearMemory = true;
		}
		return NULL;
	}
}

CCachedDirectory * CSVNStatusCache::GetDirectoryCacheEntryNoCreate(const CTSVNPath& path)
{
	ATLASSERT(path.IsDirectory() || !PathFileExists(path.GetWinPath()));

	CCachedDirectory::ItDir itMap;
	itMap = m_directoryCache.find(path);
	if(itMap != m_directoryCache.end())
	{
		// We've found this directory in the cache 
		return itMap->second;
	}
	return NULL;
}

CStatusCacheEntry CSVNStatusCache::GetStatusForPath(const CTSVNPath& path, DWORD flags,  bool bFetch /* = true */)
{
	bool bRecursive = !!(flags & TSVNCACHE_FLAGS_RECUSIVE_STATUS);

	// Check a very short-lived 'mini-cache' of the last thing we were asked for.
	long now = (long)GetTickCount();
	if(now-m_mostRecentExpiresAt < 0)
	{
		if(path.IsEquivalentToWithoutCase(m_mostRecentPath))
		{
			return m_mostRecentStatus;
		}
	}
	m_mostRecentPath = path;
	m_mostRecentExpiresAt = now+1000;

	if (IsPathGood(path))
	{
		// Stop the crawler starting on a new folder while we're doing this much more important task...
		// Please note, that this may be a second "lock" used concurrently to the one in RemoveCacheForPath().
		CCrawlInhibitor crawlInhibit(&m_folderCrawler);

		CTSVNPath dirpath = path.GetContainingDirectory();
		if (dirpath.IsEmpty())
			dirpath = path;
		CCachedDirectory * cachedDir = GetDirectoryCacheEntry(dirpath);
		if (cachedDir != NULL)
		{
			m_mostRecentStatus = cachedDir->GetStatusForMember(path, bRecursive, bFetch);
			return m_mostRecentStatus;
		}
	}
	ATLTRACE("ignored no good path %ws\n", path.GetWinPath());
	m_mostRecentStatus = CStatusCacheEntry();
	return m_mostRecentStatus;
}

void CSVNStatusCache::AddFolderForCrawling(const CTSVNPath& path)
{
	m_folderCrawler.AddDirectoryForUpdate(path);
}

void CSVNStatusCache::CloseWatcherHandles(HDEVNOTIFY hdev)
{
	CTSVNPath path = watcher.CloseInfoMap(hdev);
	m_folderCrawler.BlockPath(path);
}

//////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
static class StatusCacheTests
{
public:
	StatusCacheTests()
	{
		//apr_initialize();
		//CSVNStatusCache::Create();
		//{
		//	CSVNStatusCache& cache = CSVNStatusCache::Instance();

		//	cache.GetStatusForPath(CTSVNPath(_T("D:/Development/SVN/TortoiseSVN")), TSVNCACHE_FLAGS_RECUSIVE_STATUS);
		//	cache.GetStatusForPath(CTSVNPath(_T("D:/Development/SVN/Subversion")), TSVNCACHE_FLAGS_RECUSIVE_STATUS);
		//}

		//CSVNStatusCache::Destroy();
		//apr_terminate();
	}

} StatusCacheTests;

#endif
