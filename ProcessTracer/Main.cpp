
// Main.cpp
// Contains WinMain()
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/08/13 Initial version
//

#include "CommonInclude.h"
#include "WndProc.h"

HWND g_hMainWnd;
HINSTANCE g_hMainInstance;


// WinMain()
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    MSG MainWndMsg;
	WNDCLASS MainWndClass;

	WCHAR szAppName[] = STR_APPNAME;

    int iScreenX, iScreenY, iWndX, iWndY, iWidth, iHeight;
	RECT rcMainWnd;

	g_hMainInstance = hInstance;

	// UI Window Class
	MainWndClass.style		 = CS_HREDRAW | CS_VREDRAW;
	MainWndClass.lpfnWndProc = WndProc;
	MainWndClass.cbClsExtra	 = 0;
	MainWndClass.cbWndExtra  = 0;
	MainWndClass.hInstance	 = g_hMainInstance;
	MainWndClass.hIcon		 = NULL;//LoadIcon(g_hMainInstance, MAKEINTRESOURCE(IDB_BITMAP1));
	MainWndClass.hCursor	 = LoadCursor(NULL, IDC_ARROW);
	MainWndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	MainWndClass.lpszMenuName  = MAKEINTRESOURCE(IDM_MAIN);
	MainWndClass.lpszClassName = szAppName;

	if(!RegisterClass(&MainWndClass))
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"), szAppName, MB_ICONERROR);
		return 1;
	}

    // Initialize common controls
	InitCommonControls();

	// Create the main window
	g_hMainWnd = CreateWindow(szAppName,		// class name
						szAppName,				// caption
						WS_CAPTION | 
						WS_MINIMIZEBOX |
						WS_SYSMENU,				// window style
						CW_USEDEFAULT,			// initial X position
						CW_USEDEFAULT,			// initial Y position
						1025,					// initial X size
						576,					// initial Y size
						NULL,					// parent window handle
						NULL,					// window menu handle
						g_hMainInstance,		// program instance handle
						NULL);

	// exit if window was not created
	if( !g_hMainWnd )
	{
		MessageBox(0, L"Main Window creation error. Cannot continue.", 0, 0);
		return 1;
	}

	// centre the main window in the screen

	// get the screen co-ordinates
	iScreenX = GetSystemMetrics(SM_CXSCREEN);
	iScreenY = GetSystemMetrics(SM_CYSCREEN);

	// get window rect and calculate the main window dimensions
	GetWindowRect(g_hMainWnd, &rcMainWnd);
	iWidth = rcMainWnd.right - rcMainWnd.left;
	iHeight = rcMainWnd.bottom - rcMainWnd.top;

	// calculate the new co-ordinates for the main window
	iWndX = iScreenX / 2 - iWidth / 2;
	iWndY = iScreenY / 2 - iHeight / 2;
	
	MoveWindow(g_hMainWnd, iWndX, iWndY, iWidth, iHeight, FALSE);

    ShowWindow(g_hMainWnd, iCmdShow);
	UpdateWindow(g_hMainWnd);

	while( GetMessage(&MainWndMsg, NULL, 0, 0) )
	{
		TranslateMessage(&MainWndMsg);
		DispatchMessage(&MainWndMsg);
	}
	
	return MainWndMsg.wParam;

} // WinMain()
