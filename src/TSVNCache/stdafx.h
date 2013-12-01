// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER                  // Specifies that the minimum required platform is Windows Vista.
#define WINVER 0x0600           // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS          // Specifies that the minimum required platform is Windows 98.
#define _WIN32_WINDOWS 0x0410   // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE               // Specifies that the minimum required platform is Internet Explorer 7.0.
#define _WIN32_IE 0x0700        // Change this to the appropriate value to target other versions of IE.
#endif

#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>

#include <atlbase.h>
#include <atlstr.h>

#include <conio.h>

#define CSTRING_AVAILABLE


using namespace ATL;

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <deque>

#pragma warning(push)
#include "svn_wc.h"
#include "svn_client.h"
#include "svn_path.h"
#include "svn_pools.h"
#include "svn_utf.h"
#include "svn_props.h"
#pragma warning(pop)

#include "DebugOutput.h"

typedef CComCritSecLock<CComAutoCriticalSection> AutoLocker;

#ifdef _WIN64
#   define APP_X64_STRING   "x64"
#else
#   define APP_X64_STRING   ""
#endif

#include "ProfilingInfo.h"
#include "CrashReport.h"
