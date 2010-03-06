// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006,2008-2010 - TortoiseSVN

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
#include "wininet.h"

// The name of the named-pipe for the cache
#ifdef WIN64
#define TSVN_CACHE_PIPE_NAME _T("\\\\.\\pipe\\TSVNCache64")
#define TSVN_CACHE_COMMANDPIPE_NAME _T("\\\\.\\pipe\\TSVNCacheCommand64")
#define TSVN_CACHE_WINDOW_NAME _T("TSVNCacheWindow64")
#define TSVN_CACHE_MUTEX_NAME _T("TSVNCacheMutex64")
#else
#define TSVN_CACHE_PIPE_NAME _T("\\\\.\\pipe\\TSVNCache")
#define TSVN_CACHE_COMMANDPIPE_NAME _T("\\\\.\\pipe\\TSVNCacheCommand")
#define TSVN_CACHE_WINDOW_NAME _T("TSVNCacheWindow")
#define TSVN_CACHE_MUTEX_NAME _T("TSVNCacheMutex")
#endif

CString GetCachePipeName();
CString GetCacheCommandPipeName();
CString GetCacheMutexName();

CString GetCacheID();
bool	SendCacheCommand(BYTE command, const WCHAR * path = NULL);

/**
 * \ingroup TSVNCache
 * A structure passed as a request from the shell (or other client) to the external cache
 */ 
struct TSVNCacheRequest
{
	DWORD flags;
	WCHAR path[MAX_PATH+1];
};

// CustomActions will use this header but does not need nor understand the SVN types ...

#ifdef SVN_WC_H

/**
 * \ingroup TSVNCache
 * The structure returned as a response
 */
struct TSVNCacheResponse
{
	svn_wc_status2_t m_status;
	svn_wc_entry_t m_entry;
	svn_node_kind_t m_kind;
	char m_url[INTERNET_MAX_URL_LENGTH+1];
	char m_owner[255];		///< owner of the lock
	char m_author[255];
	bool m_needslock;		///< whether the file has the svn:needs-lock property set or not (only works with the new working copy version)
	bool m_tree_conflict;	///< whether the item has a tree conflict
};

#endif // SVN_WC_H

/**
 * \ingroup TSVNCache
 * a cache command
 */
struct TSVNCacheCommand
{
	BYTE command;				///< the command to execute
	WCHAR path[MAX_PATH+1];		///< path to do the command for
};

#define		TSVNCACHECOMMAND_END		0		///< ends the thread handling the pipe communication
#define		TSVNCACHECOMMAND_CRAWL		1		///< start crawling the specified path for changes
#define		TSVNCACHECOMMAND_REFRESHALL	2		///< Refreshes the whole cache, usually necessary after the "treat unversioned files as modified" option changed.
#define		TSVNCACHECOMMAND_RELEASE	3		///< Releases all open handles for the specified path and all paths below
#define		TSVNCACHECOMMAND_BLOCK		4		///< Blocks a path from getting crawled for a specific amount of time or until the TSVNCACHECOMMAND_UNBLOCK command is sent for that path
#define		TSVNCACHECOMMAND_UNBLOCK	5		///< Removes a path from the list of paths blocked from getting crawled

/// Set this flag if you already know whether or not the item is a folder
#define TSVNCACHE_FLAGS_FOLDERISKNOWN		0x01
/// Set this flag if the item is a folder
#define TSVNCACHE_FLAGS_ISFOLDER			0x02
/// Set this flag if you want recursive folder status (safely ignored for file paths)
#define TSVNCACHE_FLAGS_RECUSIVE_STATUS		0x04
/// Set this flag if notifications to the shell are not allowed
#define TSVNCACHE_FLAGS_NONOTIFICATIONS		0x08
/// all of the above flags or-gated:
#define TSVNCACHE_FLAGS_MASK            	0x0f