#pragma once

#ifdef DEBUG
#include <stdarg.h>
#include <stdio.h>
static void PSD_LOG_DBG(const char* text, ...)
{
	va_list va;
	va_start(va, text);
	char temp[1024];
	vsprintf(temp, text, va);
	va_end(va);

	printf("%s", temp);
	printf("\n");
}
#else
static inline void PSD_LOG_DBG(const char*, ...)
{
}
#endif
