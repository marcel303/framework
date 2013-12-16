#pragma once

#include "Timer.h"

/*
 
 The time tracker implements a manually incremented timer
 
 Instead of using a real-time clock, time is managed manually, using increments
 
 usage:
 
 TimeTracker timeTracker;
 
 PolledTimer timer;
 timer.Initialize(&timeTracker);
 timer.SetFrequency(1.0f);
 timer.Start();
 
 while (true)
 {
 	while (timer.ReadTick())
	{
 		LOG(LogLevel_Debug, "tick");
 	}
 
	 timeTracker.Increment(0.1f);
 }
 
 */

class TimeTracker : public ITimer
{
public:
	TimeTracker();
	
	void Increment(float dt);
	
	inline float Time_get() const
	{
		return m_Time;
	}
	
	inline void Time_set(float time)
	{
		m_Time = time;
	}
	
	inline float TimeDelta_get() const
	{
		return m_TimeDelta;
	}
	
	inline uint64_t TimeMS_get() const
	{
		return static_cast<uint64_t>(m_Time * 1000.0f);
	}
	
	inline uint64_t TimeUS_get() const
	{
		return static_cast<uint64_t>(m_Time * 1000000.0f);
	}
	
private:
	float m_Time;
	float m_TimeDelta;
};
