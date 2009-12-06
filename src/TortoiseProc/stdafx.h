// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once
#define XMESSAGEBOX_APPREGPATH "Software\\TortoiseSVN\\"

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

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdlgs.h>
#include <afxctl.h>
#include <afxtempl.h>
#include <afxmt.h>
#include <afxext.h>         // MFC extensions
#include <afxcontrolbars.h>     // MFC support for ribbons and control bars

#include <atlbase.h>

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER     0x00010000
#endif

#pragma warning(push)
#include "apr_general.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_config.h"
#include "svn_error_codes.h"
#include "svn_subst.h"
#include "svn_repos.h"
#include "svn_time.h"
#include "svn_props.h"
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4702)	// Unreachable code warnings in xtree
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <deque>
#include <regex>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4201)	// nonstandard extension used : nameless struct/union (in MMSystem.h)
#include <vfw.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include <shlguid.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <dlgs.h>
#include <wininet.h>
#include <assert.h>
#include <math.h>
#include <gdiplus.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4005)	// macro redefinition
#include "..\..\ext\apr\include\apr_version.h"
#include "..\..\ext\apr-util\include\apu_version.h"
//#include "..\..\ext\berkeley-db\db4.3-win32\include\db.h"
#include "..\..\ext\neon\config.h"
#include "..\..\..\common\openssl\inc32\openssl\opensslv.h"
#include "..\..\..\common\zlib\zlib.h"
#pragma warning(pop)

#define __WIN32__
#include "boost/pool/object_pool.hpp"

#define USE_GDI_GRADIENT
#define HISTORYCOMBO_WITH_SYSIMAGELIST

#include "ProfilingInfo.h"

#ifdef _WIN64
#	define APP_X64_STRING	"x64"
#else
#	define APP_X64_STRING ""
#endif

#pragma warning(disable: 4512)	// assignment operator could not be generated
#pragma warning(disable: 4355)	// used in base member initializer list

