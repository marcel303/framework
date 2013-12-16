#include "Graphics.h"

#if defined(IPHONEOS) || defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(BBOS)
Graphics_OpenGL gGraphics;
#elif defined(PSP)
Graphics_Psp gGraphics;
#else
	#error unknown system
#endif
