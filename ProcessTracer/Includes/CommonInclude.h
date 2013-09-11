
// CommonInclude.h
// Contains common includes, defs and typedefs
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      06/23/13 Initial version
//

#ifndef _COMMONINCLUDE_H
#define _COMMONINCLUDE_H

#include <stdio.h>
#include <errno.h>
#include <Windows.h>
#include <WinUser.h>
#include <wchar.h>
#include <Psapi.h>  // EnumProcesses()
#include <CommCtrl.h>
#include <strsafe.h>

#include "..\Libs\CHelpLibDll.h"
#include "Assert.h"
#include "DbgHelpers.h"
#include "..\Resources\resource.h"

#define STR_OPMSG       L"ProcessTracer\nTrace a 32bit's program's execution. :P\nCoder\t: Shishir K Prasad (http://www.shishirprasad.net)\n"
#define STR_VER         L"Version\t: 1.0\n"
#define STR_APPNAME     L"ProcessTracer - v1.0"

#define SD_TESTING
#define MAX_SDBG_MSG    255

// Event strings
#define MAX_GW_EVENTS   6
#define MAX_WD_EVENTS   7   // includes WD and DW events

#define INDEX_GW_TERM       0
#define INDEX_GW_SUSPEND    1
#define INDEX_GW_RESUME     2
#define INDEX_GW_DETACH     3
#define INDEX_GW_START      4
#define INDEX_GW_EXITTHREAD 5

#define INDEX_WD_INITACK    0
#define INDEX_WD_TERMACK    1
#define INDEX_WD_TERM       2
#define INDEX_WD_SUSPEND    3
#define INDEX_WD_RESUME     4
#define INDEX_WD_DETACH     5
#define INDEX_DW_TERMINATED 6

#endif // _COMMONINCLUDE_H
