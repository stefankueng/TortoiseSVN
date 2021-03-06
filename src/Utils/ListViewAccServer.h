// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010, 2012, 2021 - TortoiseSVN

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
#pragma once

/**
 * \ingroup Utils
 * Callback interface.
 * Implement this to provide help strings for list view items used
 * in MSAA (accessibility, e.g, screen readers).
 */
class ListViewAccProvider
{
public:
    virtual ~ListViewAccProvider() = default;
    /**
     * Called to retrieve the help string text.
     * \param hControl the HWND of the list view control
     * \param index the list view index the help string is asked for
     * \return a help string for the list view index
     */
    virtual CString GetListviewHelpString(HWND hControl, int index) = 0;
};

/**
 * Helper class to implement a help string callback for MSAA.
 * Use ListViewAccServer::CreateProvider() to create start the
 * callback.
 */
class ListViewAccServer : public IAccPropServer
{
public:
    ListViewAccServer(IAccPropServices* pAccPropSvc)
        : m_ref(1)
        , m_pAccPropSvc(pAccPropSvc)
        , m_pAccProvider(nullptr)
    {
        m_pAccPropSvc->AddRef();
    }

    virtual ~ListViewAccServer()
    {
        m_pAccPropSvc->Release();
    }

    static ListViewAccServer* CreateProvider(HWND hControl, ListViewAccProvider* provider);
    /// must be called in the WM_DESTROY handler!
    static void ClearProvider(HWND hControl);

    HRESULT STDMETHODCALLTYPE GetPropValue(const BYTE* pIDString, DWORD dwIDStringLen, MSAAPROPID idProp, VARIANT* pvarValue, BOOL* pfGotProp) override;

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG STDMETHODCALLTYPE   AddRef() override;
    ULONG STDMETHODCALLTYPE   Release() override;

private:
    ULONG                m_ref;
    IAccPropServices*    m_pAccPropSvc;
    ListViewAccProvider* m_pAccProvider;
};
