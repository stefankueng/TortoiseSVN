// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include ".\directorywatcher.h"

CDirectoryWatcher::CDirectoryWatcher(void) :
	m_hCompPort(NULL) ,
	m_bRunning(true),
	m_FolderCrawler(NULL)
{
	// enable the required privileges for this process
	
	LPCTSTR arPrivelegeNames[] = {	SE_BACKUP_NAME,
									SE_RESTORE_NAME,
									SE_CHANGE_NOTIFY_NAME
								 };

	for (int i=0; i<(sizeof(arPrivelegeNames)/sizeof(LPCTSTR)); ++i)
	{
		HANDLE hToken;    
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) 	
		{        
			TOKEN_PRIVILEGES tp = { 1 };        

			if ( LookupPrivilegeValue(NULL, arPrivelegeNames[i],  &tp.Privileges[0].Luid))
			{
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
			}
			CloseHandle(hToken);	
		}	
	}

	unsigned int threadId;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
}

CDirectoryWatcher::~CDirectoryWatcher(void)
{
	m_bRunning = false;
	CloseHandle(m_hThread);
	CloseHandle(m_hCompPort);
	AutoLocker lock(m_critSec);
	ClearInfoMap();
}

void CDirectoryWatcher::Stop()
{
	m_bRunning = false;
	CloseHandle(m_hThread);
	m_hThread = INVALID_HANDLE_VALUE;
	CloseHandle(m_hCompPort);
	m_hCompPort = INVALID_HANDLE_VALUE;
}

void CDirectoryWatcher::SetFolderCrawler(CFolderCrawler * crawler)
{
	m_FolderCrawler = crawler;
}

bool CDirectoryWatcher::AddPath(const CTSVNPath& path)
{
	if (!m_shellCache.IsPathAllowed(path.GetWinPath()))
		return false;
	AutoLocker lock(m_critSec);
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		if (watchedPaths[i].IsAncestorOf(path))
			return false;		// already watched (recursively)
	}
	
	// now check if with the new path we might have a new root
	CTSVNPath newroot;
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		const CString& watched = watchedPaths[i].GetWinPathString();
		const CString& sPath = path.GetWinPathString();
		int minlen = min(sPath.GetLength(), watched.GetLength());
		int len = 0;
		for (len = 0; len < minlen; ++len)
		{
			if (watched.GetAt(len) != sPath.GetAt(len))
			{
				if ((len > 1)&&(len < minlen))
				{
					if (sPath.GetAt(len)=='\\')
					{
						newroot = CTSVNPath(sPath.Left(len));
					}
					else if (watched.GetAt(len)=='\\')
					{
						newroot = CTSVNPath(watched.Left(len));
					}
				}
				break;
			}
		}
		if (len == minlen)
		{
			if (sPath.GetLength() == minlen)
			{
				if (watched.GetLength() > minlen)
				{
					if (watched.GetAt(len)=='\\')
					{
						newroot = path;
					}
					else if (sPath.GetLength() == 3 && sPath[1] == ':')
					{
						newroot = path;
					}
				}
			}
			else
			{
				if (sPath.GetLength() > minlen)
				{
					if (sPath.GetAt(len)=='\\')
					{
						newroot = CTSVNPath(watched);
					}
					else if (watched.GetLength() == 3 && watched[1] == ':')
					{
						newroot = CTSVNPath(watched);
					}
				}
			}
		}
	}
	if (!newroot.IsEmpty())
	{
		ATLTRACE("add path to watch %ws\n", newroot.GetWinPath());
		watchedPaths.AddPath(newroot);
		watchedPaths.RemoveChildren();
		CloseInfoMap();
		CloseHandle(m_hCompPort);
		m_hCompPort = INVALID_HANDLE_VALUE;
		return true;
	}
	ATLTRACE("add path to watch %ws\n", path.GetWinPath());
	watchedPaths.AddPath(path);
	CloseInfoMap();
	CloseHandle(m_hCompPort);
	m_hCompPort = INVALID_HANDLE_VALUE;
	return true;
}


