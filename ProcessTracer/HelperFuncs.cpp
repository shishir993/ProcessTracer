
// HelperFuncs.cpp
// Contains functions that help in performing common tasks
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/08/13 Initial version
//


#include "HelperFuncs.h"


// PrintToListBox()
// --
BOOL PrintToListBox(HWND hListBox, const WCHAR *wszMsg)
{
	int iRetVal;
	SYSTEMTIME stCurrentTime;
	WCHAR szStatusMsg[MAX_SDBG_MSG + 1];

	// get time
	GetLocalTime(&stCurrentTime);
	
	//construct the string to be displayed
	swprintf_s(szStatusMsg, MAX_SDBG_MSG + 1, L"[%02d:%02d:%02d.%03d] %s", stCurrentTime.wHour, 
		stCurrentTime.wMinute, stCurrentTime.wSecond, stCurrentTime.wMilliseconds, wszMsg);
	
	// add the string
	iRetVal = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)szStatusMsg);
	if( iRetVal == LB_ERR || iRetVal == LB_ERRSPACE )
		return FALSE;
	
	// make sure the added string is visible
	iRetVal = SendMessage(hListBox, LB_SETTOPINDEX, iRetVal, 0);
	if( iRetVal == LB_ERR )
		return FALSE;

	return TRUE;
}

// PrintToStatusBar()
// --
BOOL PrintToStatusBar(HWND hStatusBar, const WCHAR *wszMsg, int iPart)
{
	// returns TRUE if successful, FALSE otherwise
	return SendMessage(hStatusBar, SB_SETTEXT, iPart, (LPARAM)(LPSTR)wszMsg);
}

// AppendToEditControl()
// Returns TRUE always
BOOL AppendToEditControl(HWND hEditControl, const WCHAR *wszMsg)
{
    int nTextLen;
	SYSTEMTIME stCurrentTime;
	WCHAR szStatusMsg[MAX_SDBG_MSG + 1];

	// get time
	GetLocalTime(&stCurrentTime);
	
	//construct the string to be displayed
	swprintf_s(szStatusMsg, MAX_SDBG_MSG + 1, L"[%02d:%02d:%02d.%03d] %s\r\n", stCurrentTime.wHour, 
		stCurrentTime.wMinute, stCurrentTime.wSecond, stCurrentTime.wMilliseconds, wszMsg);

    nTextLen = GetWindowTextLength(hEditControl);

    // append send line
    SendMessage(hEditControl, EM_SETSEL, nTextLen, nTextLen);
    SendMessage(hEditControl, EM_REPLACESEL, 0, (LPARAM)szStatusMsg);
    return TRUE;
}

// DoCenterWindow()
// Given the window handle, centers the window within the
// parent window's client area.
//
BOOL DoCenterWindow(HWND hWnd)
{
    int x, y;
	int iWidth, iHeight;
	RECT rect, rectP;
	HWND hwndParent = NULL;
	int iScreenWidth, iScreenHeight;

    // Make the window relative to its parent
    if( (hwndParent = GetParent(hWnd)) == NULL)
		return FALSE;

    GetWindowRect(hWnd, &rect);
    GetWindowRect(hwndParent, &rectP);
        
    iWidth  = rect.right  - rect.left;
    iHeight = rect.bottom - rect.top;

    x = ((rectP.right-rectP.left) -  iWidth) / 2 + rectP.left;
    y = ((rectP.bottom-rectP.top) - iHeight) / 2 + rectP.top;

    iScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
    iScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Make sure that the dialog box never moves outside of the screen
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x + iWidth  > iScreenWidth)  x = iScreenWidth  - iWidth;
    if(y + iHeight > iScreenHeight) y = iScreenHeight - iHeight;

    MoveWindow(hWnd, x, y, iWidth, iHeight, FALSE);
    return TRUE;

}// DoCenterWindow()

// CreateGWEvents()
// Create events that are used between GUI and WatchDog Thread (GW events)
//
BOOL CreateGWEvents(LPHANDLE lpEventHandles, DWORD pid)
{
    int rcIndex;
    HINSTANCE hInst;
    WCHAR wsEventName[MAX_PATH+15];
    WCHAR wsEventString[MAX_PATH+15];

    hInst = GetModuleHandle(NULL);

    for(rcIndex = IDS_GW_TERM; rcIndex <= IDS_GW_EXITTHREAD; ++rcIndex)
    {
        if(LoadString(hInst, rcIndex, wsEventString, _countof(wsEventString)) <= 0)
            return FALSE;
        swprintf_s(wsEventName, _countof(wsEventName), L"%s_%d", wsEventString, pid);
        if(!(lpEventHandles[rcIndex-IDS_GW_TERM] = CreateEvent(NULL, TRUE, FALSE, wsEventName)))
            return FALSE;
    }
    return TRUE;

}// CreateGWEvents()


