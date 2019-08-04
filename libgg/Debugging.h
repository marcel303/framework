#pragma once

#ifdef DEBUG
	void HandleAssert(const char * func, int line, const char * expr, ...);
	#define Assert(x) do { if (!(x)) { HandleAssert(__FUNCTION__, __LINE__, #x); } } while (false)
	#define AssertMsg(x, msg, ...) do { if (!(x)) { HandleAssert(__FUNCTION__, __LINE__, "%s: " msg, #x, __VA_ARGS__); } } while (false)
	#define Verify(x) Assert(x)
	#define VerifyMsg AssertMsg
#else
	#define Assert(x) do { } while (false)
	#define AssertMsg(x, msg, ...) do { } while (false)
	#define Verify(x) do { const bool y = x; (void)y; } while (false)
	#define VerifyMsg(x, msg, ...) do { const bool y = x; (void)y; } while (false)
#endif
