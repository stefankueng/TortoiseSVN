// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#define NOMINMAX
#include <algorithm>
using std::max;
using std::min;

#include <SDKDDKVer.h>

#define ISOLATION_AWARE_ENABLED 1
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#include <windows.h>

#include <commctrl.h>
#pragma warning(push)
#pragma warning(disable: 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <shlobj.h>
#pragma warning(pop)
#include <Shlwapi.h>
#include <wininet.h>
#include <Aclapi.h>

#include <atlbase.h>
#include <atlexcept.h>
#include <atlstr.h>

#include <string>
#include <set>
#include <map>
#include <vector>
#include <algorithm>

#pragma warning(push)
#include "apr_general.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_config.h"
#include "svn_subst.h"
#include "svn_props.h"
#pragma warning(pop)

#include "DebugOutput.h"

#define CSTRING_AVAILABLE
