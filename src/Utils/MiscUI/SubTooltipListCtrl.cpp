// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2011-2012, 2014, 2017, 2021 - TortoiseSVN

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
#include "SubTooltipListCtrl.h"
#include <strsafe.h>

BEGIN_MESSAGE_MAP(CSubTooltipListCtrl, CListCtrl)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, &CSubTooltipListCtrl::OnToolTipText)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, &CSubTooltipListCtrl::OnToolTipText)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CSubTooltipListCtrl, CListCtrl)

CSubTooltipListCtrl::CSubTooltipListCtrl()
    : CListCtrl()
    , pProvider(nullptr)
{
}

CSubTooltipListCtrl::~CSubTooltipListCtrl()
{
}

INT_PTR CSubTooltipListCtrl::OnToolHitTest(CPoint point, TOOLINFO* pTi) const
{
    if (pProvider == nullptr)
        return -1;

    LVHITTESTINFO lvhitTestInfo;

    lvhitTestInfo.pt = point;

    int nItem = ListView_SubItemHitTest(
        this->m_hWnd,
        &lvhitTestInfo);
    int nSubItem = lvhitTestInfo.iSubItem;

    UINT nFlags = lvhitTestInfo.flags;

    //nFlags is 0 if the SubItemHitTest fails
    //Therefore, 0 & <anything> will equal false
    if (nFlags & LVHT_ONITEM)
    {
        //If it did fall on a list item,
        //and it was also hit one of the
        //item specific sub-areas we wish to show tool tips for

        //Get the client (area occupied by this control
        RECT rcClient;
        GetClientRect(&rcClient);

        //Fill in the TOOLINFO structure
        pTi->hwnd     = m_hWnd;
        pTi->uId      = static_cast<UINT_PTR>((static_cast<UINT_PTR>(nItem) << 10) + (static_cast<UINT_PTR>(nSubItem) & 0x3ff) + 1);
        pTi->lpszText = LPSTR_TEXTCALLBACK;
        pTi->rect     = rcClient;

        return pTi->uId; //By returning a unique value per listItem,
        //we ensure that when the mouse moves over another list item,
        //the tooltip will change
    }
    else
    {
        //Otherwise, we aren't interested, so let the message propagate
        return -1;
    }
}

BOOL CSubTooltipListCtrl::OnToolTipText(UINT /*id*/, NMHDR* pNMHDR, LRESULT* pResult)
{
    if (pProvider == nullptr)
        return FALSE;

    TOOLTIPTEXTA* pTTTA = reinterpret_cast<NMTTDISPINFOA*>(pNMHDR);
    TOOLTIPTEXTW* pTTTW = reinterpret_cast<NMTTDISPINFOW*>(pNMHDR);

    // Ignore messages from the built in tooltip, we are processing them internally
    if ((pNMHDR->idFrom == reinterpret_cast<UINT_PTR>(m_hWnd)) &&
        (((pNMHDR->code == TTN_NEEDTEXTA) && (pTTTA->uFlags & TTF_IDISHWND)) ||
         ((pNMHDR->code == TTN_NEEDTEXTW) && (pTTTW->uFlags & TTF_IDISHWND))))
    {
        return FALSE;
    }

    *pResult = 0;

    //Get the mouse position
    const MSG* pMessage = GetCurrentMessage();

    CPoint pt;
    pt = pMessage->pt;
    ScreenToClient(&pt);

    // Check if the point falls onto a list item
    LVHITTESTINFO lvhitTestInfo;
    lvhitTestInfo.pt = pt;

    int nItem = SubItemHitTest(&lvhitTestInfo);

    if (lvhitTestInfo.flags & LVHT_ONITEM)
    {
        // we want multiline tooltips
        ::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, SHRT_MAX);
        CString strTipText = pProvider->GetToolTipText(nItem, lvhitTestInfo.iSubItem);

        //Deal with UNICODE
#ifndef _UNICODE
        if (pNMHDR->code == TTN_NEEDTEXTA)
            StringCchCopy(pTTTA->szText, _countof(pTTTA->szText), strTipText);
        else
            _mbstowcsz(pTTTW->szText, strTipText, 80);
#else
        if (pNMHDR->code == TTN_NEEDTEXTA)
            _wcstombsz(pTTTA->szText, strTipText, 80);
        else
            StringCchCopy(pTTTW->szText, _countof(pTTTW->szText), strTipText);
#endif
        return TRUE; //We found a tool tip,
        //tell the framework this message has been handled
    }

    return FALSE; //We didn't handle the message,
    //let the framework continue propagating the message
}
