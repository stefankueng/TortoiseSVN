// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2009, 2011-2012, 2014-2016, 2021 - TortoiseSVN

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
#include <shellapi.h>
#include "TSVNCache.h"
#include "SVNStatusCache.h"
#include "CacheInterface.h"
#include "resource.h"
#include "registry.h"
#include "CrashReport.h"
#include "SVNAdminDir.h"
#include "Dbt.h"
// ReSharper disable once CppUnusedIncludeDirective
#include <initguid.h>
#include <ioevent.h>
#include "svn_dso.h"
#include "SmartHandle.h"
#include "LoadIconEx.h"

#ifndef GET_X_LPARAM
#    define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#    define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

CCrashReportTSVN crasher(L"TSVNCache " TEXT(APP_X64_STRING));

unsigned int __stdcall InstanceThread(LPVOID);
unsigned int __stdcall PipeThread(LPVOID lpvParam);
unsigned int __stdcall CommandWaitThread(LPVOID);
unsigned int __stdcall CommandThread(LPVOID);
LRESULT CALLBACK        WndProc(HWND, UINT, WPARAM, LPARAM);
bool                    bRun = true;
NOTIFYICONDATA          niData;
HWND                    hWndHidden;
HWND                    hTrayWnd;
wchar_t                 szCurrentCrawledPath[MAX_CRAWLEDPATHS][MAX_CRAWLEDPATHSLEN];
int                     nCurrentCrawledpathIndex = 0;
CComAutoCriticalSection critSec;

#define PACKVERSION(major, minor) MAKELONG(minor, major)

svn_error_t* svnErrorHandleMalfunction(svn_boolean_t canReturn,
                                       const char* file, int line,
                                       const char* expr)
{
    // we get here every time Subversion encounters something very unexpected.
    svn_error_t* err = svn_error_raise_on_malfunction(TRUE, file, line, expr);

    if (err)
    {
        svn_error_t* errTemp = err;
        do
        {
            OutputDebugStringA(errTemp->message);
            OutputDebugStringA("\n");
        } while ((errTemp = errTemp->child) != nullptr);
        if (canReturn)
            return err;
        if (CRegDWORD(L"Software\\TortoiseSVN\\Debug", FALSE) == FALSE)
        {
            CCrashReport::Instance().Uninstall();
            CAutoWriteWeakLock writeLock(CSVNStatusCache::Instance().GetGuard(), 5000);
            bRun = false;
            CSVNStatusCache::Instance().Stop();
            abort(); // ugly, ugly! But at least we showed a messagebox first
        }
    }
    CStringA sFormatErr;
    sFormatErr.Format("Subversion error in TSVNCache: file %s, line %ld, error %s\n", file, line, expr);
    OutputDebugStringA(sFormatErr);
    if (CRegDWORD(L"Software\\TortoiseSVN\\Debug", FALSE) == FALSE)
    {
        CCrashReport::Instance().Uninstall();
        CAutoWriteWeakLock writeLock(CSVNStatusCache::Instance().GetGuard(), 5000);
        bRun = false;
        CSVNStatusCache::Instance().Stop();
        abort(); // ugly, ugly! But at least we showed a messagebox first
    }
    return nullptr; // never reached, only to silence compiler warning
}

