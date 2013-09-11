
// Logger.h
// Contains logging functions that write to a log file
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      07/09/13 Initial version(thread-unsafe)
//

#include "Logger.h"

#define LOGFILEPATH     L".\\cwdlog.txt"

static HANDLE hLogFile = NULL;
static FILE *fp = NULL;
static BOOL fLoggerInitialized = FALSE;

// fLoInitLogger()
// Opens the log file. Truncates if it exists. Creates a new file
// if one doesn't exist.
BOOL fLoInitLogger()
{
    int iRetVal = 0;

    // open file in write mode
	if( (iRetVal = _wfopen_s(&fp, LOGFILEPATH, L"w")) != 0 )
		return FALSE;

    fLoWriteLogLine(L"Logger initialized", LOG_INFO);
    fLoggerInitialized = TRUE;
    return TRUE;
}


BOOL logErrorDetail(WCHAR *pwsFormat, ...)
{
    ASSERT(pwsFormat);

    // WCHAR wsLogMessage[MAX_LOG_STR];

    //swprintf_s(wsLogMessage, _countof(wsLogMessage), pwsFormat, );
    return FALSE;
}


BOOL fLoWriteLogLine(const WCHAR *pwsMessage, INT sev)
{
    ASSERT(fp);
    ASSERT(pwsMessage);
    ASSERT(sev >= LOG_INFO && sev <= LOG_TRACE);

    WCHAR wsFormattedMsg[MAX_LOG_STR+32];
    static WCHAR awsSeverity[][7] = {L"[INFO]", L"[WARN]", L"[EROR]", L"[TRCE]"};

    SYSTEMTIME stCurrentTime;

    int iRetVal = 0;

	// get time
	GetLocalTime(&stCurrentTime);
	
	//construct the string to be written
    swprintf_s(wsFormattedMsg, MAX_LOG_STR+32, L"%s[%02d:%02d:%02d.%03d]%s\n", awsSeverity[sev], stCurrentTime.wHour, 
		stCurrentTime.wMinute, stCurrentTime.wSecond, stCurrentTime.wMilliseconds, pwsMessage);

    //return WriteFile(hLogFile, wsFormattedMsg, wcslen(wsFormattedMsg), NULL, NULL);
    if( (iRetVal = fwprintf_s(fp, L"%s", wsFormattedMsg)) < 0 )
		return FALSE;
    return TRUE;
}


BOOL fLoCloseLogger()
{
    ASSERT(fp);

    fLoWriteLogLine(L"Logger terminating", LOG_INFO);
    fclose(fp);
    fp = NULL;
    fLoggerInitialized = FALSE;
    return TRUE;
}

BOOL fIsLoggerInitialized()
{
    return fLoggerInitialized;
}
