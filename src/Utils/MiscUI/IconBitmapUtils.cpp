// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2012, 2014-2015, 2018, 2020-2021 - TortoiseSVN
// Copyright (C) 2016 - TortoiseGit

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
#include "IconBitmapUtils.h"
#include "OnOutOfScope.h"
#include "LoadIconEx.h"
#include "SmartHandle.h"

#pragma comment(lib, "UxTheme.lib")

IconBitmapUtils::IconBitmapUtils()
{
}

IconBitmapUtils::~IconBitmapUtils()
{
    for (const auto &[id, handle] : bitmaps)
        ::DeleteObject(handle);
    bitmaps.clear();
}

HBITMAP IconBitmapUtils::IconToBitmap(HINSTANCE hInst, UINT uIcon)
{
    auto bitmapIt = bitmaps.lower_bound(uIcon);
    if (bitmapIt != bitmaps.end() && bitmapIt->first == uIcon)
        return bitmapIt->second;

    CAutoIcon hIcon = LoadIconEx(hInst, MAKEINTRESOURCE(uIcon), 12, 12);
    if (!hIcon)
        return nullptr;

    RECT rect{};

    rect.right  = ::GetSystemMetrics(SM_CXMENUCHECK);
    rect.bottom = ::GetSystemMetrics(SM_CYMENUCHECK);

    rect.left = rect.top = 0;

    HWND desktop = ::GetDesktopWindow();
    if (desktop == nullptr)
        return nullptr;

    HDC screenDev = ::GetDC(desktop);
    if (screenDev == nullptr)
        return nullptr;
    OnOutOfScope(::ReleaseDC(desktop, screenDev););

    // Create a compatible DC
    HDC dstHdc = ::CreateCompatibleDC(screenDev);
    if (dstHdc == nullptr)
        return nullptr;
    OnOutOfScope(::DeleteDC(dstHdc););

    // Create a new bitmap of icon size
    HBITMAP bmp = ::CreateCompatibleBitmap(screenDev, rect.right, rect.bottom);
    if (bmp == nullptr)
        return nullptr;

    // Select it into the compatible DC
    HBITMAP oldDstBmp = static_cast<HBITMAP>(::SelectObject(dstHdc, bmp));
    if (oldDstBmp == nullptr)
        return nullptr;

    // Fill the background of the compatible DC with the white color
    // that is taken by menu routines as transparent
    ::SetBkColor(dstHdc, RGB(255, 255, 255));
    ::ExtTextOut(dstHdc, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);

    // Draw the icon into the compatible DC
    ::DrawIconEx(dstHdc, 0, 0, hIcon, rect.right, rect.bottom, 0, nullptr, DI_NORMAL);

    // Restore settings
    ::SelectObject(dstHdc, oldDstBmp);

    bitmaps.insert(bitmapIt, std::make_pair(uIcon, bmp));
    return bmp;
}

HBITMAP IconBitmapUtils::IconToBitmapPARGB32(HINSTANCE hInst, UINT uIcon)
{
    auto bitmapIt = bitmaps.lower_bound(uIcon);
    if (bitmapIt != bitmaps.end() && bitmapIt->first == uIcon)
        return bitmapIt->second;

    int       iconWidth  = GetSystemMetrics(SM_CXSMICON);
    int       iconHeight = GetSystemMetrics(SM_CYSMICON);
    CAutoIcon hIcon      = LoadIconEx(hInst, MAKEINTRESOURCE(uIcon), iconWidth, iconHeight);

    HBITMAP hBmp = IconToBitmapPARGB32(hIcon, iconWidth, iconHeight);

    if (hBmp)
        bitmaps.insert(bitmapIt, std::make_pair(uIcon, hBmp));

    return hBmp;
}
HBITMAP IconBitmapUtils::IconToBitmapPARGB32(HICON hIcon) const
{
    return IconToBitmapPARGB32(hIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
}

HBITMAP IconBitmapUtils::IconToBitmapPARGB32(HICON hIcon, int width, int height)
{
    if (!hIcon)
        return nullptr;

    SIZE sizIcon{};
    sizIcon.cx = width;
    sizIcon.cy = height;

    RECT rcIcon{};
    SetRect(&rcIcon, 0, 0, sizIcon.cx, sizIcon.cy);

    HDC hdcDest = CreateCompatibleDC(nullptr);
    if (!hdcDest)
        return nullptr;
    OnOutOfScope(DeleteDC(hdcDest););

    HBITMAP hBmp = nullptr;
    if (FAILED(Create32BitHBITMAP(hdcDest, &sizIcon, NULL, &hBmp)))
        return nullptr;

    HBITMAP hBmpOld = static_cast<HBITMAP>(SelectObject(hdcDest, hBmp));
    if (!hBmpOld)
        return hBmp;

    BLENDFUNCTION  bfAlpha     = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    BP_PAINTPARAMS paintParams = {0};
    paintParams.cbSize         = sizeof(paintParams);
    paintParams.dwFlags        = BPPF_ERASE;
    paintParams.pBlendFunction = &bfAlpha;

    HDC          hdcBuffer;
    HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hdcDest, &rcIcon, BPBF_DIB, &paintParams, &hdcBuffer);
    if (hPaintBuffer)
    {
        if (DrawIconEx(hdcBuffer, 0, 0, hIcon, sizIcon.cx, sizIcon.cy, 0, nullptr, DI_NORMAL))
        {
            // If icon did not have an alpha channel we need to convert buffer to PARGB
            ConvertBufferToPARGB32(hPaintBuffer, hdcDest, hIcon, sizIcon);
        }

        // This will write the buffer contents to the destination bitmap
        EndBufferedPaint(hPaintBuffer, TRUE);
    }

    SelectObject(hdcDest, hBmpOld);

    return hBmp;
}