static HWND CreateHiddenWindow(HINSTANCE hInstance)
{
    wchar_t szWindowClass[] = {TSVN_CACHE_WINDOW_NAME};

    WNDCLASSEX wcex    = {};
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = static_cast<WNDPROC>(WndProc);
    wcex.hInstance     = hInstance;
    wcex.lpszClassName = szWindowClass;
    RegisterClassEx(&wcex);
    return CreateWindow(TSVN_CACHE_WINDOW_NAME, TSVN_CACHE_WINDOW_NAME, WS_CAPTION, 0, 0, 800, 300, NULL, nullptr, hInstance, nullptr);
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*cmdShow*/)
{
    SetDllDirectory(L"");
    CAutoGeneralHandle hReloadProtection = ::CreateMutex(nullptr, FALSE, GetCacheMutexName());

    if ((!hReloadProtection) || (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        // An instance of TSVNCache is already running
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": TSVNCache ignoring restart\n");
        return 0;
    }

    // set the current directory to the users temp dir
    wchar_t pathbuf[MAX_PATH] = {0};
    GetTempPath(_countof(pathbuf), pathbuf);
    SetCurrentDirectory(pathbuf);

    apr_initialize();
    svn_dso_initialize2();
    apr_pool_t* utfPool = svn_pool_create(NULL);
    svn_utf_initialize2(FALSE, utfPool);
    svn_error_set_malfunction_handler(svnErrorHandleMalfunction);
    g_SVNAdminDir.Init();
    CSVNStatusCache::Create();
    CSVNStatusCache::Instance().Init();

    SecureZeroMemory(szCurrentCrawledPath, sizeof(szCurrentCrawledPath));

    // create a hidden window to receive window messages.
    hWndHidden = CreateHiddenWindow(hInstance);
    hTrayWnd   = hWndHidden;
    if (hWndHidden == nullptr)
    {
        return 0;
    }

    // Create a thread which waits for incoming pipe connections
    unsigned int threadId = 0;

    CAutoGeneralHandle hPipeThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, PipeThread, &bRun, 0, &threadId));
    if (!hPipeThread)
        return 0;

    // Create a thread which waits for incoming pipe connections
    CAutoGeneralHandle hCommandWaitThread =
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, CommandWaitThread, &bRun, 0, &threadId));
    if (hCommandWaitThread)
    {
        if (CRegStdDWORD(L"Software\\TortoiseSVN\\CacheTrayIcon", FALSE) == TRUE)
        {
            SecureZeroMemory(&niData, sizeof(NOTIFYICONDATA));

            niData.cbSize = sizeof(NOTIFYICONDATA);
            niData.uID    = TRAY_ID; // own tray icon ID
            niData.hWnd   = hWndHidden;
            niData.uFlags = NIF_ICON | NIF_MESSAGE;

            // load the icon
            niData.hIcon = LoadIconEx(hInstance,
                                      MAKEINTRESOURCE(IDI_TSVNCACHE));

            // set the message to send
            // note: the message value should be in the
            // range of WM_APP through 0xBFFF
            niData.uCallbackMessage = TRAY_CALLBACK;
            Shell_NotifyIcon(NIM_ADD, &niData);
            // free icon handle
            if (niData.hIcon && DestroyIcon(niData.hIcon))
                niData.hIcon = nullptr;
        }

        // loop to handle window messages.
        MSG msg;
        while (bRun)
        {
            const BOOL bLoopRet = GetMessage(&msg, nullptr, 0, 0);
            if ((bLoopRet != -1) && (bLoopRet != 0))
            {
                DispatchMessage(&msg);
            }
        }
    }

    bRun = false;

    Shell_NotifyIcon(NIM_DELETE, &niData);
    CSVNStatusCache::Destroy();
    g_SVNAdminDir.Close();
    svn_pool_destroy(utfPool);
    apr_terminate();

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case TRAY_CALLBACK:
        {
            switch (lParam)
            {
                case WM_LBUTTONDBLCLK:
                    if (IsWindowVisible(hWnd))
                        ShowWindow(hWnd, SW_HIDE);
                    else
                        ShowWindow(hWnd, SW_RESTORE);
                    break;
                case WM_MOUSEMOVE:
                {
                    CString sInfoTip;
                    sInfoTip.Format(L"Cached Directories : %Id\nWatched paths : %d",
                                    CSVNStatusCache::Instance().GetCacheSize(),
                                    CSVNStatusCache::Instance().GetNumberOfWatchedPaths());

                    NOTIFYICONDATA systemTray = {};
                    systemTray.cbSize         = sizeof(NOTIFYICONDATA);
                    systemTray.hWnd           = hTrayWnd;
                    systemTray.uID            = TRAY_ID;
                    systemTray.uFlags         = NIF_TIP;
                    wcscpy_s(systemTray.szTip, sInfoTip);
                    Shell_NotifyIcon(NIM_MODIFY, &systemTray);
                }
                break;
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU:
                {
                    POINT pt;
                    GetCursorPos(&pt);
                    HMENU hMenu = CreatePopupMenu();
                    if (hMenu)
                    {
                        InsertMenu(hMenu, static_cast<UINT>(-1), MF_BYPOSITION, TRAYPOP_EXIT, L"Exit");
                        SetForegroundWindow(hWnd);
                        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, nullptr);
                        DestroyMenu(hMenu);
                    }
                }
                break;
            }
        }
        break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hdc = BeginPaint(hWnd, &ps);
            RECT        rect;
            GetClientRect(hWnd, &rect);
            // clear the background
            HBRUSH  background = CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
            HGDIOBJ oldBrush   = SelectObject(hdc, background);
            FillRect(hdc, &rect, background);

            int        line     = 0;
            SIZE       fontSize = {0};
            AutoLocker print(critSec);
            GetTextExtentPoint32(hdc, szCurrentCrawledPath[0], static_cast<int>(wcslen(szCurrentCrawledPath[0])), &fontSize);
            for (int i = nCurrentCrawledpathIndex; i < MAX_CRAWLEDPATHS; ++i)
            {
                TextOut(hdc, 0, line * fontSize.cy, szCurrentCrawledPath[i], static_cast<int>(wcslen(szCurrentCrawledPath[i])));
                ++line;
            }
            for (int i = 0; i < nCurrentCrawledpathIndex; ++i)
            {
                TextOut(hdc, 0, line * fontSize.cy, szCurrentCrawledPath[i], static_cast<int>(wcslen(szCurrentCrawledPath[i])));
                ++line;
            }

            SelectObject(hdc, oldBrush);
            EndPaint(hWnd, &ps);
            DeleteObject(background);
            return 0L;
        }
        case WM_COMMAND:
        {
            WORD wmId = LOWORD(wParam);

            switch (wmId)
            {
                case TRAYPOP_EXIT:
                    DestroyWindow(hWnd);
                    break;
            }
            return 1;
        }
        case WM_QUERYENDSESSION:
        {
            ATLTRACE("WM_QUERYENDSESSION\n");
            bRun = false;
            CAutoWriteWeakLock writeLock(CSVNStatusCache::Instance().GetGuard(), 200);
            CSVNStatusCache::Instance().Stop();

            return TRUE;
        }
        case WM_CLOSE:
        case WM_ENDSESSION:
        case WM_DESTROY:
        case WM_QUIT:
        {
            ATLTRACE("WM_CLOSE/DESTROY/ENDSESSION/QUIT\n");
            bRun = false;
            CAutoWriteLock writeLock(CSVNStatusCache::Instance().GetGuard());
            CSVNStatusCache::Instance().Stop();
            CSVNStatusCache::Instance().SaveCache();
            if (message != WM_QUIT)
                PostQuitMessage(0);
            return 1;
        }
        case WM_DEVICECHANGE:
        {
            auto* phdr = reinterpret_cast<DEV_BROADCAST_HDR*>(lParam);
            switch (wParam)
            {
                case DBT_CUSTOMEVENT:
                {
                    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": WM_DEVICECHANGE with DBT_CUSTOMEVENT\n");
                    if (phdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
                    {
                        auto* phandle = reinterpret_cast<DEV_BROADCAST_HANDLE*>(lParam);
                        if (IsEqualGUID(phandle->dbch_eventguid, GUID_IO_VOLUME_DISMOUNT))
                        {
                            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Device to be dismounted\n");
                            CAutoWriteLock writeLock(CSVNStatusCache::Instance().GetGuard());
                            CSVNStatusCache::Instance().CloseWatcherHandles(phandle->dbch_handle);
                        }
                        if (IsEqualGUID(phandle->dbch_eventguid, GUID_IO_VOLUME_LOCK))
                        {
                            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Device lock event\n");
                            CAutoWriteLock writeLock(CSVNStatusCache::Instance().GetGuard());
                            CSVNStatusCache::Instance().CloseWatcherHandles(phandle->dbch_handle);
                        }
                    }
                }
                break;
                case DBT_DEVICEREMOVEPENDING:
                case DBT_DEVICEQUERYREMOVE:
                case DBT_DEVICEREMOVECOMPLETE:
                    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": WM_DEVICECHANGE with DBT_DEVICEREMOVEPENDING/QUERYREMOVE/REMOVECOMPLETE\n");
                    if (phdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
                    {
                        auto*          phandle = reinterpret_cast<DEV_BROADCAST_HANDLE*>(lParam);
                        CAutoWriteLock writeLock(CSVNStatusCache::Instance().GetGuard());
                        CSVNStatusCache::Instance().CloseWatcherHandles(phandle->dbch_handle);
                    }
                    else if (phdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
                    {
                        auto*          pVolume = reinterpret_cast<DEV_BROADCAST_VOLUME*>(lParam);
                        CAutoWriteLock writeLock(CSVNStatusCache::Instance().GetGuard());
                        for (BYTE i = 0; i < 26; ++i)
                        {
                            if (pVolume->dbcv_unitmask & (1 << i))
                            {
                                wchar_t driveLetter = 'A' + i;
                                CString drive       = CString(driveLetter);
                                drive += L":\\";
                                CSVNStatusCache::Instance().CloseWatcherHandles(CTSVNPath(drive));
                            }
                        }
                    }
                    else
                    {
                        CAutoWriteLock writeLock(CSVNStatusCache::Instance().GetGuard());
                        CSVNStatusCache::Instance().CloseWatcherHandles(nullptr);
                    }
                    break;
            }
        }
        break;
        default:
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////

VOID GetAnswerToRequest(const TSVNCacheRequest* pRequest, TSVNCacheResponse* pReply, DWORD* pResponseLength)
{
    CTSVNPath path;
    *pResponseLength = 0;
    if (pRequest->flags & TSVNCACHE_FLAGS_FOLDERISKNOWN)
    {
        path.SetFromWin(pRequest->path, !!(pRequest->flags & TSVNCACHE_FLAGS_ISFOLDER));
    }
    else
    {
        path.SetFromWin(pRequest->path);
    }

    CAutoReadWeakLock readLock(CSVNStatusCache::Instance().GetGuard(), 2000);

    if (readLock.IsAcquired())
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": app asked for status of %s\n", pRequest->path);
        CSVNStatusCache::Instance().GetStatusForPath(path, pRequest->flags, false).BuildCacheResponse(*pReply, *pResponseLength);
    }
    else
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": timeout for asked status of %s\n", pRequest->path);
        CStatusCacheEntry entry;
        entry.BuildCacheResponse(*pReply, *pResponseLength);
    }
}

