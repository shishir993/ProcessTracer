
// DebugThread.h
// Contains functions that handle the actual debugging functions.
// Contains the entry point for the debug thread.
// The debug thread communicates with the main thread via events.
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/10/13 Initial version
//      08/10/13 Major design change. GUI - WatchDog - Debug threads.
//               Event communication between each other. No communication
//               between GUI - Debug except maybe a few GUI calls.
//               Trying to keep everything strictly asynchronous - 
//               no indirect INFINITE WaitFor*Object[s]() calls.

#include "DebugThread.h"

extern HWND hDebugEventsTextBox;

// ****************************************************************************
// ToDo
// We will eventually have to move these global variables to within the thread
// entry point when we want to be able to debug multiple processes at the same
// time.
// 
// ****************************************************************************

static DEBUGGEE_INFO stDebuggeeInfo;
static DWORD dwExceptions;
static DWORD dwProcsCreated;
static DWORD dwThreadsCreated;
static DWORD dwDllsLoaded;

static HANDLE ahWDEvents[MAX_WD_EVENTS];
static WCHAR wsEventName[MAX_PATH];
static BOOL fCreateProcFirstTime = TRUE;

static BOOL fEndDebugLoop = FALSE;

static CHL_HTABLE *phtDLLTable = NULL;

// File-local Functions
static BOOL fDoDebugNewProcess(DEBUGTHREAD_ARGS *pTargetInfo);
static BOOL fAttachToActiveProcess(DEBUGTHREAD_ARGS *pTargetInfo);
static BOOL fDoDebugActiveProcess();
static BOOL fDoDebugEvtLoop();
static BOOL fOnExceptionEvent(DEBUG_EVENT *pde, DWORD *pdwContinueStatus);
static void vCleanUpDebugThread();

