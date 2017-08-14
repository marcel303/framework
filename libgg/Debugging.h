#pragma once

#ifdef DEBUG
void HandleAssert(const char * func, int line, const char * expr, ...);
#define Assert(x) do { if (!(x)) { HandleAssert(__FUNCTION__, __LINE__, #x); } } while (false)
#define AssertMsg(x, msg, ...) do { if (!(x)) { HandleAssert(__FUNCTION__, __LINE__, #x, msg, __VA_ARGS__); } } while (false)
#define Verify(x) Assert(x)
#define VerifyMsg AssertMsg
#else
#define Assert(x) do { } while (false)
#define AssertMsg(x, msg, ...) do { } while (false)
#define Verify(x) do { x; } while (false)
#define VerifyMsg(x, msg, ...) do { x; } while (false)
#endif

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
