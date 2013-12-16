#pragma once

#if defined(IPHONEOS) || defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(BBOS)
	#include "Graphics_OpenGL.h"
	extern Graphics_OpenGL gGraphics;
#elif defined(PSP)
	#include "Graphics_Psp.h"
	extern Graphics_Psp gGraphics;
#else
	#error unknown system
#endif
