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
			#if defined(DEBUG) && defined(DEBUG_MEDIAPLAYER)
			static char string[4096];

			va_list list;
			va_start(list, format);
			vsprintf(string, format, list);

			std::cerr << string << std::endl;

			#if defined(SYSTEM_WINDOWS)
			strcat(string, "\n");
			OutputDebugString(string);
			#endif
			#endif
		}
	};
};