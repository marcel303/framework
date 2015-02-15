#pragma once

#include <assert.h>
#include "Log.h"

#ifdef DEBUG
void HandleAssert(const char * func, int line, const char * expr, ...);
#define Assert(x) do { if (!(x)) { HandleAssert(__FUNCTION__, __LINE__, #x); } } while (false)
#define AssertMsg(x, msg, ...) do { if (!(x)) { HandleAssert(__FUNCTION__, __LINE__, #x, msg, __VA_ARGS__); } } while (false)
#define Verify(x) Assert(x)
#else
#define Assert(x) do { } while (false)
#define AssertMsg(x, msg, ...) do { } while (false)
#define Verify(x) do { x; } while (false)
#endif

class AllocState
{
public:
	AllocState();
	
	size_t allocationCount;
	size_t allocationSize;
	size_t totalAllocationCount;
	size_t totalAllocationSize;
	size_t maxAllocationCount;
	size_t maxAllocationSize;
};

AllocState GetAllocState();
void DBG_PrintAllocState();

#ifdef MSVC

#define  _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#endif
