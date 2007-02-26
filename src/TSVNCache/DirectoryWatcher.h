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
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once
#include "TSVNPath.h"
#include "FolderCrawler.h"
#include "ShellCache.h"

#define READ_DIR_CHANGE_BUFFER_SIZE 4096

/**
 * \ingroup TSVNCache
 * Watches the filesystem for changes.
 * When changes are detected, those changes are reported back to the CFolderCrawler
 * which then can update the status cache.
 *
 * When a CDirectoryWatcher object is created, a new thread is started which
 * waits for filesystem change notifications.
 * To add folders to the list of watched folders, call \c AddPath().
 *
 * The folders are watched recursively. To prevent having too many folders watched,
 * children of already watched folders are automatically removed from watching.
 * This leads to having only the roots of filesystems watched (e.g. C:\, D:\,...)
 * after a few paths have been added to the watched list (at least, when the
 * CSVNStatusCache adds those paths).
 */
class CDirectoryWatcher
{
public:
	CDirectoryWatcher(void);
	~CDirectoryWatcher(void);
	
	/**
	 * Adds a new path to be watched. The path \b must point to a directory.
	 * If the path is already watched because a parent of that path is already
	 * watched recursively, then the new path is just ignored and the method
	 * returns false.
	 */
	bool AddPath(const CTSVNPath& path);
	/**
	 * Removes a path and all its children from the watched list.
	 */
	bool RemovePathAndChildren(const CTSVNPath& path);
	
	/**
	 * Returns the number of recursively watched paths.
	 */
	int GetNumberOfWatchedPaths() {return watchedPaths.GetCount();}
	
	/**
	 * Sets the CFolderCrawler object which the change notifications are sent to.
	 */
	void SetFolderCrawler(CFolderCrawler * crawler);
	
	/**
	 * Stops the watching thread.
	 */
	void Stop();

	CTSVNPath CloseInfoMap(HDEVNOTIFY hdev = INVALID_HANDLE_VALUE);

private:
	static unsigned int __stdcall ThreadEntry(void* pContext);
	void WorkerThread();

	void ClearInfoMap();

private:
	CComAutoCriticalSection	m_critSec;
	HANDLE					m_hThread;
	HANDLE					m_hCompPort;
	volatile LONG			m_bRunning;

	CFolderCrawler *		m_FolderCrawler;	///< where the change reports go to
	
	CTSVNPathList			watchedPaths;	///< list of watched paths.

	/**
	 * \ingroup TSVNCache
	 * Helper class: provides information about watched directories.
	 */
	class CDirWatchInfo 
	{
	private:
		CDirWatchInfo();	// private & not implemented
		CDirWatchInfo & operator=(const CDirWatchInfo & rhs);//so that they're aren't accidentally used. -- you'll get a linker error
	public:
		CDirWatchInfo(HANDLE hDir, const CTSVNPath& DirectoryName);
		~CDirWatchInfo();

	protected:
	public:
		bool	CloseDirectoryHandle();

		HANDLE		m_hDir;			///< handle to the directory that we're watching
		CTSVNPath	m_DirName;		///< the directory that we're watching
		CHAR		m_Buffer[READ_DIR_CHANGE_BUFFER_SIZE]; ///< buffer for ReadDirectoryChangesW
		DWORD		m_dwBufLength;	///< length or returned data from ReadDirectoryChangesW -- ignored?...
		OVERLAPPED  m_Overlapped;
		CString		m_DirPath;		///< the directory name we're watching with a backslash at the end
		HDEVNOTIFY	m_hDevNotify;	///< Notification handle
	};

	std::map<HANDLE, CDirWatchInfo *> watchInfoMap;
	
	HDEVNOTIFY		m_hdev;

};
