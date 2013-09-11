// DebugThreadHelpers.h
// Contains functions that help in performing common tasks within the debug thread
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      08/04/13 Initial version
//

#ifndef _DEBUGTHREADHELPERS_H
#define _DEBUGTHREADHELPERS_H

#include "CommonInclude.h"
#include "DebugThread.h"

BOOL fGetExceptionName(DWORD excode, __out WCHAR *pwsBuffer, int bufSize);

BOOL fAddThread(CHL_HTABLE *phtThreads, DWORD id, HANDLE hthread);
BOOL fRemoveThread(CHL_HTABLE *phtThreads, DWORD id);
BOOL fGetThreadHandle(CHL_HTABLE *phtThreads, DWORD id, HANDLE *phThread);
BOOL fDeleteThreadsTable(CHL_HTABLE *phtThreads);
BOOL fSuspendAllThreads(CHL_HTABLE *phtThreads, HWND hDbgEventsTextBox);
BOOL fResumeAllThreads(CHL_HTABLE *phtThreads, HWND hDbgEventsTextBox);
BOOL fIsNtDllLoaded(CHL_HTABLE *phtDllTable, DWORD *pdwBaseAddress);

#endif // _DEBUGTHREADHELPERS_H
