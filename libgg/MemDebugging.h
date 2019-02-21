#pragma once

class AllocState
{
public:
	typedef unsigned long long counter_t;
	
	AllocState();
	
	counter_t allocationCount;
	counter_t allocationSize;
	counter_t totalAllocationCount;
	counter_t totalAllocationSize;
	counter_t maxAllocationCount;
	counter_t maxAllocationSize;
};

AllocState GetAllocState();
void DBG_PrintAllocState();

#ifdef MSVC
	#define  _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
#endif
