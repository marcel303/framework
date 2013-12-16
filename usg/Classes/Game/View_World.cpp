#if defined(WIN32) || defined(LINUX) || defined(MACOS)
#include "EventHandler_PlayerController_Gamepad.h"
#include "EventHandler_PlayerController_Keyboard.h"
#include "EventManager.h"
#endif
#if defined(PSP)
#include "EventHandler_PlayerController_Gamepad_Psp.h"
#include "EventManager.h"
#endif
#if defined(BBOS)
#include "EventHandler_PlayerController_Gamepad.h"
#include "EventManager.h"
#endif
#include "GameRound.h"
#include "GameScore.h"
#include "GameState.h"
//#include "grs.h"
#include "GrsIntegration.h"
#include "grs_types.h"
#include "MenuMgr.h"
#include "PerfCount.h"
#include "System.h"
#include "TempRender.h"
#include "UsgResources.h"
#include "View_World.h"
#include "World.h"

namespace Game
{
	// todo: grs rank reset on game start
	
	View_World::View_World()
	{
		m_GrsUpdateTimer.Initialize(g_GameState->m_TimeTracker_Global);
		m_GrsUpdateTimer.SetInterval(10.0f);
		
		m_GrsRank.Setup();
	}
	
	void View_World::GrsClient_set(GRS::HttpClient* client)
	{
		//m_GrsClient = client;
		//m_GrsClient->OnQueryRankResult = CallBack(this, HandleQueryRankResult);
	}
	
	void View_World::HandleWaveBegin()
	{
		m_CriticalGauge.EnableZoom(2.0f);
	}
	
	void View_World::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_InGame);
		
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_CONTROLLERS);
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_CONTROLLER_FIRE);
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_WORLD);

#if defined(WIN32) || defined(LINUX) || defined(MACOS)
		EventManager::I().Enable(EVENT_PRIO_JOYSTICK);
		EventManager::I().Enable(EVENT_PRIO_KEYBOARD);
		//EventHandler_PlayerController_Gamepad::I().Clear();
		EventHandler_PlayerController_Keyboard::I().Clear();
#elif defined(PSP)
		EventManager::I().Enable(EVENT_PRIO_JOYSTICK);
#elif defined(BBOS)
		EventManager::I().Enable(EVENT_PRIO_JOYSTICK);
#endif
		
		g_World->IsPaused_set(false);
		
		//
		
		m_CriticalGauge.Setup();
		
		//
		
		m_GrsUpdateTimer.FireImmediately_set(XTRUE);
		m_GrsUpdateTimer.Start();
	}
	
	void View_World::HandleFocusLost()
	{
		m_GrsUpdateTimer.Stop();
		
		g_GameState->m_TouchDLG->Disable(USG::TOUCH_PRIO_CONTROLLERS);
		g_GameState->m_TouchDLG->Disable(USG::TOUCH_PRIO_CONTROLLER_FIRE);
		g_GameState->m_TouchDLG->Disable(USG::TOUCH_PRIO_WORLD);
		
#if defined(WIN32) || defined(LINUX) || defined(MACOS)
		EventManager::I().Disable(EVENT_PRIO_JOYSTICK);
		EventManager::I().Disable(EVENT_PRIO_KEYBOARD);
#elif defined(PSP)
		EventManager::I().Disable(EVENT_PRIO_JOYSTICK);
		EventHandler_PlayerController_Gamepad_Psp::I().Clear();
#elif defined(BBOS)
		EventManager::I().Disable(EVENT_PRIO_JOYSTICK);
		EventHandler_PlayerController_Gamepad::I().Clear();
#endif

		g_World->IsPaused_set(true);
	}
	
	void View_World::Update(float dt)
	{
		Assert(g_GameState->m_GameRound->GameModeTest(GMF_Classic | GMF_Invaders));
		
		UsingBegin(PerfTimer timer(PC_UPDATE_SB_CLEAR))
		{
			// Clear dirty areas of selection buffer
		
			g_GameState->ClearSB();
		}
		UsingEnd()
		
		UsingBegin(PerfTimer timer(PC_UPDATE_SB_DRAW))
		{
			// Update selection buffer
			
			g_World->UpdateSB(&g_GameState->m_SelectionBuffer);
		}
		UsingEnd()
		
		// Update game logic
		
		g_World->Update(dt);
		
		// Update round logic
		
		g_GameState->m_GameRound->Update(dt);
		
		if (g_GameState->m_GameRound->GameModeIsClassic())
		{
			// Update critical gauge
			
			m_CriticalGauge.Update(dt);
		}
		
#if 0
		// Update real-time ranking
		
		if (m_GrsUpdateTimer.ReadTick())
		{
			m_GrsUpdateTimer.FireImmediately_set(XFALSE);
			m_GrsUpdateTimer.ClearTick();
			
			if (g_GameState->m_GameSettings->m_GrsEnabled)
			{
				RefreshRank();
			}
		}
#endif
	}
	
	void View_World::Render()
	{
		// Render critical gauge
		
		if (g_GameState->DBG_RenderMask & RenderMask_Interface)
		{
			if (g_GameState->m_GameRound->GameModeIsClassic())
			{
				m_CriticalGauge.Render();
			}
			
#if !defined(PSP_UI) && 0
			// Render GRS score
			
			m_GrsRank.Render();
#endif
		}
	}
	
	int View_World::RenderMask_get()
	{
		return RenderMask_HudInfo | RenderMask_HudPlayer | RenderMask_Indicators | RenderMask_Interface | RenderMask_Intermezzo | RenderMask_Particles | (RenderMask_SprayAngles * 0) | RenderMask_WorldBackground | RenderMask_WorldPrimary;
	}
	
	void View_World::RefreshRank()
	{
#ifdef IPHONEOS
		// todo: query game center for current rank.. but how? ;)
#elif 0
		GRS::RankQuery query;
		
		query.Setup(m_GrsClient->GameInfo_get().m_Id, GameToGrs(g_GameState->m_GameRound->m_WaveInfo.difficulty), (float)g_GameState->m_Score->Score_get());
		
		m_GrsClient->RequestHighScoreRank(query);
#endif
	}
	
	void View_World::HandleQueryRankResult(void* obj, void* arg)
	{
		View_World* self = (View_World*)obj;
		GRS::QueryResult* result = (GRS::QueryResult*)arg;
		
		self->m_GrsRank.Rank_set(result->m_Position);
		
		g_System.HasNetworkConnectivity_set(true);
	}
}
