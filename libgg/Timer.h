#pragma once

#include <stdint.h>

extern double GetSystemTime();

/*
 
 ITimer is the timer interface for classes which can somehow 'tell time', either tracking time themselves or using a real-time clock
 
 */

class ITimer
{
public:
	virtual ~ITimer();
	
	virtual float Time_get() const = 0;
	virtual uint64_t TimeMS_get() const = 0;
	virtual uint64_t TimeUS_get() const = 0;
};

/*
 
 The timer class implements a (high-accuracy) real-time clock
 
 usage:
 
 Timer timer;
 
 LOG(LogLevel_Debug, "time: %f", timer.Time_get());
 
 */
class Timer : public ITimer
{
public:
	Timer();
	virtual ~Timer();

	virtual float Time_get() const;
	void Time_set(float time);
	virtual uint64_t TimeMS_get() const;
	virtual uint64_t TimeUS_get() const;

private:
	double m_StartTime;
};

// global real time timer

extern Timer g_TimerRT;
