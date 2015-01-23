#include "TimeTracker.h"

TimeTracker::TimeTracker()
{
	m_Time = 0.0f;
}

void TimeTracker::Increment(float dt)
{
	m_Time += dt;
	m_TimeDelta = dt;
}
