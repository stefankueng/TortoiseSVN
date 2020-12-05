﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2014-2017, 2020 - TortoiseSVN

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
#include "resource.h"
#include "StandAloneDlg.h"
#include "Monitor.h"

#pragma comment(lib, "Dwmapi.lib")

struct DPIAdjustData
{
    HWND hDlg;
    double zoom;
};

static BOOL CALLBACK DPIAdjustChildren(HWND hWnd, LPARAM lParam)
{
    DPIAdjustData* data = (DPIAdjustData*)lParam;
    if (data->hDlg != ::GetParent(hWnd))
    {
        return TRUE;
    }
    double zoom = data->zoom;

    RECT rect = {};
    ::GetWindowRect(hWnd, &rect);

    POINT pt = {rect.left, rect.top};
    ::ScreenToClient(::GetParent(hWnd), &pt);

    ::MoveWindow(hWnd,
                 (int)std::round((double)pt.x * zoom),
                 (int)std::round((double)pt.y * zoom),
                 (int)std::round(((double)rect.right - (double)rect.left) * zoom),
                 (int)std::round(((double)rect.bottom - (double)rect.top) * zoom),
                 TRUE);

    HFONT font = (HFONT)::SendMessage(hWnd, WM_GETFONT, 0, 0);
    if (font)
    {
        LOGFONT lf = { 0 };
        GetObject(font, sizeof(LOGFONT), &lf);
        lf.lfHeight = (LONG)(lf.lfHeight * zoom);
        auto newFont = CreateFontIndirect(&lf);
        ::SendMessage(hWnd, WM_SETFONT, (WPARAM)newFont, TRUE);
    }

    return TRUE;
}

#ifndef _DLL
// custom macro, adjusted from the MFC macro IMPLEMENT_DYNAMIC to make it work for templated
// base classes.
#define _RUNTIME_CLASS_T(class_name, T1) ((CRuntimeClass*)(&class_name<class T1>::class##class_name))
#define RUNTIME_CLASS_T(class_name, T1) _RUNTIME_CLASS_T(class_name, T1)

#define IMPLEMENT_RUNTIMECLASS_T(class_name, T1, base_class_name, wSchema, pfnNew, class_init) \
    AFX_COMDAT const CRuntimeClass class_name::class##class_name = { \
        #class_name, sizeof(class class_name), wSchema, pfnNew, \
            RUNTIME_CLASS_T(base_class_name, T1), NULL, class_init }; \
    CRuntimeClass* class_name::GetRuntimeClass() const \
        { return RUNTIME_CLASS(class_name); }

#define IMPLEMENT_DYNAMIC_T(class_name, T1, base_class_name) \
    IMPLEMENT_RUNTIMECLASS_T(class_name, T1, base_class_name, 0xFFFF, NULL, NULL)
#endif


const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");


BEGIN_TEMPLATE_MESSAGE_MAP(CStandAloneDialogTmpl, BaseType, BaseType)
    ON_WM_ERASEBKGND()
    ON_WM_SYSCOLORCHANGE()
    ON_WM_QUERYDRAGICON()
    ON_WM_PAINT()
    ON_WM_NCHITTEST()
    ON_WM_DWMCOMPOSITIONCHANGED()
    ON_REGISTERED_MESSAGE( TaskBarButtonCreated, OnTaskbarButtonCreated )
    ON_MESSAGE(WM_DPICHANGED, OnDPIChanged)
    END_MESSAGE_MAP()

#ifndef _DLL
// Can't find a way to make the IMPLEMENT_DYNAMIC macro work for templated classes
// (see above for templated base classes). So we do it manually here for all the templated classes
// we use:
// CStandAloneDialogTmpl<CDialog>               used also as the base class of CStandAloneDialog
// CStandAloneDialogTmpl<CResizableDialog>      used also as the base class of CResizableStandAloneDialog
// CStandAloneDialogTmpl<CStateDialog>          used also as the base class of CStateStandAloneDialog
__declspec(selectany) const CRuntimeClass CStandAloneDialogTmpl<CDialog>::classCStandAloneDialogTmpl =
{
"CStandAloneDialogTmpl", sizeof(class CStandAloneDialogTmpl), 0xFFFF, 0,
((CRuntimeClass*)(&CDialog::classCDialog)), 0, 0
};

