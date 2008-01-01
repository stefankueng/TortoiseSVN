// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2007-2008 - TortoiseSVN

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

CString GetCachePipeName()
{
	CString s = TSVN_CACHE_PIPE_NAME;
	HANDLE token;
	DWORD len;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
	if(result)
	{
		GetTokenInformation(token, TokenStatistics, NULL, 0, &len);
		LPBYTE data = new BYTE[len];
		GetTokenInformation(token, TokenStatistics, data, len, &len);
		LUID uid = ((PTOKEN_STATISTICS)data)->AuthenticationId;
		delete [ ] data;
		CString t;
		t.Format(_T("-%08x%08x"), uid.HighPart, uid.LowPart);
		CloseHandle(token);
		return s + t;
	}
	return s;
}

CString GetCacheCommandPipeName()
{
	CString s = TSVN_CACHE_COMMANDPIPE_NAME;
	HANDLE token;
	DWORD len;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
	if(result)
	{
		GetTokenInformation(token, TokenStatistics, NULL, 0, &len);
		LPBYTE data = new BYTE[len];
		GetTokenInformation(token, TokenStatistics, data, len, &len);
		LUID uid = ((PTOKEN_STATISTICS)data)->AuthenticationId;
		delete [ ] data;
		CString t;
		t.Format(_T("-%08x%08x"), uid.HighPart, uid.LowPart);
		CloseHandle(token);
		return s + t;
	}
	return s;
}
