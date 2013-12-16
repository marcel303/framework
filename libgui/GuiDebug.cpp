#include "libgui_precompiled.h"
#include <stdarg.h>
#include <stdio.h>
#include "GuiDebug.h"

namespace Gui
{
	namespace Debug
	{
		void Print(const char* format, ...)
		{
			#if defined(DEBUG)
			static char string[4096];

			va_list list;
			va_start(list, format);
			vsprintf(string, format, list);

			printf("%s\n", string);
			#endif
		}
	};
};
