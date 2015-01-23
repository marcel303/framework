#pragma once

#include "libgg_forward.h"
#include "TimeTracker.h"
#include "TypesBase.h"

class TriggerTimer
{
public:
	TriggerTimer();
	void Initialize(TimeTracker* timeTracker);
	
	void Start(float interval);
	void Stop();
	
	inline bool Read()
	{
		if (!IsRunning_get() || m_TimeTracker->Time_get() < m_NextTime)
			return false;
		
		Stop();
		
		return true;
	}
	
	inline bool IsRunning_get() const
	{
		return m_NextTime >= 0.0f;
	}
	
	inline float Progress_get() const
	{
		return (m_TimeTracker->Time_get() - m_StartTime) / (m_NextTime - m_StartTime);
	}
	
private:
	TimeTracker* m_TimeTracker;
	float m_NextTime;
	float m_StartTime;
};
