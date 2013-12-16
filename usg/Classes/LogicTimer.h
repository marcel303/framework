#pragma once

#include "Timer.h"

/*
 
 The logic timer is used to regulate the execution of logic code at a fixed frequency and with a maximum frame skip
 
 note: The logic timer currently always returns tru to Tick and BeginUpdate
 
 usage:
 
 LogicTimer timer;
 
 timer.Setup(&g_TimerRT, false, 60.0f, 2); // use a max frame skip of 2
 
 while (true)
 {
 	if (timer.BeginUpdate())
 	{
 		while (timer.Tick())
 		{
 			// note: dt = 1 / frequency
 
 			float dt = timer.TimeStep_get();
 
 			Update(dt);
 		}
 	}
 }
 
 */

class LogicTimer
{
public:
	LogicTimer();
	~LogicTimer();
	void Initialize();
	
	void Setup(Timer* timer, bool ownTimer, float frequency, int maxIterationCount);
	
	float TimeStep_Get() const;
	
	void Start();
	void Pause();
	void Resume();

	bool BeginUpdate();
	void EndUpdate();
	
	bool Tick();
	
private:
	Timer* m_Timer;
	bool m_OwnTimer;
	bool m_IsPaused;
	
	float m_Frequency;
	float m_FrequencyRcp; // aka interval
	int m_MaxIterationCount;
	int m_IterationCount;
	
	float m_LastTime;
	float m_NewTime;
};
