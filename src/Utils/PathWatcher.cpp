﻿// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2007-2012, 2014-2015 - TortoiseSVN

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
#include "Dbt.h"
#include "PathWatcher.h"

CPathWatcher::CPathWatcher(void) : m_hCompPort(NULL)
    , m_bRunning(TRUE)
    , m_bLimitReached(false)
{
    // enable the required privileges for this process

    LPCTSTR arPrivelegeNames[] = {  SE_BACKUP_NAME,
                                    SE_RESTORE_NAME,
                                    SE_CHANGE_NOTIFY_NAME
                                 };

    for (int i=0; i<(sizeof(arPrivelegeNames)/sizeof(LPCTSTR)); ++i)
    {
        CAutoGeneralHandle hToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, hToken.GetPointer()))
        {
            TOKEN_PRIVILEGES tp = { 1 };

            if (LookupPrivilegeValue(NULL, arPrivelegeNames[i],  &tp.Privileges[0].Luid))
            {
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
            }
        }
    }

    unsigned int threadId = 0;
    m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
}

CPathWatcher::~CPathWatcher(void)
{
    Stop();
    AutoLocker lock(m_critSec);
    ClearInfoMap();
}

void CPathWatcher::Stop()
{
    InterlockedExchange(&m_bRunning, FALSE);
    if (m_hCompPort)
    {
        PostQueuedCompletionStatus(m_hCompPort, 0, NULL, NULL);
        m_hCompPort.CloseHandle();
    }

    if (m_hThread)
    {
        // the background thread sleeps for 200ms,
        // so lets wait for it to finish for 1000 ms.

        WaitForSingleObject(m_hThread, 1000);
        m_hThread.CloseHandle();
    }
}

bool CPathWatcher::RemovePathAndChildren(const CTSVNPath& path)
{
    bool bRemoved = false;
    AutoLocker lock(m_critSec);
repeat:
    for (int i=0; i<watchedPaths.GetCount(); ++i)
    {
        if (path.IsAncestorOf(watchedPaths[i]))
        {
            watchedPaths.RemovePath(watchedPaths[i]);
            bRemoved = true;
            goto repeat;
        }
    }
    return bRemoved;
}

bool CPathWatcher::AddPath(const CTSVNPath& path)
{
    AutoLocker lock(m_critSec);
    for (int i=0; i<watchedPaths.GetCount(); ++i)
    {
        if (watchedPaths[i].IsAncestorOf(path))
            return false;       // already watched (recursively)
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
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": add path to watch %s\n", newroot.GetWinPath());
        watchedPaths.AddPath(newroot);
        watchedPaths.RemoveChildren();
        m_hCompPort.CloseHandle();
        return true;
    }
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": add path to watch %s\n", path.GetWinPath());
    watchedPaths.AddPath(path);
    m_hCompPort.CloseHandle();
    return true;
}


unsigned int CPathWatcher::ThreadEntry(void* pContext)
{
    CCrashReportThread crashthread;
    ((CPathWatcher*)pContext)->WorkerThread();
    return 0;
}

