#pragma once

#if defined(__GNUC__)
#define ALIGN_CLASS(x) class __attribute__ ((aligned(x)))
#else
#define ALIGN_CLASS(x) __declspec(align(x)) class
#endif

namespace Mem
{
	extern void ClearZero(void* p, size_t byteCount);
	extern void ClearFill(void* p, unsigned char c, size_t byteCount);
}
