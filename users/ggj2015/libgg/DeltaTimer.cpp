#include "DeltaTimer.h"

DeltaTimer::DeltaTimer()
{
	m_startTime = 0.0f;
}

void DeltaTimer::Initialize(ITimer* timer)
{
	m_timer.Initialize(timer);
	
	m_startTime = 0.0f;
}

void DeltaTimer::Start()
{
	m_timer.Start();
	
	m_startTime = m_timer.TimeS_get();
}

void DeltaTimer::Stop()
{
	m_timer.Stop();
}

void DeltaTimer::Reset()
{
	m_timer.Restart();
}

float DeltaTimer::Delta_get() const
{
	return m_timer.TimeS_get() - m_startTime;
}