CRuntimeClass* CStandAloneDialogTmpl<CDialog>::GetRuntimeClass() const
{
    return ((CRuntimeClass*)(&CStandAloneDialogTmpl<CDialog>::classCStandAloneDialogTmpl));
}

__declspec(selectany) const CRuntimeClass CStandAloneDialogTmpl<CResizableDialog>::classCStandAloneDialogTmpl =
{
    "CStandAloneDialogTmpl", sizeof(class CStandAloneDialogTmpl), 0xFFFF, 0,
    ((CRuntimeClass*)(&CResizableDialog::classCResizableDialog)), 0, 0
};
CRuntimeClass* CStandAloneDialogTmpl<CResizableDialog>::GetRuntimeClass() const
{
    return ((CRuntimeClass*)(&CStandAloneDialogTmpl<CResizableDialog>::classCStandAloneDialogTmpl));
}


__declspec(selectany) const CRuntimeClass CStandAloneDialogTmpl<CStateDialog>::classCStandAloneDialogTmpl =
{
    "CStandAloneDialogTmpl", sizeof(class CStandAloneDialogTmpl), 0xFFFF, 0,
    ((CRuntimeClass*)(&CStateDialog::classCStateDialog)), 0, 0
};

CRuntimeClass* CStandAloneDialogTmpl<CStateDialog>::GetRuntimeClass() const
{
    return ((CRuntimeClass*)(&CStandAloneDialogTmpl<CStateDialog>::classCStandAloneDialogTmpl));
}
#endif

#ifndef _DLL
IMPLEMENT_DYNAMIC_T(CStandAloneDialog, CDialog, CStandAloneDialogTmpl)
#else
IMPLEMENT_DYNAMIC(CStandAloneDialog, CStandAloneDialogTmpl<CDialog>)
#endif
CStandAloneDialog::CStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
: CStandAloneDialogTmpl<CDialog>(nIDTemplate, pParentWnd)
{
}
BEGIN_MESSAGE_MAP(CStandAloneDialog, CStandAloneDialogTmpl<CDialog>)
END_MESSAGE_MAP()

#ifndef _DLL
IMPLEMENT_DYNAMIC_T(CStateStandAloneDialog, CStateDialog, CStandAloneDialogTmpl)
#else
IMPLEMENT_DYNAMIC(CStateStandAloneDialog, CStandAloneDialogTmpl<CStateDialog>)
#endif
CStateStandAloneDialog::CStateStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
: CStandAloneDialogTmpl<CStateDialog>(nIDTemplate, pParentWnd)
{
}
BEGIN_MESSAGE_MAP(CStateStandAloneDialog, CStandAloneDialogTmpl<CStateDialog>)
END_MESSAGE_MAP()


#ifndef _DLL
IMPLEMENT_DYNAMIC_T(CResizableStandAloneDialog, CResizableDialog, CStandAloneDialogTmpl)
#else
IMPLEMENT_DYNAMIC(CResizableStandAloneDialog, CStandAloneDialogTmpl<CResizableDialog>)
#endif
CResizableStandAloneDialog::CResizableStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
    : CStandAloneDialogTmpl<CResizableDialog>(nIDTemplate, pParentWnd)
    , m_bVertical(false)
    , m_bHorizontal(false)
    , m_stickySize(CRegDWORD(L"Software\\TortoiseSVN\\DlgStickySize", 3))
{
}

BEGIN_MESSAGE_MAP(CResizableStandAloneDialog, CStandAloneDialogTmpl<CResizableDialog>)
    ON_WM_SIZING()
    ON_WM_MOVING()
    ON_WM_NCMBUTTONUP()
    ON_WM_NCRBUTTONUP()
    ON_WM_NCHITTEST()
END_MESSAGE_MAP()

void CResizableStandAloneDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
    m_bVertical = m_bVertical && (fwSide == WMSZ_LEFT || fwSide == WMSZ_RIGHT);
    m_bHorizontal = m_bHorizontal && (fwSide == WMSZ_TOP || fwSide == WMSZ_BOTTOM);
    if (m_nResizeBlock & DIALOG_BLOCKVERTICAL)
    {
        // don't allow the dialog to be changed in height
        switch (fwSide)
        {
            case WMSZ_BOTTOM:
            case WMSZ_BOTTOMLEFT:
            case WMSZ_BOTTOMRIGHT:
                pRect->bottom = pRect->top + m_height;
                break;
            case WMSZ_TOP:
            case WMSZ_TOPLEFT:
            case WMSZ_TOPRIGHT:
                pRect->top = pRect->bottom - m_height;
                break;
        }
    }
    if (m_nResizeBlock & DIALOG_BLOCKHORIZONTAL)
    {
        // don't allow the dialog to be changed in width
        switch (fwSide)
        {
            case WMSZ_RIGHT:
            case WMSZ_TOPRIGHT:
            case WMSZ_BOTTOMRIGHT:
                pRect->right = pRect->left + m_width;
                break;
            case WMSZ_LEFT:
            case WMSZ_TOPLEFT:
            case WMSZ_BOTTOMLEFT:
                pRect->left = pRect->right - m_width;
                break;
        }
    }
    CStandAloneDialogTmpl<CResizableDialog>::OnSizing(fwSide, pRect);
}

void CResizableStandAloneDialog::OnMoving(UINT fwSide, LPRECT pRect)
{
    m_bVertical = m_bHorizontal = false;
    if (pRect)
    {
        HMONITOR hMonitor = MonitorFromRect(pRect, MONITOR_DEFAULTTONEAREST);
        if (hMonitor)
        {
            MONITORINFO minfo = { 0 };
            minfo.cbSize = sizeof(minfo);
            if (GetMonitorInfo(hMonitor, &minfo))
            {
                int width = pRect->right - pRect->left;
                int heigth = pRect->bottom - pRect->top;
                if (abs(pRect->left - minfo.rcWork.left) < m_stickySize)
                {
                    pRect->left = minfo.rcWork.left;
                    pRect->right = pRect->left + width;
                }
                if (abs(pRect->right - minfo.rcWork.right) < m_stickySize)
                {
                    pRect->right = minfo.rcWork.right;
                    pRect->left = pRect->right - width;
                }
                if (abs(pRect->top - minfo.rcWork.top) < m_stickySize)
                {
                    pRect->top = minfo.rcWork.top;
                    pRect->bottom = pRect->top + heigth;
                }
                if (abs(pRect->bottom - minfo.rcWork.bottom) < m_stickySize)
                {
                    pRect->bottom = minfo.rcWork.bottom;
                    pRect->top = pRect->bottom - heigth;
                }
            }
        }
    }

    CStandAloneDialogTmpl<CResizableDialog>::OnMoving(fwSide, pRect);
}

void CResizableStandAloneDialog::OnNcMButtonUp(UINT nHitTest, CPoint point)
{
    WINDOWPLACEMENT windowPlacement;
    if ((nHitTest == HTMAXBUTTON) && GetWindowPlacement(&windowPlacement) && windowPlacement.showCmd == SW_SHOWNORMAL)
    {
        CRect rcWindowRect;
        GetWindowRect(&rcWindowRect);

        MONITORINFO mi = {0};
        mi.cbSize = sizeof(MONITORINFO);

        if (m_bVertical)
        {
            rcWindowRect.top = m_rcOrgWindowRect.top;
            rcWindowRect.bottom = m_rcOrgWindowRect.bottom;
        }
        else if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi))
        {
            m_rcOrgWindowRect.top = rcWindowRect.top;
            m_rcOrgWindowRect.bottom = rcWindowRect.bottom;
            rcWindowRect.top = mi.rcWork.top;
            rcWindowRect.bottom = mi.rcWork.bottom;
        }
        m_bVertical = !m_bVertical;
        //m_bHorizontal = m_bHorizontal;
        MoveWindow(&rcWindowRect);
    }
    CStandAloneDialogTmpl<CResizableDialog>::OnNcMButtonUp(nHitTest, point);
}

