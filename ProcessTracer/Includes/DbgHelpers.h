
// DbgHelpers.h
// Defines and functions to help in debugging
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      06/23/13 Initial version
//

#ifndef _DBGHELPERS_H
#define _DBGHELPERS_H

/**
 * Original work from http://c.learncodethehardway.org/book/ex20.html
 * Modified/additions to my liking
 */

//#define clean_errno() (errno == 0 ? "None" : strerror(errno))
//#define logtrace(M, ...) fprintf(stdout, "[TRACE] " M "\n", ##__VA_ARGS__)
//#define logerr(M, ...)  fprintf(stdout, "[ERROR](LastError: %d) " M "\n", ##__VA_ARGS__, GetLastError())
//#define logwarn(M, ...) fprintf(stdout, "[WARN]  " M "\n", ##__VA_ARGS__)
//#define loginfo(M, ...) fprintf(stdout, "[INFO]  " M "\n", ##__VA_ARGS__)
//
//#ifdef _DEBUG
//
//    #define logdbg(M, ...)  fprintf(stdout, "[DEBUG] " M "\n", ##__VA_ARGS__)
//
//    #define DBG_MEMSET(mem, size) ( memset(mem, 0xCC, size) )
//
//#else
//    #define logdbg(M, ...)
//
//    #define DBG_MEMSET(mem, size)
//#endif

#endif // _DBGHELPERS_H
