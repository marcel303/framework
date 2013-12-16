#pragma once

#include "Timer.h"
#include "types.h"

enum AnimationTimerMode
{
	AnimationTimerMode_FrameBased,
	AnimationTimerMode_TimeBased
};

class AnimationTimer
{
public:
	AnimationTimer()
	{
		Initialize();
	}

	~AnimationTimer()
	{
		delete m_Timer;
	}

	void Initialize()
	{
		m_Mode = AnimationTimerMode_FrameBased;
		m_Duration = 0.0f;
		m_Frame = 0;
		m_StartTime = 0.0f;
		m_Timer = new Timer();
	}

	void Start(AnimationTimerMode mode, float duration)
	{
		m_Mode = mode;
		m_Duration = duration;
		m_Frame = 0;
		m_StartTime = m_Timer->GetTime();
	}

	float Progress_get()
	{
		float t = 0.0f;

		switch (m_Mode)
		{
		case AnimationTimerMode_FrameBased:
			t = m_Frame / m_Duration;
			break;
		case AnimationTimerMode_TimeBased:
			t = (m_Timer->GetTime() - m_StartTime) / m_Duration;
			break;
		}

		return MidF(t, 0.0f, 1.0f);
	}

	void Tick()
	{
		m_Frame++;
	}

private:
	AnimationTimerMode m_Mode;
	float m_Duration;
	int m_Frame;
	float m_StartTime;

	Timer* m_Timer;
};