void CPathWatcher::WorkerThread()
{
    DWORD numBytes;
    CDirWatchInfo * pdi = NULL;
    LPOVERLAPPED lpOverlapped;
    const int bufferSize = MAX_PATH * 4;
    TCHAR buf[bufferSize] = {0};
    while (m_bRunning)
    {
        if (watchedPaths.GetCount())
        {
            if (!m_hCompPort || !GetQueuedCompletionStatus(m_hCompPort,
                                                           &numBytes,
                                                           (PULONG_PTR) &pdi,
                                                           &lpOverlapped,
                                                           INFINITE))
            {
                // Error retrieving changes
                // Clear the list of watched objects and recreate that list
                if (!m_bRunning)
                    return;
                {
                    AutoLocker lock(m_critSec);
                    ClearInfoMap();
                }
                DWORD lasterr = GetLastError();
                if ((m_hCompPort)&&(lasterr!=ERROR_SUCCESS)&&(lasterr!=ERROR_INVALID_HANDLE))
                {
                    m_hCompPort.CloseHandle();
                }
                for (int i=0; i<watchedPaths.GetCount(); ++i)
                {
                    CAutoFile hDir = CreateFile(watchedPaths[i].GetWinPath(),
                                                FILE_LIST_DIRECTORY,
                                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                NULL, //security attributes
                                                OPEN_EXISTING,
                                                FILE_FLAG_BACKUP_SEMANTICS | //required privileges: SE_BACKUP_NAME and SE_RESTORE_NAME.
                                                FILE_FLAG_OVERLAPPED,
                                                NULL);
                    if (!hDir)
                    {
                        // this could happen if a watched folder has been removed/renamed
                        m_hCompPort.CloseHandle();
                        AutoLocker lock(m_critSec);
                        watchedPaths.RemovePath(watchedPaths[i]);
                        i--; if (i<0) i=0;
                        break;
                    }

                    std::unique_ptr<CDirWatchInfo> pDirInfo (new CDirWatchInfo(std::move(hDir), watchedPaths[i]));// the new CDirWatchInfo object owns the handle now
                    m_hCompPort = CreateIoCompletionPort(pDirInfo->m_hDir, m_hCompPort, (ULONG_PTR)pDirInfo.get(), 0);
                    if (m_hCompPort == NULL)
                    {
                        AutoLocker lock(m_critSec);
                        ClearInfoMap();
                        watchedPaths.RemovePath(watchedPaths[i]);
                        i--; if (i<0) i=0;
                        break;
                    }
                    if (!ReadDirectoryChangesW(pDirInfo->m_hDir,
                                                pDirInfo->m_Buffer,
                                                bufferSize,
                                                TRUE,
                                                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                                &numBytes,// not used
                                                &pDirInfo->m_Overlapped,
                                                NULL))  //no completion routine!
                    {
                        AutoLocker lock(m_critSec);
                        ClearInfoMap();
                        watchedPaths.RemovePath(watchedPaths[i]);
                        i--; if (i<0) i=0;
                        break;
                    }
                    AutoLocker lock(m_critSec);
                    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": watching path %s\n", pDirInfo->m_DirName.GetWinPath());
                    watchInfoMap[pDirInfo->m_hDir] = pDirInfo.get();
                    pDirInfo.release();
                }
            }
            else
            {
                if (!m_bRunning)
                    return;
                // NOTE: the longer this code takes to execute until ReadDirectoryChangesW
                // is called again, the higher the chance that we miss some
                // changes in the file system!
                if (pdi)
                {
                    if (numBytes == 0)
                    {
                        goto continuewatching;
                    }
                    PFILE_NOTIFY_INFORMATION pnotify = (PFILE_NOTIFY_INFORMATION)pdi->m_Buffer;
                    if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > bufferSize)
                        goto continuewatching;
                    DWORD nOffset = pnotify->NextEntryOffset;
                    do
                    {
                        nOffset = pnotify->NextEntryOffset;
                        SecureZeroMemory(buf, bufferSize*sizeof(TCHAR));
                        wcsncpy_s(buf, bufferSize, pdi->m_DirPath, bufferSize - 1);
                        errno_t err = wcsncat_s(buf+pdi->m_DirPath.GetLength(), bufferSize-pdi->m_DirPath.GetLength(), pnotify->FileName, min(bufferSize-pdi->m_DirPath.GetLength(), int(pnotify->FileNameLength/sizeof(TCHAR))));
                        if (err == STRUNCATE)
                        {
                            pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
                            continue;
                        }
                        buf[min((decltype((pnotify->FileNameLength / sizeof(WCHAR)))) bufferSize - 1, pdi->m_DirPath.GetLength() + (pnotify->FileNameLength / sizeof(WCHAR)))] = L'\0';
                        pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
                        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": change notification: %s\n", buf);
                        {
                            AutoLocker lock(m_critSec);
                            if (m_changedPaths.GetCount() < MAX_CHANGED_PATHS)
                                m_changedPaths.AddPath(CTSVNPath(buf));
                            else
                                m_bLimitReached = true;
                        }
                        if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > bufferSize)
                            break;
                    } while (nOffset);
continuewatching:
                    {
                        AutoLocker lock(m_critSec);
                        m_changedPaths.RemoveDuplicates();
                    }
                    SecureZeroMemory(pdi->m_Buffer, sizeof(pdi->m_Buffer));
                    SecureZeroMemory(&pdi->m_Overlapped, sizeof(OVERLAPPED));
                    if (!ReadDirectoryChangesW(pdi->m_hDir,
                                                pdi->m_Buffer,
                                                bufferSize,
                                                TRUE,
                                                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                                &numBytes,// not used
                                                &pdi->m_Overlapped,
                                                NULL))  //no completion routine!
                    {
                        // Since the call to ReadDirectoryChangesW failed, just
                        // wait a while. We don't want to have this thread
                        // running using 100% CPU if something goes completely
                        // wrong.
                        Sleep(200);
                    }
                }
            }
        }// if (watchedPaths.GetCount())
        else
            Sleep(200);
    }// while (m_bRunning)
}

void CPathWatcher::ClearInfoMap()
{
    if (!watchInfoMap.empty())
    {
        AutoLocker lock(m_critSec);
        for (std::map<HANDLE, CDirWatchInfo *>::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
        {
            CPathWatcher::CDirWatchInfo * info = I->second;
            delete info;
            info = NULL;
        }
    }
    watchInfoMap.clear();
    m_hCompPort.CloseHandle();
}

CPathWatcher::CDirWatchInfo::CDirWatchInfo(CAutoFile && hDir, const CTSVNPath& DirectoryName)
    : m_hDir(std::move(hDir))
    , m_DirName(DirectoryName)
{
    ATLASSERT( m_hDir && !DirectoryName.IsEmpty());
    m_Buffer[0] = 0;
    SecureZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
    m_DirPath = m_DirName.GetWinPathString();
    if (m_DirPath.GetAt(m_DirPath.GetLength()-1) != '\\')
        m_DirPath += L"\\";
}

CPathWatcher::CDirWatchInfo::~CDirWatchInfo()
{
    CloseDirectoryHandle();
}

bool CPathWatcher::CDirWatchInfo::CloseDirectoryHandle()
{
    return m_hDir.CloseHandle();
}
