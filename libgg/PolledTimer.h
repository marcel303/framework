#pragma once

#include "libgg_forward.h"
#include "Types.h"

/*
 
 The polled timer class tells time and allows the reading of 'tick' events during a regular interval or set frequency
 
 usage:
 
 PolledTimer timer;
 
 timer.Initialize(&g_TimerRT); // use the real time timer to track time
 timer.SetFrequency(10.0);
 timer.Start();
 
 while (timer.ReadTick())
 {
 	float time = timer.Time_get();
 
 	LOG(LogLevel_Debug, "time: %f", time);
 }
 
*/

class PolledTimer
{
public:
	PolledTimer();
	void Initialize(ITimer* timer);
	
	void Start();
	void Stop();
	void Restart();
	
	void SetInterval(float seconds);
	void SetIntervalMS(int miliseconds);
	void SetFrequency(float frequency);
	
	XBOOL PeekTick() const;
	XBOOL ReadTick();
	void ClearTick();
	
	int TickCount_get() const;
	float TimeS_get() const;
	uint32_t TimeMS_get() const;
	uint64_t TimeUS_get() const;
	
	inline XBOOL IsActive_get() const
	{
		return m_IsActive;
	}
	
	inline XBOOL FireImmediately_get() const
	{
		return m_FireImmediately;
	}
	
	inline void FireImmediately_set(XBOOL value)
	{
		m_FireImmediately = value;
	}
	
	inline float Interval_get() const
	{
		return (float)(m_intervalMS / 1000.0);
	}
	
private:
	ITimer* m_timer;
	
	int m_intervalMS;
	int m_lastReadMS;
	
	XBOOL m_IsActive;
	XBOOL m_FireImmediately;
};
