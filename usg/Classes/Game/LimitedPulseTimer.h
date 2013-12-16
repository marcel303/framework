#pragma once

#include "PolledTimer.h"

namespace Game
{
	class LimitedPulseTimer
	{
	public:
		LimitedPulseTimer();
		
		void Start(int count, float interval);
		bool ReadTick();
		bool IsEmpty_get() const;
		
	private:
		PolledTimer mTimer;
		int mTodo;
	};
}