unsigned int __stdcall PipeThread(LPVOID lpvParam)
{
    CCrashReportThread crashThread;
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": PipeThread started\n");
    bool* bThreadRun = static_cast<bool*>(lpvParam);
    // The main loop creates an instance of the named pipe and
    // then waits for a client to connect to it. When the client
    // connects, a thread is created to handle communications
    // with that client, and the loop is repeated.
    unsigned int dwThreadId = 0;
    BOOL         fConnected = FALSE;
    CAutoFile    hPipe;

    while (*bThreadRun)
    {
        hPipe = CreateNamedPipe(
            GetCachePipeName(),
            PIPE_ACCESS_DUPLEX,         // read/write access
            PIPE_TYPE_MESSAGE |         // message type pipe
                PIPE_READMODE_MESSAGE | // message-read mode
                PIPE_WAIT,              // blocking mode
            PIPE_UNLIMITED_INSTANCES,   // max. instances
            BUFSIZE,                    // output buffer size
            BUFSIZE,                    // input buffer size
            NMPWAIT_USE_DEFAULT_WAIT,   // client time-out
            nullptr);                   // NULL DACL

        if (!hPipe)
        {
            //OutputDebugStringA("TSVNCache: CreatePipe failed\n");
            //DebugOutputLastError();
            if (*bThreadRun)
                Sleep(200);
            continue; // never leave the thread!
        }

        // Wait for the client to connect; if it succeeds,
        // the function returns a nonzero value. If the function returns
        // zero, GetLastError returns ERROR_PIPE_CONNECTED.
        fConnected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (fConnected && (*bThreadRun))
        {
            // Create a thread for this client.
            CAutoGeneralHandle hInstanceThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, InstanceThread, static_cast<HANDLE>(hPipe), 0, &dwThreadId));

            if (!hInstanceThread)
            {
                //OutputDebugStringA("TSVNCache: Could not create Instance thread\n");
                //DebugOutputLastError();
                DisconnectNamedPipe(hPipe);
                // since we're now closing this thread, we also have to close the whole application!
                // otherwise the thread is dead, but the app is still running, refusing new instances
                // but no pipe will be available anymore.
                PostMessage(hWndHidden, WM_CLOSE, 0, 0);
                return 1;
            }
            // detach the handle, since we passed it to the thread
            hPipe.Detach();
        }
        else
        {
            // The client could not connect, so close the pipe.
            //OutputDebugStringA("TSVNCache: ConnectNamedPipe failed\n");
            //DebugOutputLastError();
            hPipe.CloseHandle();
            if (*bThreadRun)
                Sleep(200);
            continue; // don't end the thread!
        }
    }
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Pipe thread exited\n");
    return 0;
}

