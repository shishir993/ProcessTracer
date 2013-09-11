
// WndProc.cpp
// Contains the WndProc() function for the main debugger UI window
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/08/13 Initial version
//      08/10/13 Major design change. GUI - WatchDog - Debug threads.
//               Event communication between each other. No communication
//               between GUI - Debug except maybe a few GUI calls.
//               Trying to keep everything strictly asynchronous - no INFINITE
//               WaitFor*Object[s]() calls.

#include "WndProc.h"

extern HINSTANCE g_hMainInstance;


#define IDC_EDIT_DEBUG_EVENTS   300
#define IDC_EDIT_DEBUG_INFO     301
#define IDC_EDIT_SELF_DEBUG     302


// Global variables
HMENU hMainMenu;
HWND hDebugEventsTextBox;
HWND hDebuggeeInfoTextBox;
HWND hDisassemblyTextBox;
HWND hStatusBar;
WD_BEGINDBG_ARGS st_WD_BeginDebugArgs;

static WD_THREADARGS st_WD_ThreadArgs;
static HANDLE ahGWEvents[MAX_GW_EVENTS];
static HANDLE hWatchDogThread = NULL;
static HBRUSH hbrBackground = NULL;

static BOOL fDebugActiveProcess;
static DWORD dwActiveProcessID;
static WCHAR wsDebuggeeFileName[MAX_PATH+1];
static WCHAR wsDebuggeeFilePath[MAX_PATH+1];

// File-local Functions
static BOOL fStartWatchDogThread(HWND hMainWindow);
static void vOnDebuggerExit();
static BOOL fCreateConsoleWindow();

static int iConInHandle = -1;
static int iConOutHandle = -1;
static int iConErrHandle = -1;

