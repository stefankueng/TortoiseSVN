// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2007,2009-2010 - TortoiseSVN

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
#include "CacheInterface.h"
#include "auto_buffer.h"

CString GetCachePipeName()
{
	return TSVN_CACHE_PIPE_NAME + GetCacheID();
}

CString GetCacheCommandPipeName()
{
	return TSVN_CACHE_COMMANDPIPE_NAME + GetCacheID();
}

CString GetCacheMutexName()
{
	return TSVN_CACHE_MUTEX_NAME + GetCacheID();
}

CString GetCacheID()
{
    CString t;
	HANDLE token;
	DWORD len;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
	if(result)
	{
		GetTokenInformation(token, TokenStatistics, NULL, 0, &len);
        if (len >= sizeof (TOKEN_STATISTICS))
        {
            auto_buffer<BYTE> data (len);
		    GetTokenInformation(token, TokenStatistics, data, len, &len);
		    LUID uid = ((PTOKEN_STATISTICS)data.get())->AuthenticationId;
		    t.Format(_T("-%08x%08x"), uid.HighPart, uid.LowPart);
        }

        CloseHandle(token);
	}
    return t;
}

bool SendCacheCommand(BYTE command, const WCHAR * path /* = NULL */)
{
	HANDLE hPipe = CreateFile( 
		GetCacheCommandPipeName(),		// pipe name 
		GENERIC_READ |					// read and write access 
		GENERIC_WRITE, 
		0,								// no sharing 
		NULL,							// default security attributes
		OPEN_EXISTING,					// opens existing pipe 
		FILE_FLAG_OVERLAPPED,			// default attributes 
		NULL);							// no template file 

	if (hPipe == INVALID_HANDLE_VALUE)
		return false;

	// The pipe connected; change to message-read mode. 
	DWORD dwMode = PIPE_READMODE_MESSAGE; 
	if (SetNamedPipeHandleState( 
		hPipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL))    // don't set maximum time 
	{
		DWORD cbWritten; 
		TSVNCacheCommand cmd;
		SecureZeroMemory(&cmd, sizeof(TSVNCacheCommand));
		cmd.command = command;
		if (path)
			_tcsncpy_s(cmd.path, MAX_PATH+1, path, _TRUNCATE);
		BOOL fSuccess = WriteFile( 
			hPipe,			// handle to pipe 
			&cmd,			// buffer to write from 
			sizeof(cmd),	// number of bytes to write 
			&cbWritten,		// number of bytes written 
			NULL);			// not overlapped I/O 

		if (! fSuccess || sizeof(cmd) != cbWritten)
		{
			DisconnectNamedPipe(hPipe); 
			CloseHandle(hPipe); 
			hPipe = INVALID_HANDLE_VALUE;
			return false;
		}
		// now tell the cache we don't need it's command thread anymore
		SecureZeroMemory(&cmd, sizeof(TSVNCacheCommand));
		cmd.command = TSVNCACHECOMMAND_END;
		WriteFile( 
			hPipe,			// handle to pipe 
			&cmd,			// buffer to write from 
			sizeof(cmd),	// number of bytes to write 
			&cbWritten,		// number of bytes written 
			NULL);			// not overlapped I/O 
		DisconnectNamedPipe(hPipe); 
		CloseHandle(hPipe); 
	}
	else
	{
		ATLTRACE("SetNamedPipeHandleState failed"); 
		CloseHandle(hPipe);
		return false;
	}

	return true;
}