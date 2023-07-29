﻿// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2012, 2014-2015, 2020-2021 - TortoiseSVN

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
#include "MainFrm.h"
#include "LocatorBar.h"
#include "LeftView.h"
#include "RightView.h"
#include "BottomView.h"
#include "DiffColors.h"
#include "AppUtils.h"
#include "Theme.h"

IMPLEMENT_DYNAMIC(CLocatorBar, CPaneDialog)
CLocatorBar::CLocatorBar()
    : CPaneDialog()
    , m_pCacheBitmap(nullptr)
    , m_minWidth(0)
    , m_nLines(-1)
    , m_regUseFishEye(L"Software\\TortoiseMerge\\UseFishEye", TRUE)
    , m_bDark(false)
    , m_themeCallbackId(0)
    , m_pMainFrm(nullptr)
{
    m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
        [this]() {
            SetTheme(CTheme::Instance().IsDarkTheme());
        });
    SetTheme(CTheme::Instance().IsDarkTheme());
}

CLocatorBar::~CLocatorBar()
{
    CTheme::Instance().RemoveRegisteredCallback(m_themeCallbackId);
    if (m_pCacheBitmap)
    {
        m_pCacheBitmap->DeleteObject();
        delete m_pCacheBitmap;
        m_pCacheBitmap = nullptr;
    }
}

BEGIN_MESSAGE_MAP(CLocatorBar, CPaneDialog)
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

void CLocatorBar::DocumentUpdated()
{
    m_pMainFrm = static_cast<CMainFrame*>(this->GetParentFrame());
    if (!m_pMainFrm)
        return;

    m_nLines = 0;
    DocumentUpdated(m_pMainFrm->m_pwndLeftView, m_arLeftIdent, m_arLeftState);
    DocumentUpdated(m_pMainFrm->m_pwndRightView, m_arRightIdent, m_arRightState);
    DocumentUpdated(m_pMainFrm->m_pwndBottomView, m_arBottomIdent, m_arBottomState);

    if (m_pMainFrm->m_pwndLeftView)
    {
        m_nLines = m_pMainFrm->m_pwndLeftView->GetLineCount();
        if (m_pMainFrm->m_pwndRightView)
        {
            m_nLines = std::max<int>(m_nLines, m_pMainFrm->m_pwndRightView->GetLineCount());
            if (m_pMainFrm->m_pwndBottomView)
            {
                m_nLines = std::max<int>(m_nLines, m_pMainFrm->m_pwndBottomView->GetLineCount());
            }
        }
    }
    Invalidate();
}

void CLocatorBar::DocumentUpdated(CBaseView* view, CDWordArray& indents, CDWordArray& states)
{
    indents.RemoveAll();
    states.RemoveAll();
    CViewData* viewData = view->m_pViewData;
    if (viewData == nullptr)
        return;

    long       identCount  = 1;
    const int  linesInView = view->GetLineCount();
    DiffStates state       = DIFFSTATE_UNKNOWN;
    if (linesInView)
        state = viewData->GetState(0);
    for (int i = 1; i < linesInView; i++)
    {
        const DiffStates lineState = viewData->GetState(view->GetViewLineForScreen(i));
        if (state == lineState)
        {
            identCount++;
        }
        else
        {
            indents.Add(identCount);
            states.Add(state);
            state      = lineState;
            identCount = 1;
        }
    }
    indents.Add(identCount);
    states.Add(state);
}

void CLocatorBar::SetTheme(bool bDark)
{
    m_bDark = bDark;
    if (IsWindow(GetSafeHwnd()))
        Invalidate();
}

CSize CLocatorBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
    auto s = __super::CalcFixedLayout(bStretch, bHorz);
    s.cx   = m_minWidth;
    return s;
}

