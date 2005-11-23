// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - Will Dean

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
#include ".\foldercrawler.h"
#include "SVNStatusCache.h"
#include "registry.h"
#include "TSVNCache.h"


CFolderCrawler::CFolderCrawler(void)
{
	m_hWakeEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hTerminationEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	m_hThread = INVALID_HANDLE_VALUE;
	m_lCrawlInhibitSet = 0;
	m_crawlHoldoffReleasesAt = (long)GetTickCount();
}

CFolderCrawler::~CFolderCrawler(void)
{
	Stop();
}

void CFolderCrawler::Stop()
{
	if (m_hTerminationEvent != INVALID_HANDLE_VALUE)
	{
		SetEvent(m_hTerminationEvent);
		if(WaitForSingleObject(m_hThread, 5000) != WAIT_OBJECT_0)
		{
			ATLTRACE("Error terminating crawler thread\n");
		}
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
		CloseHandle(m_hTerminationEvent);
		m_hTerminationEvent = INVALID_HANDLE_VALUE;
		CloseHandle(m_hWakeEvent);
		m_hWakeEvent = INVALID_HANDLE_VALUE;
	}
}

void CFolderCrawler::Initialise()
{
	// Don't call Initalise more than once
	ATLASSERT(m_hThread == INVALID_HANDLE_VALUE);

	// Just start the worker thread. 
	// It will wait for event being signalled.
	// If m_hWakeEvent is already signalled the worker thread 
	// will behave properly (with normal priority at worst).

	unsigned int threadId;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
	SetThreadPriority(m_hThread, THREAD_PRIORITY_LOWEST);
}

void CFolderCrawler::AddDirectoryForUpdate(const CTSVNPath& path)
{
	{
		AutoLocker lock(m_critSec);
		m_foldersToUpdate.push_back(path);
		
		// set this flag while we are sync'ed 
		// with the worker thread
		m_bItemsAddedSinceLastCrawl = true;
	}
	if (SetHoldoff())
		SetEvent(m_hWakeEvent);
}

void CFolderCrawler::AddPathForUpdate(const CTSVNPath& path)
{
	{
		AutoLocker lock(m_critSec);
		m_pathsToUpdate.push_back(path);
		m_bPathsAddedSinceLastCrawl = true;
	}
	if (SetHoldoff())
		SetEvent(m_hWakeEvent);
}

unsigned int CFolderCrawler::ThreadEntry(void* pContext)
{
	((CFolderCrawler*)pContext)->WorkerThread();
	return 0;
}