unsigned int __stdcall CommandWaitThread(LPVOID lpvParam)
{
    CCrashReportThread crashthread;
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CommandWaitThread started\n");
    bool* bThreadRun = static_cast<bool*>(lpvParam);
    // The main loop creates an instance of the named pipe and
    // then waits for a client to connect to it. When the client
    // connects, a thread is created to handle communications
    // with that client, and the loop is repeated.
    unsigned int dwThreadId = 0;
    BOOL         fConnected = FALSE;
    CAutoFile    hPipe;

    while (*bThreadRun)
    {
        hPipe = CreateNamedPipe(
            GetCacheCommandPipeName(),
            PIPE_ACCESS_DUPLEX,         // read/write access
            PIPE_TYPE_MESSAGE |         // message type pipe
                PIPE_READMODE_MESSAGE | // message-read mode
                PIPE_WAIT,              // blocking mode
            PIPE_UNLIMITED_INSTANCES,   // max. instances
            BUFSIZE,                    // output buffer size
            BUFSIZE,                    // input buffer size
            NMPWAIT_USE_DEFAULT_WAIT,   // client time-out
            nullptr);                   // NULL DACL

        if (!hPipe)
        {
            //OutputDebugStringA("TSVNCache: CreatePipe failed\n");
            //DebugOutputLastError();
            if (*bThreadRun)
                Sleep(200);
            continue; // never leave the thread!
        }

        // Wait for the client to connect; if it succeeds,
        // the function returns a nonzero value. If the function returns
        // zero, GetLastError returns ERROR_PIPE_CONNECTED.
        fConnected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (fConnected && (*bThreadRun))
        {
            // Create a thread for this client.
            CAutoGeneralHandle hCommandThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, CommandThread, static_cast<HANDLE>(hPipe), 0, &dwThreadId));

            if (!hCommandThread)
            {
                //OutputDebugStringA("TSVNCache: Could not create Command thread\n");
                //DebugOutputLastError();
                DisconnectNamedPipe(hPipe);
                // since we're now closing this thread, we also have to close the whole application!
                // otherwise the thread is dead, but the app is still running, refusing new instances
                // but no pipe will be available anymore.
                PostMessage(hWndHidden, WM_CLOSE, 0, 0);
                return 1;
            }
            // detach the handle, since we passed it to the thread
            hPipe.Detach();
        }
        else
        {
            // The client could not connect, so close the pipe.
            //OutputDebugStringA("TSVNCache: ConnectNamedPipe failed\n");
            //DebugOutputLastError();
            hPipe.CloseHandle();
            if (*bThreadRun)
                Sleep(200);
            continue; // don't end the thread!
        }
    }
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CommandWait thread exited\n");
    return 0;
}

