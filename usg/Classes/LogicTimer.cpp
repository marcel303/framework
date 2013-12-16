#include <math.h>
#include "LogicTimer.h"

LogicTimer::LogicTimer()
{
	Initialize();
}

LogicTimer::~LogicTimer()
{
	if (m_OwnTimer)
	{
		delete m_Timer;
		m_Timer = 0;
	}
}

void LogicTimer::Initialize()
{
	m_IsPaused = false;
	m_LastTime = 0.0f;
	m_Timer = 0;
	m_OwnTimer = 0;
	m_Frequency = 0.0f;
	m_FrequencyRcp = 0.0f;
	m_MaxIterationCount = 0;
}

void LogicTimer::Setup(Timer* timer, bool ownTimer, float frequency, int maxIterationCount)
{
	m_Timer = timer;
	m_OwnTimer = ownTimer;
	m_Frequency = frequency;
	m_FrequencyRcp = 1.0f / m_Frequency;
	m_MaxIterationCount = maxIterationCount;
}

float LogicTimer::TimeStep_Get() const
{
	return m_FrequencyRcp;
}

void LogicTimer::Start()
{
	m_LastTime = m_Timer->Time_get();
	
	m_IsPaused = false;
}

void LogicTimer::Pause()
{
	m_IsPaused = true;
}

void LogicTimer::Resume()
{
	m_LastTime = m_Timer->Time_get();
	
	m_IsPaused = false;
}

bool LogicTimer::BeginUpdate()
{
	if (m_IsPaused)
		return false;
	
//#if TARGET_OS_IPHONE
#ifndef DEBUG
	m_IterationCount = 1;
	
	return true;
#else
	m_NewTime = m_Timer->Time_get();
	
	const float deltaTime = m_NewTime - m_LastTime;
	
	m_IterationCount = (int)floorf(deltaTime / TimeStep_Get());
	
	if (m_IterationCount > m_MaxIterationCount)
		m_IterationCount = m_MaxIterationCount;
	
	if (m_IterationCount == 0)
		return false;
	else
		return true;
#endif
}

void LogicTimer::EndUpdate()
{
	m_LastTime = m_NewTime;
}

bool LogicTimer::Tick()
{
	if (m_IterationCount == 0)
		return false;
	
	m_IterationCount--;
	
	return true;
}
