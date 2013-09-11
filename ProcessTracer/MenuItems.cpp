
// MenuItems.h
// Contains functions that help in handling enabling/disabling of menu items
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/08/13 Initial version
//

#include "MenuItems.h"

// fMiDebuggerInit()
//
BOOL fMiDebuggerInit(HMENU hMenu)
{
    ASSERT(hMenu && hMenu != INVALID_HANDLE_VALUE);

    EnableMenuItem(hMenu, IDM_DEBUG_RECHOOSEPROCESS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_STARTDEBUGSESSION, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_TERMINATEDEBUGGEE, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_DETACHFROMDEBUGGEE, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_SUSPENDTHREADS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_RESUMETHREADS, MF_GRAYED);
    return TRUE;
}

// fMiProcessOpened()
// 
BOOL fMiProcessOpened(HMENU hMenu)
{
    EnableMenuItem(hMenu, IDM_DEBUG_STARTDEBUGSESSION, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DEBUG_RECHOOSEPROCESS, MF_ENABLED);

    EnableMenuItem(hMenu, IDM_DEBUG_OPENACTIVEPROCESS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_LOADNEWPROCESS, MF_GRAYED);
    return TRUE;
}

// fMiDebugSessionStart()
//
BOOL fMiDebugSessionStart(HMENU hMenu)
{
    EnableMenuItem(hMenu, IDM_DEBUG_TERMINATEDEBUGGEE, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DEBUG_DETACHFROMDEBUGGEE, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DEBUG_SUSPENDTHREADS, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DEBUG_RESUMETHREADS, MF_ENABLED);

    EnableMenuItem(hMenu, IDM_DEBUG_STARTDEBUGSESSION, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_RECHOOSEPROCESS, MF_GRAYED);
    return TRUE;
}

// fMiDebugSessionEnd()
//
BOOL fMiDebugSessionEnd(HMENU hMenu)
{
    EnableMenuItem(hMenu, IDM_DEBUG_OPENACTIVEPROCESS, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DEBUG_LOADNEWPROCESS, MF_ENABLED);

    EnableMenuItem(hMenu, IDM_DEBUG_RECHOOSEPROCESS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_STARTDEBUGSESSION, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_TERMINATEDEBUGGEE, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_DETACHFROMDEBUGGEE, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_SUSPENDTHREADS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DEBUG_RESUMETHREADS, MF_GRAYED);
    return TRUE;
}
