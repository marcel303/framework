#pragma once

#if defined(IPHONEOS)
#include "System_iPhone.h"
extern System_iPhone g_System;
#elif defined(WIN32)
#include "System_Win32.h"
extern System_Win32 g_System;
#elif defined(LINUX)
#include "System_Linux.h"
extern System_Linux g_System;
#elif defined(MACOS)
#include "System_Linux.h"
extern System_Linux g_System; // TODO: create MacOS system
#elif defined(PSP)
#include "System_Psp.h"
#elif defined(BBOS)
#include "System_Bbos.h"
extern System_Bbos g_System;
#else
	#error system not set
#endif

#
