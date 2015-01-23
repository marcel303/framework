#include <stdarg.h>
#include <stdio.h>
#include "Exception.h"
#include "StringEx.h"

#if defined(PSP) || defined(__GNUC__)
#define sprintf_s(dst, dstSize, ...) sprintf(dst, __VA_ARGS__)
#define vsprintf_s(dst, dstSize, format, va) vsprintf(dst, format, va)
#endif

Exception::Exception(const char* function, int line) throw() : std::exception()
{
	sprintf_s(m_What, sizeof(m_What) / sizeof(char), "%s: %d", function, line);
}

Exception::Exception(const char* function, int line, const char* _what, ...) throw() : std::exception()
{
	va_list va;
	va_start(va, _what);
	char temp[1024];
	vsprintf_s(temp, sizeof(temp) / sizeof(char), _what, va);
	va_end(va);
	
	sprintf_s(m_What, sizeof(m_What) / sizeof(char), "%s: %d: %s", function, line, temp);
}

Exception::~Exception() throw()
{
}

const char* Exception::what() const throw()
{
	return m_What;
}
