#include "PolledTimer.h"
#include "Timer.h"

PolledTimer::PolledTimer()
{
	Initialize(0);
}

void PolledTimer::Initialize(ITimer* timer)
{
	m_intervalMS = 1000;
	m_lastReadMS = 0;
	m_IsActive = XFALSE;
	m_FireImmediately = XFALSE;
	m_timer = timer;
}

void PolledTimer::Start()
{
	ClearTick();
	
	m_IsActive = XTRUE;
}

void PolledTimer::Stop()
{
	m_IsActive = XFALSE;
}

void PolledTimer::Restart()
{
	Stop();

	Start();
}

void PolledTimer::SetInterval(float seconds)
{
	SetIntervalMS((int)(seconds * 1000.0f));
}

void PolledTimer::SetIntervalMS(int miliseconds)
{
	m_intervalMS = miliseconds;

	ClearTick();
}

void PolledTimer::SetFrequency(float frequency)
{
	SetInterval(1.0f / frequency);
}

XBOOL PolledTimer::PeekTick() const
{
	if (!m_IsActive)
		return XFALSE;
	
	int timeMS = TimeMS_get();
	
	if (timeMS >= m_lastReadMS + m_intervalMS)
		return XTRUE;
	else
		return XFALSE;
}

XBOOL PolledTimer::ReadTick()
{
	if (!PeekTick())
		return XFALSE;

	m_lastReadMS += m_intervalMS;

	return XTRUE;
}

void PolledTimer::ClearTick()
{
	m_lastReadMS = TimeMS_get();
	
	if (m_FireImmediately)
		m_lastReadMS -= m_intervalMS;
}

int PolledTimer::TickCount_get() const
{
	return (TimeMS_get() - m_lastReadMS) / m_intervalMS;
}

float PolledTimer::TimeS_get() const
{
	return m_timer->Time_get();
}

uint32_t PolledTimer::TimeMS_get() const
{
	return static_cast<uint32_t>(m_timer->TimeMS_get());
}

uint64_t PolledTimer::TimeUS_get() const
{
	return m_timer->TimeUS_get();
}