// WndProc()
// 
//
LRESULT CALLBACK WndProc(HWND hMainWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
	{
		case WM_CREATE:
		{
            // create a console window for debugging messages output
            fCreateConsoleWindow();

            if(!fLoInitLogger())
            {
                SendMessage(hMainWindow, WM_CLOSE, 0, 0);
                return 0;
            }
            if(!fInitUIElements(hMainWindow))
            {
                logerror(L"Failed to init UI elements");
                SendMessage(hMainWindow, WM_CLOSE, 0, 0);
                return 0;
            }
            if(!fStartWatchDogThread(hMainWindow))
            {
                logerror(L"Failed to init watch dog thread");
                SendMessage(hMainWindow, WM_CLOSE, 0, 0);
                return 0;
            }
            logtrace(L"WndProc() Initialization done");
            return 0;
        }

        case WM_CLOSE:
		{
			break;
		}

		case WM_DESTROY:
		{
            vOnDebuggerExit();
			PostQuitMessage(0);
			return 0;
		}

        case WM_CTLCOLORSTATIC:
        {
            HWND hEditCtrl = (HWND)lParam;
            HDC hdcStatic = (HDC)wParam;

            switch(GetDlgCtrlID(hEditCtrl))
            {
                case IDC_EDIT_DEBUG_EVENTS:
                    SetTextColor(hdcStatic, RGB(255, 255, 255));    // white
                    SetBkColor(hdcStatic, RGB(0, 0, 0));
                    if (hbrBackground == NULL)
                        hbrBackground = CreateSolidBrush(RGB(0,0,0));
                    return (DWORD)hbrBackground;

                case IDC_EDIT_DEBUG_INFO:
                    SetTextColor(hdcStatic, RGB(255, 0, 255));    // magenta
                    SetBkColor(hdcStatic, RGB(0, 0, 0));
                    if (hbrBackground == NULL)
                        hbrBackground = CreateSolidBrush(RGB(0,0,0));
                    return (DWORD)hbrBackground;

                case IDC_EDIT_SELF_DEBUG:
                    SetTextColor(hdcStatic, RGB(0, 255, 0));    // green
                    SetBkColor(hdcStatic, RGB(0, 0, 0));
                    if (hbrBackground == NULL)
                        hbrBackground = CreateSolidBrush(RGB(0,0,0));
                    return (DWORD)hbrBackground; 
            }
            break;
        }

        case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDM_FILE_EXIT:
				{
                    // todo: stop debuggee, if any?
                    SendMessage(hMainWindow, WM_DESTROY, 0, 0);
                    return 0;
                }

                case IDM_CONSOLE_CLOSECONSOLE:
                {
                    if(iConInHandle != -1) _close(iConInHandle);
                    if(iConOutHandle != -1) _close(iConOutHandle);
                    if(iConErrHandle != -1) _close(iConErrHandle);
                    iConInHandle = iConOutHandle = iConErrHandle = -1;

                    if(!FreeConsole())
                    {
                        DWORD lasterror  = GetLastError();
                        logerror(L"WndProc(): Could not free console");
                    }

                    EnableMenuItem(hMainMenu, IDM_CONSOLE_CLOSECONSOLE, MF_GRAYED);
                    return 0;
                }

                case IDM_CONSOLE_TESTCONSOLE:
                {
                    wprintf(L"wprintf(): Testing new console...\n");
                    wprintf(L"Does it work??!!\n");
                    wprintf(L"I dunno. You tell me, mister...\n");

                    printf("printf():    Testing new console...\n");
                    printf("Does it work??!!\n");
                    printf("I dunno. You tell me, mister...\n");

                    //fFindNtDllInCurProcess(NULL, NULL);

                    return 0;
                }

                case IDM_VIEW_CLEARDEBUGEVENTS:
				{
                    if(IS_VALID_HANDLE(hDebugEventsTextBox))
                        SendMessage(hDebugEventsTextBox, WM_SETTEXT, 0, (LPARAM)L"");
                    return 0;
                }

                case IDM_DEBUG_LOADNEWPROCESS:
				{
                    // global variables init
                    fDebugActiveProcess = FALSE;
                    ZeroMemory(wsDebuggeeFileName, sizeof(wsDebuggeeFileName));
                    ZeroMemory(wsDebuggeeFilePath, sizeof(wsDebuggeeFilePath));
                    fOnLoadNewProcess(hMainWindow);
                    return 0;
                }

                case IDM_DEBUG_OPENACTIVEPROCESS:
				{
                    dwActiveProcessID = 0;
                    DialogBox(g_hMainInstance, MAKEINTRESOURCE(IDD_GETPROCID), hMainWindow, fGetProcIdDP);
                    if(dwActiveProcessID > 0)
                    {
                        fDebugActiveProcess = TRUE;
                        fMiProcessOpened(hMainMenu);
                    }
                    else
                    {
                        logerror(L"Unable to open active");
                        MessageBox(hMainWindow, L"Unable to start debug session", L"Error", MB_ICONEXCLAMATION);
                        fDebugActiveProcess = FALSE;
                        dwActiveProcessID = -1;
                        wsDebuggeeFileName[0] = 0;
                        wsDebuggeeFilePath[0] = 0;
                        fMiDebugSessionEnd(hMainMenu);
                    }
                    return 0;
                }

                case IDM_DEBUG_RECHOOSEPROCESS:
                {
                    fMiDebugSessionEnd(hMainMenu);
                    return 0;
                }

                case IDM_DEBUG_STARTDEBUGSESSION:
				{
                    if(!fOnStartDebugSession())
                    {
                        logerror(L"Unable to start debug session");
                        MessageBox(hMainWindow, L"Unable to start debug session", L"Error", MB_ICONEXCLAMATION);
                        fDebugActiveProcess = FALSE;
                        dwActiveProcessID = -1;
                        wsDebuggeeFileName[0] = 0;
                        wsDebuggeeFilePath[0] = 0;
                        fMiDebugSessionEnd(hMainMenu);
                    }
                    return 0;
                }

                case IDM_DEBUG_TERMINATEDEBUGGEE:
				{
                    SetEvent(ahGWEvents[INDEX_GW_TERM]);
                    return 0;
                }

                case IDM_DEBUG_DETACHFROMDEBUGGEE:
				{
                    SetEvent(ahGWEvents[INDEX_GW_DETACH]);
                    return 0;
                }

                case IDM_DEBUG_SUSPENDTHREADS:
                {
                    SetEvent(ahGWEvents[INDEX_GW_SUSPEND]);
                    return 0;
                }

                case IDM_DEBUG_RESUMETHREADS:
                {
                    SetEvent(ahGWEvents[INDEX_GW_RESUME]);
                    return 0;
                }

                case IDM_HELP_ABOUT:
				{
                    INT_PTR iptr = DialogBox(g_hMainInstance, MAKEINTRESOURCE(IDD_ABOUT), hMainWindow, fAboutDP);
                    DWORD dwerror = GetLastError();
                    return 0;
                }

            }// switch(LOWORD...)
            
            break;

        }// case WM_COMMAND
    
    }// switch(message)

    return DefWindowProc(hMainWindow, message, wParam, lParam);

}// WndProc()


