// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
#include "StdAfx.h"
#include "Remotecachelink.h"
#include "ShellExt.h"
#include "..\TSVNCache\CacheInterface.h"
#include "TSVNPath.h"
#include "PathUtils.h"
#include "CreateProcessHelper.h"

CRemoteCacheLink::CRemoteCacheLink(void) 
	: m_hPipe(INVALID_HANDLE_VALUE)
	, m_hCommandPipe(INVALID_HANDLE_VALUE)
{
	SecureZeroMemory(&m_dummyStatus, sizeof(m_dummyStatus));
	m_dummyStatus.text_status = svn_wc_status_none;
	m_dummyStatus.prop_status = svn_wc_status_none;
	m_dummyStatus.repos_text_status = svn_wc_status_none;
	m_dummyStatus.repos_prop_status = svn_wc_status_none;
	m_lastTimeout = 0;
	m_critSec.Init();
}

CRemoteCacheLink::~CRemoteCacheLink(void)
{
	ClosePipe();
	CloseCommandPipe();
	m_critSec.Term();
}

bool CRemoteCacheLink::InternalEnsurePipeOpen ( HANDLE& hPipe
                                              , const CString& pipeName)
{
	if (hPipe != INVALID_HANDLE_VALUE)
	{
		return true;
	}

	hPipe = CreateFile(
		pipeName,				        // pipe name
		GENERIC_READ |					// read and write access
		GENERIC_WRITE,
		0,								// no sharing
		NULL,							// default security attributes
		OPEN_EXISTING,					// opens existing pipe
		FILE_FLAG_OVERLAPPED,			// default attributes
		NULL);							// no template file

	if (hPipe == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PIPE_BUSY)
	{
		// TSVNCache is running but is busy connecting a different client.
		// Do not give up immediately but wait for a few milliseconds until
		// the server has created the next pipe instance
		if (WaitNamedPipe (pipeName, 50))
		{
			hPipe = CreateFile(
				pipeName,				        // pipe name
				GENERIC_READ |					// read and write access
				GENERIC_WRITE,
				0,								// no sharing
				NULL,							// default security attributes
				OPEN_EXISTING,					// opens existing pipe
				FILE_FLAG_OVERLAPPED,			// default attributes
				NULL);							// no template file
		}
	}

	if (hPipe != INVALID_HANDLE_VALUE)
	{
		// The pipe connected; change to message-read mode.
		DWORD dwMode;

		dwMode = PIPE_READMODE_MESSAGE;
		if(!SetNamedPipeHandleState(
			hPipe,    // pipe handle
			&dwMode,  // new pipe mode
			NULL,     // don't set maximum bytes
			NULL))    // don't set maximum time
		{
			ATLTRACE("SetNamedPipeHandleState failed");
			CloseHandle(hPipe);
			hPipe = INVALID_HANDLE_VALUE;
			return false;
		}

		return true;
	}

	return false;
}

bool CRemoteCacheLink::EnsurePipeOpen()
{
	AutoLocker lock(m_critSec);

    if (InternalEnsurePipeOpen (m_hPipe, GetCachePipeName()))
	{
		// create an unnamed (=local) manual reset event for use in the overlapped structure
		m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (m_hEvent)
			return true;

		ATLTRACE("CreateEvent failed");
		ClosePipe();
	}

	return false;
}

bool CRemoteCacheLink::EnsureCommandPipeOpen()
{
	AutoLocker lock(m_critSec);
    return InternalEnsurePipeOpen (m_hCommandPipe, GetCacheCommandPipeName());
}

void CRemoteCacheLink::ClosePipe()
{
	AutoLocker lock(m_critSec);

	if(m_hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hPipe);
		CloseHandle(m_hEvent);
		m_hPipe = INVALID_HANDLE_VALUE;
		m_hEvent = INVALID_HANDLE_VALUE;
	}
}

void CRemoteCacheLink::CloseCommandPipe()
{
	AutoLocker lock(m_critSec);

	if(m_hCommandPipe != INVALID_HANDLE_VALUE)
	{
		// now tell the cache we don't need it's command thread anymore
		DWORD cbWritten; 
		TSVNCacheCommand cmd;
		SecureZeroMemory(&cmd, sizeof(TSVNCacheCommand));
		cmd.command = TSVNCACHECOMMAND_END;
		WriteFile( 
			m_hCommandPipe,			// handle to pipe 
			&cmd,			// buffer to write from 
			sizeof(cmd),	// number of bytes to write 
			&cbWritten,		// number of bytes written 
			NULL);			// not overlapped I/O 
		DisconnectNamedPipe(m_hCommandPipe); 
		CloseHandle(m_hCommandPipe); 
		m_hCommandPipe = INVALID_HANDLE_VALUE;
	}
}