// dwDebugThreadEntry()
// 
DWORD WINAPI dwDebugThreadEntry(LPVOID lpArgs)
{
    ASSERT(lpArgs);
    ASSERT(hDebugEventsTextBox && hDebugEventsTextBox != INVALID_HANDLE_VALUE);

    int nMainThreadEvtIndex = -1;
    HANDLE hInitEvent = INVALID_HANDLE_VALUE;
    
    DEBUGTHREAD_ARGS *pThreadArgs = (DEBUGTHREAD_ARGS*)lpArgs;

    // arguments validation
    #ifdef _DEBUG
        if(pThreadArgs->fDebugActiveProcess)
            ASSERT(pThreadArgs->info.dwDebuggeePID > 0);
        else
            ASSERT(pThreadArgs->info.pwsDebuggeePath);
    #endif

    LoadString(GetModuleHandle(NULL), IDS_WD_INITACK, wsEventName, _countof(wsEventName));
    hInitEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, wsEventName);

    // Create the hash table to hold loaded DLLs
    if(!HT_fCreate(&phtDLLTable, 2, HT_KEY_DWORD, HT_VAL_STR))
        goto error_init_exit;

    __try
    {
        if(pThreadArgs->fDebugActiveProcess)
        {
            fAttachToActiveProcess(pThreadArgs);
        }
        else
        {
            fDoDebugNewProcess(pThreadArgs);
        }   
    }
    __finally
    {
        if(pThreadArgs->fErrorStartingDebugSession)
        {
            logerror(L"Cannot start new process/attach to process to debug");
            SetEvent(hInitEvent);
            PURGE_HANDLE(hInitEvent);
            vCleanUpDebugThread();
            ExitThread(TXERR_START_DEBUG);
        }
    }
    
    // initialization done
    SetEvent(hInitEvent);
    PURGE_HANDLE(hInitEvent);

    // create/open all comm event handles
    // Target PID has been placed in pThreadArgs->dwDebuggeePID
    if(!CreateWDEvents(ahWDEvents, pThreadArgs->dwDebuggeePID))
    {
        logerror(L"Unable to create comm event handles");
        goto error_exit;
    }

    if(stDebuggeeInfo.fDebuggingActiveProcess)
    {
        if(!fDoDebugActiveProcess())
        {
            logerror(L"Couldn't begin debugging active process");
            goto error_exit;
        }
    }

    // start listen_main_loop and debug_event_loop
    while(!fEndDebugLoop)
    {
        switch((nMainThreadEvtIndex = WaitForMultipleObjects(_countof(ahWDEvents), ahWDEvents, FALSE, 10)))
        {
            case INDEX_WD_TERM:
                // terminate debuggee
                logtrace(L"dwDebugThreadEntry(): Received TERM event from watch dog thread");
                if(!TerminateProcess(stDebuggeeInfo.hProcess, 666))
                {
                    DWORD error = GetLastError();
                    logerror(L"Unable to terminate process. Exiting debug thread as a last ditch effort...");
                    fEndDebugLoop = TRUE;
                }
                ResetEvent(ahWDEvents[INDEX_WD_TERM]);
                break;

            case INDEX_WD_SUSPEND:
                if(!fSuspendAllThreads(stDebuggeeInfo.phtThreads, hDebugEventsTextBox))
                    logerror(L"Couldn't suspend all threads");
                ResetEvent(ahWDEvents[INDEX_WD_SUSPEND]);
                break;

            case INDEX_WD_RESUME:
                if(!fResumeAllThreads(stDebuggeeInfo.phtThreads, hDebugEventsTextBox))
                    logerror(L"Couldn't resume all threads");
                ResetEvent(ahWDEvents[INDEX_WD_RESUME]);
                break;

            case INDEX_WD_DETACH:
            {
                WCHAR wsDbgMessage[128];

                swprintf_s(wsDbgMessage, _countof(wsDbgMessage), L"Detaching from process: %d", stDebuggeeInfo.dwProcessID);
                logtrace(wsDbgMessage);
                AppendToEditControl(hDebugEventsTextBox, wsDbgMessage);
                if(!DebugActiveProcessStop(stDebuggeeInfo.dwProcessID))
                    logerror(L"Unable to detach from process");
                
                // No need for close proc and thread handles because
                // detaching takes care of them

                fEndDebugLoop = TRUE;
                ResetEvent(ahWDEvents[INDEX_WD_DETACH]);
                break;
            }

            case WAIT_FAILED:
                logerror(L"WAIT_FAILED WaitForMultipleObjects()!!!");
                break;

            default:
                if(!fDoDebugEvtLoop())
                {
                    logtrace(L"fProcessDebugEvtLoop() returned FALSE...");
                    fEndDebugLoop = TRUE;
                }
                break;
        }// switch
    }// while !fEndDebugLoop
    
    logtrace(L"dwDebugThreadEntry(): Out of debug event loop...");

    SetEvent(ahWDEvents[INDEX_DW_TERMINATED]);
    vCleanUpDebugThread();
    ExitThread(0);

    error_init_exit:
    pThreadArgs->fErrorStartingDebugSession = TRUE;
    SetEvent(hInitEvent); PURGE_HANDLE(hInitEvent);
    vCleanUpDebugThread();
    ExitThread(1);

    error_exit:
    SetEvent(ahWDEvents[INDEX_DW_TERMINATED]);
    vCleanUpDebugThread();
    ExitThread(1);

}// dwDebugThreadEntry()


// fDebugNewProcess()
// 
BOOL fDoDebugNewProcess(DEBUGTHREAD_ARGS *pTargetInfo)
{
    ASSERT(pTargetInfo && pTargetInfo->info.pwsDebuggeePath);

    STARTUPINFO StartUpInfo;
    PROCESS_INFORMATION ProcInfo;

    ZeroMemory(&StartUpInfo, sizeof(StartUpInfo));
    StartUpInfo.cb = sizeof(StartUpInfo);

    // Create the process first
    if(!CreateProcess(  pTargetInfo->info.pwsDebuggeePath,    // app name
                        NULL,               // command line
                        NULL,               // process attr
                        NULL,               // thread attr
                        FALSE,              // inherit handles
                        DEBUG_ONLY_THIS_PROCESS|
                        CREATE_NEW_CONSOLE,    // creation flags
                        NULL,               // environment
                        NULL,               // cur directory
                        &StartUpInfo,
                        &ProcInfo))
    {
        logerror(L"Unable to create debuggee process %d");
        goto error_return;
    }

    PURGE_HANDLE(ProcInfo.hThread);
    PURGE_HANDLE(ProcInfo.hProcess);
    
    stDebuggeeInfo.dwProcessID = ProcInfo.dwProcessId;
    stDebuggeeInfo.dwMainThreadID = ProcInfo.dwThreadId;
    if(!HT_fCreate(&(stDebuggeeInfo.phtThreads), 2, HT_KEY_DWORD, HT_VAL_STR))
    {
        logerror(L"HT_fCreate() returned FALSE");
        goto error_return;
    }

    pTargetInfo->dwDebuggeePID = ProcInfo.dwProcessId;
    pTargetInfo->fErrorStartingDebugSession = FALSE;
    return TRUE;

    error_return:
    pTargetInfo->fErrorStartingDebugSession = TRUE;
    return FALSE;
}

