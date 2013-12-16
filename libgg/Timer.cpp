#ifdef IPHONEOS
	#include <CoreFoundation/CoreFoundation.h>
#endif
#ifdef MACOS
	#include <CoreFoundation/CoreFoundation.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif
#ifdef LINUX
#include <time.h>
#endif
#ifdef BBOS
#include <time.h>
#endif
#ifdef PSP
#include <rtcsvc.h>
#endif

#include "Debugging.h"
#include "Timer.h"

//

Timer g_TimerRT;

//

ITimer::~ITimer()
{
}

//

#ifdef WIN32
	double GetSystemTime()
	{
		LARGE_INTEGER freq;
		LARGE_INTEGER time;

		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&time);

		return time.QuadPart / (double)freq.QuadPart;
	}
#elif defined(IPHONEOS)
	double GetSystemTime() { return CFAbsoluteTimeGetCurrent(); }
#elif defined(MACOS)
	double GetSystemTime() { return CFAbsoluteTimeGetCurrent(); }
#elif defined(LINUX) || defined(BBOS)
	static timespec sBeginTime;
	static bool sBeginTimeIsInit = false;
	double GetSystemTime()
	{
		if (sBeginTimeIsInit == false)
		{
			clock_gettime(CLOCK_REALTIME, &sBeginTime);
			sBeginTimeIsInit = true;
		}
		timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		return (ts.tv_sec - sBeginTime.tv_sec) + ts.tv_nsec / 1000000000.0;
	}
#elif defined(PSP)
	double GetSystemTime()
	{
		SceRtcTick tick;

		sceRtcGetCurrentTick(&tick);

		return tick.tick / 1000000.0;
	}
#else
	#error no timer implementation specified for the current platform
#endif

Timer::Timer()
{
	m_StartTime = GetSystemTime();
}

Timer::~Timer()
{
}

float Timer::Time_get() const
{
	return (float)(GetSystemTime() - m_StartTime);
}

void Timer::Time_set(float time)
{
	m_StartTime = GetSystemTime() - (double)time;
}

uint64_t Timer::TimeMS_get() const
{
	double time = GetSystemTime() - m_StartTime;
	return static_cast<uint64_t>(time * 1000.0);
}

uint64_t Timer::TimeUS_get() const
{
	double time = GetSystemTime() - m_StartTime;
	return static_cast<uint64_t>(time * 1000000.0);
}
