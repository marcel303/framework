#include "System.h"

#if defined(IPHONEOS)
System_iPhone g_System;
#elif defined(WIN32)
System_Win32 g_System;
#elif defined(LINUX)
System_Linux g_System;
#elif defined(MACOS)
System_Linux g_System;
#elif defined(PSP)
System_Psp g_System;
#elif defined(BBOS)
System_Bbos g_System;
#else
#error
#endif
