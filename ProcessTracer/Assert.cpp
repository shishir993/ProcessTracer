
// Assert.cpp
// Contains functions that provides the ASSERT() service
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      06/23/13 Initial version
//

#include "Includes\Assert.h"

void vAssert(const char* psFile, unsigned int uLine)
{
	fflush(NULL);		// ensure that all buffers are written out first
	fprintf(stderr, "Assertion failed in %s at Line %u\n", psFile, uLine);
	fflush(stderr);

    if(fIsLoggerInitialized())
    {
        WCHAR wsErrorMessage[1024];

        swprintf_s(wsErrorMessage, _countof(wsErrorMessage), 
            L"vAssert(): Assertion failed in %S at Line %u", psFile, uLine);
        logerror(wsErrorMessage);
        fLoCloseLogger();
    }

	exit(0x666);
}// _Assert()
