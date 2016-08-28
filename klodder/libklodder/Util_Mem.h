#pragma once

#include <string.h>

#ifdef IPHONEOS
	#define ClearMemory(ptr, size) bzero(ptr, size)
#else
	#define ClearMemory(ptr, size) memset(ptr, 0x00, size)
#endif

namespace Mem
{
	inline void Copy(const void * __restrict src, void * __restrict dst, int size)
	{
		memcpy(dst, src, size);
	}
}
