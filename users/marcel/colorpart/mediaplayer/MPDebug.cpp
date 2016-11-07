#include "MPDebug.h"
#include <stdarg.h>
#include <stdio.h>

#if defined(SYSTEM_WINDOWS)
	#include <windows.h>
#endif

namespace MP
{
	namespace Debug
	{
		void Print(const char* format, ...)
		{
			#if DEBUG_MEDIAPLAYER
			static char string[4096];

			va_list list;
			va_start(list, format);
			vsprintf(string, format, list);

			fprintf(stderr, "%s\n", string);

			#if defined(SYSTEM_WINDOWS)
			strcat(string, "\n");
			OutputDebugString(string);
			#endif
			#endif
		}
	};
};