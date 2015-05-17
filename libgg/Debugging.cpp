#include <exception>
#include <string.h>
#if defined(DEBUG) && defined(WIN32) && 0
#include <vld.h>
#endif
#include "Debugging.h"
#include "Heap.h"
#include "Log.h"
#include "MemOps.h"

#ifdef DEBUG
#include <stdlib.h>
#include <new>
#endif

#ifdef DEBUG
	#define DEBUG_MEM 0
#else
	#define DEBUG_MEM 0 // do not alter
#endif

#if DEBUG_MEM
	#define DEBUG_MEM_VERBOSE 0 // additional logging
#else
	#define DEBUG_MEM_VERBOSE 0 // do not alter
#endif

//

#ifdef DEBUG

void HandleAssert(const char * func, int line, const char * expr, ...)
{
	char text[1024];
	va_list args;
	va_start(args, expr);
	vsprintf_s(text, expr, args);
	va_end(args);

	LOG_ERR("assertion failed: %s: %d: %s", func, line, text);
	//assert(false);
}

#endif

//

static LogCtx g_Log("dbg");

static AllocState gAllocState;

AllocState::AllocState()
{
	allocationCount = 0;
	allocationSize = 0;
	totalAllocationCount = 0;
	totalAllocationSize = 0;
	maxAllocationCount = 0;
	maxAllocationSize = 0;
}

#include <new>

class MemoryException : public std::bad_alloc
{
public:
	MemoryException()
	{
	}

	virtual const char* what() const throw()
	{
		return "out of memory";
	}
};

#if DEBUG_MEM

typedef struct AllocInfo
{
	size_t size;
} AllocInfo;

static inline uintptr_t PadAddr(uintptr_t addr, uintptr_t align) { (addr + align - 1) & ~align; }
static const int kAllocFront = PadAddr(sizeof(AllocInfo) + 4, 16);
static const int kAllocBack = 4;

#endif

static void* Alloc(size_t size)
{
#if DEBUG_MEM
	// reserve space for debug info
	size_t allocSize = kAllocFront + size + kAllocBack;
	
	// allocate
	uintptr_t p = reinterpret_cast<uintptr_t>(HeapAlloc(allocSize));
	
	if (p == 0)
	{
		LOG_DBG("attempted to allocate %lu bytes", size);
		DBG_PrintAllocState();
		throw MemoryException();
	}
	
	// allocation record
	AllocInfo* a = (AllocInfo*)p;
	a->size = size;
	p += kAllocFront;
	void * result = (void*)p;
	p -= 4;
	// guard region
	{
		unsigned char * f = (unsigned char*)p;
		f[0] = 0x12; f[1] = 0x23; f[2] = 0x34; f[3] = 0x45;
	}
	p += 4;
	p += size;
	{
		unsigned char * f = (unsigned char*)p;
		f[0] = 0x67; f[1] = 0x78; f[2] = 0x89; f[3] = 0x9a;
	}
	
	// fill pattern
	Mem::ClearFill(result, 0xDC, size);

	// book keeping
	gAllocState.allocationSize += size;
	gAllocState.allocationCount++;
	gAllocState.totalAllocationSize += size;
	gAllocState.totalAllocationCount++;
	if (gAllocState.allocationSize > gAllocState.maxAllocationSize)
		gAllocState.maxAllocationSize = gAllocState.allocationSize;
	if (gAllocState.allocationCount > gAllocState.maxAllocationCount)
		gAllocState.maxAllocationCount = gAllocState.allocationCount;

#if DEBUG_MEM_VERBOSE
	if (size >= 1024 * 64)
	{
		LOG_DBG("allocated %lu bytes", size);
		DBG_PrintAllocState();
	}
#endif

	return result;
#else
	void* p = HeapAlloc(size);

	if (p == 0)
	{
		LOG_DBG("attempted to allocate %lu bytes", size);
		DBG_PrintAllocState();
		throw MemoryException();
	}

	return p;
#endif
}

static void Free(void* _p)
{
#if DEBUG_MEM
	if (_p == 0)
		return;

	uintptr_t p = reinterpret_cast<uintptr_t>(_p);

	p -= kAllocFront;
	_p = (void*)p;

	// allocation record
	AllocInfo* a = (AllocInfo*)p;
	size_t size = a->size;
	p += kAllocFront;
	p -= 4;
	// guard region
	{
		unsigned char * f = (unsigned char*)p;
		Assert(f[0] == 0x12); Assert(f[1] == 0x23); Assert(f[2] == 0x34); Assert(f[3] == 0x45);
	}
	p += 4;
	p += size;
	{
		unsigned char * f = (unsigned char*)p;
		Assert(f[0] == 0x67); Assert(f[1] == 0x78); Assert(f[2] == 0x89); Assert(f[3] == 0x9a);
	}
	p -= size;
	// fill pattern
	Mem::ClearFill((void*)p, 0xDD, size);

	// book keeping
	gAllocState.allocationSize -= size;
	gAllocState.allocationCount--;

	// free
	HeapFree(_p);
#else
	HeapFree(_p);
#endif
}

#if DEBUG_MEM

void* operator new(size_t size) throw(std::bad_alloc)
{
	return Alloc(size);
}

void* operator new[](size_t size) throw(std::bad_alloc)
{
	return Alloc(size);
}

void operator delete(void* p) throw()
{
	Free(p);
}

void operator delete[](void* p) throw()
{
	Free(p);
}

#endif

AllocState GetAllocState()
{
	return gAllocState;
}

void DBG_PrintAllocState()
{
	LOG_DBG("allocSize: %lu", gAllocState.allocationSize);
	LOG_DBG("allocCount: %lu", gAllocState.allocationCount);
	LOG_DBG("totalAllocSize: %lu", gAllocState.totalAllocationSize);
	LOG_DBG("totalAllocCount: %lu", gAllocState.totalAllocationCount);
	LOG_DBG("maxAllocSize: %lu", gAllocState.maxAllocationSize);
	LOG_DBG("maxAllocCount: %lu", gAllocState.maxAllocationCount);
}