unsigned int CDirectoryWatcher::ThreadEntry(void* pContext)
{
	((CDirectoryWatcher*)pContext)->WorkerThread();
	return 0;
}

void CDirectoryWatcher::WorkerThread()
{
	DWORD numBytes;
	CDirWatchInfo * pdi = NULL;
	LPOVERLAPPED lpOverlapped;
	WCHAR buf[MAX_PATH] = {0};
	WCHAR * pFound = NULL;
	while (m_bRunning)
	{
		if (watchedPaths.GetCount())
		{
			if (!GetQueuedCompletionStatus(m_hCompPort,
											&numBytes,
											(PULONG_PTR) &pdi,
											&lpOverlapped,
											INFINITE))
			{
				// Error retrieving changes
				// Clear the list of watched objects and recreate that list
				if (!m_bRunning)
					return;
				AutoLocker lock(m_critSec);
				ClearInfoMap();
				if (m_hCompPort != INVALID_HANDLE_VALUE)
					CloseHandle(m_hCompPort);
				// Since we pass m_hCompPort to CreateIoCompletionPort, we
				// have to set this to NULL to have that API create a new
				// handle.
				m_hCompPort = NULL;
				for (int i=0; i<watchedPaths.GetCount(); ++i)
				{
					HANDLE hDir = CreateFile(watchedPaths[i].GetWinPath(), 
											FILE_LIST_DIRECTORY, 
											FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
											NULL, //security attributes
											OPEN_EXISTING,
											FILE_FLAG_BACKUP_SEMANTICS | //required privileges: SE_BACKUP_NAME and SE_RESTORE_NAME.
											FILE_FLAG_OVERLAPPED,
											NULL);
					if (hDir == INVALID_HANDLE_VALUE)
					{
						// this could happen if a watched folder has been removed/renamed
						ATLTRACE("CDirectoryWatcher: CreateFile failed. Can't watch directory %ws\n", watchedPaths[i].GetWinPath());
						CloseHandle(m_hCompPort);
						watchedPaths.RemovePath(watchedPaths[i]);
						i--; if (i<0) i=0;
						break;
					}
					CDirWatchInfo * pDirInfo = new CDirWatchInfo(hDir, watchedPaths[i]);
					m_hCompPort = CreateIoCompletionPort(hDir, m_hCompPort, (ULONG_PTR)pDirInfo, 0);
					if (m_hCompPort == NULL)
					{
						ATLTRACE("CDirectoryWatcher: CreateIoCompletionPort failed. Can't watch directory %ws\n", watchedPaths[i].GetWinPath());
						ClearInfoMap();
						CloseHandle(m_hCompPort);
						delete pDirInfo;
						pDirInfo = NULL;
						watchedPaths.RemovePath(watchedPaths[i]);
						i--; if (i<0) i=0;
						break;
					}
					if (!ReadDirectoryChangesW(pDirInfo->m_hDir,
												pDirInfo->m_Buffer,
												READ_DIR_CHANGE_BUFFER_SIZE,
												TRUE,
												FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
												&numBytes,// not used
												&pDirInfo->m_Overlapped,
												NULL))	//no completion routine!
					{
						ATLTRACE("CDirectoryWatcher: ReadDirectoryChangesW failed. Can't watch directory %ws\n", watchedPaths[i].GetWinPath());
						ClearInfoMap();
						CloseHandle(m_hCompPort);
						delete pDirInfo;
						pDirInfo = NULL;
						watchedPaths.RemovePath(watchedPaths[i]);
						i--; if (i<0) i=0;
						break;
					}
					watchInfoMap[pDirInfo->m_hDir] = pDirInfo;
					ATLTRACE("watching path %ws\n", pDirInfo->m_DirName.GetWinPath());
				}
			}
			else
			{
				if (!m_bRunning)
					return;
				// NOTE: the longer this code takes to execute until ReadDirectoryChangesW
				// is called again, the higher the chance that we miss some
				// changes in the filesystem! 
				if (pdi)
				{
					if (numBytes == 0)
					{
						goto continuewatching;
					}
					PFILE_NOTIFY_INFORMATION pnotify = (PFILE_NOTIFY_INFORMATION)pdi->m_Buffer;
					if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
						goto continuewatching;
					DWORD nOffset = pnotify->NextEntryOffset;
					do 
					{
						nOffset = pnotify->NextEntryOffset;
						switch (pnotify->Action)
						{
						case FILE_ACTION_RENAMED_OLD_NAME:
							{
								pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
								if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
									break;
								continue;
							}
							break;
						}
						if (m_FolderCrawler)
						{
							ZeroMemory(buf, MAX_PATH*sizeof(TCHAR));
							_tcsncpy(buf, pdi->m_DirPath, MAX_PATH);
							_tcsncat(buf+pdi->m_DirPath.GetLength(), pnotify->FileName, min(MAX_PATH-1, pnotify->FileNameLength / sizeof(WCHAR)));
							buf[min(MAX_PATH-1, pdi->m_DirPath.GetLength()+(pnotify->FileNameLength/sizeof(WCHAR)))] = 0;
							pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
							if ((pFound = wcsstr(buf, L"\\tmp"))!=NULL)
							{
								pFound += 4;
								if ((*pFound)=='\\')
								{
									if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
										break;
									continue;
								}
								if (size_t(pFound-buf) == _tcslen(buf))
								{
									if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
										break;
									continue;
								}
							}
							ATLTRACE("change notification: %ws\n", buf);
							m_FolderCrawler->AddPathForUpdate(CTSVNPath(buf));
							if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
								break;
						}
					} while (nOffset);
continuewatching:
					ZeroMemory(pdi->m_Buffer, sizeof(pdi->m_Buffer));
					ZeroMemory(&pdi->m_Overlapped, sizeof(OVERLAPPED));
					if (!ReadDirectoryChangesW(pdi->m_hDir,
												pdi->m_Buffer,
												READ_DIR_CHANGE_BUFFER_SIZE,
												TRUE,
												FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
												&numBytes,// not used
												&pdi->m_Overlapped,
												NULL))	//no completion routine!
					{
						// Since the call to ReadDirectoryChangesW failed, just
						// wait a while. We don't want to have this thread
						// running using 100% CPU if something goes completely
						// wrong.
						Sleep(200);
					}
				} // if (pdi)
			}
		}// if (watchedPaths.GetCount())
		else
			Sleep(200);
	}// while (m_bRunning)
}