void CLocatorBar::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    CRect    rect;
    GetClientRect(rect);
    const long height      = rect.Height();
    const long width       = rect.Width();
    long       nTopLine    = 0;
    long       nBottomLine = 0;
    if ((m_pMainFrm) && (m_pMainFrm->m_pwndLeftView))
    {
        nTopLine    = m_pMainFrm->m_pwndLeftView->m_nTopLine;
        nBottomLine = nTopLine + m_pMainFrm->m_pwndLeftView->GetScreenLines();
    }
    CDC cacheDC;
    VERIFY(cacheDC.CreateCompatibleDC(&dc));

    if (!m_pCacheBitmap)
    {
        m_pCacheBitmap = new CBitmap;
        VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(&dc, width, height));
    }
    CBitmap* pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);

    COLORREF color, color2;
    CDiffColors::GetInstance().GetColors(DIFFSTATE_UNKNOWN, CTheme::Instance().IsDarkTheme() || CTheme::Instance().IsHighContrastModeDark(), color, color2);
    cacheDC.FillSolidRect(rect, color);

    if (m_nLines)
    {
        cacheDC.FillSolidRect(rect.left, height * nTopLine / m_nLines,
                              width, (height * nBottomLine / m_nLines) - (height * nTopLine / m_nLines), CTheme::Instance().GetThemeColor(RGB(180, 180, 255), true));

        if (m_pMainFrm)
        {
            PaintView(cacheDC, m_pMainFrm->m_pwndLeftView, m_arLeftIdent, m_arLeftState, rect, 0);
            PaintView(cacheDC, m_pMainFrm->m_pwndRightView, m_arRightIdent, m_arRightState, rect, 2);
            PaintView(cacheDC, m_pMainFrm->m_pwndBottomView, m_arBottomIdent, m_arBottomState, rect, 1);
        }
    }
    auto vertLineClr = CTheme::Instance().GetThemeColor(RGB(0, 0, 0), true);
    if (m_nLines == 0)
        m_nLines = 1;
    cacheDC.FillSolidRect(rect.left, height * nTopLine / m_nLines,
                          width, 2, vertLineClr);
    cacheDC.FillSolidRect(rect.left, height * nBottomLine / m_nLines,
                          width, 2, vertLineClr);
    //draw two vertical lines, so there are three rows visible indicating the three panes
    cacheDC.FillSolidRect(rect.left + (width / 3), rect.top, 1, height, vertLineClr);
    cacheDC.FillSolidRect(rect.left + (width * 2 / 3), rect.top, 1, height, vertLineClr);

    // draw the fish eye
    DWORD pos        = GetMessagePos();
    CRect screenRect = rect;
    ClientToScreen(screenRect);
    POINT pt;
    pt.x = GET_X_LPARAM(pos);
    pt.y = GET_Y_LPARAM(pos);

    if ((screenRect.PtInRect(pt)) && static_cast<DWORD>(m_regUseFishEye))
        DrawFishEye(cacheDC, rect);

    VERIFY(dc.BitBlt(rect.left, rect.top, width, height, &cacheDC, 0, 0, SRCCOPY));

    cacheDC.SelectObject(pOldBitmap);
    cacheDC.DeleteDC();
}

void CLocatorBar::OnSize(UINT nType, int cx, int cy)
{
    CPaneDialog::OnSize(nType, cx, cy);

    if (m_pCacheBitmap)
    {
        m_pCacheBitmap->DeleteObject();
        delete m_pCacheBitmap;
        m_pCacheBitmap = nullptr;
    }
    Invalidate();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
BOOL CLocatorBar::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE;
}

void CLocatorBar::OnLButtonDown(UINT nFlags, CPoint point)
{
    ScrollOnMouseMove(point);
    Invalidate();
    CPaneDialog::OnLButtonDown(nFlags, point);
}

void CLocatorBar::OnMouseMove(UINT nFlags, CPoint point)
{
    m_mousePos = point;

    if (nFlags & MK_LBUTTON)
    {
        SetCapture();
        ScrollOnMouseMove(point);
    }

    TRACKMOUSEEVENT tme{};
    tme.cbSize    = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags   = TME_LEAVE;
    tme.hwndTrack = m_hWnd;
    TrackMouseEvent(&tme);

    Invalidate();
}

LRESULT CLocatorBar::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    Invalidate();
    return 0;
}

void CLocatorBar::OnLButtonUp(UINT nFlags, CPoint point)
{
    ReleaseCapture();
    Invalidate();

    CPaneDialog::OnLButtonUp(nFlags, point);
}

void CLocatorBar::ScrollOnMouseMove(const CPoint& point) const
{
    if (m_pMainFrm == nullptr)
        return;

    CRect rect;
    GetClientRect(rect);

    int nLine = point.y * m_nLines / rect.Height();
    if (nLine < 0)
        nLine = 0;

    ScrollViewToLine(m_pMainFrm->m_pwndBottomView, nLine);
    ScrollViewToLine(m_pMainFrm->m_pwndLeftView, nLine);
    ScrollViewToLine(m_pMainFrm->m_pwndRightView, nLine);
}