void CFolderCrawler::WorkerThread()
{
	HANDLE hWaitHandles[2];
	hWaitHandles[0] = m_hTerminationEvent;	
	hWaitHandles[1] = m_hWakeEvent;

	for(;;)
	{
		bool bRecursive = !!(DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\RecursiveOverlay"), TRUE);

		DWORD waitResult = WaitForMultipleObjects(sizeof(hWaitHandles)/sizeof(hWaitHandles[0]), hWaitHandles, FALSE, INFINITE);
		
		// exit event/working loop if the first event (m_hTerminationEvent)
		// has been signalled or if one of the events has been abandoned
		// (i.e. ~CFolderCrawler() is being executed)
		if(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED_0 || waitResult == WAIT_ABANDONED_0+1)
		{
			// Termination event
			break;
		}

		// If we get here, we've been woken up by something being added to the queue.
		// However, it's important that we don't do our crawling while
		// the shell is still asking for items
		// 

		for(;;)
		{
			CTSVNPath workingPath;

			// Any locks today?
			
			if(m_lCrawlInhibitSet > 0)
			{
				// We're in crawl hold-off 
				ATLTRACE("Crawl hold-off\n");
				Sleep(50);
				continue;
			}
			if (m_blockReleasesAt < GetTickCount())
			{
				ATLTRACE("stop blocking path %ws\n", m_blockedPath.GetWinPath());
				m_blockedPath.Reset();
			}
			// Take a short nap if there has been a request recently.
			// Note, that this is an optimization and not sensitive
			// w.r.t. synchronization.
			// (moved out of the critical section as sync is not required)
			
			if(((long)GetTickCount() - m_crawlHoldoffReleasesAt) < 0)
			{
				Sleep(50);
				continue;
			}
	
			if ((m_foldersToUpdate.empty())&&(m_pathsToUpdate.empty()))
			{
				// Nothing left to do 
				break;
			}
			
			if (!m_pathsToUpdate.empty())
			{
				{
					AutoLocker lock(m_critSec);

					if (m_bPathsAddedSinceLastCrawl)
					{
						// The queue has changed - it's worth sorting and de-duping
						std::sort(m_pathsToUpdate.begin(), m_pathsToUpdate.end());
						m_pathsToUpdate.erase(std::unique(m_pathsToUpdate.begin(), m_pathsToUpdate.end(), &CTSVNPath::PredLeftEquivalentToRight), m_pathsToUpdate.end());
						m_bPathsAddedSinceLastCrawl = false;
					}

					workingPath = m_pathsToUpdate.front();
					m_pathsToUpdate.pop_front();
				}
				if ((!m_blockedPath.IsEmpty())&&(m_blockedPath.IsAncestorOf(workingPath)))
					continue;
				// check if the changed path is inside an .svn folder
				if ((workingPath.HasAdminDir()&&workingPath.IsDirectory())||workingPath.IsAdminDir())
				{
					// we don't crawl for paths changed in a tmp folder inside an .svn folder.
					// Because we also get notifications for those even if we just ask for the status!
					// And changes there don't affect the file status at all, so it's safe
					// to ignore notifications on those paths.
					if (workingPath.GetWinPathString().Find(_T("\\tmp\\"))>0)
						continue;
					if (workingPath.GetWinPathString().Find(_T("\\tmp")) == (workingPath.GetWinPathString().GetLength()-4))
						continue;
					do 
					{
						workingPath = workingPath.GetContainingDirectory();	
					} while(workingPath.IsAdminDir());

					ATLTRACE("Invalidating and refreshing folder: %ws\n", workingPath.GetWinPath());
					{
						AutoLocker print(critSec);
						_stprintf(szCurrentCrawledPath[nCurrentCrawledpathIndex], _T("Invalidating and refreshing folder: %s"), workingPath.GetWinPath());
						nCurrentCrawledpathIndex++;
						if (nCurrentCrawledpathIndex >= MAX_CRAWLEDPATHS)
							nCurrentCrawledpathIndex = 0;
					}
					InvalidateRect(hWnd, NULL, FALSE);
					CSVNStatusCache::Instance().WaitToRead();
					// Invalidate the cache of this folder, to make sure its status is fetched again.
					CCachedDirectory * pCachedDir = CSVNStatusCache::Instance().GetDirectoryCacheEntry(workingPath);
					if (pCachedDir)
					{
						svn_wc_status_kind status = pCachedDir->GetCurrentFullStatus();
						pCachedDir->Invalidate();
						if (PathFileExists(workingPath.GetWinPath()))
						{
							pCachedDir->RefreshStatus(bRecursive);
							// if the previous status wasn't normal and now it is, then
							// send a notification too.
							// We do this here because GetCurrentFullStatus() doesn't send
							// notifications for 'normal' status - if it would, we'd get tons
							// of notifications when crawling a working copy not yet in the cache.
							if ((status != svn_wc_status_normal)&&(pCachedDir->GetCurrentFullStatus() != status))
								CSVNStatusCache::Instance().UpdateShell(workingPath);
						}
						else
							CSVNStatusCache::Instance().RemoveCacheForPath(workingPath);
					}
					CSVNStatusCache::Instance().Done();
					//In case that svn_client_stat() modified a file and we got
					//a notification about that in the directory watcher,
					//remove that here again - this is to prevent an endless loop
					AutoLocker lock(m_critSec);
					m_pathsToUpdate.erase(std::remove(m_pathsToUpdate.begin(), m_pathsToUpdate.end(), workingPath), m_pathsToUpdate.end());
				}
				else if (workingPath.HasAdminDir())
				{
					ATLTRACE("Updating path: %ws\n", workingPath.GetWinPath());
					{
						AutoLocker print(critSec);
						_stprintf(szCurrentCrawledPath[nCurrentCrawledpathIndex], _T("Updating path: %s"), workingPath.GetWinPath());
						nCurrentCrawledpathIndex++;
						if (nCurrentCrawledpathIndex >= MAX_CRAWLEDPATHS)
							nCurrentCrawledpathIndex = 0;
					}
					InvalidateRect(hWnd, NULL, FALSE);
					// HasAdminDir() already checks if the path points to a dir
					DWORD flags = TSVNCACHE_FLAGS_FOLDERISKNOWN;
					flags |= (workingPath.IsDirectory() ? TSVNCACHE_FLAGS_ISFOLDER : 0);
					flags |= (bRecursive ? TSVNCACHE_FLAGS_RECUSIVE_STATUS : 0);
					CSVNStatusCache::Instance().WaitToRead();
					// Invalidate the cache of folders manually. The cache of files is invalidated
					// automatically if the status is asked for it and the filetimes don't match
					// anymore, so we don't need to manually invalidate those.
					if (workingPath.IsDirectory())
						CSVNStatusCache::Instance().GetDirectoryCacheEntry(workingPath)->Invalidate();
					CSVNStatusCache::Instance().GetStatusForPath(workingPath, flags);
					CSVNStatusCache::Instance().UpdateShell(workingPath);
					CSVNStatusCache::Instance().Done();
					AutoLocker lock(m_critSec);
					m_pathsToUpdate.erase(std::remove(m_pathsToUpdate.begin(), m_pathsToUpdate.end(), workingPath), m_pathsToUpdate.end());
				}
			}
			else if (!m_foldersToUpdate.empty())
			{
				{
					AutoLocker lock(m_critSec);

					if (m_bItemsAddedSinceLastCrawl)
					{
						// The queue has changed - it's worth sorting and de-duping
						std::sort(m_foldersToUpdate.begin(), m_foldersToUpdate.end());
						m_foldersToUpdate.erase(std::unique(m_foldersToUpdate.begin(), m_foldersToUpdate.end(), &CTSVNPath::PredLeftEquivalentToRight), m_foldersToUpdate.end());
						m_bItemsAddedSinceLastCrawl = false;
					}

					workingPath = m_foldersToUpdate.front();
					m_foldersToUpdate.pop_front();
				}
				if ((!m_blockedPath.IsEmpty())&&(m_blockedPath.IsAncestorOf(workingPath)))
					continue;

				ATLTRACE("Crawling folder: %ws\n", workingPath.GetWinPath());
				{
					AutoLocker print(critSec);
					_stprintf(szCurrentCrawledPath[nCurrentCrawledpathIndex], _T("Crawling folder: %s"), workingPath.GetWinPath());
					nCurrentCrawledpathIndex++;
					if (nCurrentCrawledpathIndex >= MAX_CRAWLEDPATHS)
						nCurrentCrawledpathIndex = 0;
				}
				InvalidateRect(hWnd, NULL, FALSE);
				CSVNStatusCache::Instance().WaitToRead();
				// Now, we need to visit this folder, to make sure that we know its 'most important' status
				CSVNStatusCache::Instance().GetDirectoryCacheEntry(workingPath)->RefreshStatus(bRecursive);
				CSVNStatusCache::Instance().Done();
			}
		}
	}
	_endthread();
}

bool CFolderCrawler::SetHoldoff(DWORD milliseconds /* = 100*/)
{
	long tick = (long)GetTickCount();
	bool ret = ((tick - m_crawlHoldoffReleasesAt) > 0);
	m_crawlHoldoffReleasesAt = tick + milliseconds;
	return ret;
}

void CFolderCrawler::BlockPath(const CTSVNPath& path)
{
	ATLTRACE("block path %ws from being crawled\n", path.GetWinPath());
	m_blockedPath = path;
	m_blockReleasesAt = GetTickCount()+10000;
}