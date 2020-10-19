#pragma once

#include <thread>

inline void SetCurrentThreadName(const char * name)
{
#if defined(MACOS)
	pthread_setname_np(name);
#elif defined(LINUX)
	pthread_setname_np(pthread_self(), name);
#elif defined(WINDOWS)
	SetThreadName(name);
#else
	#error
#endif
}
