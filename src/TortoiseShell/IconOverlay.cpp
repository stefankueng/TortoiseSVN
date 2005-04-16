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
#include "stdafx.h"
#include "ShellExt.h"
#include "Guids.h"
#include "PreserveChdir.h"
#include "UnicodeStrings.h"
#include "SVNStatus.h"
#include "..\TSVNCache\CacheInterface.h"

// "The Shell calls IShellIconOverlayIdentifier::GetOverlayInfo to request the
//  location of the handler's icon overlay. The icon overlay handler returns
//  the name of the file containing the overlay image, and its index within
//  that file. The Shell then adds the icon overlay to the system image list."

STDMETHODIMP CShellExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
    // HACK: Under NT (and 95?), file open dialog crashes apps upon shutdown
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if( !GetVersionEx(&osv) )
    {
        return S_FALSE;
    }
    // Under NT (and 95?), file open dialog crashes apps upon shutdown,
    // so we disable our icon overlays unless we are in Explorer process
    // space.
    bool bAllowOverlayInFileDialogs = osv.dwMajorVersion > 4 || // allow anything major > 4
        (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
        osv.dwMajorVersion == 4 && osv.dwMinorVersion > 0); // plus Windows 98/Me
	
	// There is also a user-option to suppress this
	if(bAllowOverlayInFileDialogs && CRegStdWORD(_T("Software\\TortoiseSVN\\OverlaysOnlyInExplorer"), FALSE))
	{
		bAllowOverlayInFileDialogs = false;
	}

	if(!bAllowOverlayInFileDialogs)
	{
		// Test if we are in Explorer
		DWORD modpathlen = 0;
		TCHAR * buf = NULL;
		DWORD pathLength = 0;
		do 
		{
			modpathlen += MAX_PATH;		// MAX_PATH is not the limit here!
			if (buf)
				delete buf;
			buf = new TCHAR[modpathlen];
			pathLength = GetModuleFileName(NULL, buf, modpathlen);
		} while (pathLength == modpathlen);
		if(pathLength >= 13)
		{
			if ((_tcsicmp(&buf[pathLength-13], _T("\\explorer.exe"))) != 0)
			{
				delete buf;
				return S_FALSE;
			}
		}
		delete buf;
	}

    // Get folder icons from registry
	// Default icons are stored in LOCAL MACHINE
	// User selected icons are stored in CURRENT USER
	TCHAR regVal[1024];
	DWORD len = 1024;

	stdstring icon;
	stdstring iconName;

	HKEY hkeys [] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
	switch (m_State)
	{
		case Versioned : iconName = _T("InSubversionIcon"); break;
		case Modified  : iconName = _T("ModifiedIcon"); break;
		case Conflict  : iconName = _T("ConflictIcon"); break;
		case Deleted   : iconName = _T("DeletedIcon"); break;
		case ReadOnly  : iconName = _T("ReadOnlyIcon"); break;
	}

	for (int i = 0; i < 2; ++i)
	{
		HKEY hkey = 0;

		if (::RegOpenKeyEx (hkeys[i],
			_T("Software\\TortoiseSVN"),
                    0,
                    KEY_QUERY_VALUE,
                    &hkey) != ERROR_SUCCESS)
			continue;

		if (icon.empty() == true
			&& (::RegQueryValueEx (hkey,
							 iconName.c_str(),
							 NULL,
							 NULL,
							 (LPBYTE) regVal,
							 &len)) == ERROR_SUCCESS)
			icon.assign (regVal, len);

		::RegCloseKey(hkey);

	}

    // Add name of appropriate icon
    if (icon.empty() == false)
#ifdef UNICODE
        wcsncpy (pwszIconFile, icon.c_str(), cchMax);
#else
        wcsncpy (pwszIconFile, MultibyteToWide(icon.c_str()).c_str(), cchMax);
#endif		
    else
        return S_FALSE;

	// Now here's where we can find out if due to lack of enough overlay
	// slots some of our overlays won't be shown.
	// To do that we have to mark every overlay handler that's successfully
	// loaded, so we can later check if some are missing
	switch (m_State)
	{
		case Versioned : g_normalovlloaded = true; break;
		case Modified  : g_modifiedovlloaded = true; break;
		case Conflict  : g_conflictedovlloaded = true; break;
		case Deleted   : g_deletedovlloaded = true; break;
		case ReadOnly  : g_readonlyovlloaded = true; break;
	}

	ATLTRACE2(_T("Icon loaded : %s\n"), icon.c_str());
    *pIndex = 0;
    *pdwFlags = ISIOI_ICONFILE;
    return S_OK;
};

