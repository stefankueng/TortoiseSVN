// TortoiseOverlays - an overlay handler for Tortoise clients
// Copyright (C) 2007, 2009-2011, 2013-2014 - TortoiseSVN
#include "stdafx.h"
#include "ShellExt.h"
#include "Guids.h"

// "The Shell calls IShellIconOverlayIdentifier::GetOverlayInfo to request the
//  location of the handler's icon overlay. The icon overlay handler returns
//  the name of the file containing the overlay image, and its index within
//  that file. The Shell then adds the icon overlay to the system image list."
STDMETHODIMP CShellExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
    if(pwszIconFile == 0)
        return E_POINTER;
    if(pIndex == 0)
        return E_POINTER;
    if(pdwFlags == 0)
        return E_POINTER;
    if(cchMax < 1)
        return E_INVALIDARG;

    // Set "out parameters" in case we return S_FALSE or any code called from here
    // forgets to set them.
    *pwszIconFile = 0;
    *pIndex = 0;
    *pdwFlags = 0;

    int nInstalledOverlays = GetInstalledOverlays();

    // only a limited number of overlay slots can be used (determined by testing,
    // since not all overlay handlers are registered in the registry, e.g., the
    // shortcut (arrow) overlay isn't listed there).
    // The following overlays must be accounted for but are not listed under ShellIconOverlayIdentifiers:
    // * Shortcut arrow
    // * gray X on Vista+ (black dot on XP) for archived files
    // * Shared Hand (Windows XP only)
    // * UAC shield (Windows Vista+ only)

    const int nOverlayLimit = 12;

    bool dropIgnored = DropHandler(L"ShowIgnoredOverlay");
    if (dropIgnored)
        nInstalledOverlays--;

    bool dropUnversioned = DropHandler(L"ShowUnversionedOverlay");
    if (dropUnversioned)
        nInstalledOverlays--;

    bool dropAdded = DropHandler(L"ShowAddedOverlay");
    if (dropAdded)
        nInstalledOverlays--;

    bool dropLocked = DropHandler(L"ShowLockedOverlay");
    if (dropLocked)
        nInstalledOverlays--;

    bool dropReadonly = DropHandler(L"ShowReadonlyOverlay");
    if (dropReadonly)
        nInstalledOverlays--;

    bool dropDeleted = DropHandler(L"ShowDeletedOverlay");
    if (dropDeleted)
        nInstalledOverlays--;
    // The conflict, modified and normal overlays must not be disabled since
    // those are essential for Tortoise clients.


    if (dropIgnored     && (m_State == FileStateIgnored))
        return S_FALSE;
    if (dropUnversioned && (m_State == FileStateUnversioned))
        return S_FALSE;
    if (dropAdded       && (m_State == FileStateAdded))
        return S_FALSE;
    if (dropLocked      && (m_State == FileStateLocked))
        return S_FALSE;
    if (dropReadonly    && (m_State == FileStateReadOnly))
        return S_FALSE;
    if (dropDeleted     && (m_State == FileStateDeleted))
        return S_FALSE;

    //
    // If there are more than the maximum number of handlers registered, then
    // we have to drop some of our handlers to make sure that
    // the 'important' handlers are loaded properly:
    //
    // max     registered: drop the locked overlay
    // max + 1 registered: drop the locked and the ignored overlay
    // max + 2 registered: drop the locked, ignored and readonly overlay
    // max + 3 or more registered: drop the locked, ignored, readonly and unversioned overlay

    if ((m_State == FileStateUnversioned)&&(nInstalledOverlays > nOverlayLimit + 3))
        return S_FALSE;     // don't use the 'unversioned' overlay
    if ((m_State == FileStateReadOnly)&&(nInstalledOverlays > nOverlayLimit + 2))
        return S_FALSE;     // don't show the 'needs-lock' overlay
    if ((m_State == FileStateIgnored)&&(nInstalledOverlays > nOverlayLimit + 1))
        return S_FALSE;     // don't use the 'ignored' overlay
    if ((m_State == FileStateLocked)&&(nInstalledOverlays > nOverlayLimit))
        return S_FALSE;     // don't show the 'locked' overlay

    const TCHAR* iconName = 0;

    switch (m_State)
    {
        case FileStateNormal        : iconName = L"NormalIcon"; break;
        case FileStateModified      : iconName = L"ModifiedIcon"; break;
        case FileStateConflict      : iconName = L"ConflictIcon"; break;
        case FileStateDeleted       : iconName = L"DeletedIcon"; break;
        case FileStateReadOnly      : iconName = L"ReadOnlyIcon"; break;
        case FileStateLocked        : iconName = L"LockedIcon"; break;
        case FileStateAdded         : iconName = L"AddedIcon"; break;
        case FileStateIgnored       : iconName = L"IgnoredIcon"; break;
        case FileStateUnversioned   : iconName = L"UnversionedIcon"; break;
        default: return S_FALSE;
    }

    // Get folder icons from registry
    // Default icons are stored in LOCAL MACHINE
    // User selected icons are stored in CURRENT USER
    TCHAR regVal[1024] = { 0 };

    std::wstring icon;
    HKEY hkeys [] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    for (int i = 0; i < _countof(hkeys); ++i)
    {
        if (!icon.empty())
            continue;

        HKEY hkey = 0;
        if (::RegOpenKeyEx (hkeys[i],
            L"Software\\TortoiseOverlays",
                    0,
                    KEY_QUERY_VALUE,
                    &hkey) != ERROR_SUCCESS)
        {
            continue;
        }

        // in-out parameter, needs to be reinitialized prior to the call
        DWORD len = _countof(regVal);
        if (::RegQueryValueEx (hkey,
                             iconName,
                             NULL,
                             NULL,
                             (LPBYTE) regVal,
                             &len) == ERROR_SUCCESS)
        {
            icon.assign (regVal, len);
        }

        ::RegCloseKey(hkey);
    }

    // now load the Tortoise handlers and call their GetOverlayInfo method
    // note: we overwrite any changes a Tortoise handler will do to the
    // params and overwrite them later.
    LoadHandlers(pwszIconFile, cchMax, pIndex, pdwFlags);

    // Add name of appropriate icon
    if (icon.empty())
        return S_FALSE;

    if (icon.size() >= (size_t)cchMax)
        return E_INVALIDARG;

    wcsncpy_s (pwszIconFile, cchMax, icon.c_str(), cchMax);

    *pIndex = 0;
    *pdwFlags = ISIOI_ICONFILE;
    return S_OK;
}

