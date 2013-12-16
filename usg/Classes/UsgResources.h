#pragma once

#if defined(IPAD)
#include "UsgResourcesIosHD.h"
#elif defined(WIN32)
#include "UsgResourcesWin.h"
#elif defined(IPHONEOS) || defined(LINUX)
#include "UsgResourcesIos.h"
#elif defined(MACOS)
#include "UsgResourcesMac.h"
#elif defined(PSP)
#include "UsgResourcesPsp.h"
#elif defined(BBOS)
#include "UsgResourcesBbos.h"
#else
#error
#endif
