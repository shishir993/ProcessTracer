
// HelperFuncs.h
// Contains functions that help in performing common tasks
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/08/13 Initial version
//

#ifndef _HELPERFUNCS_H
#define _HELPERFUNCS_H

#include "CommonInclude.h"
//#include "DASMUtils.h"

#define IS_VALID_HANDLE(handle) (handle && handle != INVALID_HANDLE_VALUE)
#define PURGE_HANDLE(handle)                                                \
                            if(IS_VALID_HANDLE(handle))                     \
                            {                                               \
                                CloseHandle(handle);                        \
                                handle = INVALID_HANDLE_VALUE;              \
                            }
#define BYTELEN_wcsnlen_s(wstring, maxcount)   ( (wcsnlen_s(wstring, maxcount) + 1) * sizeof(WCHAR) )

// Functions
BOOL PrintToListBox(HWND hListBox, const WCHAR *wszMsg);
BOOL PrintToStatusBar(HWND hStatusBar, const WCHAR *wszMsg, int iPart);
BOOL AppendToEditControl(HWND hEditControl, const WCHAR *wszMsg);
BOOL DoCenterWindow(HWND hWnd);

BOOL CreateGWEvents(LPHANDLE lpEventHandles, DWORD pid);
BOOL CreateWDEvents(LPHANDLE lpEventHandles, DWORD pid);

void vCloseHandles(LPHANDLE pHandles, int nHandles);

BOOL GetProcNameFromID(DWORD pid, WCHAR *pwsProcName, DWORD dwCount);
BOOL fFindNtDllInCurProcess(DWORD *pdwBaseAddress, DWORD *pdwCodeSize);

#endif // _HELPERFUNCS_H
