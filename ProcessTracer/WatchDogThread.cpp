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

#include "WatchDogThread.h"


// ***************************************************************************
// ToDo
// Eventually we will move most of the debugging session start and end code
// to this module. The GUI thread will only have to send events saying do 
// this do that.
// 
// We would also want to support multiple debug session in future.
//
// ****************************************************************************

#define MAX_EVT_WAIT_GUI    500
#define MAX_EVT_WAIT_DBG    250

extern HWND hStatusBar;
extern WD_BEGINDBG_ARGS st_WD_BeginDebugArgs;

// File-local variables
static HANDLE       hGUIWindow;
static HMENU        hMainMenu;
static HINSTANCE    hMainInstance;

static HANDLE ahGWEvents[MAX_GW_EVENTS];
static HANDLE ahWDEvents[MAX_WD_EVENTS];
static HANDLE hDebugThread; // ToDo: array to hold multiple debug thread handles
// ToDo structure to hold current debug threads' information: handle, comm events

// File-local functions
static void vCleanUpWatchDog();

static BOOL fOnGWEvent_Start(WD_BEGINDBG_ARGS *pTargetInfo);
static BOOL fOnGWEvent_Term();
static BOOL fOnGWEvent_Detach();
static BOOL fOnGWEvent_Suspend();
static BOOL fOnGWEvent_Resume();
static BOOL fIsDebugThreadActive(HANDLE hDebugThread);
static BOOL fTermAndWaitForDebugThread(HANDLE hDebugThread);

// dwWatchDebugThread()
// Watch dog thread that watches for events from the debug thread
// This is the one and only entry-point for this module/thread.
// 
DWORD WINAPI WatchDogThreadEntry(LPVOID lpArgs)
{
    ASSERT(lpArgs);

    WD_THREADARGS *pthArgs = (WD_THREADARGS*)lpArgs;
    ASSERT(IS_VALID_HANDLE(pthArgs->hGUIWindow) && 
            IS_VALID_HANDLE(pthArgs->hMainInstance) &&
            IS_VALID_HANDLE(pthArgs->hMainMenu));

    BOOL fExitPollingLoop = FALSE;
    BOOL fDebugging = FALSE;
    int nGUIEvtIndex = 0;
    int nDBGEvtIndex = 0;

    hGUIWindow = pthArgs->hGUIWindow;
    hMainMenu = pthArgs->hMainMenu;
    hMainInstance = pthArgs->hMainInstance;

    // 0. Create GW events - only once per thread invocation
    if(!CreateGWEvents(ahGWEvents, GetCurrentProcessId()))
        goto error_exit;

    while(!fExitPollingLoop)
    {
        // 1. Check for events from GUI thread - highest priority
        // 2. Process those events
        switch((nGUIEvtIndex = WaitForMultipleObjects(_countof(ahGWEvents), ahGWEvents, FALSE, MAX_EVT_WAIT_GUI)))
        {
            case INDEX_GW_TERM:
                fOnGWEvent_Term();
                ResetEvent(ahGWEvents[INDEX_GW_TERM]);
                break;

            case INDEX_GW_SUSPEND:
                fOnGWEvent_Suspend();
                ResetEvent(ahGWEvents[INDEX_GW_SUSPEND]);
                break;

            case INDEX_GW_RESUME:
                fOnGWEvent_Resume();
                ResetEvent(ahGWEvents[INDEX_GW_RESUME]);
                break;

            case INDEX_GW_DETACH:
                fOnGWEvent_Detach();
                ResetEvent(ahGWEvents[INDEX_GW_DETACH]);
                break;

            case INDEX_GW_START:
                fOnGWEvent_Start(&st_WD_BeginDebugArgs);
                fDebugging = TRUE;
                ResetEvent(ahGWEvents[INDEX_GW_START]);
                break;

            case INDEX_GW_EXITTHREAD:
                // ToDo
                // what to do if debug thread(s) fail to terminate in time
                if(fDebugging && fIsDebugThreadActive(hDebugThread))
                    fTermAndWaitForDebugThread(hDebugThread);
                vCleanUpWatchDog();
                fDebugging = FALSE;
                fExitPollingLoop = TRUE;
                ResetEvent(ahGWEvents[INDEX_GW_EXITTHREAD]);
                break;

            case WAIT_FAILED:
            {
                DWORD dwError = GetLastError();
                logerror(L"WaitForMultipleObjects() failed");
                break;
            }

        }// switch GUI event

        // 3. Check for events from DebugThread - assume only one debug thread for now
        // 4. Process those events
        if(fDebugging)
        {
            // We will receive only IDS_DW_TERMINATED event
            switch(WaitForSingleObject(ahWDEvents[INDEX_DW_TERMINATED], MAX_EVT_WAIT_DBG))
            {
                case WAIT_OBJECT_0:
                {
                    logtrace(L"WatchDogThreadEntry(): DebugThread terminated");
                    fDebugging = FALSE;
                    vCloseHandles(ahWDEvents, _countof(ahWDEvents));
                    PURGE_HANDLE(hDebugThread);
                    fMiDebugSessionEnd(hMainMenu);
                    break;
                }

                case WAIT_ABANDONED:
                    logerror(L"Waiting for debug thread events abandoned");
                    break;

                case WAIT_FAILED:
                    logerror(L"Waiting for debug thread events failed");
                    break;
            }
        }

        // continue

    }// while polling loop

    logtrace(L"Watchdog: Exiting with success...");
    ExitThread(0);

    error_exit:
    logtrace(L"Watchdog: Exiting with failure...");
    ExitThread(666);

}// dwWatchDebugThread()