// fInitUIElements()
// Draw the three text boxes - debug events, debuggee info, self debug -
// and the status bar.
BOOL fInitUIElements(HWND hMainWindow)
{
    BOOL fSuccess = TRUE;
    RECT rClientArea;
	int aPartsWidth[SB_NPARTS];
	int nWidth;
	int iRetVal;

    hMainMenu = GetMenu(hMainWindow);

    __try
    {
        // Create StatusBar
	    hStatusBar = CreateWindowEx(
							    0,
							    STATUSCLASSNAME,
							    NULL,
							    WS_CHILD | SBARS_SIZEGRIP | WS_VISIBLE,
							    0, 0, 0, 0,
							    hMainWindow,						
							    NULL,
							    g_hMainInstance,
							    NULL);

	    if(!hStatusBar)
	    {
		    MessageBox(NULL, L"Status Bar not created!", NULL, MB_OK );
		    fSuccess = FALSE; __leave;
	    }

        // divide the status bar into 3 parts
		iRetVal = GetClientRect(hMainWindow, (LPRECT)&rClientArea);
		nWidth = rClientArea.right / SB_NPARTS;
		for(int i=0; i < SB_NPARTS; ++i)
		{
			aPartsWidth[i] = nWidth;
			nWidth += nWidth;
		}

		// Tell the status bar to create the window parts.
		SendMessage(hStatusBar, SB_SETPARTS, (WPARAM) SB_NPARTS, (LPARAM)aPartsWidth);

        // edit control for debug events
        hDisassemblyTextBox = CreateWindowEx(
                                0, L"EDIT",   // predefined class 
                                NULL,         // no window title 
                                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | 
                                ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 
                                5, 5,          // x, y 
                                (int)(rClientArea.right*0.7),		           // x size
							    (int)(rClientArea.bottom*0.7),                 // y size
                                hMainWindow,                                   // parent window 
                                (HMENU)IDC_EDIT_DEBUG_EVENTS,                  // edit control ID 
                                g_hMainInstance,
                                NULL);        // pointer not needed
        if(!hDisassemblyTextBox)
		{
			MessageBox(NULL, L"hDisassemblyTextBox not created", 0, 0);
			fSuccess = FALSE; __leave;
		}

        SendMessage(hDisassemblyTextBox, WM_SETTEXT, 0, (LPARAM)L"-- Under Construction --\r\nDisassembly area");

        // Create Debuggee information Window, which is a ListBox
		hDebuggeeInfoTextBox = CreateWindowEx(
							    0, L"EDIT",
							    NULL,
							    WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | 
                                ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 
							    (int)(rClientArea.right*0.7)+10,                     // x pos
                                5,
							    (int)(rClientArea.right*0.3)-15,		            // x size
							    (int)(rClientArea.bottom*0.7),                      // y size
							    hMainWindow,
							    (HMENU)IDC_EDIT_DEBUG_INFO,
							    g_hMainInstance,
							    NULL);
		if(!hDebuggeeInfoTextBox)
		{
			MessageBox(NULL, L"hDebuggeeInfoTextBox not created", 0, 0);
			fSuccess = FALSE; __leave;
		}
        SendMessage(hDebuggeeInfoTextBox, WM_SETTEXT, 0, (LPARAM)L"-- Under Construction --\r\nMemory info area");

        // Create Self Debug information Window, which is a ListBox
		hDebugEventsTextBox = CreateWindowEx(
							    0, L"EDIT",
							    NULL,
							    WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | 
                                ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 
							    5,                       // x pos
							    rClientArea.bottom - (int)(rClientArea.bottom*0.3) + 5,	// y pos
							    (int)(rClientArea.right)-10,		                    // x size
							    (int)(rClientArea.bottom*0.2-7),                        // y size
							    hMainWindow,
							    (HMENU)IDC_EDIT_SELF_DEBUG,
							    g_hMainInstance,
							    NULL);
		if(!hDebugEventsTextBox)
		{
			MessageBox(NULL, L"hDebugEventsTextBox not created", 0, 0);
			fSuccess = FALSE; __leave;
		}

        // configure menu items
        fMiDebuggerInit(hMainMenu);

    }
    __finally
    {
        if(fSuccess)
            wprintf(L"UI Initialization complete!\n");
        else
            wprintf(L"UI Initialization failure!\n");
    }

    return fSuccess;

}// fInitUIElements()


