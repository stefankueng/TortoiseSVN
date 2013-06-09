// TortoiseOverlays - an overlay handler for Tortoise clients
// Copyright (C) 2007, 2010-2011, 2013 - TortoiseSVN

#pragma once
#include <vector>

#include "resource.h"

extern  volatile LONG       g_cRefThisDll;          // Reference count of this DLL.


enum FileState
{
    FileStateNormal,
    FileStateModified,
    FileStateConflict,
    FileStateLocked,
    FileStateReadOnly,
    FileStateDeleted,
    FileStateAdded,
    FileStateIgnored,
    FileStateUnversioned,
    FileStateInvalid
};

struct DLLPointers
{
    DLLPointers() : hDll(NULL)
        , pDllGetClassObject(NULL)
        , pDllCanUnloadNow(NULL)
        , pShellIconOverlayIdentifier(NULL)
    {
    }

    HINSTANCE hDll;
    LPFNGETCLASSOBJECT pDllGetClassObject;
    LPFNCANUNLOADNOW pDllCanUnloadNow;
    IShellIconOverlayIdentifier * pShellIconOverlayIdentifier;
};

// The actual OLE Shell context menu handler
/**
 * The main class of our COM object / Shell Extension.
 * It contains all Interfaces we implement for the shell to use.
 */
class CShellExt : public IShellIconOverlayIdentifier
{
protected:
    FileState m_State;
    ULONG   m_cRef;

    std::vector<DLLPointers>         m_dllpointers;

private:
    int             GetInstalledOverlays(void);     ///< returns the maximum number of overlays TSVN shall use
    void            LoadRealLibrary(LPCTSTR ModuleName, LPCTSTR clsid, LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    void            LoadHandlers(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    bool            DropHandler(LPCWSTR registryKey);
public:
    CShellExt(FileState state);
    virtual ~CShellExt();

    /** \name IUnknown
     * IUnknown members
     */
    //@{
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    //@}


    /** \name IShellIconOverlayIdentifier
     * IShellIconOverlayIdentifier methods
     */
    //@{
    STDMETHODIMP    GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP    GetPriority(int *pPriority);
    STDMETHODIMP    IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);
    //@}

};