STDMETHODIMP CShellExt::GetPriority(int *pPriority)
{
    if(pPriority == 0)
        return E_POINTER;
    switch (m_State)
    {
        case FileStateConflict:
            *pPriority = 0;
            break;
        case FileStateModified:
            *pPriority = 1;
            break;
        case FileStateDeleted:
            *pPriority = 2;
            break;
        case FileStateAdded:
            *pPriority = 3;
            break;
        case FileStateNormal:
            *pPriority = 4;
            break;
        case FileStateUnversioned:
            *pPriority = 5;
            break;
        case FileStateReadOnly:
            *pPriority = 6;
            break;
        case FileStateIgnored:
            *pPriority = 7;
            break;
        case FileStateLocked:
            *pPriority = 8;
            break;
        default:
            *pPriority = 100;
            return S_FALSE;
    }
    return S_OK;
}

STDMETHODIMP CShellExt::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
    if(pwszPath == 0)
        return E_INVALIDARG;
    for (std::vector<DLLPointers>::iterator it = m_dllpointers.begin(); it != m_dllpointers.end(); ++it)
    {
        if (it->pShellIconOverlayIdentifier)
        {
            if (it->pShellIconOverlayIdentifier->IsMemberOf(pwszPath, dwAttrib) == S_OK)
                return S_OK;
        }
    }
    return S_FALSE;
}

int CShellExt::GetInstalledOverlays()
{
    // if there are more than 12 overlay handlers installed, then that means not all
    // of the overlay handlers can be shown. Windows chooses the ones first
    // returned by RegEnumKeyEx() and just drops the ones that come last in
    // that enumeration.
    int nInstalledOverlayhandlers = 0;
    // scan the registry for installed overlay handlers
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers",
        0, KEY_ENUMERATE_SUB_KEYS, &hKey)==ERROR_SUCCESS)
    {
        TCHAR value[2048] = { 0 };
        TCHAR keystring[2048] = { 0 };
        for (int i = 0, rc = ERROR_SUCCESS; rc == ERROR_SUCCESS; i++)
        {
            DWORD size = _countof(value);
            FILETIME last_write_time;
            rc = RegEnumKeyEx(hKey, i, value, &size, NULL, NULL, NULL, &last_write_time);
            if (rc == ERROR_SUCCESS)
            {
                // check if there's a 'default' entry with a guid
                wcscpy_s(keystring, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\");
                wcscat_s(keystring, value);
                DWORD dwType = 0;
                DWORD dwSize = _countof(value); // the API docs only specify "The size of the destination data buffer",
                                                // but better be safe than sorry using _countof instead of sizeof
                if (SHGetValue(HKEY_LOCAL_MACHINE,
                    keystring,
                    NULL,
                    &dwType, value, &dwSize) == ERROR_SUCCESS)
                {
                    if ((dwSize > 10)&&(value[0] == '{'))
                        nInstalledOverlayhandlers++;
                }
            }
        }
        RegCloseKey(hKey);
    }
    return nInstalledOverlayhandlers;
}

