// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011, 2013-2015, 2017, 2021 - TortoiseSVN

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

class CTraceToOutputDebugString
{
public:
    static CTraceToOutputDebugString& Instance()
    {
        if (m_pInstance == nullptr)
            m_pInstance = new CTraceToOutputDebugString;
        return *m_pInstance;
    }

    static bool Active()
    {
        return Instance().m_bActive;
    }

    // Non Unicode output helper
    void operator()(PCSTR pszFormat, ...) const
    {
        if (m_bActive)
        {
            va_list ptr;
            va_start(ptr, pszFormat);
            TraceV(pszFormat, ptr);
            va_end(ptr);
        }
    }

    // Unicode output helper
    void operator()(PCWSTR pszFormat, ...) const
    {
        if (m_bActive)
        {
            va_list ptr;
            va_start(ptr, pszFormat);
            TraceV(pszFormat, ptr);
            va_end(ptr);
        }
    }

private:
    CTraceToOutputDebugString()
    {
        m_lastTick = GetTickCount64();
        m_bActive  = !!CRegStdDWORD(L"Software\\TortoiseSVN\\DebugOutputString", FALSE);
    }
    ~CTraceToOutputDebugString()
    {
        delete m_pInstance;
    }

    ULONGLONG                         m_lastTick;
    bool                              m_bActive;
    static CTraceToOutputDebugString* m_pInstance;

    // Non Unicode output helper
    void TraceV(PCSTR pszFormat, va_list args) const
    {
        // Format the output buffer
        char szBuffer[1024];
        _vsnprintf_s(szBuffer, _countof(szBuffer), _countof(szBuffer) - 1, pszFormat, args);
        OutputDebugStringA(szBuffer);
    }

    // Unicode output helper
    void TraceV(PCWSTR pszFormat, va_list args) const
    {
        wchar_t szBuffer[1024];
        _vsnwprintf_s(szBuffer, _countof(szBuffer), _countof(szBuffer) - 1, pszFormat, args);
        OutputDebugStringW(szBuffer);
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    bool IsActive()
    {
#ifdef DEBUG
        return true;
#else
        if (GetTickCount64() - m_lastTick > 10000)
        {
            m_lastTick = GetTickCount64();
            m_bActive  = !!CRegStdDWORD(L"Software\\TortoiseSVN\\DebugOutputString", FALSE);
        }
        return m_bActive;
#endif
    }
};

class ProfileTimer
{
public:
    ProfileTimer(LPCWSTR text)
        : info(text)
    {
        QueryPerformanceCounter(&startTime);
    }
    ~ProfileTimer()
    {
        LARGE_INTEGER endTime;
        QueryPerformanceCounter(&endTime);
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        LARGE_INTEGER milliseconds;
        milliseconds.QuadPart = endTime.QuadPart - startTime.QuadPart;
        milliseconds.QuadPart *= 1000;
        milliseconds.QuadPart /= frequency.QuadPart;
        CTraceToOutputDebugString::Instance()(L"%s : %lld ms\n", info.c_str(), milliseconds.QuadPart);
    }

private:
    LARGE_INTEGER startTime;
    std::wstring  info;
};
