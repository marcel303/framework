#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "Debugging.h"
#include "Log.h"
#include "StringEx.h"

//

#ifdef DEBUG

void HandleAssert(const char * func, int line, const char * expr, ...)
{
	char text[1024];
	va_list args;
	va_start(args, expr);
#if defined(IPHONEOS) || defined(MACOS) || defined(LINUX)
    vsprintf(text, expr, args);
#else
	vsprintf_s(text, expr, args);
#endif
	va_end(args);

	LOG_ERR("assertion failed: %s: %d: %s", func, line, text);
	//assert(false);
}

#endif

//

//static LogCtx g_Log("dbg");
