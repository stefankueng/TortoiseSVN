#include "StdAfx.h"
#include "Remotecachelink.h"
#include "ShellExt.h"
#include "..\TSVNCache\CacheInterface.h"

CRemoteCacheLink::CRemoteCacheLink(void) :
	m_hPipe(INVALID_HANDLE_VALUE)
{
	ZeroMemory(&m_dummyStatus, sizeof(m_dummyStatus));
	m_dummyStatus.text_status = svn_wc_status_none;
	m_dummyStatus.prop_status = svn_wc_status_none;
	m_dummyStatus.repos_text_status = svn_wc_status_none;
	m_dummyStatus.repos_prop_status = svn_wc_status_none;
	m_lastTimeout = 0;
	m_critSec.Init();
}

CRemoteCacheLink::~CRemoteCacheLink(void)
{
	m_critSec.Term();
}



bool CRemoteCacheLink::EnsurePipeOpen()
{
	AutoLocker lock(m_critSec);
	if(m_hPipe != INVALID_HANDLE_VALUE)
	{
		return true;
	}

	m_hPipe = CreateFile( 
		_T("\\\\.\\pipe\\TSVNCache"),   // pipe name 
		GENERIC_READ |					// read and write access 
		GENERIC_WRITE, 
		0,								// no sharing 
		NULL,							// default security attributes
		OPEN_EXISTING,				// opens existing pipe 
		FILE_FLAG_OVERLAPPED,			// default attributes 
		NULL);							// no template file 


	if (m_hPipe != INVALID_HANDLE_VALUE) 
	{
		// The pipe connected; change to message-read mode. 
		DWORD dwMode; 

		dwMode = PIPE_READMODE_MESSAGE; 
		if(!SetNamedPipeHandleState( 
			m_hPipe,    // pipe handle 
			&dwMode,  // new pipe mode 
			NULL,     // don't set maximum bytes 
			NULL))    // don't set maximum time 
		{
			ATLTRACE("SetNamedPipeHandleState failed"); 
			CloseHandle(m_hPipe);
			m_hPipe = INVALID_HANDLE_VALUE;
			return false;
		}
		m_hEvent = CreateEvent(NULL, FALSE, FALSE, _T("TSVNCache_OverlappedClientWait"));
		if (m_hEvent)
			return true;
		ATLTRACE("CreateEvent failed"); 
		CloseHandle(m_hPipe);
		m_hPipe = INVALID_HANDLE_VALUE;
		CloseHandle(m_hEvent);
		m_hEvent = INVALID_HANDLE_VALUE;
		return false;
		
	}

	return false;
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
		STARTUPINFO startup;
		PROCESS_INFORMATION process;
		memset(&startup, 0, sizeof(startup));
		startup.cb = sizeof(startup);
		memset(&process, 0, sizeof(process));

		CRegStdString cachePath(_T("Software\\TortoiseSVN\\CachePath"), _T("TSVNCache.exe"), false, HKEY_LOCAL_MACHINE);
		CString sCachePath = cachePath;
		if (CreateProcess(sCachePath.GetBuffer(sCachePath.GetLength()+1), _T(""), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
		{
			// It's not appropriate to do a message box here, because there may be hundreds of calls
			sCachePath.ReleaseBuffer();
			ATLTRACE("Failed to start cache\n");
			return false;
		} 
		sCachePath.ReleaseBuffer();

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
	request.flags = 0;
	if(bRecursive)
	{
		request.flags |= TSVNCACHE_FLAGS_RECUSIVE_STATUS;
	}
	wcsncpy(request.path, Path.GetWinPath(), MAX_PATH);
	ZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));
	m_Overlapped.hEvent = m_hEvent;
	// Do the transaction in overlapped mode.
	// That way, if anything happens which might block this call
	// we still can get out of it. We NEVER MUST BLOCK THE SHELL!
	// A blocked shell is a very bad user impression, because users
	// who don't know why it's blocked might find the only solution
	// to such a problem is a reboot and therefore they might loose
	// valuable data. 
	// Sure, it would be better to have no situations where the shell
	// even can get blocked, but the timeout of 5 seconds is long enough
	// so that users still recognize that something might be wrong and
	// report back to us so we can investigate further.
	if (!TransactNamedPipe(m_hPipe, &request, sizeof(request), pReturnedStatus, sizeof(*pReturnedStatus), &nBytesRead, &m_Overlapped))
	{
		if (GetLastError()!=ERROR_IO_PENDING)
		{
			OutputDebugStringA("TortoiseShell: TransactNamedPipe failed\n"); 
			CloseHandle(m_hPipe);
			CloseHandle(m_hEvent);
			m_hPipe = INVALID_HANDLE_VALUE;
			m_hEvent = INVALID_HANDLE_VALUE;
			return false;
		}
	}

	DWORD dwWait = WaitForSingleObject(m_hEvent, INFINITE);
	if (dwWait == WAIT_OBJECT_0)
	{
		if (GetOverlappedResult(m_hPipe, &m_Overlapped, &nBytesRead, FALSE))
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
	}
	OutputDebugStringA("TortoiseShell: WaitForSingleObject failed - Pipe timeout\n"); 
	CloseHandle(m_hPipe);
	CloseHandle(m_hEvent);
	m_hPipe = INVALID_HANDLE_VALUE;
	m_hEvent = INVALID_HANDLE_VALUE;
	return false;
}