// fOnLoadNewProcess()
//
BOOL fOnLoadNewProcess(HWND hMainWnd)
{
    OPENFILENAME ofn;
    WCHAR wsFilters[] = L"Executables\0*.exe\0\0";
    DWORD dwFlags = 0;
    WORD wFilenameOffset = 0;
    WORD wFileExOffset = 0;
    DWORD dwRetVal = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WCHAR wsProcInfo[MAX_PATH] = L"";


    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = wsFilters;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = wsDebuggeeFilePath;
    ofn.nMaxFile = sizeof(wsDebuggeeFilePath);
    dwFlags = OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NONETWORKBUTTON;

    if(!GetOpenFileName(&ofn))
    {
        wprintf(L"fOnLoadNewProcess(): Could not get a file name");
        dwRetVal = CommDlgExtendedError();
        return FALSE;
    }

    wprintf(L"User specified debuggee file\n");
    wprintf(L"%s\n", wsDebuggeeFilePath);
    wprintf(L"%s\n", wsDebuggeeFilePath+ofn.nFileOffset);
    wcscpy_s(wsDebuggeeFileName, _countof(wsDebuggeeFileName), wsDebuggeeFilePath+ofn.nFileOffset);
    // todo: check the returned file path

    // open the image file
    hFile = CreateFile(wsDebuggeeFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        if(GetLastError() == ERROR_FILE_NOT_FOUND)
            MessageBox(NULL, L"File not found!", L"Error", MB_OK);
        else
            MessageBox(NULL, L"Unable to open file!", L"Error", MB_OK);
        return FALSE;
    }
    CloseHandle(hFile);
    
    wprintf_s(L"Debuggee: %s\n", wsDebuggeeFileName);
    wprintf_s(L"Path: %s\n", wsDebuggeeFilePath);

    fMiProcessOpened(hMainMenu);
    
    return TRUE;

}// fOnLoadNewProcess()


// fOnStartDebugSession()
// 
BOOL fOnStartDebugSession()
{
    st_WD_BeginDebugArgs.fDebugActiveProcess = fDebugActiveProcess;
    st_WD_BeginDebugArgs.dwTargetProcessID = dwActiveProcessID;
    wcscpy_s(st_WD_BeginDebugArgs.wsTargetFilename, _countof(wsDebuggeeFileName), wsDebuggeeFileName);
    wcscpy_s(st_WD_BeginDebugArgs.wsTargetPath, _countof(wsDebuggeeFilePath), wsDebuggeeFilePath);

    return SetEvent(ahGWEvents[INDEX_GW_START]);
}// fOnStartDebugSession()


// fAboutDP()
// 
BOOL CALLBACK fAboutDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
	{
		case WM_INITDIALOG:
		{
			DoCenterWindow(hDlg);
            return TRUE;
        }

        case WM_COMMAND:	
		{
			// handle command sent from child window controls
			switch(LOWORD(wParam))
			{
                case IDB_OK:
                    SendMessage(hDlg, WM_CLOSE, 0, 0);
					return TRUE;
            }
        }

        case WM_CLOSE:
		{
			EndDialog(hDlg, 0);
			return TRUE;
		}

    }
    return FALSE;

}// fAboutDP()


