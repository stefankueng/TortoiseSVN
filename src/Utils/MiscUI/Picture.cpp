﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2017, 2021 - TortoiseSVN
// Copyright (C) 2020 - TortoiseGit

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

#include "stdafx.h"
#include <olectl.h>
#include <Shlwapi.h>
#include <locale>
#include <algorithm>
#include "Picture.h"
#include <memory>
#include <atlbase.h>
#include <Wincodec.h>
#include "OnOutOfScope.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")
// note: linking with Windowscodecs.lib does not make the exe require the dll
// which means it still runs on XP even if the WIC isn't installed - WIC won't work
// but the exe still runs
#pragma comment(lib, "Windowscodecs.lib")

#define HIMETRIC_INCH 2540

CPicture::CPicture()
    : m_height(0)
    , m_width(0)
    , m_colorDepth(0)
    , m_weight(0)
    , gdiplusToken(NULL)
    , m_ip(InterpolationModeDefault)
    , bIsIcon(false)
    , bIsTiff(false)
    , nCurrentIcon(0)
    , m_nSize(0)
{
}

CPicture::~CPicture()
{
    FreePictureData(); // Important - Avoid Leaks...
    if (gdiplusToken)
        GdiplusShutdown(gdiplusToken);
}

void CPicture::FreePictureData()
{
    if (m_iPicture != nullptr)
    {
        m_iPicture = nullptr;
        m_height   = 0;
        m_weight   = 0;
        m_width    = 0;
        m_nSize    = 0;
    }
    m_hIcons        = nullptr;
    m_lpIcons       = nullptr;
    m_pBitmap       = nullptr;
    m_pBitmapBuffer = nullptr;
}

// Util function to ease loading of FreeImage library
static FARPROC sGetProcAddressEx(HMODULE hDll, const char* procName, bool& valid)
{
    FARPROC proc = nullptr;

    if (valid)
    {
        proc = GetProcAddress(hDll, procName);

        if (!proc)
            valid = false;
    }

    return proc;
}

std::wstring CPicture::GetFileSizeAsText(bool bAbbrev /* = true */) const
{
    wchar_t buf[100] = {0};
    if (bAbbrev)
        StrFormatByteSize(m_nSize, buf, _countof(buf));
    else
        swprintf_s(buf, L"%lu Bytes", m_nSize);

    return std::wstring(buf);
}