bool CRemoteCacheLink::GetStatusFromRemoteCache(const CTSVNPath& Path, TSVNCacheResponse* pReturnedStatus, bool bRecursive)
{
	if(!EnsurePipeOpen())
	{
		// We've failed to open the pipe - try and start the cache
		// but only if the last try to start the cache was a certain time
		// ago. If we just try over and over again without a small pause
		// in between, the explorer is rendered unusable!
		// Failing to start the cache can have different reasons: missing exe,
		// missing registry key, corrupt exe, ...
		if (((long)GetTickCount() - m_lastTimeout) < 0)
			return false;
		// if we're in protected mode, don't try to start the cache: since we're
		// here, we know we can't access it anyway and starting a new process will
		// trigger a warning dialog in IE7+ on Vista - we don't want that.
		if (GetProcessIntegrityLevel() < SECURITY_MANDATORY_MEDIUM_RID)
			return false;

		CString sCachePath = CPathUtils::GetAppDirectory(g_hmodThisDll) + _T("TSVNCache.exe");
		if (!CCreateProcessHelper::CreateProcessDetached( sCachePath, NULL))
		{
			// It's not appropriate to do a message box here, because there may be hundreds of calls
			ATLTRACE("Failed to start cache\n");
			return false;
		}

		// Wait for the cache to open
		long endTime = (long)GetTickCount()+1000;
		while(!EnsurePipeOpen())
		{
			if(((long)GetTickCount() - endTime) > 0)
			{
				m_lastTimeout = (long)GetTickCount()+10000;
				return false;
			}
		}
	}

	AutoLocker lock(m_critSec);

	DWORD nBytesRead;
	TSVNCacheRequest request;
	request.flags = TSVNCACHE_FLAGS_NONOTIFICATIONS;
	if(bRecursive)
	{
		request.flags |= TSVNCACHE_FLAGS_RECUSIVE_STATUS;
	}
	wcsncpy_s(request.path, MAX_PATH+1, Path.GetWinPath(), MAX_PATH);
	SecureZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));
	m_Overlapped.hEvent = m_hEvent;
	// Do the transaction in overlapped mode.
	// That way, if anything happens which might block this call
	// we still can get out of it. We NEVER MUST BLOCK THE SHELL!
	// A blocked shell is a very bad user impression, because users
	// who don't know why it's blocked might find the only solution
	// to such a problem is a reboot and therefore they might loose
	// valuable data.
	// One particular situation where the shell could hang is when
	// the cache crashes and our crash report dialog comes up.
	// Sure, it would be better to have no situations where the shell
	// even can get blocked, but the timeout of 10 seconds is long enough
	// so that users still recognize that something might be wrong and
	// report back to us so we can investigate further.

	BOOL fSuccess = TransactNamedPipe(m_hPipe,
		&request, sizeof(request),
		pReturnedStatus, sizeof(*pReturnedStatus),
		&nBytesRead, &m_Overlapped);

	if (!fSuccess)
	{
		if (GetLastError()!=ERROR_IO_PENDING)
		{
			//OutputDebugStringA("TortoiseShell: TransactNamedPipe failed\n");
			ClosePipe();
			return false;
		}

		// TransactNamedPipe is working in an overlapped operation.
		// Wait for it to finish
		DWORD dwWait = WaitForSingleObject(m_hEvent, 10000);
		if (dwWait == WAIT_OBJECT_0)
		{
			fSuccess = GetOverlappedResult(m_hPipe, &m_Overlapped, &nBytesRead, FALSE);
		}
		else
		{
			// the cache didn't respond!
			fSuccess = FALSE;
		}
	}

	if (fSuccess)
	{
		if(nBytesRead == sizeof(TSVNCacheResponse))
		{
			// This is a full response - we need to fix-up some pointers
			pReturnedStatus->m_status.entry = &pReturnedStatus->m_entry;
			pReturnedStatus->m_entry.url = pReturnedStatus->m_url;
		}
		else
		{
			pReturnedStatus->m_status.entry = NULL;
		}

		return true;
	}
	ClosePipe();
	return false;
}

bool CRemoteCacheLink::ReleaseLockForPath(const CTSVNPath& path)
{
	EnsureCommandPipeOpen();
	if (m_hCommandPipe != INVALID_HANDLE_VALUE) 
	{
		DWORD cbWritten; 
		TSVNCacheCommand cmd;
		SecureZeroMemory(&cmd, sizeof(TSVNCacheCommand));
		cmd.command = TSVNCACHECOMMAND_RELEASE;
		wcsncpy_s(cmd.path, MAX_PATH+1, path.GetDirectory().GetWinPath(), MAX_PATH);
		BOOL fSuccess = WriteFile( 
			m_hCommandPipe,	// handle to pipe 
			&cmd,			// buffer to write from 
			sizeof(cmd),	// number of bytes to write 
			&cbWritten,		// number of bytes written 
			NULL);			// not overlapped I/O 
		if (! fSuccess || sizeof(cmd) != cbWritten)
		{
			CloseCommandPipe();
			return false;
		}
		return true;
	}
	return false;
}

DWORD CRemoteCacheLink::GetProcessIntegrityLevel()
{
	HANDLE hToken;
	HANDLE hProcess;

	DWORD dwLengthNeeded;
	DWORD dwError = ERROR_SUCCESS;

	PTOKEN_MANDATORY_LABEL pTIL = NULL;
	DWORD dwIntegrityLevel = SECURITY_MANDATORY_MEDIUM_RID;

	hProcess = GetCurrentProcess();
	if (OpenProcessToken(hProcess, TOKEN_QUERY | 
		TOKEN_QUERY_SOURCE, &hToken)) 
	{
		// Get the Integrity level.
		if (!GetTokenInformation(hToken, TokenIntegrityLevel, 
			NULL, 0, &dwLengthNeeded))
		{
			dwError = GetLastError();
			if (dwError == ERROR_INSUFFICIENT_BUFFER)
			{
				pTIL = (PTOKEN_MANDATORY_LABEL)LocalAlloc(0, 
					dwLengthNeeded);
				if (pTIL != NULL)
				{
					if (GetTokenInformation(hToken, TokenIntegrityLevel, 
						pTIL, dwLengthNeeded, &dwLengthNeeded))
					{
						dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid, 
							(DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid)-1));
					}
					LocalFree(pTIL);
				}
			}
		}
		CloseHandle(hToken);
	}

	return dwIntegrityLevel;
}