// fGetProcIdDP()
//
BOOL CALLBACK fGetProcIdDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hProcIDList;
    static DWORD *paProcIDs;
    static DWORD nProcIDs;
    static DWORD nListItems;
    
    static int iSelectedListItem;
    LPNMLISTVIEW lpNMListView = NULL;

    switch(message)
	{
		case WM_INITDIALOG:
		{
            LVCOLUMN lvColumn = {0};

            paProcIDs = NULL;
            nProcIDs = 0;
            iSelectedListItem = -1;
            dwActiveProcessID = 0;

			DoCenterWindow(hDlg);

            hProcIDList = GetDlgItem(hDlg, IDC_LIST_PROCIDS);

            // List view headers
			lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;    //type of mask

			// first column
			lvColumn.pszText = L"Index";                      //First Header Text
			lvColumn.cx = 0x30;                                   //width of column
			SendMessage(hProcIDList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn); //Insert the column

			// second column
			lvColumn.pszText = L"Process ID"; 
			lvColumn.cx = 0x60;
			SendMessage(hProcIDList, LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);
			
			// third column
			lvColumn.pszText = L"Process Name";                            
			lvColumn.cx = 0x90;
			SendMessage(hProcIDList, LVM_INSERTCOLUMN, 2, (LPARAM)&lvColumn);

            // populate list with active processes' ID
            if(!fDisplayActiveProcesses(hProcIDList, &paProcIDs, &nProcIDs, &nListItems))
            {
                ChlvMemFree((void**)&paProcIDs);
                MessageBox(hDlg, L"Cannot display active processes", L"Error", MB_ICONERROR);
                SendMessage(hDlg, WM_CLOSE, 0, 0);
            }

            return TRUE;
        }

        case WM_COMMAND:
		{
			// handle command sent from child window controls
			switch(LOWORD(wParam))
			{
                case IDB_OPENPROC:
                    if(iSelectedListItem < 0 || iSelectedListItem > nListItems)
                    {
                        MessageBox(hDlg, L"You did not select a process to debug", L"No Process", MB_ICONMASK);
                    }
                    else
                    {
                        LVITEM lvSelItem = {0};
                        WCHAR wsProcID[11];

                        lvSelItem.iItem = iSelectedListItem;
                        lvSelItem.iSubItem = 1;
                        lvSelItem.mask = LVIF_TEXT;
                        lvSelItem.pszText = wsProcID;
                        lvSelItem.cchTextMax = _countof(wsProcID);

                        if(!SendDlgItemMessage(hDlg, IDC_LIST_PROCIDS, LVM_GETITEM, 0, (LPARAM)&lvSelItem))
                            MessageBox(hDlg, L"Error retrieving selected process's PID", L"Error", MB_OK);
                        else
                            dwActiveProcessID = _wtoi(lvSelItem.pszText);
                        ChlvMemFree((void**)&paProcIDs);
                        SendMessage(hDlg, WM_CLOSE, 0, 0);
                    }
                    return TRUE;

                case IDB_REFRESH:
                    iSelectedListItem = -1;
                    // populate list with active processes' ID
                    if(!fDisplayActiveProcesses(hProcIDList, &paProcIDs, &nProcIDs, &nListItems))
                    {
                        ChlvMemFree((void**)&paProcIDs);
                        MessageBox(hDlg, L"Cannot display active processes", L"Error", MB_ICONERROR);
                        SendMessage(hDlg, WM_CLOSE, 0, 0);
                    }
                    return TRUE;
                
                case IDB_CANCEL:
                    ChlvMemFree((void**)&paProcIDs);
                    SendMessage(hDlg, WM_CLOSE, 0, 0);
					return TRUE;
            }
            break;
        }

        case WM_NOTIFY:
        {
            switch ( ((LPNMHDR)lParam)->code )
			{
                case LVN_ITEMCHANGED:
				{
					if( (lpNMListView = (LPNMLISTVIEW)lParam) == NULL )
					{
						// error
						return 0;
					}
					if( lpNMListView->uNewState & LVIS_SELECTED )
						if( lpNMListView->iItem != -1 )
							iSelectedListItem = lpNMListView->iItem;
					
					return 0;	// actually, no return value is required
				}
            }
            break;

        }// WM_NOTIFY

        case WM_CLOSE:
		{
			EndDialog(hDlg, 0);
			return TRUE;
		}

    }
    return FALSE;
}// fGetProcIdDP()


