﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009-2011, 2016, 2021 - TortoiseSVN

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
#include "ShellExt.h"
#include "ShellExtClassFactory.h"

CShellExtClassFactory::CShellExtClassFactory(FileState state)
    : m_stateToMake(state)
{
    m_cRef = 0L;

    InterlockedIncrement(&g_cRefThisDll);
}

CShellExtClassFactory::~CShellExtClassFactory()
{
    InterlockedDecrement(&g_cRefThisDll);
}

HRESULT STDMETHODCALLTYPE CShellExtClassFactory::QueryInterface(REFIID riid,
                                                                LPVOID FAR *ppv)
{
    if (ppv == nullptr)
        return E_POINTER;

    *ppv = nullptr;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = static_cast<LPCLASSFACTORY>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CShellExtClassFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE CShellExtClassFactory::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;

    return 0L;
}

HRESULT STDMETHODCALLTYPE CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                                REFIID    riid,
                                                                LPVOID *  ppvObj)
{
    if (ppvObj == nullptr)
        return E_POINTER;

    *ppvObj = nullptr;

    // Shell extensions typically don't support aggregation (inheritance)

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.

    CShellExt *pShellExt = new (std::nothrow) CShellExt(m_stateToMake); //Create the CShellExt object

    if (nullptr == pShellExt)
        return E_OUTOFMEMORY;

    const HRESULT hr = pShellExt->QueryInterface(riid, ppvObj);
    if (FAILED(hr))
        delete pShellExt;
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellExtClassFactory::LockServer(BOOL /*fLock*/)
{
    return E_NOTIMPL;
}
