#include "AnimTimer.h"
#include "Calc.h"
#include "TimeTracker.h"

AnimTimer::AnimTimer()
{
	Initialize(0, false);
}

AnimTimer::AnimTimer(TimeTracker* timeTracker, bool own)
{
	Initialize(timeTracker, own);
}

AnimTimer::~AnimTimer()
{
	if (m_OwnTimeTracker)
	{
		delete m_TimeTracker;
		m_TimeTracker = 0;
	}
}

void AnimTimer::Initialize(TimeTracker* timeTracker, bool own)
{
	m_Started = false;
	m_Mode = AnimTimerMode_FrameBased;
	m_Duration = 0.0f;
	m_Frame = 0;
	m_StartTime = 0.0f;
	m_TimeTracker = timeTracker;
	m_OwnTimeTracker = own;
}

void AnimTimer::Start(AnimTimerMode mode, bool inverseProgress, float duration, AnimTimerRepeat repeat)
{
	m_Started = true;
	m_Mode = mode;
	m_InverseProgress = inverseProgress;
	m_Duration = duration;
	m_DurationRcp = 1.0f / duration;
	m_Repeat = repeat;
	m_Frame = 0;
	m_StartTime = m_TimeTracker->Time_get();
}

void AnimTimer::Stop()
{
	m_Started = false;
}

void AnimTimer::Restart()
{
	Stop();
	
	Start(m_Mode, m_InverseProgress, m_Duration, m_Repeat);
}

float AnimTimer::Progress_get()
{
	if (!m_Started)
		return 0.0f;
	
	float t = 0.0f;

	switch (m_Mode)
	{
	case AnimTimerMode_FrameBased:
		t = m_Frame * m_DurationRcp;
		break;
	case AnimTimerMode_TimeBased:
		t = (m_TimeTracker->Time_get() - m_StartTime) * m_DurationRcp;
		break;
	default:
		throw ExceptionVA("unknown anim mode: %d", (int)m_Mode);
	}

	switch (m_Repeat)
	{
	case AnimTimerRepeat_None:
		{
			if (t > 1.0f)
			{
				Stop();
				
				t = 1.0f;
			}
		}
		break;

	case AnimTimerRepeat_Loop:
		{
			t = t - (int)t;
		}
		break;

	case AnimTimerRepeat_Mirror:
		{
			const int count = (int)t;
			t = t - count;
			const int odd = count & 1;
			if (odd)
				t = 1.0f - t;
		}
		break;

	default:
		throw ExceptionVA("unknown anim repeat: %d", (int)m_Repeat);
	}

	if (m_InverseProgress)
	{
		t = 1.0f - t;
	}
	
	return t;
}

void AnimTimer::Tick()
{
	m_Frame++;
}

bool AnimTimer::IsRunning_get()
{
	// note: Progress_get updates running flag, we need to call it here
	
	Progress_get();
	
	return m_Started;
}
