// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010, 2014 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "PathEdit.h"
#include "StringUtils.h"

// CPathEdit

IMPLEMENT_DYNAMIC(CPathEdit, CEdit)

CPathEdit::CPathEdit()
    : m_bInternalCall(false)
    , m_bBold(false)
{
    m_tooltips.Create(this);
}

CPathEdit::~CPathEdit()
{
    if ((HFONT)m_boldFont)
        m_boldFont.DeleteObject();
}

BEGIN_MESSAGE_MAP(CPathEdit, CEdit)
END_MESSAGE_MAP()

// CPathEdit message handlers

LRESULT CPathEdit::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (m_bInternalCall)
        return CEdit::DefWindowProc(message, wParam, lParam);

    switch (message)
    {
    case WM_SIZE:
        {
            CString path = m_sRealText;
            FitPathToWidth(path);
            m_bInternalCall = true;
            CEdit::SendMessage(WM_SETTEXT, 0, (LPARAM)(LPCTSTR)path);
            m_bInternalCall = false;
            return CEdit::DefWindowProc(message, wParam, lParam);
        }
        break;
    case WM_SETTEXT:
        {
            m_sRealText = (LPCTSTR)lParam;
            m_tooltips.AddTool(this, m_sRealText);
            CString path = m_sRealText;
            FitPathToWidth(path);
            lParam = (LPARAM)(LPCTSTR)path;
            LRESULT ret = CEdit::DefWindowProc(message, wParam, lParam);
            return ret;
        }
        break;
    case WM_GETTEXT:
        {
            // return the real text
            wcsncpy_s((TCHAR*)lParam, wParam, (LPCTSTR)m_sRealText, _TRUNCATE);
            return wcslen((TCHAR*)lParam);
        }
        break;
    case WM_GETTEXTLENGTH:
        {
            return m_sRealText.GetLength();
        }
        break;
    case WM_COPY:
        {
            int start, end;
            m_bInternalCall = true;
            CEdit::GetSel(start, end);
            m_bInternalCall = false;
            CString selText = m_sFitText.Mid(start, end-start);
            if (m_sFitText.Find(L"..")<0)
                return CStringUtils::WriteAsciiStringToClipboard(selText, m_hWnd);
            if (selText.Find(L"...")>=0)
            {
                int dotLength = m_sRealText.GetLength()-m_sFitText.GetLength();
                selText = m_sRealText.Mid(start, end-start+dotLength);
                return CStringUtils::WriteAsciiStringToClipboard(selText, m_hWnd);
            }
            if ((m_sFitText.Left(start).Find(L"...")>=0) ||
                (m_sFitText.Mid(end).Find(L"...")>=0))
            {
                return CStringUtils::WriteAsciiStringToClipboard(selText, m_hWnd);
            }
            // we shouldn't get here, but just in case: copy *all* the text, not just the selected text
            return CStringUtils::WriteAsciiStringToClipboard(m_sRealText, m_hWnd);
        }
        break;
    }

    return CEdit::DefWindowProc(message, wParam, lParam);
}

void CPathEdit::FitPathToWidth(CString& path)
{
    CRect rect;
    GetClientRect(&rect);
    rect.right -= 5;    // assume a border size of 5 pixels

    CDC * pDC = GetDC();
    if (pDC)
    {
        CFont* previousFont = pDC->SelectObject(GetFont());
        path = path.Left(MAX_PATH - 1);
        PathCompactPath(pDC->m_hDC, path.GetBuffer(MAX_PATH), rect.Width());
        path.ReleaseBuffer();
        m_sFitText = path;
        pDC->SelectObject(previousFont);
        ReleaseDC(pDC);
    }
    path.Replace('\\', '/');
}

void CPathEdit::SetBold()
{
    m_bBold = true;
    SetFont(GetFont());
}

CFont * CPathEdit::GetFont()
{
    if (!m_bBold)
        return __super::GetFont();

    if ((HFONT)m_boldFont == NULL)
    {
        HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
        LOGFONT lf = {0};
        GetObject(hFont, sizeof(LOGFONT), &lf);
        lf.lfWeight = FW_BOLD;
        m_boldFont.CreateFontIndirect(&lf);
    }
    return &m_boldFont;
}

BOOL CPathEdit::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    return CEdit::PreTranslateMessage(pMsg);
}