// fAttachToActiveProcess()
// Attaches to an existing process for debugging
// Also creates the hashtable to hold thread info
BOOL fAttachToActiveProcess(DEBUGTHREAD_ARGS *pTargetInfo)
{
    stDebuggeeInfo.dwProcessID = pTargetInfo->info.dwDebuggeePID;
    if(!HT_fCreate(&(stDebuggeeInfo.phtThreads), 2, HT_KEY_DWORD, HT_VAL_STR))
    {
        logerror(L"HT_fCreate() returned FALSE");
        goto error_return;
    }
    if(!DebugActiveProcess(stDebuggeeInfo.dwProcessID))
        goto error_return;

    stDebuggeeInfo.fDebuggingActiveProcess = TRUE;
    pTargetInfo->dwDebuggeePID = stDebuggeeInfo.dwProcessID;
    pTargetInfo->fDebugActiveProcess = FALSE;
    return TRUE;

    error_return:
    pTargetInfo->fErrorStartingDebugSession = TRUE;
    return FALSE;
}


// fDebugActiveProcess()
// 
BOOL fDoDebugActiveProcess()
{
    ASSERT(phtDLLTable);

    DEBUG_EVENT de;

    WCHAR wsDebugEventMsg[512];

    logtrace(L"Beginning to debug active proc");

    // Capture and read all the events that follow until we receive a EXCEPTION_DEBUG_EVENT
    while(1)
    {
        if(WaitForDebugEvent(&de, INFINITE))
        {
            if(de.dwProcessId != stDebuggeeInfo.dwProcessID)
            {
                logtrace(L"Debug thread: Received debug event for sibling process %u");
                wprintf(L"Debug thread: Received debug event for sibling process %u != %u", de.dwProcessId, stDebuggeeInfo.dwProcessID);
            }
            else
            {
                switch(de.dwDebugEventCode)
                {
                    case EXCEPTION_DEBUG_EVENT:
                        logtrace(L"Received EXCEPTION_DEBUG_EVENT... beginning normal process for debug wait loop...");
                        AppendToEditControl(hDebugEventsTextBox, 
                            L"Received EXCEPTION_DEBUG_EVENT... beginning normal debug wait loop...");
                        goto debug_init_done;

                    case CREATE_PROCESS_DEBUG_EVENT:
                    {
                        WCHAR wsImageName[MAX_PATH+1];
                        DWORD dwRet = GetFinalPathNameByHandle(de.u.CreateProcessInfo.hFile, wsImageName, MAX_PATH, VOLUME_NAME_NT);

                        if(dwRet == 0)
                            logerror(L"Could not get filename from handle");
                        else if(dwRet > MAX_PATH)
                            logerror(L"Insufficient buffer size for GetFinalPathNameByHandle()");
                        else
                        {
                            swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"Process create : [%u] : %s", de.dwProcessId, wsImageName);
                            AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                        }

                        if(fCreateProcFirstTime)
                        {
                            fCreateProcFirstTime = FALSE;
                            stDebuggeeInfo.hProcess = de.u.CreateProcessInfo.hProcess;
                            stDebuggeeInfo.hMainThread = de.u.CreateProcessInfo.hThread;

                            if(!fAddThread(stDebuggeeInfo.phtThreads, de.dwThreadId, de.u.CreateProcessInfo.hThread))
                                wprintf(L"Unable to insert Main thread to hashtable");
                            
                            memcpy(&stDebuggeeInfo.procDebugInfo, &de.u.CreateProcessInfo, sizeof(de.u.CreateProcessInfo));
                        }

                        CloseHandle(de.u.CreateProcessInfo.hFile);
                        ++dwThreadsCreated;
                        ++dwProcsCreated;
                        break;
                    }

                    case CREATE_THREAD_DEBUG_EVENT:
                        swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"Thread create: [%u]", de.dwThreadId);
                        AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                        if(!fAddThread(stDebuggeeInfo.phtThreads, de.dwThreadId, de.u.CreateThread.hThread))
                            wprintf(L"Unable to insert thread %u to hashtable", de.dwThreadId);
                        ++dwThreadsCreated;
                        break;

                    case LOAD_DLL_DEBUG_EVENT:
                    {
                        WCHAR wsImageName[MAX_PATH+1];
                        DWORD dwRet = GetFinalPathNameByHandle(de.u.CreateProcessInfo.hFile, wsImageName, MAX_PATH, VOLUME_NAME_NT);

                        if(dwRet == 0)
                            logerror(L"Could not get filename from handle");
                        else if(dwRet > MAX_PATH)
                            logerror(L"Insufficient buffer size for GetFinalPathNameByHandle()");
                        else
                        {
                            swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"DLL Load : [0x%08x] : %s", de.u.LoadDll.lpBaseOfDll, wsImageName);
                            AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                        }
                        ++dwDllsLoaded;
                        CloseHandle(de.u.LoadDll.hFile);
                        int nlen = wcsnlen_s(wsImageName, MAX_PATH+1);
                        if(!HT_fInsert( phtDLLTable, &(de.u.LoadDll.lpBaseOfDll), sizeof(DWORD), wsImageName, 
                                BYTELEN_wcsnlen_s(wsImageName, MAX_PATH+1) ))
                        {
                            swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"Unable to insert 0x%08x:%s into hash", 
                                                                                *((DWORD*)de.u.LoadDll.lpBaseOfDll), wsImageName);
                            logerror(wsDebugEventMsg);
                        }
                        break;
                    }

                    case EXIT_PROCESS_DEBUG_EVENT:
                    case EXIT_THREAD_DEBUG_EVENT:
                    case UNLOAD_DLL_DEBUG_EVENT:
                    case OUTPUT_DEBUG_STRING_EVENT: ASSERT(FALSE); break;
                }// switch()
            }
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
        }// if(WaitFor...)
    
    }// while(1)

    debug_init_done:
    ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
    swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), 
                            L"Active Debuggee stats ==>\r\n"
                            L"Processes created    : %u\r\n"
                            L"Threads created      : %u\r\n"
                            L"DLLs loaded          : %u" , dwProcsCreated, dwThreadsCreated, dwDllsLoaded);
    AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
    return TRUE;

}// fDoDebugActiveProcess()


