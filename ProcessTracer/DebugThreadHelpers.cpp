// DebugThreadHelpers.cpp
// Contains functions that help in performing common tasks within the debug thread
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      08/04/13 Initial version
//

#include "DebugThreadHelpers.h"

BOOL fGetExceptionName(DWORD excode, __out WCHAR *pwsBuffer, int bufSize)
{
    int stringID = 0;

    switch(excode)
    {
        case EXCEPTION_ACCESS_VIOLATION: stringID = IDS_EX_ACCESSVIOL; break;

        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: stringID = IDS_EX_ARRBOUNDS; break;

        case EXCEPTION_BREAKPOINT: stringID = IDS_EX_BREAKPNT; break;

        case EXCEPTION_DATATYPE_MISALIGNMENT: stringID = IDS_EX_DATAMISALIGN; break;

        case EXCEPTION_FLT_DENORMAL_OPERAND: stringID = IDS_EX_FLT_DENOP; break;

        case EXCEPTION_FLT_DIVIDE_BY_ZERO: stringID = IDS_EX_FLT_DIVZERO; break;

        case EXCEPTION_FLT_INEXACT_RESULT: stringID = IDS_EX_FLT_INEXACT; break;

        case EXCEPTION_FLT_INVALID_OPERATION: stringID = IDS_EX_FLT_INVOP; break;

        case EXCEPTION_FLT_OVERFLOW: stringID = IDS_EX_FLT_OVRFLOW; break;

        case EXCEPTION_FLT_STACK_CHECK: stringID = IDS_EX_FLT_STKFLOW; break;

        case EXCEPTION_FLT_UNDERFLOW: stringID = IDS_EX_FLT_UNDFLOW; break;

        case EXCEPTION_ILLEGAL_INSTRUCTION: stringID = IDS_EX_INVINST; break;

        case EXCEPTION_IN_PAGE_ERROR: stringID = IDS_EX_INPAGEERR; break;

        case EXCEPTION_INT_DIVIDE_BY_ZERO: stringID = IDS_EX_INT_DIVZERO; break;

        case EXCEPTION_INT_OVERFLOW: stringID = IDS_EX_INT_OVRFLOW; break;

        case EXCEPTION_INVALID_DISPOSITION: stringID = IDS_EX_INVDISP; break;

        case EXCEPTION_NONCONTINUABLE_EXCEPTION: stringID = IDS_EX_NONCONT; break;

        case EXCEPTION_PRIV_INSTRUCTION: stringID = IDS_EX_PRIVINST; break;

        case EXCEPTION_SINGLE_STEP: stringID = IDS_EX_SINGLESTEP; break;

        case EXCEPTION_STACK_OVERFLOW: stringID = IDS_EX_STKOVRLFLOW; break;

        default:
            swprintf_s(pwsBuffer, bufSize, L"0x%08x: Unknown Exception", excode);
            return FALSE;
    }

    return (LoadString(GetModuleHandle(NULL), stringID, pwsBuffer, bufSize) != 0);
}


BOOL fAddThread(CHL_HTABLE *phtThreads, DWORD id, HANDLE hthread)
{
    ASSERT(phtThreads && IS_VALID_HANDLE(hthread));

    DEBUGGEE_THREAD *pdbgthread = NULL;

    if((pdbgthread = (DEBUGGEE_THREAD*)malloc(sizeof(DEBUGGEE_THREAD))) == NULL)
        return FALSE;

    pdbgthread->dwThreadID = id;
    pdbgthread->hThread = hthread;

    return HT_fInsert(phtThreads, &id, sizeof(DWORD), &pdbgthread, sizeof(DEBUGGEE_THREAD*));
}


BOOL fRemoveThread(CHL_HTABLE *phtThreads, DWORD id)
{
    ASSERT(phtThreads);

    return HT_fRemove(phtThreads, &id, sizeof(DWORD));
}


BOOL fGetThreadHandle(CHL_HTABLE *phtThreads, DWORD id, HANDLE *phThread)
{
    ASSERT(phtThreads);
    ASSERT(phThread);

    DEBUGGEE_THREAD *pthreadinfo = NULL;
    int outValSize = 0;

    if(!HT_fFind(phtThreads, &id, sizeof(DWORD), &pthreadinfo, &outValSize))
        return FALSE;

    ASSERT(outValSize == sizeof(DEBUGGEE_THREAD*));
    ASSERT(pthreadinfo);

    *phThread = pthreadinfo->hThread;
    return TRUE;
}


