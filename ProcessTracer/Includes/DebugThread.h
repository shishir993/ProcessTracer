
// DebugThread.h
// Contains functions that handle the actual debugging functions.
// Contains the entry point for the debug thread.
// The debug thread communicates with the main thread via events.
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/09/13 Initial version
//

#ifndef _DEBUGTHREAD_H
#define _DEBUGTHREAD_H

#include "CommonInclude.h"
#include "DbgHelpers.h"
#include "HelperFuncs.h"
#include "Logger.h"
#include "DebugThreadHelpers.h"

#define MAX_EXCEPTION_NAMESTR   127
#define MAX_EXCEPTION_MSG       255

// thread exit error codes
#define TXERR_START_DEBUG       101
#define TXERR_TERM_DEBUG        102
#define TXERR_CHLP_LIB          110

typedef struct _DebugThreadArgs {
    BOOL fDebugActiveProcess;           // IN
    BOOL fErrorStartingDebugSession;    // OUT
    DWORD dwDebuggeePID;                // OUT
    union {                             // IN
        WCHAR *pwsDebuggeePath;
        DWORD dwDebuggeePID;
    }info;
}DEBUGTHREAD_ARGS;

typedef struct _DebuggeeThread {
    DWORD dwThreadID;
    HANDLE hThread;
}DEBUGGEE_THREAD;

typedef struct _DebuggeeInfo {
    BOOL fDebuggingActiveProcess;
    DWORD dwProcessID;
    DWORD dwMainThreadID;
    HANDLE hProcess;
    HANDLE hMainThread;
    CHL_HTABLE *phtThreads;     // key = threadID, value = address of the DEBUGGEE_THREAD structure
    CREATE_PROCESS_DEBUG_INFO procDebugInfo;
}DEBUGGEE_INFO;

// Functions
DWORD WINAPI dwDebugThreadEntry(LPVOID lpArgs);

#endif // _DEBUGTHREAD_H
