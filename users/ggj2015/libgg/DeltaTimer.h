#pragma once

#include "PolledTimer.h"

/*
 
 The delta timer tracks the time since start
 
 usage:
 
 DeltaTimer timer;
 
 timer.Initialize(&g_TimerRT); // use the real time timer to track time
 timer.Start();
 
 while (true)
 {
 	float delta = timer.Delta_get();
 
 	LOG(LogLevel_Debug, "delta: %f", delta);
 }
 
 */

class DeltaTimer
{
public:
	DeltaTimer();
	void Initialize(ITimer* timer);

	void Start();
	void Stop();
	void Reset();

	float Delta_get() const;

private:
	PolledTimer m_timer;
	float m_startTime;
};
