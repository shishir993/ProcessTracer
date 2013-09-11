
// Helpers.h
// Contains handy helper defines and functions
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      06/23/13 Initial version
//

#ifndef _HELPERS_H
#define _HELPERS_H

// Pass a pointer and value to this, assignment takes place only if
// the pointer is not NULL
#define IFPTR_SETVAL(ptr, val)  { if(ptr) *ptr = val; }


#endif // _HELPERS_H
