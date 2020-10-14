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
	vsprintf_s(text, sizeof(text), expr, args);
	va_end(args);

	LOG_ERR("assertion failed: %s: %d: %s", func, line, text);
	assert(false);
}

#endif
