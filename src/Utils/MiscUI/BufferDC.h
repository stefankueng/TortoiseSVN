// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseSVN

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
#include <afxwin.h>

class CBufferDC :
    public CPaintDC
{
    DECLARE_DYNAMIC(CBufferDC)

private:
    HDC m_hOutputDC;
    HDC m_hAttributeDC;
    HDC m_hMemoryDC;

    HBITMAP  m_hPaintBitmap;
    HBITMAP  m_hOldBitmap;

    RECT m_ClientRect;

    BOOL m_bBoundsUpdated;

public:
    CBufferDC(CWnd* pWnd);
    ~CBufferDC(void);

private:
    void Flush();

public:
    UINT SetBoundsRect(LPCRECT lpRectBounds, UINT flags);
    virtual BOOL RestoreDC(int nSavedDC);
};
