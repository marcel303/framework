#if defined(PSP)
#include <libvfpu.h>
#endif
#include <string.h>
#include "MemOps.h"

namespace Mem
{
	void ClearZero(void* p, size_t byteCount)
	{
#if defined(IPHONEOS)
		bzero(p, byteCount);
#else
		ClearFill(p, 0, byteCount);
#endif
	}

	void ClearFill(void* p, unsigned char c, size_t byteCount)
	{
#if defined(PSP) && 0
		sceVfpuMemset(p, c, byteCount);
#else
		memset(p, c, byteCount);
#endif
	}
}