// fDoDebugEvtLoop()
//
BOOL fDoDebugEvtLoop()
{
    DEBUG_EVENT de;

    BOOL fRetVal = TRUE;
    WCHAR wsDebugEventMsg[512];

    //logtrace("Debug thread: Waiting for debug event...");
    if(WaitForDebugEvent(&de, 10))
    {
        if(de.dwProcessId != stDebuggeeInfo.dwProcessID)
        {
            logtrace(L"Debug thread: Received debug event for sibling process %u");
            wprintf(L"Debug thread: Received debug event for sibling process %u != %u", de.dwProcessId, stDebuggeeInfo.dwProcessID);
        }
        else
        {
            switch(de.dwDebugEventCode)
            {
                case EXCEPTION_DEBUG_EVENT:
                {
                    DWORD continueStatus = DBG_EXCEPTION_NOT_HANDLED;
                    swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg),
                        L"Exception 0x%08X at 0x%08x", de.u.Exception.ExceptionRecord.ExceptionCode, 
                            de.u.Exception.ExceptionRecord.ExceptionAddress);
                    AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                    ++dwExceptions;
                    fOnExceptionEvent(&de, &continueStatus);
                    ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continueStatus);
                    return TRUE;
                }

                case CREATE_PROCESS_DEBUG_EVENT:
                {
                    WCHAR wsImageName[MAX_PATH+1];
                    DWORD dwRet = GetFinalPathNameByHandle(de.u.CreateProcessInfo.hFile, wsImageName, MAX_PATH, VOLUME_NAME_NT);

                    if(dwRet == 0)
                        logerror(L"Could not get filename from handle");
                    else if(dwRet > MAX_PATH)
                        logerror(L"Insufficient buffer size for GetFinalPathNameByHandle()");
                    else
                    {
                        swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"Process create: [%u] : %s", de.dwProcessId, wsImageName);
                        AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                    }

                    ++dwThreadsCreated;
                    ++dwProcsCreated;

                    if(fCreateProcFirstTime)
                    {
                        fCreateProcFirstTime = FALSE;
                        stDebuggeeInfo.hProcess = de.u.CreateProcessInfo.hProcess;
                        stDebuggeeInfo.hMainThread = de.u.CreateProcessInfo.hThread;

                        if(!fAddThread(stDebuggeeInfo.phtThreads, de.dwThreadId, de.u.CreateProcessInfo.hThread))
                            wprintf(L"Unable to insert Main thread to hashtable");

                        memcpy(&stDebuggeeInfo.procDebugInfo, &de.u.CreateProcessInfo, sizeof(de.u.CreateProcessInfo));
                    }
                    
                    CloseHandle(de.u.CreateProcessInfo.hFile);
                    break;
                }

                case CREATE_THREAD_DEBUG_EVENT:
                    swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"Thread create: [%u]", de.dwThreadId);
                    AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                    if(!fAddThread(stDebuggeeInfo.phtThreads, de.dwThreadId, de.u.CreateThread.hThread))
                        wprintf(L"Unable to insert thread to hashtable");
                    ++dwThreadsCreated;
                    break;

                case EXIT_PROCESS_DEBUG_EVENT:
                    swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"Process exit: [%u] : ExitCode %d", de.dwProcessId, 
                                                                                                    de.u.ExitProcess.dwExitCode);
                    AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                    if(de.dwProcessId == stDebuggeeInfo.dwProcessID)
                    {
                        swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), 
                            L"Debuggee process EXIT! Gotta quit debug thread...\r\n"
                            L"Processes created    : %u\r\n"
                            L"Threads created      : %u\r\n"
                            L"DLLs loaded          : %u\r\n"
                            L"Exceptions           : %u", dwProcsCreated, dwThreadsCreated, dwDllsLoaded, dwExceptions);
                        AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                        fRetVal = FALSE;
                    }
                    break;

                case EXIT_THREAD_DEBUG_EVENT:
                    swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"Thread exit: [%u] : ExitCode %d", de.dwThreadId, 
                                                                                                        de.u.ExitThread.dwExitCode);
                    AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                    if(!fRemoveThread(stDebuggeeInfo.phtThreads, de.dwThreadId))
                        logerror(L"Failed to remove thread from hashtable");
                    break;

                case LOAD_DLL_DEBUG_EVENT:
                {
                    WCHAR wsImageName[MAX_PATH+1];
                    DWORD dwRet = GetFinalPathNameByHandle(de.u.LoadDll.hFile, wsImageName, 
                                        MAX_PATH, VOLUME_NAME_NT);

                    if(dwRet == 0)
                        logerror(L"Could not get filename from handle");
                    else if(dwRet > MAX_PATH)
                        logerror(L"Insufficient buffer size for GetFinalPathNameByHandle()");
                    else
                    {
                        swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"DLL Load: [0x%08x] : %s", 
                                                                            de.u.LoadDll.lpBaseOfDll, wsImageName);
                        AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                    }
                    
                    if(!HT_fInsert( phtDLLTable, &(de.u.LoadDll.lpBaseOfDll), sizeof(DWORD), wsImageName, 
                            BYTELEN_wcsnlen_s(wsImageName, MAX_PATH+1)+1 ))
                    {
                        swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"Unable to insert 0x%08x:%s into hash", 
                                                                            *((DWORD*)de.u.LoadDll.lpBaseOfDll), wsImageName);
                        logerror(wsDebugEventMsg);
                    }

                    ++dwDllsLoaded;
                    CloseHandle(de.u.LoadDll.hFile);

                    break;
                }

                case UNLOAD_DLL_DEBUG_EVENT:
                {
                    WCHAR *pws = NULL;
                    int outValSize = 0;
                    if(!HT_fFind(phtDLLTable, &(de.u.UnloadDll.lpBaseOfDll), sizeof(DWORD), &pws, &outValSize))
                    {
                        swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"DLL Unload : [0x%08x] : Image name not found", 
                                                                            (DWORD)(de.u.UnloadDll.lpBaseOfDll));
                        AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                    }
                    else
                    {
                        swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"DLL Unload : [0x%08x] : %s", 
                                                                            (DWORD)de.u.UnloadDll.lpBaseOfDll, pws);
                        AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                        if(!HT_fRemove(phtDLLTable, &(de.u.UnloadDll.lpBaseOfDll), sizeof(DWORD)))
                            logerror(L"HT_fRemove() returned FALSE");
                    }
                    break;
                }

                case OUTPUT_DEBUG_STRING_EVENT:
                    WCHAR *pwsDebugString = NULL;
                    SIZE_T bytesRead = 0;

                    if(!ChlfMemAlloc((void**)&pwsDebugString, de.u.DebugString.nDebugStringLength * sizeof(WCHAR), NULL))
                        logerror(L"OUTPUT_DEBUG_STRING_EVENT: Could not allocate memory!!");
                    else
                    {
                        // read the string from debuggee's memory
                        if(de.u.DebugString.fUnicode)
                        {
                            ReadProcessMemory(stDebuggeeInfo.hProcess, de.u.DebugString.lpDebugStringData, pwsDebugString,
                                                de.u.DebugString.nDebugStringLength * sizeof(WCHAR), &bytesRead);
                            swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"OutputDebugString: [%u][%s]", bytesRead, pwsDebugString);
                            AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                        }
                        else
                        {
                            ReadProcessMemory(stDebuggeeInfo.hProcess, de.u.DebugString.lpDebugStringData, pwsDebugString,
                                                de.u.DebugString.nDebugStringLength * sizeof(char), &bytesRead);
                            swprintf_s(wsDebugEventMsg, _countof(wsDebugEventMsg), L"OutputDebugString: [%u][%S]", bytesRead, pwsDebugString);
                            AppendToEditControl(hDebugEventsTextBox, wsDebugEventMsg);
                        }
                        ChlvMemFree((void**)&pwsDebugString);
                    }
                    break;
            }// switch(de.dwDebugEventCode);
        
        }//

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
    
    }// if(WaitForDebugEvent())

    return fRetVal;

}// fDoDebugEvtLoop()


