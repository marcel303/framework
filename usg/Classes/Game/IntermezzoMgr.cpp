#include "GameState.h"
#include "IntermezzoMgr.h"
#include "StringBuilder.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_World.h"
#include "World.h"

#define WAVE_TIME_FADEIN 0.5f
#define WAVE_TIME_WAIT 1.0f
#define WAVE_TIME_FADEOUT 0.5f

#define WAVE_BAR_Y 200.0f
#define WAVE_BAR_SY 40.0f

#define WAVE_TILE_Y 100.0f

#define LEVEL_DURATION 1.0f

#define GAMEOVER_DURATION 4.0f

namespace Game
{
	IntermezzoMgr::IntermezzoMgr()
	{
		Initialize();
	}

	void IntermezzoMgr::Initialize()
	{
	}

	void IntermezzoMgr::Start(IntermezzoType type)
	{
		switch (type)
		{
			case IntermezzoType_LevelBegin:
//				g_GameState->m_SoundEffects.Play(Resources::SOUND_ROUND_WIN, SfxFlag_MustFinish);
				break;
				
			case IntermezzoType_WaveBanner:
				m_WaveBanner.Start();
				break;
				
			case IntermezzoType_MiniBossAlert:
				break;
				
			case IntermezzoType_KillStreak:
				break;
		}
	}

	void IntermezzoMgr::Update(float dt)
	{
		m_WaveBanner.Update(dt);
	}

	void IntermezzoMgr::Render_Overlay()
	{
		m_WaveBanner.Render();
	}
	
	IntermezzoMgr::WaveBanner::WaveBanner()
	{
		m_State = State_Idle;
		
		m_Timer.Initialize(g_GameState->m_TimeTracker_Global);
		m_AnimTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
	}
	
	void IntermezzoMgr::WaveBanner::Start()
	{
		m_State = State_FadeIn;
		
		m_Timer.SetInterval(WAVE_TIME_FADEIN);
		m_Timer.Start();
		m_AnimTimer.Start(AnimTimerMode_TimeBased, false, WAVE_TIME_FADEIN, AnimTimerRepeat_None);
		
		View_World* view = (View_World*)g_GameState->GetView(::View_InGame);
		
		view->HandleWaveBegin();
		
//		g_GameState->m_SoundEffects.Play(Resources::SOUND_ROUND_START, SfxFlag_MustFinish);
	}
	
	void IntermezzoMgr::WaveBanner::Update(float dt)
	{
		switch (m_State)
		{
			case State_Idle:
				break;
				
			case State_FadeIn:
			{
				if (m_Timer.ReadTick())
				{
					m_State = State_Wait;
					m_Timer.SetInterval(WAVE_TIME_WAIT);
					m_Timer.Start();
				}
				break;
			}
				
			case State_Wait:
			{
				if (m_Timer.ReadTick())
				{
					m_State = State_FadeOut;
					m_Timer.SetInterval(WAVE_TIME_FADEOUT);
					m_Timer.Start();
					m_AnimTimer.Start(AnimTimerMode_TimeBased, false, WAVE_TIME_FADEOUT, AnimTimerRepeat_None);
				}
				break;
			}
				
			case State_FadeOut:
			{
				if (m_Timer.ReadTick())
				{
					m_State = State_Idle;
				}
				break;
			}
		}
	}
	
	void IntermezzoMgr::WaveBanner::Render()
	{
		switch (m_State)
		{
			case State_Idle:
				break;
				
			case State_FadeIn:
			{
//				const VectorShape* shape = g_GameState->GetShape(Resources::INDICATOR_WAVE);
//				float x = -(1.0f - m_AnimTimer.Progress_get()) * shape->m_Texture->m_Size[0];
				float x = 0.0f;
				float y = WAVE_TILE_Y;
				float a = m_AnimTimer.Progress_get();
				Render(x, y, a);
				break;
			}
				
			case State_Wait:
			{
				Render(0.0f, WAVE_TILE_Y, 1.0f);
				break;
			}
				
			case State_FadeOut:
			{
//				const VectorShape* shape = g_GameState->GetShape(Resources::INDICATOR_WAVE);
//				float x = -m_AnimTimer.Progress_get() * shape->m_Texture->m_Size[0];
				float x = 0.0f;
				float y = WAVE_TILE_Y;
				float a = 1.0f - m_AnimTimer.Progress_get();
				Render(x, y, a);
				break;
			}
		}
	}
	
	void IntermezzoMgr::WaveBanner::Render(float x, float y, float a)
	{
#if 1
		return;
#else	
		float py = VIEW_SY-80.0f;
		float sy = 15.0f;
		
		Vec2F pos = Vec2F(0.0f, py - sy);
		Vec2F size = Vec2F((float)VIEW_SX, sy * 2.0f);
		
		StringBuilder<32> sb;
		sb.AppendFormat("wave %d/%d", g_GameState->m_GameRound->m_WaveInfo.wave + 1, g_GameState->m_GameRound->m_WaveInfo.waveCount);

//		RenderRect(pos, size, 0.0f, 0.0f, 0.0f, a, g_GameState->GetTexture(Textures::COLOR_WHITE));
//		RenderText(pos, size, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, a), TextAlignment_Center, TextAlignment_Center, true, "critical in T -%d", g_GameState->m_GameRound->m_WaveInfo.waveCount - g_GameState->m_GameRound->m_WaveInfo.wave);
		RenderText(pos, size, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, a), TextAlignment_Center, TextAlignment_Center, true, sb.ToString());
#endif
	}
}
