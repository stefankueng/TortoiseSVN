// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "stdafx.h"
#include "ShellExt.h"
#include "Guids.h"
#include "ShellExtClassFactory.h"

UINT      g_cRefThisDll = 0;				///< reference count of this DLL.
HINSTANCE g_hmodThisDll = NULL;				///< handle to this DLL itself.
SVNFolderStatus g_CachedStatus;				///< status cache
ShellCache g_ShellCache;					///< caching of registry entries, ...
CRegStdWORD			g_regLang;
DWORD				g_langid;
HINSTANCE			g_hResInst;
stdstring			g_filepath;
svn_wc_status_kind	g_filestatus;	///< holds the corresponding status to the file/dir above
CRITICAL_SECTION	g_csCacheGuard;

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /* lpReserved */)
{
#ifdef _DEBUG
	// if no debugger is present, then don't load the dll.
	// this prevents other apps from loading the dll and locking
	// it.

	bool bInShellTest = false;
	TCHAR buf[_MAX_PATH + 1];
	DWORD pathLength = GetModuleFileName(NULL, buf, _MAX_PATH);
	if(pathLength >= 14)
	{
		if ((_tcsicmp(&buf[pathLength-14], _T("\\ShellTest.exe"))) == 0)
		{
			bInShellTest = true;
		}
	}

	if (!::IsDebuggerPresent() && !bInShellTest)
	{
		ATLTRACE("In debug load preventer\n");
		return FALSE;
	}
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Extension DLL one-time initialization
        g_hmodThisDll = hInstance;
		InitializeCriticalSection(&g_csCacheGuard);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
    }
    return 1;   // ok
}

STDAPI DllCanUnloadNow(void)
{
	DeleteCriticalSection(&g_csCacheGuard);
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
	
    FileState state = Invalid;

    if (IsEqualIID(rclsid, CLSID_TortoiseSVN_UPTODATE))
        state = Versioned;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_MODIFIED))
        state = Modified;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_CONFLICTING))
        state = Conflict;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_UNCONTROLLED))
        state = Uncontrolled;
	else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_DROPHANDLER))
		state = DropHandler;
	else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_DELETED))
		state = Deleted;
	else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_ADDED))
		state = Added;
	
    if (state != Invalid)
    {
		CShellExtClassFactory *pcf = new CShellExtClassFactory(state);
		return pcf->QueryInterface(riid, ppvOut);
    }
	
    return CLASS_E_CLASSNOTAVAILABLE;

}


