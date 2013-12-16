#pragma once

#if defined(IPAD)
	#include "UsgTexturesIosHD.h"
#elif defined(IPHONEOS)
	#include "UsgTexturesIos.h"
#elif defined(MACOS)
	#include "UsgTexturesMac.h"
#elif defined(PSP)
	#include "UsgTexturesPsp.h"
#elif defined(WIN32)
	#include "UsgTexturesWin.h"
#elif defined(BBOS)
	#include "UsgTexturesBbos.h"
#endif
