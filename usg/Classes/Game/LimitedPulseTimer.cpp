#include "GameState.h"
#include "LimitedPulseTimer.h"
#include "TimeTracker.h"

namespace Game
{
	LimitedPulseTimer::LimitedPulseTimer()
	{
		mTimer.Initialize(g_GameState->m_TimeTracker_World);
	}
	
	void LimitedPulseTimer::Start(int count, float interval)
	{
		mTimer.SetInterval(interval);
		mTimer.Start();
		
		mTodo = count;
	}
	
	bool LimitedPulseTimer::ReadTick()
	{
		if (mTodo <= 0)
			return false;
		
		if (!mTimer.ReadTick())
			return false;
		
		mTodo--;
		
		return true;
	}
	
	bool LimitedPulseTimer::IsEmpty_get() const
	{
		return mTodo <= 0;
	}
}
