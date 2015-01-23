#include "TimeTracker.h"
#include "TriggerTimer.h"

TriggerTimer::TriggerTimer()
{
	m_TimeTracker = 0;
	m_NextTime = -1.0f;
}

void TriggerTimer::Initialize(TimeTracker* timeTracker)
{
	m_TimeTracker = timeTracker;
}

void TriggerTimer::Start(float interval)
{
	m_StartTime = m_TimeTracker->Time_get();
	m_NextTime = m_StartTime + interval;	
}

void TriggerTimer::Stop()
{
	m_NextTime = -1.0f;
}
