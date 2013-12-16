#pragma once

#include "DeltaTimer.h"

namespace Game
{
	class InvincibilityTimer
	{
	public:
		InvincibilityTimer();
		void Initialize(ITimer* timer);
		
		void Start(float time1, float time2);
		void Stop();
		void Tick();
		
		bool FlickerEffect_get() const;
		bool IsActive_get() const;
		
	private:
		enum State
		{
			State_Time1,
			State_Time2,
			State_Stopped
		};
		
		void UpdateState();
		
		void State_set(State state);
		
		State m_State;
		DeltaTimer m_Timer;
		float m_Time1;
		float m_Time2;
		int m_FlickerValue;
		int m_FlickerInterval;
	};
}