unsigned int __stdcall InstanceThread(LPVOID lpvParam)
{
    CCrashReportThread crashThread;
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": InstanceThread started\n");
    TSVNCacheResponse response{};
    DWORD             cbBytesRead = 0, cbWritten = 0;

    // The thread's parameter is a handle to a pipe instance.

    CAutoFile hPipe = std::move(static_cast<HANDLE>(lpvParam));

    while (bRun)
    {
        // Read client requests from the pipe.
        TSVNCacheRequest request;
        BOOL             fSuccess = ReadFile(
            hPipe,           // handle to pipe
            &request,        // buffer to receive data
            sizeof(request), // size of buffer
            &cbBytesRead,    // number of bytes read
            nullptr);        // not overlapped I/O

        if (!fSuccess || cbBytesRead == 0)
        {
            DisconnectNamedPipe(hPipe);
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Instance thread exited\n");
            return 1;
        }

        // sanitize request:
        // * Make sure the string properly 0-terminated
        //   by resetting overlong paths to the empty string
        // * Set all trailing chars to 0.
        // * Clear unknown flags
        // This is more or less paranoia code but maybe something
        // is feeding garbage into our queue.
        for (size_t i = MAX_PATH; (i > 0) && (request.path[i - 1] != 0); --i)
            request.path[i - 1] = 0;

        size_t pathLength = wcslen(request.path);
        SecureZeroMemory(request.path + pathLength, sizeof(request.path) - pathLength * sizeof(wchar_t));

        request.flags &= TSVNCACHE_FLAGS_MASK;

        // process request
        DWORD responseLength;
        GetAnswerToRequest(&request, &response, &responseLength);

        // Write the reply to the pipe.
        fSuccess = WriteFile(
            hPipe,          // handle to pipe
            &response,      // buffer to write from
            responseLength, // number of bytes to write
            &cbWritten,     // number of bytes written
            nullptr);       // not overlapped I/O

        if (!fSuccess || responseLength != cbWritten)
        {
            DisconnectNamedPipe(hPipe);
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Instance thread exited\n");
            return 1;
        }
    }

    // Flush the pipe to allow the client to read the pipe's contents
    // before disconnecting. Then disconnect the pipe, and close the
    // handle to this pipe instance.

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Instance thread exited\n");
    return 0;
}