void CResizableStandAloneDialog::OnNcRButtonUp(UINT nHitTest, CPoint point)
{
    WINDOWPLACEMENT windowPlacement;
    if ((nHitTest == HTMAXBUTTON) && GetWindowPlacement(&windowPlacement) && windowPlacement.showCmd == SW_SHOWNORMAL)
    {
        CRect rcWindowRect;
        GetWindowRect(&rcWindowRect);

        MONITORINFO mi = {0};
        mi.cbSize = sizeof(MONITORINFO);

        if (m_bHorizontal)
        {
            rcWindowRect.left = m_rcOrgWindowRect.left;
            rcWindowRect.right = m_rcOrgWindowRect.right;
        }
        else if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi))
        {
            m_rcOrgWindowRect.left = rcWindowRect.left;
            m_rcOrgWindowRect.right = rcWindowRect.right;
            rcWindowRect.left = mi.rcWork.left;
            rcWindowRect.right = mi.rcWork.right;
        }
        //m_bVertical = m_bVertical;
        m_bHorizontal = !m_bHorizontal;
        MoveWindow(&rcWindowRect);
        // WORKAROUND
        // for some reasons, when the window is resized horizontally, its menu size is not get adjusted.
        // so, we force it to happen.
        SetMenu(GetMenu());
    }
    CStandAloneDialogTmpl<CResizableDialog>::OnNcRButtonUp(nHitTest, point);
}

void CResizableStandAloneDialog::OnCantStartThread()
{
    ::MessageBox(this->m_hWnd, (LPCTSTR)CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)), (LPCTSTR)CString(MAKEINTRESOURCE(IDS_APPNAME)), MB_OK | MB_ICONERROR);
}

bool CResizableStandAloneDialog::OnEnterPressed()
{
    if (GetAsyncKeyState(VK_CONTROL)&0x8000)
    {
        CWnd * pOkBtn = GetDlgItem(IDOK);
#ifdef ID_OK
        if (pOkBtn == NULL)
            pOkBtn = GetDlgItem(ID_OK);
#endif
        if ( pOkBtn && pOkBtn->IsWindowEnabled() )
        {
            if (DWORD(CRegStdDWORD(L"Software\\TortoiseSVN\\CtrlEnter", TRUE)))
                PostMessage(WM_COMMAND, IDOK);
        }
        return true;
    }
    return false;
}

