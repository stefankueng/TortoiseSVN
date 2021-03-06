// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2013, 2017, 2021 - TortoiseSVN

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
#include "DIB.h"
#include <vector>

CDib::CDib()
{
    m_hBitmap = nullptr;
    DeleteObject();
}

CDib::~CDib()
{
    DeleteObject();
}

int CDib::BytesPerLine(int nWidth, int nBitsPerPixel)
{
    int nBytesPerLine = nWidth * nBitsPerPixel;
    nBytesPerLine     = ((nBytesPerLine + 31) & (~31)) / 8;
    return nBytesPerLine;
}

void CDib::DeleteObject()
{
    m_pBits = nullptr;
    if (m_hBitmap)
        ::DeleteObject(m_hBitmap);
    m_hBitmap = nullptr;

    SecureZeroMemory(&m_bmInfo, sizeof(m_bmInfo));
}

void CDib::Create32BitFromPicture(CPictureHolder* pPicture, int iWidth, int iHeight)
{
    CRect     r;
    CBitmap   newBmp;
    CWindowDC dc(nullptr);
    CDC       tempDC;

    tempDC.CreateCompatibleDC(&dc);

    newBmp.CreateDiscardableBitmap(&dc, iWidth, iHeight);

    CBitmap* pOldBitmap = tempDC.SelectObject(&newBmp);

    r.SetRect(0, 0, iWidth, iHeight);
    pPicture->Render(&tempDC, r, r);

    // Create a 32 bit bitmap
    std::vector<DWORD> pBits(iWidth * iHeight);

    BITMAPINFO bi;
    bi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth         = iWidth;
    bi.bmiHeader.biHeight        = iHeight;
    bi.bmiHeader.biPlanes        = 1;
    bi.bmiHeader.biBitCount      = 32;
    bi.bmiHeader.biCompression   = BI_RGB;
    bi.bmiHeader.biSizeImage     = 0;
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed       = 0;
    bi.bmiHeader.biClrImportant  = 0;

    SetBitmap(&bi, pBits.data());

    DWORD* pAr = static_cast<DWORD*>(GetDIBits());

    // Copy data into the 32 bit dib..
    for (int i = 0; i < iHeight; i++)
    {
        for (int j = 0; j < iWidth; j++)
        {
            pAr[(i * iWidth) + j] = FixColorRef(tempDC.GetPixel(j, i));
        }
    }

    tempDC.SelectObject(pOldBitmap);
}

BOOL CDib::SetBitmap(const LPBITMAPINFO lpBitmapInfo, const LPVOID lpBits)
{
    DeleteObject();

    if (!lpBitmapInfo || !lpBits)
        return FALSE;

    HDC hDC = nullptr;

    DWORD dwBitmapInfoSize = sizeof(BITMAPINFO);

    memcpy(&m_bmInfo, lpBitmapInfo, dwBitmapInfoSize);

    hDC = ::GetDC(nullptr);
    if (!hDC)
    {
        DeleteObject();
        return FALSE;
    }

    m_hBitmap = CreateDIBSection(hDC, &m_bmInfo,
                                 DIB_RGB_COLORS, &m_pBits, nullptr, 0);
    ::ReleaseDC(nullptr, hDC);
    if (!m_hBitmap)
    {
        DeleteObject();
        return FALSE;
    }

    DWORD dwImageSize = m_bmInfo.bmiHeader.biSizeImage;
    if (dwImageSize == 0)
    {
        int nBytesPerLine = BytesPerLine(lpBitmapInfo->bmiHeader.biWidth,
                                         lpBitmapInfo->bmiHeader.biBitCount);
        dwImageSize       = nBytesPerLine * lpBitmapInfo->bmiHeader.biHeight;
    }

    GdiFlush();

    memcpy(m_pBits, lpBits, dwImageSize);

    return TRUE;
}

BOOL CDib::Draw(CDC* pDC, CPoint ptDest) const
{
    if (!m_hBitmap)
        return FALSE;

    CSize  size      = GetSize();
    CPoint srcOrigin = CPoint(0, 0);

    BOOL resVal = SetDIBitsToDevice(pDC->GetSafeHdc(),
                                    ptDest.x, ptDest.y,
                                    size.cx, size.cy,
                                    srcOrigin.x, srcOrigin.y,
                                    srcOrigin.y, size.cy - srcOrigin.y,
                                    GetDIBits(), &m_bmInfo,
                                    DIB_RGB_COLORS);

    return resVal;
}

COLORREF CDib::FixColorRef(COLORREF clr)
{
    int r = GetRValue(clr);
    int g = GetGValue(clr);
    int b = GetBValue(clr);

    return RGB(b, g, r);
}