// fDisplayActiveProcesses()
//
BOOL fDisplayActiveProcesses(HWND hProcIDList, DWORD **paProcIDs, DWORD *pnProcIDs, DWORD *pnItemsShown)
{
    ASSERT(hProcIDList && hProcIDList != INVALID_HANDLE_VALUE);

    DWORD dwBytesReturned = 0;
    DWORD numProcIDsReturned = 0;
    WCHAR wsDbgMessage[512];
    LVITEM lvItem;

    WCHAR wsIndex[5];
    WCHAR wsProcID[11];
    WCHAR wsProcName[MAX_PATH+1];

    int iRetVal = 0;
    int nItemsShown = -1;

    if(!ChlfMemAlloc((void**)paProcIDs, 1024*sizeof(DWORD), NULL))
    {
        swprintf_s(wsDbgMessage, _countof(wsDbgMessage), L"fDisplayActiveProcesses(): mem alloc failed");
        logerror(wsDbgMessage);
        return FALSE;
    }
    ZeroMemory(*paProcIDs, 1024*sizeof(DWORD));

    // Clear list view
    if(!SendMessage(hProcIDList, LVM_DELETEALLITEMS, 0, 0))
        return FALSE;

    // list processes in the system
    if(!EnumProcesses(*paProcIDs, 1024*sizeof(DWORD), &dwBytesReturned))
    {
        swprintf_s(wsDbgMessage, _countof(wsDbgMessage), 
            L"Failed to enumerate processes in the system. Sorry. %d", GetLastError());
        logerror(wsDbgMessage);
        return FALSE;
    }

    swprintf_s(wsDbgMessage, _countof(wsDbgMessage), 
            L"Enumerated %d processes", dwBytesReturned/sizeof(DWORD));
    loginfo(wsDbgMessage);
    *pnProcIDs = dwBytesReturned/sizeof(DWORD);
    for(DWORD index = 0; index < dwBytesReturned/sizeof(DWORD); ++index)
    {
        if(!GetProcNameFromID((*paProcIDs)[index], wsProcName, _countof(wsProcName)))
        {
            swprintf_s(wsDbgMessage, _countof(wsDbgMessage), 
                L"Error reading proc name for %d %d", (*paProcIDs)[index], GetLastError());
            logerror(wsDbgMessage);
            continue;
        }
        else
        {
            swprintf_s(wsDbgMessage, _countof(wsDbgMessage), 
                L"%5d %s", (*paProcIDs)[index], wsProcName);
            loginfo(wsDbgMessage);
        }

        // insert into the list
        ZeroMemory(&lvItem, sizeof(lvItem));

        ++nItemsShown;

        // index
        swprintf_s(wsIndex, _countof(wsIndex), L"%d", index);
        lvItem.iItem = nItemsShown;
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = wsIndex;
		lvItem.cchTextMax = wcsnlen_s(wsIndex, _countof(wsIndex));
		if( (iRetVal = SendMessage(hProcIDList, LVM_INSERTITEM, 0, (LPARAM)&lvItem)) == -1 )
		{
			// error
			iRetVal = GetLastError();
            swprintf_s(wsDbgMessage, _countof(wsDbgMessage), L"fDisplayActiveProcesses(): List insert failed %d", iRetVal);
            logerror(wsDbgMessage);
			break;
		}

        // proc ID
        swprintf_s(wsProcID, _countof(wsProcID), L"%d", (*paProcIDs)[index]);
        lvItem.iSubItem = 1;
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = wsProcID;
		lvItem.cchTextMax = wcsnlen_s(wsProcID, _countof(wsProcID));
		if( (iRetVal = SendMessage(hProcIDList, LVM_SETITEM, 0, (LPARAM)&lvItem)) == -1 )
		{
			// error
			iRetVal = GetLastError();
            swprintf_s(wsDbgMessage, _countof(wsDbgMessage), L"fDisplayActiveProcesses(): List insert failed %d", iRetVal);
            logerror(wsDbgMessage);
			break;
		}

        // proc name
        lvItem.iSubItem = 2;
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = wsProcName;
		lvItem.cchTextMax = wcsnlen_s(wsProcName, _countof(wsProcName));
		if( (iRetVal = SendMessage(hProcIDList, LVM_SETITEM, 0, (LPARAM)&lvItem)) == -1 )
		{
			// error
			iRetVal = GetLastError();
            swprintf_s(wsDbgMessage, _countof(wsDbgMessage), L"fDisplayActiveProcesses(): List insert failed %d", iRetVal);
            logerror(wsDbgMessage);
			break;
		}
    }
    
    *pnItemsShown = nItemsShown;
    return TRUE;

}// fDisplayActiveProcesses()

