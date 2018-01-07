#pragma once

#if __SSE2__
	#include <xmmintrin.h>
	#define HAS_MM_MALLOC 1
#else
	#include <stdlib.h>
	#define HAS_MM_MALLOC 0
#endif

inline void * MemAlloc(size_t size, size_t align)
{
#if HAS_MM_MALLOC
	return _mm_malloc(size, align);
#else
	void * ptr;
	
	if (posix_memalign(&ptr, align, size) == 0)
		return ptr;
	else
		return nullptr;
	//return aligned_alloc(align, size);
#endif
}

inline void MemFree(void * ptr)
{
#if HAS_MM_MALLOC
	return _mm_free(ptr);
#else
	free(ptr);
#endif
}
