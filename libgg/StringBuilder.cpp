#include "StringBuilder.h"
#include "StringEx.h"
#include <stdarg.h>

static const int STRING_BUFFER_SIZE = (1 << 12);

void StringBuilder::AppendFormat(const char * format, ...)
{
	va_list va;
	va_start(va, format);
	char text[STRING_BUFFER_SIZE];
	vsprintf_s(text, sizeof(text), format, va);
	va_end(va);

	Append(text);
}