BOOL fStartWatchDogThread(HWND hMainWindow)
{
    ASSERT(IS_VALID_HANDLE(hMainWindow));
    
    st_WD_ThreadArgs.hGUIWindow = hMainWindow;
    st_WD_ThreadArgs.hMainMenu = hMainMenu;
    st_WD_ThreadArgs.hMainInstance = g_hMainInstance;

    // create the GW events
    if(!CreateGWEvents(ahGWEvents, GetCurrentProcessId()))
    {
        logerror(L"Could not create GW events");
        return FALSE;
    }

    hWatchDogThread = CreateThread(NULL, 0, WatchDogThreadEntry, (LPVOID)&st_WD_ThreadArgs, 0, NULL);
    if(hWatchDogThread == NULL)
    {
        logerror(L"Could not start watch dog thread");
        goto error_return;
    }

    return TRUE;

    error_return:
    vCloseHandles(ahGWEvents, _countof(ahGWEvents));
    return FALSE;
}


BOOL fCreateConsoleWindow()
{
    BOOL fError = TRUE;

    __try
    {
        if(!AllocConsole())
        {
            DWORD lasterror  = GetLastError();
            MessageBox(NULL, L"Could not allocate new console", L"Error", MB_OK);
        }
        else
        {
            // Thanks to: http://dslweb.nwnexus.com/~ast/dload/guicon.htm and MSDN
            FILE *fp = NULL;
            HANDLE hInputHandle = GetStdHandle(STD_INPUT_HANDLE);
            HANDLE hOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            HANDLE hErrorHandle = GetStdHandle(STD_ERROR_HANDLE);

            if(hInputHandle == INVALID_HANDLE_VALUE || hOutputHandle == INVALID_HANDLE_VALUE || 
                hErrorHandle == INVALID_HANDLE_VALUE)
            {
                __leave;
            }

            if( (iConInHandle = _open_osfhandle((intptr_t)hInputHandle, _O_RDONLY|_O_TEXT)) == -1 )
                __leave;

            fp = _fdopen( iConInHandle, "r" );
            *stdin = *fp;

            if( (iConOutHandle = _open_osfhandle((intptr_t)hOutputHandle, _O_APPEND|_O_TEXT)) == -1 )
                __leave;

            fp = _fdopen( iConOutHandle, "w+" );
            *stdout = *fp;

            if( (iConErrHandle = _open_osfhandle((intptr_t)hErrorHandle, _O_APPEND|_O_TEXT)) == -1 )
                __leave;

            fp = _fdopen( iConErrHandle, "w+" );
            *stderr = *fp;

            fError = FALSE;
        }
    }
    __finally
    {
        if(fError)
        {
            if(iConInHandle != -1) { _close(iConInHandle); iConInHandle = -1; }
            if(iConOutHandle != -1) { _close(iConOutHandle); iConOutHandle = -1; }
            if(iConErrHandle != -1) { _close(iConErrHandle); iConErrHandle = -1; }
            FreeConsole();
        }
    }
    
    return fError;
}


// vOnDebuggerExit()
//
void vOnDebuggerExit()
{
    // send exit event to watch dog thread
    SetEvent(ahGWEvents[INDEX_GW_EXITTHREAD]);
    
    logtrace(L"vOnDebuggerExit(): Waiting for 3s for watch dog thread to exit");
    if(WaitForSingleObject(hWatchDogThread, 3000) != WAIT_OBJECT_0)
    {
        logerror(L"Watchdog thread did not exit in alloted time. Killing it...");
        if(!TerminateThread(hWatchDogThread, 666))
            logerror(L"TerminateThread() failed! :-(");
    }
    PURGE_HANDLE(hWatchDogThread);
    vCloseHandles(ahGWEvents, _countof(ahGWEvents));

    if(iConInHandle != -1) _close(iConInHandle);
    if(iConOutHandle != -1) _close(iConOutHandle);
    if(iConErrHandle != -1) _close(iConErrHandle);
    iConInHandle = iConOutHandle = iConErrHandle = -1;

    if(!FreeConsole())
    {
        DWORD lasterror  = GetLastError();
        logerror(L"vCleanUpDebugSession(): Could not free console");
    }
    EnableMenuItem(hMainMenu, IDM_CONSOLE_CLOSECONSOLE, MF_GRAYED);

    logtrace(L"CWinDebugger closing. I'll be back...");
    fLoCloseLogger();
}
