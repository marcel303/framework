#include "ThreadName.h"

#if defined(WINDOWS)
	#include <Windows.h>
	
	static const DWORD MS_VC_EXCEPTION = 0x406D1388;

	#pragma pack(push,8)
	struct THREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	};
	#pragma pack(pop)
#else
	#include <thread>
#endif

void SetCurrentThreadName(const char * name)
{
#if defined(MACOS) || defined(IPHONEOS)
	pthread_setname_np(name);
#elif defined(LINUX) || defined(ANDROID)
	pthread_setname_np(pthread_self(), name);
#elif defined(WINDOWS)
	THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = (DWORD)-1;
    info.dwFlags = 0;
	#pragma warning(push)
	#pragma warning(disable: 6320 6322)
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
#pragma warning(pop)
#else
	#error
#endif
}
