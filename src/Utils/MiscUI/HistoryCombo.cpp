// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "HistoryCombo.h"
#include "registry.h"

#ifdef HISTORYCOMBO_WITH_SYSIMAGELIST
#include "SysImageList.h"
#endif

#define MAX_HISTORY_ITEMS 25

CHistoryCombo::CHistoryCombo(BOOL bAllowSortStyle /*=FALSE*/ )
    : CComboBoxEx()
    , m_nMaxHistoryItems(MAX_HISTORY_ITEMS)
    , m_bAllowSortStyle(bAllowSortStyle)
    , m_bURLHistory(FALSE)
    , m_bPathHistory(FALSE)
    , m_hWndToolTip(NULL)
    , m_ttShown(FALSE)
    , m_bDyn(FALSE)
    , m_bTrim(TRUE)
{
    SecureZeroMemory(&m_ToolInfo, sizeof(m_ToolInfo));
}

CHistoryCombo::~CHistoryCombo()
{
}

BOOL CHistoryCombo::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!m_bAllowSortStyle)  //turn off CBS_SORT style
        cs.style &= ~CBS_SORT;
    cs.style |= CBS_AUTOHSCROLL;
    m_bDyn = TRUE;
    return CComboBoxEx::PreCreateWindow(cs);
}

BOOL CHistoryCombo::PreTranslateMessage(MSG* pMsg)
{
    switch (pMsg->message)
    {
    case WM_KEYDOWN:
        {
            bool bShift = !!(GetKeyState(VK_SHIFT) & 0x8000);
            int nVirtKey = (int) pMsg->wParam;

            if (nVirtKey == VK_RETURN)
                return OnReturnKeyPressed();
            else if (nVirtKey == VK_DELETE && bShift && GetDroppedState() )
            {
                RemoveSelectedItem();
                return TRUE;
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if ((pMsg->wParam & MK_LBUTTON) == 0)
            {
                CPoint pt;
                pt.x = LOWORD(pMsg->lParam);
                pt.y = HIWORD(pMsg->lParam);
                OnMouseMove((UINT)pMsg->wParam, pt);
                return TRUE;
            }
        }
        break;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        if (!GetDroppedState())
            return TRUE;
    }
    return CComboBoxEx::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CHistoryCombo, CComboBoxEx)
    ON_WM_MOUSEMOVE()
    ON_WM_TIMER()
    ON_WM_CREATE()
END_MESSAGE_MAP()

int CHistoryCombo::AddString(CString str, INT_PTR pos)
{
    if (str.IsEmpty())
        return -1;

    //truncate list to m_nMaxHistoryItems
    int nNumItems = GetCount();
    for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
    {
        DeleteItem(m_nMaxHistoryItems);
        m_arEntries.RemoveAt(m_nMaxHistoryItems);
    }

    COMBOBOXEXITEM cbei;
    SecureZeroMemory(&cbei, sizeof cbei);
    cbei.mask = CBEIF_TEXT;

    if (pos < 0)
        cbei.iItem = GetCount();
    else
        cbei.iItem = pos;
    if (m_bTrim)
        str.Trim(L" ");
    CString combostring = str;
    combostring.Replace('\r', ' ');
    combostring.Replace('\n', ' ');
    cbei.pszText = const_cast<LPTSTR>(combostring.GetString());

#ifdef HISTORYCOMBO_WITH_SYSIMAGELIST
    if (m_bURLHistory)
    {
        cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(str);
        if ((cbei.iImage == NULL) || (cbei.iImage == SYS_IMAGE_LIST().GetDefaultIconIndex()))
        {
            if (str.Left(5) == L"http:")
                cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(L".html");
            else if (str.Left(6) == L"https:")
                cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(L".html");
            else if (str.Left(5) == L"file:")
                cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
            else if (str.Left(4) == L"svn:")
                cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
            else if (str.Left(8) == L"svn+ssh:")
                cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
        }
        cbei.iSelectedImage = cbei.iImage;
        cbei.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
    }
    if (m_bPathHistory)
    {
        cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(str);
        if (cbei.iImage == SYS_IMAGE_LIST().GetDefaultIconIndex())
        {
            cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
        }
        cbei.iSelectedImage = cbei.iImage;
        cbei.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
    }
#endif
    int nRet = InsertItem(&cbei);
    if (nRet >= 0)
        m_arEntries.InsertAt(nRet, str);

    //search the Combo for another string like this
    //and delete it if one is found
    if (m_bTrim)
        str.Trim();
    int nIndex = FindStringExact(0, combostring);
    if (nIndex != -1 && nIndex != nRet)
    {
        DeleteItem(nIndex);
        m_arEntries.RemoveAt(nIndex);
    }

    SetCurSel(nRet);
    return nRet;
}

CString CHistoryCombo::LoadHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix)
{
    if (lpszSection == NULL || lpszKeyPrefix == NULL || *lpszSection == '\0')
        return L"";

    m_sSection = lpszSection;
    m_sKeyPrefix = lpszKeyPrefix;

    int n = 0;
    CString sText;
    do
    {
        //keys are of form <lpszKeyPrefix><entrynumber>
        CString sKey;
        sKey.Format(L"%s\\%s%d", (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n++);
        sText = CRegString(sKey);
        if (!sText.IsEmpty())
            AddString(sText);
    } while (!sText.IsEmpty() && n < m_nMaxHistoryItems);

    SetCurSel(-1);

    ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, 0);

    // need to resize the control for correct display
    CRect rect;
    GetWindowRect(rect);
    GetParent()->ScreenToClient(rect);
    MoveWindow(rect.left, rect.top, rect.Width(), 100);

    return sText;
}