bool CPicture::TryLoadIcon(const std::wstring& sFilePathName)
{
    // Icon file, get special treatment...
    CAutoFile hFile = CreateFile(sFilePathName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!hFile)
        return false;

    BY_HANDLE_FILE_INFORMATION fileInfo;
    if (!GetFileInformationByHandle(hFile, &fileInfo))
        return false;

    auto  lpIcons = std::make_unique<BYTE[]>(fileInfo.nFileSizeLow);
    DWORD readBytes;
    if (!ReadFile(hFile, lpIcons.get(), fileInfo.nFileSizeLow, &readBytes, nullptr))
        return false;

    // we have the icon. Now gather the information we need later
    if (readBytes < sizeof(ICONDIR))
        return false;

    // we are going to open same file second time so we have to close the file now
    hFile.CloseHandle();

    auto lpIconDir = reinterpret_cast<LPICONDIR>(lpIcons.get());
    if (!((lpIconDir->idCount) && ((lpIconDir->idCount * sizeof(ICONDIR)) <= fileInfo.nFileSizeLow)))
        return false;

    try
    {
        nCurrentIcon = 0;
        std::vector<CAutoIcon> hIcons;
        // check that the pointers point to data that we just loaded
        if ((reinterpret_cast<BYTE*>(lpIconDir->idEntries) > reinterpret_cast<BYTE*>(lpIconDir)) &&
            (reinterpret_cast<BYTE*>(lpIconDir->idEntries) + (lpIconDir->idCount * sizeof(ICONDIRENTRY)) < reinterpret_cast<BYTE*>(lpIconDir) + fileInfo.nFileSizeLow))
        {
            m_width  = lpIconDir->idEntries[0].bWidth == 0 ? 256 : lpIconDir->idEntries[0].bWidth;
            m_height = lpIconDir->idEntries[0].bHeight == 0 ? 256 : lpIconDir->idEntries[0].bHeight;
            for (int i = 0; i < lpIconDir->idCount; ++i)
            {
                CAutoIcon hIcon = static_cast<HICON>(LoadImage(nullptr, sFilePathName.c_str(), IMAGE_ICON,
                                                               lpIconDir->idEntries[i].bWidth == 0 ? 256 : lpIconDir->idEntries[i].bWidth,
                                                               lpIconDir->idEntries[i].bHeight == 0 ? 256 : lpIconDir->idEntries[i].bHeight,
                                                               LR_LOADFROMFILE));
                if (!hIcon)
                {
                    // if the icon couldn't be loaded, the data is most likely corrupt
                    throw std::runtime_error("could not load icon");
                }
                hIcons.emplace_back(std::move(hIcon));
            }
        }

        m_lpIcons = std::move(lpIcons);
        m_hIcons  = std::make_unique<std::vector<CAutoIcon>>(std::move(hIcons));
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool CPicture::TryLoadWIC(const std::wstring& sFilePathName)
{
    CComPtr<IWICImagingFactory> pFactory;
    HRESULT                     hr = CoCreateInstance(CLSID_WICImagingFactory,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IWICImagingFactory, reinterpret_cast<LPVOID*>(&pFactory));

    // Create a decoder from the file.
    if (FAILED(hr))
        return false;

    CComPtr<IWICBitmapDecoder> pDecoder;
    hr = pFactory->CreateDecoderFromFilename(sFilePathName.c_str(),
                                             nullptr,
                                             GENERIC_READ,
                                             WICDecodeMetadataCacheOnDemand,
                                             &pDecoder);
    if (FAILED(hr))
        return false;

    CComPtr<IWICBitmapFrameDecode> pBitmapFrameDecode;
    if (FAILED(pDecoder->GetFrame(0, &pBitmapFrameDecode)))
        return false;

    CComPtr<IWICFormatConverter> piFormatConverter = nullptr;
    if (FAILED(pFactory->CreateFormatConverter(&piFormatConverter)))
        return false;

    if (FAILED(piFormatConverter->Initialize(pBitmapFrameDecode, GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
        return false;

    UINT uWidth  = 0;
    UINT uHeight = 0;
    if (FAILED(piFormatConverter->GetSize(&uWidth, &uHeight)))
        return false;

    WICPixelFormatGUID pixelFormat = {0};
    if (FAILED(piFormatConverter->GetPixelFormat(&pixelFormat)))
        return false;

    UINT cbStride = uWidth * 3;
    // Force the stride to be a multiple of sizeof(DWORD)
    cbStride = ((cbStride + sizeof(DWORD) - 1) / sizeof(DWORD)) * sizeof(DWORD);

    UINT cbBufferSize = cbStride * uHeight;
    // note: the buffer must exist during the lifetime of the pBitmap object created below
    auto pBitmapBuffer = std::make_unique<BYTE[]>(cbBufferSize);

    if (!pBitmapBuffer)
        return false;

    WICRect rc = {0, 0, static_cast<int>(uWidth), static_cast<int>(uHeight)};
    if (FAILED(piFormatConverter->CopyPixels(&rc, cbStride, cbStride * uHeight, pBitmapBuffer.get())))
        return false;

    m_pBitmapBuffer = std::move(pBitmapBuffer);
    m_pBitmap       = std::make_unique<Bitmap>(uWidth, uHeight, cbStride, PixelFormat24bppRGB, m_pBitmapBuffer.get());
    m_height        = uHeight;
    m_width         = uWidth;
    return true;
}

bool CPicture::TryLoadFreeImage(const std::wstring& sFilePathName)
{
    // Attempt to load the FreeImage library as an optional DLL to support additional formats

    // NOTE: Currently just loading via FreeImage & using GDI+ for drawing.
    // It might be nice to remove this dependency in the future.
    CAutoLibrary hFreeImageLib = LoadLibrary(L"FreeImage.dll");

    // FreeImage DLL functions
    using FreeImageGetVersionT         = const char*(__stdcall*)();
    using FreeImageGetFileTypeT        = int(__stdcall*)(const wchar_t* filename, int size);
    using FreeImageGetFifFromFilenameT = int(__stdcall*)(const wchar_t* filename);
    using FreeImageLoadT               = void*(__stdcall*)(int format, const wchar_t* filename, int flags);
    using FreeImageUnloadT             = void(__stdcall*)(void* dib);
    using FreeImageGetColorTypeT       = int(__stdcall*)(void* dib);
    using FreeImageGetWidthT           = unsigned(__stdcall*)(void* dib);
    using FreeImageGetHeightT          = unsigned(__stdcall*)(void* dib);
    using FreeImageConvertToRawBitsT   = void(__stdcall*)(BYTE * bits, void* dib, int pitch, unsigned bpp, unsigned redMask, unsigned greenMask, unsigned blueMask, BOOL topDown);

    //FreeImage_GetVersion_t FreeImage_GetVersion = nullptr;
    FreeImageGetFileTypeT        freeImageGetFileType        = nullptr;
    FreeImageGetFifFromFilenameT freeImageGetFifFromFilename = nullptr;
    FreeImageLoadT               freeImageLoad               = nullptr;
    FreeImageUnloadT             freeImageUnload             = nullptr;
    //FreeImage_GetColorType_t FreeImage_GetColorType = nullptr;
    FreeImageGetWidthT         freeImageGetWidth         = nullptr;
    FreeImageGetHeightT        freeImageGetHeight        = nullptr;
    FreeImageConvertToRawBitsT freeImageConvertToRawBits = nullptr;

    if (!hFreeImageLib)
        return false;

    bool exportsValid = true;

    //FreeImage_GetVersion = reinterpret_cast<FreeImage_GetVersion_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetVersion@0", valid));
    freeImageGetWidth         = reinterpret_cast<FreeImageGetWidthT>(sGetProcAddressEx(hFreeImageLib, "_FreeImage_GetWidth@4", exportsValid));
    freeImageGetHeight        = reinterpret_cast<FreeImageGetHeightT>(sGetProcAddressEx(hFreeImageLib, "_FreeImage_GetHeight@4", exportsValid));
    freeImageUnload           = reinterpret_cast<FreeImageUnloadT>(sGetProcAddressEx(hFreeImageLib, "_FreeImage_Unload@4", exportsValid));
    freeImageConvertToRawBits = reinterpret_cast<FreeImageConvertToRawBitsT>(sGetProcAddressEx(hFreeImageLib, "_FreeImage_ConvertToRawBits@32", exportsValid));

    freeImageGetFileType        = reinterpret_cast<FreeImageGetFileTypeT>(sGetProcAddressEx(hFreeImageLib, "_FreeImage_GetFileTypeU@8", exportsValid));
    freeImageGetFifFromFilename = reinterpret_cast<FreeImageGetFifFromFilenameT>(sGetProcAddressEx(hFreeImageLib, "_FreeImage_GetFIFFromFilenameU@4", exportsValid));
    freeImageLoad               = reinterpret_cast<FreeImageLoadT>(sGetProcAddressEx(hFreeImageLib, "_FreeImage_LoadU@12", exportsValid));

    //const char* version = FreeImage_GetVersion();

    // Check the DLL is using compatible exports
    if (!exportsValid)
        return false;

    // Derive file type from file header.
    int fileType = freeImageGetFileType(sFilePathName.c_str(), 0);
    if (fileType < 0)
    {
        // No file header available, attempt to parse file name for extension.
        fileType = freeImageGetFifFromFilename(sFilePathName.c_str());
    }

    // If we have a valid file type
    if (fileType < 0)
        return false;

    void* dib = freeImageLoad(fileType, sFilePathName.c_str(), 0);
    if (!dib)
        return false;
    OnOutOfScope(freeImageUnload(dib));

    unsigned width  = freeImageGetWidth(dib);
    unsigned height = freeImageGetHeight(dib);

    // Create a GDI+ bitmap to load into...
    auto pBitmap = std::make_unique<Bitmap>(width, height, PixelFormat32bppARGB);

    if (!pBitmap || pBitmap->GetLastStatus() != Ok)
        return false;

    // Write & convert the loaded data into the GDI+ Bitmap
    Rect       rect(0, 0, width, height);
    BitmapData bitmapData;
    if (pBitmap->LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData) != Ok)
        return false;

    freeImageConvertToRawBits(static_cast<BYTE*>(bitmapData.Scan0), dib, bitmapData.Stride, 32, 0xff << RED_SHIFT, 0xff << GREEN_SHIFT, 0xff << BLUE_SHIFT, FALSE);

    pBitmap->UnlockBits(&bitmapData);

    m_width   = width;
    m_height  = height;
    m_pBitmap = std::move(pBitmap);

    return true;
}

bool CPicture::Load(std::wstring sFilePathName)
{
    bool bResult = false;
    bIsIcon      = false;
    //CFile PictureFile;
    //CFileException e;
    FreePictureData(); // Important - Avoid Leaks...

    // No-op if no file specified
    if (sFilePathName.empty())
        return true;

    // Load & initialize the GDI+ library
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Attempt to load using GDI+
    auto pBitmap = std::make_unique<Bitmap>(sFilePathName.c_str(), FALSE);
    GUID guid;
    pBitmap->GetRawFormat(&guid);

    if (pBitmap->GetLastStatus() != Ok)
        pBitmap = nullptr;

    // gdiplus only loads the first icon found in an icon file
    // so we have to handle icon files ourselves :(

    // Even though gdiplus can load icons, it can't load the new
    // icons from Vista - in Vista, the icon format changed slightly.
    // But the LoadIcon/LoadImage API still can load those icons,
    // at least those dimensions which are also used on pre-Vista
    // systems.
    // For that reason, we don't rely on gdiplus telling us if
    // the image format is "icon" or not, we also check the
    // file extension for ".ico".
    auto lowerFileName = sFilePathName;
    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::towlower);
    bIsIcon = (guid == ImageFormatIcon) || (wcsstr(lowerFileName.c_str(), L".ico") != nullptr) || (wcsstr(lowerFileName.c_str(), L".cur") != nullptr);
    bIsTiff = (guid == ImageFormatTIFF) || (wcsstr(lowerFileName.c_str(), L".tiff") != nullptr);
    m_name  = sFilePathName;

    if (bIsIcon)
        bResult = TryLoadIcon(sFilePathName);
    else if (pBitmap) // Image loaded successfully with GDI+
    {
        m_height  = pBitmap->GetHeight();
        m_width   = pBitmap->GetWidth();
        m_pBitmap = std::move(pBitmap);
        bResult   = true;
    }

    // If still failed to load the file...
    if (!bResult)
        bResult = TryLoadWIC(sFilePathName);

    if (!bResult)
        bResult = TryLoadFreeImage(sFilePathName);

    if (bResult)
    {
        CAutoFile hFile = CreateFile(sFilePathName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, nullptr);
        if (hFile)
        {
            BY_HANDLE_FILE_INFORMATION fileInfo;
            if (GetFileInformationByHandle(hFile, &fileInfo))
            {
                m_nSize = fileInfo.nFileSizeLow;
            }
        }
    }

    m_colorDepth = GetColorDepth();

    return (bResult);
}

bool CPicture::LoadPictureData(BYTE* pBuffer, int nSize)
{
    bool bResult = false;

    HGLOBAL hGlobalMem = GlobalAlloc(GMEM_MOVEABLE, nSize);

    if (hGlobalMem == nullptr)
    {
        return (false);
    }

    void* pData = GlobalLock(hGlobalMem);
    if (!pData)
    {
        GlobalFree(hGlobalMem);
        return false;
    }
    memcpy(pData, pBuffer, nSize);
    GlobalUnlock(hGlobalMem);

    CComPtr<IStream> pStream;

    if ((CreateStreamOnHGlobal(hGlobalMem, true, &pStream) == S_OK) && (pStream))
    {
        HRESULT hr = OleLoadPicture(pStream, nSize, false, IID_IPicture, reinterpret_cast<LPVOID*>(&m_iPicture));
        pStream    = nullptr;

        bResult = hr == S_OK;
    }

    FreeResource(hGlobalMem); // 16Bit Windows Needs This (32Bit - Automatic Release)

    return (bResult);
}

bool CPicture::Show(HDC hDC, RECT drawRect) const
{
    if (hDC == nullptr)
        return false;
    if (bIsIcon && m_lpIcons)
    {
        ::DrawIconEx(hDC, drawRect.left, drawRect.top, m_hIcons.get()->at(nCurrentIcon), drawRect.right - drawRect.left, drawRect.bottom - drawRect.top, 0, nullptr, DI_NORMAL);
        return true;
    }
    if (!m_iPicture && !m_pBitmap)
        return false;

    if (m_iPicture)
    {
        long width  = 0;
        long height = 0;
        m_iPicture->get_Width(&width);
        m_iPicture->get_Height(&height);

        HRESULT hr = m_iPicture->Render(hDC,
                                        drawRect.left,                  // Left
                                        drawRect.top,                   // Top
                                        drawRect.right - drawRect.left, // Right
                                        drawRect.bottom - drawRect.top, // Bottom
                                        0,
                                        height,
                                        width,
                                        -height,
                                        &drawRect);

        if (SUCCEEDED(hr))
            return (true);
    }
    else if (m_pBitmap)
    {
        Graphics graphics(hDC);
        graphics.SetInterpolationMode(m_ip);
        graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
        ImageAttributes attr;
        attr.SetWrapMode(WrapModeTileFlipXY);
        Rect rect(drawRect.left, drawRect.top, drawRect.right - drawRect.left, drawRect.bottom - drawRect.top);
        graphics.DrawImage(m_pBitmap.get(), rect, 0, 0, m_width, m_height, UnitPixel, &attr);
        return true;
    }

    return (false);
}

bool CPicture::UpdateSizeOnDC(HDC hDC)
{
    if (hDC == nullptr || m_iPicture == nullptr)
    {
        m_height = 0;
        m_width  = 0;
        return (false);
    };

    m_iPicture->get_Height(&m_height);
    m_iPicture->get_Width(&m_width);

    // Get Current DPI - Dot Per Inch
    int currentDpiX = GetDeviceCaps(hDC, LOGPIXELSX);
    int currentDpiY = GetDeviceCaps(hDC, LOGPIXELSY);

    m_height = MulDiv(m_height, currentDpiY, HIMETRIC_INCH);
    m_width  = MulDiv(m_width, currentDpiX, HIMETRIC_INCH);

    return (true);
}

UINT CPicture::GetColorDepth() const
{
    if (bIsIcon && m_lpIcons)
    {
        auto lpIconDir = reinterpret_cast<LPICONDIR>(m_lpIcons.get());
        return lpIconDir->idEntries[nCurrentIcon].wBitCount;
    }

    // try first with WIC to get the pixel format since GDI+ often returns 32-bit even if it's not
    // Create the image factory.
    UINT                        bpp = 0;
    CComPtr<IWICImagingFactory> pFactory;
    HRESULT                     hr = CoCreateInstance(CLSID_WICImagingFactory,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IWICImagingFactory,
                                  reinterpret_cast<LPVOID*>(&pFactory));

    // Create a decoder from the file.
    if (SUCCEEDED(hr))
    {
        CComPtr<IWICBitmapDecoder> pDecoder;
        hr = pFactory->CreateDecoderFromFilename(m_name.c_str(),
                                                 nullptr,
                                                 GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand,
                                                 &pDecoder);
        if (SUCCEEDED(hr))
        {
            CComPtr<IWICBitmapFrameDecode> pBitmapFrameDecode;
            hr = pDecoder->GetFrame(0, &pBitmapFrameDecode);
            if (SUCCEEDED(hr))
            {
                WICPixelFormatGUID pixelFormat;
                hr = pBitmapFrameDecode->GetPixelFormat(&pixelFormat);
                if (SUCCEEDED(hr))
                {
                    CComPtr<IWICComponentInfo> piCompInfo;
                    hr = pFactory->CreateComponentInfo(pixelFormat, &piCompInfo);
                    if (SUCCEEDED(hr))
                    {
                        CComPtr<IWICPixelFormatInfo> piPixelFormatInfo;
                        hr = piCompInfo->QueryInterface(IID_IWICPixelFormatInfo, reinterpret_cast<LPVOID*>(&piPixelFormatInfo));
                        if (SUCCEEDED(hr))
                        {
                            hr = piPixelFormatInfo->GetBitsPerPixel(&bpp);
                        }
                    }
                }
            }
        }
    }
    if (bpp)
        return bpp;

    switch (GetPixelFormat())
    {
        case PixelFormat1bppIndexed:
            return 1;
        case PixelFormat4bppIndexed:
            return 4;
        case PixelFormat8bppIndexed:
            return 8;
        case PixelFormat16bppARGB1555:
        case PixelFormat16bppGrayScale:
        case PixelFormat16bppRGB555:
        case PixelFormat16bppRGB565:
            return 16;
        case PixelFormat24bppRGB:
            return 24;
        case PixelFormat32bppARGB:
        case PixelFormat32bppPARGB:
        case PixelFormat32bppRGB:
            return 32;
        case PixelFormat48bppRGB:
            // note: GDI+ converts images with bit depths > 32 automatically
            // on loading, so PixelFormat48bppRGB, PixelFormat64bppARGB and
            // PixelFormat64bppPARGB will never be used here.
            return 48;
        case PixelFormat64bppARGB:
        case PixelFormat64bppPARGB:
            return 64;
    }
    return 0;
}

UINT CPicture::GetNumberOfFrames(int dimension) const
{
    if (bIsIcon && m_lpIcons)
    {
        return 1;
    }
    if (!m_pBitmap)
        return 0;
    UINT  count         = m_pBitmap->GetFrameDimensionsCount();
    GUID* pDimensionIDs = static_cast<GUID*>(malloc(sizeof(GUID) * count));

    m_pBitmap->GetFrameDimensionsList(pDimensionIDs, count);

    UINT frameCount = m_pBitmap->GetFrameCount(&pDimensionIDs[dimension]);

    free(pDimensionIDs);
    return frameCount;
}

UINT CPicture::GetNumberOfDimensions() const
{
    if (bIsIcon && m_lpIcons)
    {
        auto lpIconDir = reinterpret_cast<LPICONDIR>(m_lpIcons.get());
        return lpIconDir->idCount;
    }
    return m_pBitmap ? m_pBitmap->GetFrameDimensionsCount() : 0;
}

long CPicture::SetActiveFrame(UINT frame)
{
    if (bIsIcon && m_lpIcons)
    {
        nCurrentIcon = frame - 1;
        m_height     = GetHeight();
        m_width      = GetWidth();
        return 0;
    }
    if (!m_pBitmap)
        return 0;
    UINT  count         = m_pBitmap->GetFrameDimensionsCount();
    GUID* pDimensionIDs = static_cast<GUID*>(malloc(sizeof(GUID) * count));

    m_pBitmap->GetFrameDimensionsList(pDimensionIDs, count);

    UINT frameCount = m_pBitmap->GetFrameCount(&pDimensionIDs[0]);

    free(pDimensionIDs);

    if (frame > frameCount)
        return 0;

    GUID pageGuid = FrameDimensionTime;
    if (bIsTiff)
        pageGuid = FrameDimensionPage;
    m_pBitmap->SelectActiveFrame(&pageGuid, frame);

    // Assume that the image has a property item of type PropertyItemEquipMake.
    // Get the size of that property item.
    int nSize = m_pBitmap->GetPropertyItemSize(PropertyTagFrameDelay);

    // Allocate a buffer to receive the property item.
    PropertyItem* pPropertyItem = static_cast<PropertyItem*>(malloc(nSize));

    Status s = m_pBitmap->GetPropertyItem(PropertyTagFrameDelay, nSize, pPropertyItem);

    UINT prevframe = frame;
    if (prevframe > 0)
        prevframe--;
    long delay = 0;
    if (s == Ok)
    {
        delay = static_cast<long*>(pPropertyItem->value)[prevframe] * 10;
    }
    free(pPropertyItem);
    m_height = GetHeight();
    m_width  = GetWidth();
    return delay;
}

UINT CPicture::GetHeight() const
{
    if ((bIsIcon) && (m_lpIcons))
    {
        auto lpIconDir = reinterpret_cast<LPICONDIR>(m_lpIcons.get());
        return lpIconDir->idEntries[nCurrentIcon].bHeight == 0 ? 256 : lpIconDir->idEntries[nCurrentIcon].bHeight;
    }
    return m_pBitmap ? m_pBitmap->GetHeight() : 0;
}

UINT CPicture::GetWidth() const
{
    if ((bIsIcon) && (m_lpIcons))
    {
        auto lpIconDir = reinterpret_cast<LPICONDIR>(m_lpIcons.get());
        return lpIconDir->idEntries[nCurrentIcon].bWidth == 0 ? 256 : lpIconDir->idEntries[nCurrentIcon].bWidth;
    }
    return m_pBitmap ? m_pBitmap->GetWidth() : 0;
}

PixelFormat CPicture::GetPixelFormat() const
{
    if ((bIsIcon) && (m_lpIcons))
    {
        auto lpIconDir = reinterpret_cast<LPICONDIR>(m_lpIcons.get());
        if (lpIconDir->idEntries[nCurrentIcon].wPlanes == 1)
        {
            if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 1)
                return PixelFormat1bppIndexed;
            if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 4)
                return PixelFormat4bppIndexed;
            if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 8)
                return PixelFormat8bppIndexed;
        }
        if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 32)
        {
            return PixelFormat32bppARGB;
        }
    }
    return m_pBitmap ? m_pBitmap->GetPixelFormat() : 0;
}
