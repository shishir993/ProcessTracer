// WatchDogThread.h
// This is the module that sits in between the GUI thread (WndProc)
// and the Debug thread (DebugThread). All communication between
// this and (GUI and DebugThread) is via events. There MUST be NO
// INIFINITE waits anywhere.
//
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      08/10/13 Initial version
//

#ifndef _WATCHDOGTHREAD_H
#define _WATCHDOGTHREAD_H

#include "CommonInclude.h"
#include "HelperFuncs.h"
#include "DebugThread.h"
#include "MenuItems.h"

typedef struct _WDThreadArgs {
    HWND hGUIWindow;
    HMENU hMainMenu;
    HINSTANCE hMainInstance;
}WD_THREADARGS;

typedef struct _BeginDebugArgs {
    BOOL fDebugActiveProcess;       // debugging active or new process?
    DWORD dwTargetProcessID;        // in case of debugging active process
    WCHAR wsTargetPath[MAX_PATH+1]; // in case of debugging a new process
    WCHAR wsTargetFilename[MAX_PATH+1]; // filename only
}WD_BEGINDBG_ARGS;

DWORD WINAPI WatchDogThreadEntry(LPVOID lpArgs);

#endif // _WATCHDOGTHREAD_H