// CreateWDEvents()
// Create events that are used between WatchDog Thread and DebugThread
// (WD and DW events)
//
BOOL CreateWDEvents(LPHANDLE lpEventHandles, DWORD pid)
{
    int rcIndex;
    HINSTANCE hInst;
    WCHAR wsEventName[MAX_PATH+15];
    WCHAR wsEventString[MAX_PATH+15];

    hInst = GetModuleHandle(NULL);

    for(rcIndex = IDS_WD_INITACK; rcIndex <= IDS_DW_TERMINATED; ++rcIndex)
    {
        if(LoadString(hInst, rcIndex, wsEventString, _countof(wsEventString)) <= 0)
            return FALSE;
        swprintf_s(wsEventName, _countof(wsEventName), L"%s_%d", wsEventString, pid);
        if(!(lpEventHandles[rcIndex-IDS_WD_INITACK] = CreateEvent(NULL, TRUE, FALSE, wsEventName)))
            return FALSE;
    }
    return TRUE;

}// CreateWDEvents()


// vCloseHandles()
// Closes all handles passed in as an array of handles
// 
void vCloseHandles(LPHANDLE pHandles, int nHandles)
{
    ASSERT(pHandles && nHandles >= 0);

    for(int i = 0; i < nHandles; ++i)
    {
        PURGE_HANDLE(pHandles[i]);
    }

}// vCloseHandles()


BOOL CenterWindow(HWND hWnd)
{
    int x, y;
	int iWidth, iHeight;
	RECT rect, rectP;
	HWND hwndParent = NULL;
	int iScreenWidth, iScreenHeight;

    //make the window relative to its parent
    if( (hwndParent = GetParent(hWnd)) == NULL)
		return FALSE;

    GetWindowRect(hWnd, &rect);
    GetWindowRect(hwndParent, &rectP);
        
    iWidth  = rect.right  - rect.left;
    iHeight = rect.bottom - rect.top;

    x = ((rectP.right-rectP.left) -  iWidth) / 2 + rectP.left;
    y = ((rectP.bottom-rectP.top) - iHeight) / 2 + rectP.top;

    iScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
    iScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    //make sure that the dialog box never moves outside of
    //the screen
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x + iWidth  > iScreenWidth)  x = iScreenWidth  - iWidth;
    if(y + iHeight > iScreenHeight) y = iScreenHeight - iHeight;

    MoveWindow(hWnd, x, y, iWidth, iHeight, FALSE);

    return TRUE;
}


// GetProcNameFromID()
// 
BOOL GetProcNameFromID(DWORD pid, WCHAR *pwsProcName, DWORD dwCount)
{
    ASSERT(dwCount > 5);    // 3(extension) + 1(.) + 1(imagename)

    HANDLE hProcess = NULL;
    HMODULE ahModules[512];
    DWORD dwNeeded = 0;

    if(pid == 0)
        //logwarn(L"You can't open the System process dude...\n");
        return FALSE;

    if((hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid)) == NULL)
    {
        //logerr("Unable to open process %d", pid);
        return FALSE;
    }

    if(!EnumProcessModules(hProcess, ahModules, sizeof(ahModules), &dwNeeded))
    {
        //logerr("Unable to enumerate modules");
        CloseHandle(hProcess);
        return FALSE;
    }

    //logdbg("#Modules = %d", dwNeeded/sizeof(HMODULE));
    if(!GetModuleBaseName(hProcess, ahModules[0], pwsProcName, dwCount))
    {
        //logerr("Unable to get process name for PID %u", pid);
        CloseHandle(hProcess);
        return FALSE;
    }

    CloseHandle(hProcess);
    return TRUE;

}// GetProcNameFromID()


BOOL fFindNtDllInCurProcess(DWORD *pdwBaseAddress, DWORD *pdwCodeSize)
{
    //HMODULE hmod = LoadLibrary(L"ntdll.dll");

    //DWORD dwCodesize = 0;

    //if(!(dwCodesize = fGetSizeOfCode(hmod)))
    //    return FALSE;

    //DWORD fullsize = (DWORD)hmod + dwCodesize;

    return FALSE;
}
