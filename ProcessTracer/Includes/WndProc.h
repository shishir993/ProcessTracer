
// WndProc.h
// Contains the WndProc() function for the main debugger UI window
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/08/13 Initial version
//

#ifndef _WNDPROC_H
#define _WNDPROC_H

#include "CommonInclude.h"
#include <io.h>
#include <fcntl.h>
#include "MenuItems.h"
#include "DebugThread.h"
#include "HelperFuncs.h"
#include "Logger.h"
#include "WatchDogThread.h"

#define SB_NPARTS   3

#define TIME_ONESEC_MILLI   1000

// Functions
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK fAboutDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK fGetProcIdDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL fInitUIElements(HWND hWnd);

BOOL fOnLoadNewProcess(HWND hMainWnd);
BOOL fOnStartDebugSession();
BOOL fOnTerminateDebuggee();
BOOL fOnDetachFromDebuggee();

BOOL fDisplayActiveProcesses(HWND hProcIDList, DWORD **paProcIDs, DWORD *pnProcIDs, DWORD *pnItemsShown);

#endif // _WNDPROC_H