void CShellExt::LoadHandlers(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
    const TCHAR* name = 0;
    switch (m_State)
    {
    case FileStateNormal        : name = L"Software\\TortoiseOverlays\\Normal"; break;
    case FileStateModified      : name = L"Software\\TortoiseOverlays\\Modified"; break;
    case FileStateConflict      : name = L"Software\\TortoiseOverlays\\Conflict"; break;
    case FileStateDeleted       : name = L"Software\\TortoiseOverlays\\Deleted"; break;
    case FileStateReadOnly      : name = L"Software\\TortoiseOverlays\\ReadOnly"; break;
    case FileStateLocked        : name = L"Software\\TortoiseOverlays\\Locked"; break;
    case FileStateAdded         : name = L"Software\\TortoiseOverlays\\Added"; break;
    case FileStateIgnored       : name = L"Software\\TortoiseOverlays\\Ignored"; break;
    case FileStateUnversioned   : name = L"Software\\TortoiseOverlays\\Unversioned"; break;
    default: return;
    }

    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        name,
        0, KEY_READ, &hKey)==ERROR_SUCCESS)
    {
        for (int i = 0, rc = ERROR_SUCCESS; rc == ERROR_SUCCESS; i++)
        {
            TCHAR k[MAX_PATH] = { 0 };
            TCHAR value[MAX_PATH] = { 0 };
            DWORD sizek = _countof(k);
            DWORD sizev = _countof(value);
            rc = RegEnumValue(hKey, i, k, &sizek, NULL, NULL, (LPBYTE)value, &sizev);
            if (rc == ERROR_SUCCESS)
            {
                TCHAR comobj[MAX_PATH] = { 0 };
                TCHAR modpath[MAX_PATH] = { 0 };
                wcscpy_s(comobj, L"CLSID\\");
                wcscat_s(comobj, value);
                wcscat_s(comobj, L"\\InprocServer32");
                if (SHRegGetPath(HKEY_CLASSES_ROOT, comobj, L"", modpath, 0) == ERROR_SUCCESS)
                {
                    LoadRealLibrary(modpath, value, pwszIconFile, cchMax, pIndex, pdwFlags);
                }
            }
        }
        RegCloseKey(hKey);
    }
}

void CShellExt::LoadRealLibrary(LPCTSTR ModuleName, LPCTSTR classIdString, LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
    static const char GetClassObject[] = "DllGetClassObject";
    static const char CanUnloadNow[] = "DllCanUnloadNow";

    IID classId;
    if (IIDFromString((LPOLESTR)classIdString, &classId) != S_OK)
        return;

    DLLPointers pointers;

    pointers.hDll = LoadLibraryEx(ModuleName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (pointers.hDll == NULL)
        return;

    pointers.pDllGetClassObject = NULL;
    pointers.pDllCanUnloadNow = NULL;
    pointers.pDllGetClassObject = (LPFNGETCLASSOBJECT)GetProcAddress(pointers.hDll, GetClassObject);
    if (pointers.pDllGetClassObject == NULL)
    {
        FreeLibrary(pointers.hDll);
        return;
    }
    pointers.pDllCanUnloadNow = (LPFNCANUNLOADNOW)GetProcAddress(pointers.hDll, CanUnloadNow);
    if (pointers.pDllCanUnloadNow == NULL)
    {
        FreeLibrary(pointers.hDll);
        return;
    }

    IClassFactory * pClassFactory = NULL;
    if (SUCCEEDED(pointers.pDllGetClassObject(classId, IID_IClassFactory, (LPVOID*)&pClassFactory)))
    {
        IShellIconOverlayIdentifier * pShellIconOverlayIdentifier = NULL;
        if (SUCCEEDED(pClassFactory->CreateInstance(NULL, IID_IShellIconOverlayIdentifier, (LPVOID*)&pShellIconOverlayIdentifier)))
        {
            pointers.pShellIconOverlayIdentifier = pShellIconOverlayIdentifier;
            if (pointers.pShellIconOverlayIdentifier->GetOverlayInfo(pwszIconFile, cchMax, pIndex, pdwFlags) != S_OK)
            {
                // the overlay handler refused to be properly initialized
                // Release obtained objects and then unload the module
                pClassFactory->Release();
                pointers.pShellIconOverlayIdentifier->Release();
                FreeLibrary(pointers.hDll);
                return;
            }
        }
        pClassFactory->Release();
    }

    m_dllpointers.push_back(pointers);
}

bool CShellExt::DropHandler(LPCWSTR registryKey)
{
    bool drop = false;
    DWORD dwType = REG_NONE;
    DWORD dwData = 0;
    DWORD dwDataSize = sizeof(dwData);
    if (SHGetValue(HKEY_CURRENT_USER, L"Software\\TortoiseOverlays", registryKey, &dwType, &dwData, &dwDataSize) == ERROR_SUCCESS)
    {
        if (dwType == REG_DWORD)
        {
            if (dwData == 0)
            {
                drop = true;
            }
        }
    }
    return drop;
}