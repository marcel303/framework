#pragma once

#include <assert.h>
#include "Log.h"

#ifdef DEBUG
inline void Assert(bool x)
{
	if (!x)
	{
		LOG_ERR("assertion failed", 0);
		//assert(x);
	}
}
#else
#define Assert(x) do { } while (false)
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
