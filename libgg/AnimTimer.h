#pragma once

#include "libgg_forward.h"
#include "Types.h"

enum AnimTimerMode
{
	AnimTimerMode_FrameBased,
	AnimTimerMode_TimeBased
};

enum AnimTimerRepeat
{
	AnimTimerRepeat_None,
	AnimTimerRepeat_Loop,
	AnimTimerRepeat_Mirror
};

/*
 
 The animation timer class does time based or frame based animation
 
 It operates using a 'mode' and 'repeat' setting
 
 Its' mode can be either frame based or time based. When the mode is time based, a timer is queried
 to determine the current progress, or the number of elapsed frames (increased every tick) in frame based mode
 
 Repeat can be none, loop or mirror.
 If repeat is none, progress will vary from 0 to 1
 If repeat is loop, progress will repeat from 0 to 1 indefinately
 If repeat is mirror, progress will vary from 0 to 1, back to 0, back to 1, etc, creating a triangle pattern /\/\/\/\/...
 
 Progress may also be inverted, which simply reversed its' direction
 
 usage:
 
 AnimTimer timer;
 
 timer.Initialize(&g_TimerRT);
 timer.Start(AnimTimerMode_TimeBased, false, 10.0f, AnimTimerRepeat_None);
 
 while (timer.IsRunning_get())
 {
 	float progress = timer.Progress_get();
 
 	LOG(LogLevel_Debug, "progress: %f", progress);
 }
 
 LOG(LogLevel_Debug, "timer no longer running");
 
 */

class AnimTimer
{
public:
	AnimTimer();
	AnimTimer(TimeTracker* timeTracker, bool own);
	~AnimTimer();
	void Initialize(TimeTracker* timeTracker, bool own);

	void Start(AnimTimerMode mode, bool inverseProgress, float duration, AnimTimerRepeat repeat);
	void Stop();
	void Restart();

	float Progress_get();

	void Tick(); // must be called each render update
	
	bool IsRunning_get();

private:
	bool m_Started;
	AnimTimerMode m_Mode;
	bool m_InverseProgress;
	float m_Duration;
	float m_DurationRcp;
	AnimTimerRepeat m_Repeat;
	int m_Frame;
	float m_StartTime;

	TimeTracker* m_TimeTracker;
	bool m_OwnTimeTracker;
};
