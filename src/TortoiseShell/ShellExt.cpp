// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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

#pragma warning (disable : 4786)

// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
#include <initguid.h>
#include "Guids.h"

#include "ShellExt.h"
#include "ShellObjects.h"
#include "..\version.h"
#include "libintl.h"
#include "auto_buffer.h"
#undef swprintf

extern ShellObjects g_shellObjects;

// *********************** CShellExt *************************
CShellExt::CShellExt(FileState state)
{
    m_State = state;

    m_cRef = 0L;
	InterlockedIncrement(&g_cRefThisDll);

	{
		AutoLocker lock(g_csGlobalCOMGuard);
		g_shellObjects.Insert(this);
	}
	
    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
			ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES
    };
    InitCommonControlsEx(&used);
	LoadLangDll();
}

CShellExt::~CShellExt()
{
	AutoLocker lock(g_csGlobalCOMGuard);
	InterlockedDecrement(&g_cRefThisDll);
	g_shellObjects.Erase(this);
}

void LoadLangDll()
{
	if ((g_langid != g_ShellCache.GetLangID())&&((g_langTimeout == 0)||(g_langTimeout < GetTickCount())))
	{
		g_langid = g_ShellCache.GetLangID();
		DWORD langId = g_langid;
		TCHAR langDll[MAX_PATH*4];
		HINSTANCE hInst = NULL;
		TCHAR langdir[MAX_PATH] = {0};
		char langdirA[MAX_PATH] = {0};
		if (GetModuleFileName(g_hmodThisDll, langdir, MAX_PATH)==0)
			return;
		if (GetModuleFileNameA(g_hmodThisDll, langdirA, MAX_PATH)==0)
			return;
		TCHAR * dirpoint = _tcsrchr(langdir, '\\');
		char * dirpointA = strrchr(langdirA, '\\');
		if (dirpoint)
			*dirpoint = 0;
		if (dirpointA)
			*dirpointA = 0;
		dirpoint = _tcsrchr(langdir, '\\');
		dirpointA = strrchr(langdirA, '\\');
		if (dirpoint)
			*dirpoint = 0;
		if (dirpointA)
			*dirpointA = 0;
		strcat_s(langdirA, MAX_PATH, "\\Languages");
		bindtextdomain ("subversion", langdirA);

		do
		{
			_stprintf_s(langDll, MAX_PATH*4, _T("%s\\Languages\\TortoiseProc%d.dll"), langdir, langId);
			BOOL versionmatch = TRUE;

			struct TRANSARRAY
			{
				WORD wLanguageID;
				WORD wCharacterSet;
			};

			DWORD dwReserved,dwBufferSize;
			dwBufferSize = GetFileVersionInfoSize((LPTSTR)langDll,&dwReserved);

			if (dwBufferSize > 0)
			{
				LPVOID pBuffer = (void*) malloc(dwBufferSize);

				if (pBuffer != (void*) NULL)
				{
					UINT        nInfoSize = 0,
						nFixedLength = 0;
					LPSTR       lpVersion = NULL;
					VOID*       lpFixedPointer;
					TRANSARRAY* lpTransArray;
					TCHAR       strLangProduktVersion[MAX_PATH];

					if (GetFileVersionInfo((LPTSTR)langDll,
						dwReserved,
						dwBufferSize,
						pBuffer))
					{
						// Query the current language
						if (VerQueryValue(	pBuffer,
							_T("\\VarFileInfo\\Translation"),
							&lpFixedPointer,
							&nFixedLength))
						{
							lpTransArray = (TRANSARRAY*) lpFixedPointer;

							_stprintf_s(strLangProduktVersion, MAX_PATH, _T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
								lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

							if (VerQueryValue(pBuffer,
								(LPTSTR)strLangProduktVersion,
								(LPVOID *)&lpVersion,
								&nInfoSize))
							{
								versionmatch = (_tcscmp((LPCTSTR)lpVersion, _T(STRPRODUCTVER)) == 0);
							}

						}
					}
					free(pBuffer);
				} // if (pBuffer != (void*) NULL) 
			} // if (dwBufferSize > 0) 
			else
				versionmatch = FALSE; 

			if (versionmatch)
				hInst = LoadLibrary(langDll);
			if (hInst != NULL)
			{
				if (g_hResInst != g_hmodThisDll)
					FreeLibrary(g_hResInst);
				g_hResInst = hInst;
			}
			else
			{
				DWORD lid = SUBLANGID(langId);
				lid--;
				if (lid > 0)
				{
					langId = MAKELANGID(PRIMARYLANGID(langId), lid);
				}
				else
					langId = 0;
			} 
		} while ((hInst == NULL) && (langId != 0));
		if (hInst == NULL)
		{
			// either the dll for the selected language is not present, or
			// it is the wrong version.
			// fall back to English and set a timeout so we don't retry
			// to load the language dll too often
			if (g_hResInst != g_hmodThisDll)
				FreeLibrary(g_hResInst);
			g_hResInst = g_hmodThisDll;
			g_langid = 1033;
			// set a timeout of 10 seconds
			if (g_ShellCache.GetLangID() != 1033)
				g_langTimeout = GetTickCount() + 10000;
		}
		else
			g_langTimeout = 0;
	} // if (g_langid != g_ShellCache.GetLangID()) 
}

tstring GetAppDirectory()
{
	tstring path;
	DWORD len = 0;
	DWORD bufferlen = MAX_PATH;		// MAX_PATH is not the limit here!
	do 
	{
		bufferlen += MAX_PATH;		// MAX_PATH is not the limit here!
		auto_buffer<TCHAR> pBuf(bufferlen);
		len = GetModuleFileName(g_hmodThisDll, pBuf, bufferlen);	
		path = tstring(pBuf, len);
	} while(len == bufferlen);
	path = path.substr(0, path.rfind('\\') + 1);

	return path;
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    if(ppv == 0)
		return E_POINTER;

	*ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppv = (LPCONTEXTMENU)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu2))
    {
        *ppv = (LPCONTEXTMENU2)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu3))
    {
        *ppv = (LPCONTEXTMENU3)this;
    }
    else if (IsEqualIID(riid, IID_IShellIconOverlayIdentifier))
    {
        *ppv = (IShellIconOverlayIdentifier*)this;
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppv = (LPSHELLPROPSHEETEXT)this;
    }
	else if (IsEqualIID(riid, IID_IColumnProvider)) 
	{ 
		*ppv = (IColumnProvider *)this;
	} 
	else if (IsEqualIID(riid, IID_IShellCopyHook))
	{
		*ppv = (ICopyHook *)this;
	}
    if (*ppv)
    {
        AddRef();
		
        return S_OK;
    }
	
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{
    if (--m_cRef)
        return m_cRef;
	
    delete this;
	
    return 0L;
}

// IPersistFile members
STDMETHODIMP CShellExt::GetClassID(CLSID *pclsid) 
{
    if(pclsid == 0)
		return E_POINTER;
	*pclsid = CLSID_TortoiseSVN_UNCONTROLLED;
    return S_OK;
}

STDMETHODIMP CShellExt::Load(LPCOLESTR /*pszFileName*/, DWORD /*dwMode*/)
{
    return S_OK;
}

// ICopyHook member
UINT __stdcall CShellExt::CopyCallback(HWND /*hWnd*/, UINT wFunc, UINT /*wFlags*/, LPCTSTR pszSrcFile, DWORD /*dwSrcAttribs*/, LPCTSTR /*pszDestFile*/, DWORD /*dwDestAttribs*/)
{
	if (wFunc == FO_COPY)
		return IDYES;	// copying is not a problem for us

	m_remoteCacheLink.ReleaseLockForPath(CTSVNPath(pszSrcFile));
	// we could now wait a little bit to give the cache time to release the handles.
	// but the explorer/shell already retries any action for about two seconds
	// if it first fails. So if the cache hasn't released the handle yet, the explorer
	// will retry anyway, so we just leave here immediately.
	return IDYES;
}
