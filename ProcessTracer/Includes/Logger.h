
// Logger.h
// Contains logging functions that write to a log file
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/09/13 Initial version
//

#ifndef _LOGGER_H
#define _LOGGER_H

#include "CommonInclude.h"

#define MAX_LOG_STR 255
#define LOG_INFO    0
#define LOG_WARN    1
#define LOG_ERROR   2
#define LOG_TRACE   3

#define loginfo(msg)    fLoWriteLogLine(msg, LOG_INFO)
#define logwarn(msg)    fLoWriteLogLine(msg, LOG_WARN)
#define logerror(msg)    fLoWriteLogLine(msg, LOG_ERROR)
#define logtrace(msg)    fLoWriteLogLine(msg, LOG_TRACE)

// Functions
BOOL fLoInitLogger();
BOOL logErrorDetail(WCHAR *pwsFormat, ...);
BOOL fLoWriteLogLine(const WCHAR *pwsMessage, INT sev);
BOOL fLoCloseLogger();
BOOL fIsLoggerInitialized();

#endif // _LOGGER_H