BOOL fOnExceptionEvent(DEBUG_EVENT *pde, DWORD *pdwContinueStatus)
{
    ASSERT(pde && pdwContinueStatus);

    DWORD dwExCode = pde->u.Exception.ExceptionRecord.ExceptionCode;
    WCHAR wsExString[MAX_EXCEPTION_NAMESTR+1];
    WCHAR wsExceptionMessage[MAX_EXCEPTION_MSG+1];

    fGetExceptionName(dwExCode, wsExString, _countof(wsExString));

    if(pde->u.Exception.dwFirstChance)
        swprintf_s(wsExceptionMessage, _countof(wsExceptionMessage), L"%s Exception(FirstChance) at: 0x%08x", wsExString, 
                    pde->u.Exception.ExceptionRecord.ExceptionAddress);
    else
        swprintf_s(wsExceptionMessage, _countof(wsExceptionMessage), L"%s Exception at: 0x%08x", wsExString, 
                    pde->u.Exception.ExceptionRecord.ExceptionAddress);

    MessageBox(NULL, wsExceptionMessage, L"Exception Raised", MB_ICONSTOP|MB_OK);
    *pdwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
    return TRUE;

}// fOnExceptionEvent()


// vCleanUpDebugThread()
// ToDo
// Have a separate cleanup and OnThreadExit() function
void vCleanUpDebugThread()
{
    if(phtDLLTable && !HT_fDestroy(phtDLLTable))
            logerror(L"Unable to destroy DLL hash table");

    if(stDebuggeeInfo.phtThreads && !fDeleteThreadsTable(stDebuggeeInfo.phtThreads))
            logerror(L"Unable to destroy threads hash table");

    ZeroMemory(&stDebuggeeInfo, sizeof(stDebuggeeInfo));

    dwExceptions = dwProcsCreated = dwThreadsCreated = dwDllsLoaded = 0;
    
    vCloseHandles(ahWDEvents, _countof(ahWDEvents));
    
    ZeroMemory(wsEventName, sizeof(wsEventName));
    
    fEndDebugLoop = FALSE;
    fCreateProcFirstTime = TRUE;
}
