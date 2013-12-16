#include "InvincibilityTimer.h"

namespace Game
{
	InvincibilityTimer::InvincibilityTimer()
	{
		m_State = State_Stopped;
		m_Time1 = 0.0f;
		m_Time2 = 0.0f;
		m_FlickerValue = 0;
		m_FlickerInterval = 1;
	}

	void InvincibilityTimer::Initialize(ITimer* timer)
	{
		m_Timer.Initialize(timer);
	}

	void InvincibilityTimer::Start(float time1, float time2)
	{
		m_Time1 = time1;
		m_Time2 = time2;
		
		State_set(State_Time1);
	}

	void InvincibilityTimer::Stop()
	{
		State_set(State_Stopped);
	}

	void InvincibilityTimer::Tick()
	{
		UpdateState();
		
		m_FlickerValue++;
	}

	bool InvincibilityTimer::FlickerEffect_get() const
	{
		if (!IsActive_get())
			return false;
		else
			return (m_FlickerValue % m_FlickerInterval) == 0;
	}

	bool InvincibilityTimer::IsActive_get() const
	{
		return m_State != State_Stopped;
	}

	void InvincibilityTimer::UpdateState()
	{
		if (m_State == State_Stopped)
			return;
		
		if (m_State == State_Time1)
		{
			if (m_Timer.Delta_get() >= m_Time1)
				State_set(State_Time2);
		}
		
		if (m_State == State_Time2)
		{
			if (m_Timer.Delta_get() >= m_Time2)
				State_set(State_Stopped);
		}
	}

	void InvincibilityTimer::State_set(State state)
	{
		m_State = state;
		
		m_Timer.Start();
		
		m_FlickerValue = 0;
		
		if (state == State_Time1)
			m_FlickerInterval = 2;
		if (state == State_Time2)
			m_FlickerInterval = 3;
	}
}
