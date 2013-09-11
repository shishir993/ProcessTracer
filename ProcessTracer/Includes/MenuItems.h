
// MenuItems.h
// Contains functions that help in performing common tasks
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/08/13 Initial version
//

#ifndef _MENUITEMS_H
#define _MENUITEMS_H

#include "CommonInclude.h"

// Functions
BOOL fMiDebuggerInit(HMENU hMenu);
BOOL fMiProcessOpened(HMENU hMenu);
BOOL fMiDebugSessionStart(HMENU hMenu);
BOOL fMiDebugSessionEnd(HMENU hMenu);

#endif // _MENUITEMS_H