unsigned int __stdcall CommandThread(LPVOID lpvParam)
{
    CCrashReportThread crashThread;
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CommandThread started\n");
    DWORD     cbBytesRead;
    CAutoFile hPipe;

    // The thread's parameter is a handle to a pipe instance.

    hPipe = std::move(static_cast<HANDLE>(lpvParam));

    while (bRun)
    {
        // Read client requests from the pipe.
        TSVNCacheCommand command;
        BOOL             fSuccess = ReadFile(
            hPipe,           // handle to pipe
            &command,        // buffer to receive data
            sizeof(command), // size of buffer
            &cbBytesRead,    // number of bytes read
            nullptr);        // not overlapped I/O

        if (!fSuccess || cbBytesRead == 0)
        {
            DisconnectNamedPipe(hPipe);
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Command thread exited\n");
            return 1;
        }

        // sanitize request:
        // * Make sure the string properly 0-terminated
        //   by resetting overlong paths to the empty string
        // * Set all trailing chars to 0.
        // This is more or less paranoia code but maybe something
        // is feeding garbage into our queue.
        for (size_t i = MAX_PATH; (i > 0) && (command.path[i - 1] != 0); --i)
            command.path[i - 1] = 0;

        size_t pathLength = wcslen(command.path);
        SecureZeroMemory(command.path + pathLength, sizeof(command.path) - pathLength * sizeof(wchar_t));

        // process request
        switch (command.command)
        {
            case TSVNCACHECOMMAND_END:
                FlushFileBuffers(hPipe);
                DisconnectNamedPipe(hPipe);
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Command thread exited\n");
                return 0;
            case TSVNCACHECOMMAND_CRAWL:
            {
                CTSVNPath changedPath;
                changedPath.SetFromWin(command.path, true);
                // remove the path from our cache - that will 'invalidate' it.
                {
                    CAutoWriteLock writeLock(CSVNStatusCache::Instance().GetGuard());
                    CSVNStatusCache::Instance().RemoveCacheForPath(changedPath);
                }
                CSVNStatusCache::Instance().AddFolderForCrawling(changedPath.GetDirectory());
            }
            break;
            case TSVNCACHECOMMAND_REFRESHALL:
            {
                CAutoWriteLock writeLock(CSVNStatusCache::Instance().GetGuard());
                CSVNStatusCache::Instance().Refresh();
            }
            break;
            case TSVNCACHECOMMAND_RELEASE:
            {
                CTSVNPath changedPath;
                changedPath.SetFromWin(command.path, true);
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": release handle for path %s\n", changedPath.GetWinPath());
                CAutoWriteLock writeLock(CSVNStatusCache::Instance().GetGuard());
                CSVNStatusCache::Instance().CloseWatcherHandles(changedPath);
                CSVNStatusCache::Instance().RemoveCacheForPath(changedPath);
            }
            break;
            case TSVNCACHECOMMAND_BLOCK:
            {
                CTSVNPath changedPath;
                changedPath.SetFromWin(command.path);
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": block path %s\n", changedPath.GetWinPath());
                CSVNStatusCache::Instance().BlockPath(changedPath, false);
            }
            break;
            case TSVNCACHECOMMAND_UNBLOCK:
            {
                CTSVNPath changedPath;
                changedPath.SetFromWin(command.path);
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": unblock path %s\n", changedPath.GetWinPath());
                CSVNStatusCache::Instance().UnBlockPath(changedPath);
            }
            break;
        }
    }

    // Flush the pipe to allow the client to read the pipe's contents
    // before disconnecting. Then disconnect the pipe, and close the
    // handle to this pipe instance.

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Command thread exited\n");
    return 0;
}