void vCleanUpWatchDog()
{
    hGUIWindow = hMainMenu = NULL;
    hMainInstance = NULL;
    vCloseHandles(ahGWEvents, _countof(ahGWEvents));
}


//
//
BOOL fOnGWEvent_Start(WD_BEGINDBG_ARGS *pTargetInfo)
{
    ASSERT(pTargetInfo);

    WCHAR wsEventName[MAX_PATH];

    HANDLE hInitEvent = NULL;

    DEBUGTHREAD_ARGS debugThreadArgs;

    // create the init event and debug thread
    LoadString(GetModuleHandle(NULL), IDS_WD_INITACK, wsEventName, _countof(wsEventName));
    if((hInitEvent = CreateEvent(NULL, TRUE, FALSE, wsEventName)) == NULL)
    {
        logerror(L"Could not create init event");
        return FALSE;
    }

    debugThreadArgs.fDebugActiveProcess = pTargetInfo->fDebugActiveProcess;
    debugThreadArgs.fErrorStartingDebugSession = FALSE;
    if(debugThreadArgs.fDebugActiveProcess)
        debugThreadArgs.info.dwDebuggeePID = pTargetInfo->dwTargetProcessID;
    else
        debugThreadArgs.info.pwsDebuggeePath = pTargetInfo->wsTargetPath;

    hDebugThread = CreateThread(NULL,           // thread attr
                                0,              // stack size
                                dwDebugThreadEntry,  // function addr
                                (LPVOID)&debugThreadArgs,  // parameter, optional
                                0,              // creation flags
                                NULL);          // OUT, thread ID
    if(hDebugThread == NULL)
    {
        logerror(L"Unable to create debug thread");
        wprintf(L"Unable to create debug thread %d", GetLastError());
        return FALSE;
    }

    logtrace(L"WD thread: Created debug thread. Waiting for init ack...");
    WaitForSingleObject(hInitEvent, INFINITE);
    PURGE_HANDLE(hInitEvent);

    if(debugThreadArgs.fErrorStartingDebugSession)
    {
        logerror(L"There was an error in the debug thread init");
        wprintf(L"There was an error in the debug thread init: %d\n", GetLastError());
        fMiDebugSessionEnd(hMainMenu);
        return FALSE;
    }

    if(!CreateWDEvents(ahWDEvents, debugThreadArgs.dwDebuggeePID))
    {
        logerror(L"Unable to create comm events: WD thread");
        wprintf(L"Unable to create comm events: WD thread\n");
        fMiDebugSessionEnd(hMainMenu);
        return FALSE;
    }

    fMiDebugSessionStart(hMainMenu);
    return TRUE;
}


BOOL fOnGWEvent_Term()
{
    return SetEvent(ahWDEvents[INDEX_WD_TERM]);
}


BOOL fOnGWEvent_Detach()
{
    return SetEvent(ahWDEvents[INDEX_WD_DETACH]);
}


BOOL fOnGWEvent_Suspend()
{
    return SetEvent(ahWDEvents[INDEX_WD_SUSPEND]);
}


BOOL fOnGWEvent_Resume()
{
    return SetEvent(ahWDEvents[INDEX_WD_RESUME]);
}


BOOL fIsDebugThreadActive(HANDLE hDebugThread)
{
    ASSERT(IS_VALID_HANDLE(hDebugThread));

    switch(WaitForSingleObject(hDebugThread, 0))
    {
        case WAIT_OBJECT_0:
            return FALSE;

        case WAIT_TIMEOUT:
            return TRUE;
    }

    logerror(L"fIsDebugThreadActive(): WaitForSingleObject() error");
    return FALSE;
}

BOOL fTermAndWaitForDebugThread(HANDLE hDebugThread)
{
    int nPolls = 0;

    SetEvent(ahWDEvents[INDEX_WD_TERM]);

    logtrace(L"Sent term event. Waiting for debug thread to terminate");

    // wait for it to terminate
    while(nPolls++ < 10)
    {
        logerror(L"Checking if debug thread has terminated");
        switch(WaitForSingleObject(ahWDEvents[INDEX_DW_TERMINATED], MAX_EVT_WAIT_DBG))
        {
            case WAIT_OBJECT_0:
                logtrace(L"fTermAndWaitForDebugThread(): DebugThread terminated");
                vCloseHandles(ahWDEvents, _countof(ahWDEvents));
                PURGE_HANDLE(hDebugThread);   
                break;

            case WAIT_ABANDONED:
                logerror(L"Waiting for debug thread events abandoned");
                break;

            case WAIT_FAILED:
                logerror(L"Waiting for debug thread events failed");
                break;
        }
    }
    
    return nPolls < 10;
}