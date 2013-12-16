#if defined(IPHONEOS) || defined(MACOS)
#include <memory.h>
#else
#include <malloc.h>
#endif
#include <stdlib.h>
#include "Debugging.h"
#include "Heap.h"

#define ALIGN 16

void HeapInit()
{
}

void* HeapAlloc(size_t size)
{
#if defined(WIN32) || defined(IPHONEOS) || defined(MACOS) || defined(BBOS)
	return malloc(size);
#else
	return memalign(ALIGN, size);
#endif
}

void HeapFree(void* p)
{
	free(p);
}