void CDirectoryWatcher::ClearInfoMap()
{
	for (std::map<HANDLE, CDirWatchInfo *>::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
	{
		CDirectoryWatcher::CDirWatchInfo * info = I->second;
		delete info;
	}
	watchInfoMap.clear();
}

void CDirectoryWatcher::CloseInfoMap()
{
	for (std::map<HANDLE, CDirWatchInfo *>::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
	{
		CDirectoryWatcher::CDirWatchInfo * info = I->second;
		info->CloseDirectoryHandle();
	}
	watchInfoMap.clear();
}

CDirectoryWatcher::CDirWatchInfo::CDirWatchInfo(HANDLE hDir, const CTSVNPath& DirectoryName) :
	m_hDir(hDir),
	m_DirName(DirectoryName)
{
	ATLASSERT( hDir != INVALID_HANDLE_VALUE 
		&& !DirectoryName.IsEmpty());
	memset(&m_Overlapped, 0, sizeof(m_Overlapped));
	m_DirPath = m_DirName.GetWinPathString();
	if (m_DirPath.GetAt(m_DirPath.GetLength()-1) != '\\')
		m_DirPath += _T("\\");
}

CDirectoryWatcher::CDirWatchInfo::~CDirWatchInfo()
{
	CloseDirectoryHandle();
}

bool CDirectoryWatcher::CDirWatchInfo::CloseDirectoryHandle()
{
	bool b = TRUE;
	if( m_hDir != INVALID_HANDLE_VALUE )
	{
		b = !!CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;
	}
	return b;
}