HRESULT IconBitmapUtils::Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP *phBmp)
{
    if (psize == nullptr)
        return E_INVALIDARG;

    if (phBmp == nullptr)
        return E_POINTER;

    *phBmp = nullptr;

    BITMAPINFO bmi              = {0};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biCompression = BI_RGB;

    bmi.bmiHeader.biWidth    = psize->cx;
    bmi.bmiHeader.biHeight   = psize->cy;
    bmi.bmiHeader.biBitCount = 32;

    HDC hdcUsed = hdc ? hdc : GetDC(nullptr);
    if (hdcUsed)
    {
        *phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, nullptr, 0);
        if (hdc != hdcUsed)
        {
            ReleaseDC(nullptr, hdcUsed);
        }
    }
    return (nullptr == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

HRESULT IconBitmapUtils::ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hIcon, SIZE &sizIcon)
{
    RGBQUAD *prgbQuad = nullptr;
    int      cxRow    = 0;
    HRESULT  hr       = GetBufferedPaintBits(hPaintBuffer, &prgbQuad, &cxRow);
    if (FAILED(hr))
        return hr;

    Gdiplus::ARGB *pArgb = reinterpret_cast<Gdiplus::ARGB *>(prgbQuad);
    if (HasAlpha(pArgb, sizIcon, cxRow))
        return S_OK;

    ICONINFO info;
    if (!GetIconInfo(hIcon, &info))
        return S_OK;
    OnOutOfScope(
        DeleteObject(info.hbmColor);
        DeleteObject(info.hbmMask););
    if (info.hbmMask)
        return ConvertToPARGB32(hdc, pArgb, info.hbmMask, sizIcon, cxRow);

    return S_OK;
}

bool IconBitmapUtils::HasAlpha(__in Gdiplus::ARGB *pargb, SIZE &sizImage, int cxRow)
{
    ULONG cxDelta = cxRow - sizImage.cx;
    for (ULONG y = sizImage.cy; y; --y)
    {
        for (ULONG x = sizImage.cx; x; --x)
        {
            if (*pargb++ & 0xFF000000)
            {
                return true;
            }
        }

        pargb += cxDelta;
    }

    return false;
}

HRESULT IconBitmapUtils::ConvertToPARGB32(HDC hdc, __inout Gdiplus::ARGB *pargb, HBITMAP hbmp, SIZE &sizImage, int cxRow)
{
    BITMAPINFO bmi              = {0};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biCompression = BI_RGB;

    bmi.bmiHeader.biWidth    = sizImage.cx;
    bmi.bmiHeader.biHeight   = sizImage.cy;
    bmi.bmiHeader.biBitCount = 32;

    HANDLE hHeap  = GetProcessHeap();
    void * pvBits = HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);
    if (pvBits == nullptr)
        return E_OUTOFMEMORY;
    OnOutOfScope(HeapFree(hHeap, 0, pvBits););

    if (GetDIBits(hdc, hbmp, 0, bmi.bmiHeader.biHeight, pvBits, &bmi, DIB_RGB_COLORS) != bmi.bmiHeader.biHeight)
        return E_UNEXPECTED;

    ULONG          cxDelta   = cxRow - bmi.bmiHeader.biWidth;
    Gdiplus::ARGB *pargbMask = static_cast<Gdiplus::ARGB *>(pvBits);

    for (ULONG y = bmi.bmiHeader.biHeight; y; --y)
    {
        for (ULONG x = bmi.bmiHeader.biWidth; x; --x)
        {
            if (*pargbMask++)
            {
                // transparent pixel
                *pargb++ = 0;
            }
            else
            {
                // opaque pixel
                *pargb++ |= 0xFF000000;
            }
        }
        pargb += cxDelta;
    }

    return S_OK;
}