void CLocatorBar::ScrollViewToLine(CBaseView* view, int nLine)
{
    if (view != nullptr)
        view->GoToLine(nLine, FALSE);
}

void CLocatorBar::PaintView(CDC& cacheDC, CBaseView* view, CDWordArray& indents,
                            CDWordArray& states, const CRect& rect, int stripeIndex) const
{
    if (!view->IsWindowVisible())
        return;

    const long height    = rect.Height();
    const long width     = rect.Width();
    const long barWidth  = (width / 3);
    long       linecount = 0;
    for (long i = 0; i < indents.GetCount(); i++)
    {
        COLORREF    color{}, color2{};
        const long  identCount = indents.GetAt(i);
        const DWORD state      = states.GetAt(i);
        CDiffColors::GetInstance().GetColors(static_cast<DiffStates>(state), CTheme::Instance().IsDarkTheme() || CTheme::Instance().IsHighContrastModeDark(), color, color2);
        if (static_cast<DiffStates>(state) != DIFFSTATE_NORMAL)
        {
            cacheDC.FillSolidRect(rect.left + (width * stripeIndex / 3), height * linecount / m_nLines,
                                  barWidth, max(static_cast<int>(height * identCount / m_nLines), 1), color);
        }
        linecount += identCount;
    }
    if (view->GetMarkedWord()[0])
    {
        COLORREF color, color2;
        CDiffColors::GetInstance().GetColors(DIFFSTATE_NORMAL, CTheme::Instance().IsDarkTheme() || CTheme::Instance().IsHighContrastModeDark(), color, color2);
        color = CAppUtils::IntenseColor(200, color);
        for (size_t i = 0; i < view->m_arMarkedWordLines.size(); ++i)
        {
            if (view->m_arMarkedWordLines[i])
            {
                cacheDC.FillSolidRect(rect.left + (width * stripeIndex / 3), static_cast<int>(height * i / m_nLines),
                                      barWidth, max(static_cast<int>(height / m_nLines), 2), color);
            }
        }
    }
    if (view->GetFindString()[0])
    {
        COLORREF color, color2;
        CDiffColors::GetInstance().GetColors(DIFFSTATE_NORMAL, CTheme::Instance().IsDarkTheme() || CTheme::Instance().IsHighContrastModeDark(), color, color2);
        color = CAppUtils::IntenseColor(30, color);
        for (size_t i = 0; i < view->m_arFindStringLines.size(); ++i)
        {
            if (view->m_arFindStringLines[i])
            {
                cacheDC.FillSolidRect(rect.left + (width * stripeIndex / 3), static_cast<int>(height * i / m_nLines),
                                      barWidth, max(static_cast<int>(height / m_nLines), 2), color);
            }
        }
    }
}

void CLocatorBar::DrawFishEye(CDC& cacheDC, const CRect& rect) const
{
    const long height     = rect.Height();
    const long width      = rect.Width();
    const int  fishStart  = m_mousePos.y - height / 20;
    const int  fishHeight = height / 10;
    auto       clr        = CTheme::Instance().GetThemeColor(RGB(0, 0, 100), true);
    cacheDC.FillSolidRect(rect.left, fishStart - 1, width, 1, clr);
    cacheDC.FillSolidRect(rect.left, fishStart + fishHeight + 1, width, 1, clr);
    VERIFY(cacheDC.StretchBlt(rect.left, fishStart, width, fishHeight,
                              &cacheDC, 0, fishStart + (3 * fishHeight / 8), width, fishHeight / 4, SRCCOPY));
    // draw the magnified area a little darker, so the
    // user has a clear indication of the magnifier
    for (int i = rect.left; i < (width - rect.left); i++)
    {
        for (int j = fishStart; j < fishStart + fishHeight; j++)
        {
            const COLORREF color = cacheDC.GetPixel(i, j);
            if (m_bDark)
            {
                int r = min(GetRValue(color) + 40, 255);
                int g = min(GetGValue(color) + 40, 255);
                int b = min(GetBValue(color) + 40, 255);
                cacheDC.SetPixel(i, j, RGB(r, g, b));
            }
            else
            {
                int r = max(GetRValue(color) - 20, 0);
                int g = max(GetGValue(color) - 20, 0);
                int b = max(GetBValue(color) - 20, 0);
                cacheDC.SetPixel(i, j, RGB(r, g, b));
            }
        }
    }
}