template<typename BaseType>
void CStandAloneDialogTmpl<BaseType>::SetTheme(bool bDark)
{
    if (bDark)
    {
        DarkModeHelper::Instance().AllowDarkModeForApp(TRUE);
        DarkModeHelper::Instance().AllowDarkModeForWindow(GetSafeHwnd(), TRUE);
        DarkModeHelper::Instance().AllowDarkModeForWindow(m_tooltips.GetSafeHwnd(), TRUE);
        SetWindowTheme(m_tooltips.GetSafeHwnd(), L"Explorer", nullptr);
        SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, (LONG_PTR)GetStockObject(BLACK_BRUSH));
        BOOL darkFlag = TRUE;
        DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data = { DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag) };
        DarkModeHelper::Instance().SetWindowCompositionAttribute(GetSafeHwnd(), &data);
        DarkModeHelper::Instance().FlushMenuThemes();
        DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
        BOOL dark = TRUE;
        DwmSetWindowAttribute(GetSafeHwnd(), 19, &dark, sizeof(dark));
    }
    else
    {
        DarkModeHelper::Instance().AllowDarkModeForWindow(GetSafeHwnd(), FALSE);
        DarkModeHelper::Instance().AllowDarkModeForWindow(m_tooltips.GetSafeHwnd(), FALSE);
        SetWindowTheme(m_tooltips.GetSafeHwnd(), L"Explorer", nullptr);
        BOOL darkFlag = FALSE;
        DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data = { DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag) };
        DarkModeHelper::Instance().SetWindowCompositionAttribute(GetSafeHwnd(), &data);
        DarkModeHelper::Instance().FlushMenuThemes();
        DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
        DarkModeHelper::Instance().AllowDarkModeForApp(FALSE);
        SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, (LONG_PTR)GetSysColorBrush(COLOR_3DFACE));
    }
    CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), bDark);
    ::RedrawWindow(GetSafeHwnd(), nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

template <typename BaseType>
LRESULT CStandAloneDialogTmpl<BaseType>::OnDPIChanged(WPARAM wParam, LPARAM lParam)
{
    CDPIAware::Instance().Invalidate();
    const RECT* rect = reinterpret_cast<RECT*>(lParam);

    auto newDPI = HIWORD(wParam);
    if (m_dpi == 0)
    {
        m_dpi = newDPI;
        return 0;
    }
    double zoom = (((double)newDPI) / (((double)m_dpi) / 100.0)) / 100.0;

    DPIAdjustData data = { GetSafeHwnd(), zoom };
    if constexpr (std::is_same_v<BaseType, CResizableDialog>)
    {
        auto anchors = GetAllAnchors();
        RemoveAllAnchors();

        auto minTrackSize = GetMinTrackSize();
        minTrackSize.cx = (LONG)(minTrackSize.cx * zoom);
        minTrackSize.cy = (LONG)(minTrackSize.cy * zoom);
        SetMinTrackSize(minTrackSize);

        SetWindowPos(NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        ::EnumChildWindows(GetSafeHwnd(), DPIAdjustChildren, (LPARAM)&data);

        AddAllAnchors(anchors);
    }
    else
    {
        SetWindowPos(NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        ::EnumChildWindows(GetSafeHwnd(), DPIAdjustChildren, (LPARAM)&data);
    }

    m_dpi = newDPI;
    m_height = rect->bottom - rect->top;
    m_width = rect->right - rect->left;

    ::RedrawWindow(GetSafeHwnd(), nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
    return 1; // let MFC handle this message as well
}


IMPLEMENT_DYNAMIC(CStateDialog, CDialog)

BEGIN_MESSAGE_MAP(CStateDialog, CDialog)
    ON_WM_DESTROY()
END_MESSAGE_MAP()


LRESULT CResizableStandAloneDialog::OnNcHitTest(CPoint point)
{
    if (m_nResizeBlock == 0)
        return CStandAloneDialogTmpl::OnNcHitTest(point);

    // when resizing is blocked in a direction, don't return
    // a hit code that would allow that resizing.
    // Using the OnNcHitTest handler tells Windows to
    // not show a resizing mouse pointer if it's not possible
    auto ht = CStandAloneDialogTmpl::OnNcHitTest(point);
    if (m_nResizeBlock & DIALOG_BLOCKVERTICAL)
    {
        switch (ht)
        {
            case HTBOTTOMLEFT:  ht = HTLEFT;   break;
            case HTBOTTOMRIGHT: ht = HTRIGHT;  break;
            case HTTOPLEFT:     ht = HTLEFT;   break;
            case HTTOPRIGHT:    ht = HTRIGHT;  break;
            case HTTOP:         ht = HTBORDER; break;
            case HTBOTTOM:      ht = HTBORDER; break;
        }
    }
    if (m_nResizeBlock & DIALOG_BLOCKHORIZONTAL)
    {
        switch (ht)
        {
            case HTBOTTOMLEFT:  ht = HTBOTTOM; break;
            case HTBOTTOMRIGHT: ht = HTBOTTOM; break;
            case HTTOPLEFT:     ht = HTTOP;    break;
            case HTTOPRIGHT:    ht = HTTOP;    break;
            case HTLEFT:        ht = HTBORDER; break;
            case HTRIGHT:       ht = HTBORDER; break;
        }
    }

    return ht;
}

// overloaded method, but since this dialog class is for non-resizable dialogs,
// the bHorzResize and bVertResize params are ignored and passed as false
// to the base method.

void CStateDialog::EnableSaveRestore(LPCWSTR pszSection, bool bRectOnly, BOOL bHorzResize, BOOL bVertResize)
{
    UNREFERENCED_PARAMETER(bHorzResize);
    UNREFERENCED_PARAMETER(bVertResize);
    m_sSection = CString(pszSection) + GetMonitorSetupHash().c_str();

    m_bEnableSaveRestore = true;
    m_bRectOnly          = bRectOnly;

    // restore immediately
    LoadWindowRect(m_sSection, bRectOnly, false, false);
}
