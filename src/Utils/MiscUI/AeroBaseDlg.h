// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010, 2013-2014 - TortoiseSVN

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

#include "registry.h"
#include "AeroGlass.h"
#include "AeroControls.h"

/**
 * \ingroup TortoiseMerge
 *
 * A template which can be used as the base-class of dialogs which form the main dialog
 * of a 'dialog-style application'
 * Just provides the boiler-plate code for dealing with application icons
 */
template <typename BaseType> class CStandAloneDialogTmpl : public BaseType
{
protected:
    CStandAloneDialogTmpl(UINT nIDTemplate, CWnd* pParentWnd = NULL) : BaseType(nIDTemplate, pParentWnd)
    {
        m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
        m_regEnableDWMFrame = CRegDWORD(L"Software\\TortoiseSVN\\EnableDWMFrame", TRUE);
        m_margins.cxLeftWidth = 0;
        m_margins.cyTopHeight = 0;
        m_margins.cxRightWidth = 0;
        m_margins.cyBottomHeight = 0;
    }
    virtual BOOL OnInitDialog()
    {
        m_Dwm.Initialize();
        BaseType::OnInitDialog();

        // Set the icon for this dialog.  The framework does this automatically
        //  when the application's main window is not a dialog
        SetIcon(m_hIcon, TRUE);         // Set big icon
        SetIcon(m_hIcon, FALSE);        // Set small icon

        return FALSE;
    }

    afx_msg void OnPaint()
    {
        if (IsIconic())
        {
            CPaintDC dc(this); // device context for painting

            SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

            // Center icon in client rectangle
            int cxIcon = GetSystemMetrics(SM_CXICON);
            int cyIcon = GetSystemMetrics(SM_CYICON);
            CRect rect;
            GetClientRect(&rect);
            int x = (rect.Width() - cxIcon + 1) / 2;
            int y = (rect.Height() - cyIcon + 1) / 2;

            // Draw the icon
            dc.DrawIcon(x, y, m_hIcon);
        }
        else
        {
            BaseType::OnPaint();
        }
    }

    BOOL OnEraseBkgnd(CDC*  pDC)
    {
        BaseType::OnEraseBkgnd(pDC);
        if ((m_Dwm.IsDwmCompositionEnabled())&&((DWORD)m_regEnableDWMFrame))
        {
            // draw the frame margins in black
            RECT rc;
            GetClientRect(&rc);
            if (m_margins.cxLeftWidth < 0)
                pDC->FillSolidRect(rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, RGB(0,0,0));
            else
            {
                pDC->FillSolidRect(rc.left, rc.top, m_margins.cxLeftWidth, rc.bottom-rc.top, RGB(0,0,0));
                pDC->FillSolidRect(rc.left, rc.top, rc.right-rc.left, m_margins.cyTopHeight, RGB(0,0,0));
                pDC->FillSolidRect(rc.right-m_margins.cxRightWidth, rc.top, m_margins.cxRightWidth, rc.bottom-rc.top, RGB(0,0,0));
                pDC->FillSolidRect(rc.left, rc.bottom-m_margins.cyBottomHeight, rc.right-rc.left, m_margins.cyBottomHeight, RGB(0,0,0));
            }
        }
        return TRUE;
    }

    LRESULT OnNcHitTest(CPoint pt)
    {
        if ((m_Dwm.IsDwmCompositionEnabled())&&((DWORD)m_regEnableDWMFrame))
        {
            CRect rc;
            GetClientRect(&rc);
            ClientToScreen(&rc);

            if (m_margins.cxLeftWidth < 0)
            {
                return rc.PtInRect(pt) ? HTCAPTION : BaseType::OnNcHitTest(pt);
            }
            else
            {
                CRect m = rc;
                m.DeflateRect(m_margins.cxLeftWidth, m_margins.cyTopHeight, m_margins.cxRightWidth, m_margins.cyBottomHeight);
                return (rc.PtInRect(pt) && !m.PtInRect(pt)) ? HTCAPTION : BaseType::OnNcHitTest(pt);
            }
        }
        return BaseType::OnNcHitTest(pt);
    }

    void OnCompositionChanged()
    {
        if (m_Dwm.IsDwmCompositionEnabled())
        {
            m_Dwm.DwmExtendFrameIntoClientArea(m_hWnd, &m_margins);
        }
        BaseType::OnCompositionChanged();
    }

    /**
     *
     */
    void ExtendFrameIntoClientArea(UINT leftControl, UINT topControl, UINT rightControl, UINT bottomControl)
    {
        if (!(DWORD)m_regEnableDWMFrame)
            return;
        RECT rc, rc2;
        GetWindowRect(&rc);
        GetClientRect(&rc2);
        ClientToScreen(&rc2);

        RECT rccontrol;
        if (leftControl)
        {
            HWND hw = GetDlgItem(leftControl)->GetSafeHwnd();
            if (hw == NULL)
                return;
            ::GetWindowRect(hw, &rccontrol);
            m_margins.cxLeftWidth = rccontrol.left - rc.left;
            m_margins.cxLeftWidth -= (rc2.left-rc.left);
        }
        else
            m_margins.cxLeftWidth = 0;

        if (topControl)
        {
            HWND hw = GetDlgItem(topControl)->GetSafeHwnd();
            if (hw == NULL)
                return;
            ::GetWindowRect(hw, &rccontrol);
            m_margins.cyTopHeight = rccontrol.top - rc.top;
            m_margins.cyTopHeight -= (rc2.top-rc.top);
        }
        else
            m_margins.cyTopHeight = 0;

        if (rightControl)
        {
            HWND hw = GetDlgItem(rightControl)->GetSafeHwnd();
            if (hw == NULL)
                return;
            ::GetWindowRect(hw, &rccontrol);
            m_margins.cxRightWidth = rc.right - rccontrol.right;
            m_margins.cxRightWidth -= (rc.right-rc2.right);
        }
        else
            m_margins.cxRightWidth = 0;

        if (bottomControl)
        {
            HWND hw = GetDlgItem(bottomControl)->GetSafeHwnd();
            if (hw == NULL)
                return;
            ::GetWindowRect(hw, &rccontrol);
            m_margins.cyBottomHeight = rc.bottom - rccontrol.bottom;
            m_margins.cyBottomHeight -= (rc.bottom-rc2.bottom);
        }
        else
            m_margins.cyBottomHeight = 0;

        if ((m_margins.cxLeftWidth == 0)&&
            (m_margins.cyTopHeight == 0)&&
            (m_margins.cxRightWidth == 0)&&
            (m_margins.cyBottomHeight == 0))
        {
            m_margins.cxLeftWidth = -1;
            m_margins.cyTopHeight = -1;
            m_margins.cxRightWidth = -1;
            m_margins.cyBottomHeight = -1;
        }
        if (m_Dwm.IsDwmCompositionEnabled())
        {
            m_Dwm.DwmExtendFrameIntoClientArea(m_hWnd, &m_margins);
        }
    }

    void ExtendFrameIntoClientArea(UINT bottomControl)
    {
        ExtendFrameIntoClientArea(0, 0, 0, bottomControl);
    }

    /**
     * Wrapper around the CWnd::EnableWindow() method, but
     * makes sure that a control that has the focus is not disabled
     * before the focus is passed on to the next control.
     */
    BOOL DialogEnableWindow(UINT nID, BOOL bEnable)
    {
        CWnd * pwndDlgItem = GetDlgItem(nID);
        if (pwndDlgItem == NULL)
            return FALSE;
        if (bEnable)
            return pwndDlgItem->EnableWindow(bEnable);
        if (GetFocus() == pwndDlgItem)
        {
            SendMessage(WM_NEXTDLGCTL, 0, FALSE);
        }
        return pwndDlgItem->EnableWindow(bEnable);
    }

    /**
     * Adjusts the size of a checkbox or radio button control.
     * Since we always make the size of those bigger than 'necessary'
     * for making sure that translated strings can fit in those too,
     * this method can reduce the size of those controls again to only
     * fit the text.
     */
    RECT AdjustControlSize(UINT nID)
    {
        CWnd * pwndDlgItem = GetDlgItem(nID);
        // adjust the size of the control to fit its content
        CString sControlText;
        pwndDlgItem->GetWindowText(sControlText);
        // next step: find the rectangle the control text needs to
        // be displayed

        CDC * pDC = pwndDlgItem->GetWindowDC();
        RECT controlrect;
        RECT controlrectorig;
        pwndDlgItem->GetWindowRect(&controlrect);
        ::MapWindowPoints(NULL, GetSafeHwnd(), (LPPOINT)&controlrect, 2);
        controlrectorig = controlrect;
        if (pDC)
        {
            CFont * font = pwndDlgItem->GetFont();
            CFont * pOldFont = pDC->SelectObject(font);
            if (pDC->DrawText(sControlText, -1, &controlrect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_CALCRECT))
            {
                // now we have the rectangle the control really needs
                if ((controlrectorig.right - controlrectorig.left) > (controlrect.right - controlrect.left))
                {
                    // we're dealing with radio buttons and check boxes,
                    // which means we have to add a little space for the checkbox
                    // the value of 3 pixels added here is necessary in case certain visual styles have
                    // been disabled. Without this, the width is calculated too short.
                    const int checkWidth  = GetSystemMetrics(SM_CXMENUCHECK) + 2 * GetSystemMetrics(SM_CXEDGE) + 3;
                    controlrectorig.right = controlrectorig.left + (controlrect.right - controlrect.left) + checkWidth;
                    pwndDlgItem->MoveWindow(&controlrectorig);
                }
            }
            pDC->SelectObject(pOldFont);
            ReleaseDC(pDC);
        }
        return controlrectorig;
    }

    /**
    * Adjusts the size of a static control.
    * \param nID control ID
    * \param rc the position of the control where this control shall
    *           be positioned next to on its right side.
    * \param spacing number of pixels to add to rc.right
    */
    RECT AdjustStaticSize(UINT nID, RECT rc, long spacing)
    {
        CWnd * pwndDlgItem = GetDlgItem(nID);
        // adjust the size of the control to fit its content
        CString sControlText;
        pwndDlgItem->GetWindowText(sControlText);
        // next step: find the rectangle the control text needs to
        // be displayed

        CDC * pDC = pwndDlgItem->GetWindowDC();
        RECT controlrect;
        RECT controlrectorig;
        pwndDlgItem->GetWindowRect(&controlrect);
        ::MapWindowPoints(NULL, GetSafeHwnd(), (LPPOINT)&controlrect, 2);
        controlrect.right += 200;   // in case the control needs to be bigger than it currently is (e.g., due to translations)
        controlrectorig = controlrect;

        long height = controlrectorig.bottom-controlrectorig.top;
        long width = controlrectorig.right-controlrectorig.left;
        controlrectorig.left = rc.right + spacing;
        controlrectorig.right = controlrectorig.left + width;
        controlrectorig.bottom = rc.bottom;
        controlrectorig.top = controlrectorig.bottom - height;

        if (pDC)
        {
            CFont * font = pwndDlgItem->GetFont();
            CFont * pOldFont = pDC->SelectObject(font);
            if (pDC->DrawText(sControlText, -1, &controlrect, DT_WORDBREAK | DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_CALCRECT))
            {
                // now we have the rectangle the control really needs
                controlrectorig.right = controlrectorig.left + (controlrect.right - controlrect.left);
                pwndDlgItem->MoveWindow(&controlrectorig);
            }
            pDC->SelectObject(pOldFont);
            ReleaseDC(pDC);
        }
        return controlrectorig;
    }

    /**
     * Refreshes the cursor by forcing a WM_SETCURSOR message.
     */
    void RefreshCursor()
    {
        POINT pt;
        GetCursorPos(&pt);
        SetCursorPos(pt.x, pt.y);
    }

    bool IsCursorOverWindowBorder()
    {
        RECT wrc, crc;
        this->GetWindowRect(&wrc);
        this->GetClientRect(&crc);
        ClientToScreen(&crc);
        DWORD pos = GetMessagePos();
        POINT pt;
        pt.x = GET_X_LPARAM(pos);
        pt.y = GET_Y_LPARAM(pos);
        return (PtInRect(&wrc, pt) && !PtInRect(&crc, pt));
    }
protected:
    CDwmApiImpl     m_Dwm;
    MARGINS         m_margins;
    CRegDWORD       m_regEnableDWMFrame;
    AeroControlBase m_aeroControls;

    DECLARE_MESSAGE_MAP()
private:
    HCURSOR OnQueryDragIcon()
    {
        return static_cast<HCURSOR>(m_hIcon);
    }

    HICON           m_hIcon;
};


class CStandAloneDialog : public CStandAloneDialogTmpl<CDialog>
{
public:
    CStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);
protected:
    DECLARE_MESSAGE_MAP()
private:
    DECLARE_DYNAMIC(CStandAloneDialog)
};