STDMETHODIMP CShellExt::GetPriority(int *pPriority)
{
	switch (m_State)
	{
		case Conflict:
			*pPriority = 0;
			break;
		case Modified:
			*pPriority = 1;
			break;
		case Deleted:
			*pPriority = 2;
			break;
		case ReadOnly:
			*pPriority = 3;
			break;
		case Versioned:
			*pPriority = 4;
			break;
		default:
			*pPriority = 100;
			return S_FALSE;
	}
    return S_OK;
}

// "Before painting an object's icon, the Shell passes the object's name to
//  each icon overlay handler's IShellIconOverlayIdentifier::IsMemberOf
//  method. If a handler wants to have its icon overlay displayed,
//  it returns S_OK. The Shell then calls the handler's
//  IShellIconOverlayIdentifier::GetOverlayInfo method to determine which icon
//  to display."

STDMETHODIMP CShellExt::IsMemberOf(LPCWSTR pwszPath, DWORD /*dwAttrib*/)
{
	PreserveChdir preserveChdir;
	svn_wc_status_kind status = svn_wc_status_unversioned;
	bool readonlyoverlay = false;
	if (pwszPath == NULL)
		return S_FALSE;
#ifdef UNICODE
	const TCHAR* pPath = pwszPath;
#else
	std::string snPath = WideToUTF8(std::basic_string<wchar_t>(pwszPath));
	const TCHAR* pPath = snPath.c_str();
#endif

	// since the shell calls each and every overlay handler with the same filepath
	// we use a small 'fast' cache of just one path here.
	// To make sure that cache expires, clear it as soon as one handler is used.
	if (_tcscmp(pPath, g_filepath.c_str())==0)
	{
		status = g_filestatus;
	}
	else
	{
		if (!g_ShellCache.IsPathAllowed(pPath))
		{
			return S_FALSE;
		}

		TSVNCacheResponse itemStatus;
		ZeroMemory(&itemStatus, sizeof(itemStatus));
		if (g_remoteCacheLink.GetStatusFromRemoteCache(CTSVNPath(pPath), &itemStatus, !!g_ShellCache.IsRecursive()))
		{
			status = SVNStatus::GetMoreImportant(itemStatus.m_status.text_status, itemStatus.m_status.prop_status);
			if ((itemStatus.m_kind == svn_node_file)&&(status == svn_wc_status_normal)&&(itemStatus.m_readonly))
				readonlyoverlay = true;
		}
	}
	g_filepath.clear();
	g_filepath = pPath;
	g_filestatus = status;
	g_readonlyoverlay = readonlyoverlay;

	ATLTRACE("Status %d for file %ws\n", status, pwszPath);

	//the priority system of the shell doesn't seem to work as expected (or as I expected):
	//as it seems that if one handler returns S_OK then that handler is used, no matter
	//if other handlers would return S_OK too (they're never called on my machine!)
	//So we return S_OK for ONLY ONE handler!
	switch (status)
	{
		// note: we can show other overlays if due to lack of enough free overlay
		// slots some of our overlays aren't loaded. But we assume that
		// at least the 'normal' and 'modified' overlay are available.
		case svn_wc_status_none:
			return S_FALSE;
		case svn_wc_status_unversioned:
			return S_FALSE;
		case svn_wc_status_normal:
		case svn_wc_status_external:
		case svn_wc_status_incomplete:
			if ((readonlyoverlay)&&(g_readonlyovlloaded))
			{
				if (m_State == ReadOnly)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else if (m_State == Versioned)
			{
				g_filepath.clear();
				return S_OK;
			}
			else
				return S_FALSE;
		case svn_wc_status_missing:
		case svn_wc_status_deleted:
			if (g_deletedovlloaded)
			{
				if (m_State == Deleted)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else
			{
				// the 'deleted' overlay isn't available (due to lack of enough
				// overlay slots). So just show the 'modified' overlay instead.
				if (m_State == Modified)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
		case svn_wc_status_replaced:
		case svn_wc_status_added:
		case svn_wc_status_modified:
		case svn_wc_status_merged:
			if (m_State == Modified)
			{
				g_filepath.clear();
				return S_OK;
			}
			else
				return S_FALSE;
		case svn_wc_status_conflicted:
		case svn_wc_status_obstructed:
			if (g_conflictedovlloaded)
			{
				if (m_State == Conflict)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else
			{
				// the 'conflicted' overlay isn't available (due to lack of enough
				// overlay slots). So just show the 'modified' overlay instead.
				if (m_State == Modified)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
		default:
			return S_FALSE;
	} // switch (status)
    //return S_FALSE;
}

