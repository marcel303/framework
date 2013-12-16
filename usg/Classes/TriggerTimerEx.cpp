#include "GameState.h"
#include "TriggerTimerEx.h"

// global and world specializations

TriggerTimerG::TriggerTimerG() : TriggerTimer()
{
	Initialize(g_GameState->m_TimeTracker_Global);
}

TriggerTimerW::TriggerTimerW() : TriggerTimer()
{
	Initialize(g_GameState->m_TimeTracker_World);
}