void CHistoryCombo::SaveHistory()
{
    if (m_sSection.IsEmpty())
        return;

    //add the current item to the history
    CString sCurItem;
    GetWindowText(sCurItem);
    if (m_bTrim)
        sCurItem.Trim();
    if (!sCurItem.IsEmpty()&&(sCurItem.GetLength() < MAX_PATH))
        AddString(sCurItem, 0);
    //save history to registry/inifile
    int nMax = min(GetCount(), m_nMaxHistoryItems + 1);
    for (int n = 0; n < nMax; n++)
    {
        CString sKey;
        sKey.Format(L"%s\\%s%d", (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
        CRegString regkey(sKey);
        regkey = m_arEntries.GetAt(n);
    }
    //remove items exceeding the max number of history items
    for (int n = nMax; ; n++)
    {
        CString sKey;
        sKey.Format(L"%s\\%s%d", (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
        CRegString regkey = CRegString(sKey);
        CString sText = regkey;
        if (sText.IsEmpty())
            break;
        regkey.removeValue(); // remove entry
    }
}

void CHistoryCombo::ClearHistory(BOOL bDeleteRegistryEntries/*=TRUE*/)
{
    ResetContent();
    if (! m_sSection.IsEmpty() && bDeleteRegistryEntries)
    {
        //remove profile entries
        CString sKey;
        for (int n = 0; ; n++)
        {
            sKey.Format(L"%s\\%s%d", (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
            CRegString regkey = CRegString(sKey);
            CString sText = regkey;
            if (sText.IsEmpty())
                break;
            regkey.removeValue(); // remove entry
        }
    }
}

void CHistoryCombo::SetURLHistory(bool bURLHistory, bool bAutoComplete)
{
    m_bURLHistory = bURLHistory;

    if ((m_bURLHistory)&&(bAutoComplete))
        SetAutoComplete(SHACF_URLALL);

    SetStylesAndImageList();
}

void CHistoryCombo::SetPathHistory(BOOL bPathHistory)
{
    m_bPathHistory = bPathHistory;

    if (m_bPathHistory)
        SetAutoComplete(SHACF_FILESYSTEM);

    SetStylesAndImageList();
}

void CHistoryCombo::SetAutoComplete(DWORD flags)
{
    // use for ComboEx
    HWND hwndEdit = (HWND)::SendMessage(this->m_hWnd, CBEM_GETEDITCONTROL, 0, 0);
    if (NULL == hwndEdit)
    {
        // Try the unofficial way of getting the edit control CWnd*
        CWnd* pWnd = this->GetDlgItem(1001);
        if(pWnd)
        {
            hwndEdit = pWnd->GetSafeHwnd();
        }
    }
    if (hwndEdit)
        SHAutoComplete(hwndEdit, flags);
}

void CHistoryCombo::SetStylesAndImageList()
{
    SetExtendedStyle(CBES_EX_PATHWORDBREAKPROC|CBES_EX_CASESENSITIVE, CBES_EX_PATHWORDBREAKPROC|CBES_EX_CASESENSITIVE);
#ifdef HISTORYCOMBO_WITH_SYSIMAGELIST
    SetImageList(&SYS_IMAGE_LIST());
#endif
}

void CHistoryCombo::SetMaxHistoryItems(int nMaxItems)
{
    m_nMaxHistoryItems = nMaxItems;

    //truncate list to nMaxItems
    int nNumItems = GetCount();
    for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
        DeleteString(m_nMaxHistoryItems);
}

CString CHistoryCombo::GetString() const
{
    CString str;
    int sel;
    sel = GetCurSel();
    int len = 0;
    if (sel != CB_ERR)
        len = GetLBTextLen(sel);
    if (sel == CB_ERR)
    {
        GetWindowText(str);
        return str;
    }
    if (len >= (MAX_PATH-1))
    {
        GetWindowText(str);
        return str;
    }
    if ((m_bURLHistory)||(m_bPathHistory)||(GetStyle()&CBS_DROPDOWNLIST))
    {
        //URL and path history combo boxes are editable, so get
        //the string directly from the combobox
        GetLBText(sel, str.GetBuffer(GetLBTextLen(sel)));
        str.ReleaseBuffer();
        return str;
    }
    return m_arEntries.GetAt(sel);
}

CString CHistoryCombo::GetWindowString() const
{
    CString str;
    GetWindowText(str);
    return str;
}

BOOL CHistoryCombo::RemoveSelectedItem()
{
    int nIndex = GetCurSel();
    if (nIndex == CB_ERR)
    {
        return FALSE;
    }

    DeleteItem(nIndex);
    m_arEntries.RemoveAt(nIndex);

    if ( nIndex < GetCount() )
    {
        // index stays the same to select the
        // next item after the item which has
        // just been deleted
    }
    else
    {
        // the end of the list has been reached
        // so we select the previous item
        nIndex--;
    }

    if ( nIndex == -1 )
    {
        // The one and only item has just been
        // deleted -> reset window text since
        // there is no item to select
        SetWindowText(L"");
    }
    else
    {
        SetCurSel(nIndex);
    }

    // Since the dialog might be canceled we
    // should now save the history. Before that
    // set the selection to the first item so that
    // the items will not be reordered and restore
    // the selection after saving.
    SetCurSel(0);
    SaveHistory();
    if ( nIndex != -1 )
    {
        SetCurSel(nIndex);
    }

    return TRUE;
}

void CHistoryCombo::PreSubclassWindow()
{
    CComboBoxEx::PreSubclassWindow();

    if (!m_bDyn)
        CreateToolTip();
}

void CHistoryCombo::OnMouseMove(UINT nFlags, CPoint point)
{
    CRect rectClient;
    GetClientRect(&rectClient);
    int nComboButtonWidth = ::GetSystemMetrics(SM_CXHTHUMB) + 2;
    rectClient.right = rectClient.right - nComboButtonWidth;

    if (rectClient.PtInRect(point))
    {
        ClientToScreen(&rectClient);

        m_ToolText = GetString();
        m_ToolInfo.lpszText = (LPTSTR)(LPCTSTR)m_ToolText;

        HDC hDC = ::GetDC(m_hWnd);

        CFont *pFont = GetFont();
        HFONT hOldFont = (HFONT) ::SelectObject(hDC, (HFONT) *pFont);

        SIZE size;
        ::GetTextExtentPoint32(hDC, m_ToolText, m_ToolText.GetLength(), &size);
        ::SelectObject(hDC, hOldFont);
        ::ReleaseDC(m_hWnd, hDC);

        if (size.cx > (rectClient.Width() - 6))
        {
            rectClient.left += 1;
            rectClient.top += 3;

            COLORREF rgbText = ::GetSysColor(COLOR_WINDOWTEXT);
            COLORREF rgbBackground = ::GetSysColor(COLOR_WINDOW);

            CWnd *pWnd = GetFocus();
            if (pWnd)
            {
                if (pWnd->m_hWnd == m_hWnd)
                {
                    rgbText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
                    rgbBackground = ::GetSysColor(COLOR_HIGHLIGHT);
                }
            }

            if (!m_ttShown)
            {
                ::SendMessage(m_hWndToolTip, TTM_SETTIPBKCOLOR, rgbBackground, 0);
                ::SendMessage(m_hWndToolTip, TTM_SETTIPTEXTCOLOR, rgbText, 0);
                ::SendMessage(m_hWndToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM) &m_ToolInfo);
                ::SendMessage(m_hWndToolTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(rectClient.left, rectClient.top));
                ::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
                SetTimer(1, 80, NULL);
                SetTimer(2, 2000, NULL);
                m_ttShown = TRUE;
            }
        }
        else
        {
            ::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
            m_ttShown = FALSE;
        }
    }
    else
    {
        ::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
        m_ttShown = FALSE;
    }

    CComboBoxEx::OnMouseMove(nFlags, point);
}

void CHistoryCombo::OnTimer(UINT_PTR nIDEvent)
{
    CPoint point;
    DWORD ptW = GetMessagePos();
    point.x = GET_X_LPARAM(ptW);
    point.y = GET_Y_LPARAM(ptW);
    ScreenToClient(&point);

    CRect rectClient;
    GetClientRect(&rectClient);
    int nComboButtonWidth = ::GetSystemMetrics(SM_CXHTHUMB) + 2;

    rectClient.right = rectClient.right - nComboButtonWidth;

    if (!rectClient.PtInRect(point))
    {
        KillTimer(nIDEvent);
        ::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
        m_ttShown = FALSE;
    }
    if (nIDEvent == 2)
    {
        // tooltip timeout, just deactivate it
        ::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
        // don't set m_ttShown to FALSE, because we don't want the tooltip to show up again
        // without the mouse pointer first leaving the control and entering it again
    }

    CComboBoxEx::OnTimer(nIDEvent);
}

void CHistoryCombo::CreateToolTip()
{
    // create tooltip
    m_hWndToolTip = ::CreateWindowEx(WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_hWnd,
        NULL,
        NULL,
        NULL);

    // initialize tool info struct
    SecureZeroMemory(&m_ToolInfo, sizeof(m_ToolInfo));
    m_ToolInfo.cbSize = sizeof(m_ToolInfo);
    m_ToolInfo.uFlags = TTF_TRANSPARENT;
    m_ToolInfo.hwnd = m_hWnd;

    ::SendMessage(m_hWndToolTip, TTM_SETMAXTIPWIDTH, 0, SHRT_MAX);
    ::SendMessage(m_hWndToolTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &m_ToolInfo);
    ::SendMessage(m_hWndToolTip, TTM_SETTIPBKCOLOR, ::GetSysColor(COLOR_HIGHLIGHT), 0);
    ::SendMessage(m_hWndToolTip, TTM_SETTIPTEXTCOLOR, ::GetSysColor(COLOR_HIGHLIGHTTEXT), 0);

    CRect rectMargins(0,-1,0,-1);
    ::SendMessage(m_hWndToolTip, TTM_SETMARGIN, 0, (LPARAM)&rectMargins);

    CFont *pFont = GetFont();
    ::SendMessage(m_hWndToolTip, WM_SETFONT, (WPARAM)(HFONT)*pFont, FALSE);
}

int CHistoryCombo::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    lpCreateStruct->dwExStyle |= CBES_EX_CASESENSITIVE;
    if (CComboBoxEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (m_bDyn)
        CreateToolTip();

    return 0;
}