BOOL fDeleteThreadsTable(CHL_HTABLE *phtThreads)
{
    ASSERT(phtThreads);

    CHL_HT_ITERATOR itr;

    DWORD dwThreadID = 0;
    DEBUGGEE_THREAD **pthreadInfo = NULL;

    int keysize, valsize;

    HT_fInitIterator(&itr);
    while(HT_fGetNext(phtThreads, &itr, &dwThreadID, &keysize, &pthreadInfo, &valsize))
    {
        ASSERT(pthreadInfo && *pthreadInfo);
        free(*pthreadInfo);
    }
    
    return HT_fDestroy(phtThreads);
}


BOOL fSuspendAllThreads(CHL_HTABLE *phtThreads, HWND hDbgEventsTextBox)
{
    ASSERT(phtThreads);

    CHL_HT_ITERATOR itr;

    DWORD dwThreadID = 0;
    DEBUGGEE_THREAD **pthreadInfo = NULL;

    int keysize, valsize;
    BOOL fRetVal = TRUE;

    WCHAR wsMessage[64];

    HT_fInitIterator(&itr);
    while(HT_fGetNext(phtThreads, &itr, &dwThreadID, &keysize, &pthreadInfo, &valsize))
    {
        ASSERT(pthreadInfo && *pthreadInfo);
        ASSERT(IS_VALID_HANDLE((*pthreadInfo)->hThread));

        swprintf_s(wsMessage, _countof(wsMessage), L"Suspending thread %d\n", dwThreadID);
        wprintf(wsMessage);
        AppendToEditControl(hDbgEventsTextBox, wsMessage);
        if( SuspendThread((*pthreadInfo)->hThread) == -1 )
        {
            DWORD dwerror = GetLastError();
            logtrace(L"SuspendThread() -1");
            fRetVal = FALSE;
        }
    }

    return fRetVal;
}


BOOL fResumeAllThreads(CHL_HTABLE *phtThreads, HWND hDbgEventsTextBox)
{
    ASSERT(phtThreads);

    CHL_HT_ITERATOR itr;

    DWORD dwThreadID = 0;
    DEBUGGEE_THREAD **pthreadInfo = NULL;

    int keysize, valsize;
    BOOL fRetVal = TRUE;

    WCHAR wsMessage[64];

    HT_fInitIterator(&itr);
    while(HT_fGetNext(phtThreads, &itr, &dwThreadID, &keysize, &pthreadInfo, &valsize))
    {
        ASSERT(pthreadInfo && *pthreadInfo);
        ASSERT(IS_VALID_HANDLE((*pthreadInfo)->hThread));

        swprintf_s(wsMessage, _countof(wsMessage), L"Resuming thread %d\n", dwThreadID);
        wprintf(wsMessage);
        AppendToEditControl(hDbgEventsTextBox, wsMessage);
        if( ResumeThread((*pthreadInfo)->hThread) == -1 )
        {
            DWORD dwerror = GetLastError();
            logtrace(L"ResumeThread() -1");
            fRetVal = FALSE;
        }
    }

    return fRetVal;
}

BOOL fIsNtDllLoaded(CHL_HTABLE *phtDllTable, DWORD *pdwBaseAddress)
{
    ASSERT(phtDllTable);

    CHL_HT_ITERATOR itr;

    DWORD dwBase = 0;
    WCHAR *psDllName = NULL;
    WCHAR *psDllFilename = NULL;

    int keysize, valsize;

    HT_fInitIterator(&itr);
    while(HT_fGetNext(phtDllTable, &itr, &dwBase, &keysize, &psDllName, &valsize))
    {
        ASSERT(psDllName && valsize > 0);
        if((psDllFilename = Chl_GetFilenameFromPath(psDllName, valsize)) == NULL)
        {
            wprintf(L"fIsNtDllLoaded(): Error reading filename from path: %s\n", psDllName);
            continue;
        }
        else if(wcscmp(L"ntdll.dll", psDllFilename) == 0)
        {
            if(pdwBaseAddress) *pdwBaseAddress = dwBase;
            return TRUE;
        }
    }

    if(pdwBaseAddress) *pdwBaseAddress = NULL;
    return FALSE;
}
