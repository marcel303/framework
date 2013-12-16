#include <xmmintrin.h>
#include "mem.h"

void * operator new(size_t size)
{
	return _mm_malloc(size, 16);
}

void operator delete(void * p)
{
	_mm_free(p);
}

void operator delete[](void * p)
{
	_mm_free(p);
}
