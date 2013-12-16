#include "EncouragementTexts.h"
#include "EventManager.h"
#include "FontMap.h"
#include "GameState.h"
#include "Grid_Effect.h"
#include "Mat3x2.h"
#include "MenuMgr.h"
#include "SpriteAnim.h"
#include "SpriteEffectMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "TouchDLG.h"
#include "TouchInfo.h"
#include "UsgResources.h"
#include "View_GameOver.h"
#include "View_KeyBoard.h"
#include "View_ScoreSubmit.h"
#include "World.h"

#if defined(IPHONEOS)
	#include "GameCenter.h"
#endif
#if defined(BBOS)
	#include "GameView_BBOS.h"
#endif

#define TOUCH_WAIT 2.0f
#define HINT_ANIM_TIME 0.5f
#define HINT_SHOW_TIME 4.0f
#define HINT_INTERVAL 1.0f
#define RIPPLE_INTERVAL 0.2f
#define EXPLOSION_INTERVAL 0.03f

namespace Game
{
	static void NextView();
	
	View_GameOver::View_GameOver()
	{
	}
	
	View_GameOver::~View_GameOver()
	{
		Shutdown();
	}

	void View_GameOver::Initialize()
	{
		TouchListener listener;
		listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_GAMEOVER, listener);
		
		m_TouchTextTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_RippleTimer.Initialize(g_GameState->m_TimeTracker_Global);
		m_ExplosionTimer.Initialize(g_GameState->m_TimeTracker_Global);
		m_RippleTimer.SetInterval(RIPPLE_INTERVAL);
		m_ExplosionTimer.SetInterval(EXPLOSION_INTERVAL);
		m_TouchEnabled = false;
		m_HintState = HintState_Disabled;
		m_Hint = 0;

		EventManager::I().AddEventHandler(this, EVENT_PRIO_INTERFACE_GAMEOVER);
	}
	
	void View_GameOver::Shutdown()
	{
		EventManager::I().RemoveEventHandler(this, EVENT_PRIO_INTERFACE_GAMEOVER);
	}

	void View_GameOver::Update(float dt)
	{
		// Update game logic
		
		g_GameState->ClearSB();
		g_World->Update(dt);
		
		// Update touch enabled
		
		if (m_TouchTrigger.Read())
		{
			m_TouchEnabled = true;
			m_HintTrigger.Start(HINT_ANIM_TIME);
		}
		
		// Update hints
		
		if (m_HintTrigger.Read())
		{
			switch (m_HintState)
			{
				case HintState_Disabled:
					m_HintState = HintState_AnimIn;
					m_HintTrigger.Start(HINT_ANIM_TIME);
					GameText::GetEncouragementText(false, &m_Hint);
					break;
				case HintState_AnimIn:
					m_HintState = HintState_Show;
					m_HintTrigger.Start(HINT_SHOW_TIME);
					break;
				case HintState_Show:
					m_HintState = HintState_AnimOut;
					m_HintTrigger.Start(HINT_ANIM_TIME);
					break;
				case HintState_AnimOut:
					m_HintState = HintState_Disabled;
					m_HintTrigger.Start(HINT_INTERVAL);
					break;
			}
		}
		
		// Update explosions & ripples
		
		while (m_RippleTimer.ReadTick())
		{
			g_World->m_GridEffect->Impulse(Vec2F(Calc::Random(0.0f, (float)WORLD_SX), Calc::Random(0.0f, (float)WORLD_SY)), 0.1f);
		}
		
		while (m_ExplosionTimer.ReadTick())
		{
			Vec2F explosionSize(64.0f, 64.0f);
			Vec2F pos(Calc::Random((float)WORLD_SX), Calc::Random((float)WORLD_SY));
			Game::SpriteAnim anim;
			anim.Setup(g_GameState->m_ResMgr.Get(Resources::EXPLOSION_01), 4, 4, 0, 15, 32, g_GameState->m_TimeTracker_World);
			anim.Start();
			g_GameState->m_SpriteEffectMgr->Add(anim, pos, explosionSize, SpriteColors::Black);
//			g_World->m_GridEffect.Impulse(pos, 0.2f);
		}
	}
	
	void View_GameOver::Render()
	{
		if (m_TouchEnabled && m_TouchTextTimer.Progress_get() <= 0.5f)
		{
#if defined(PSP_UI)
			const char* text = "press " PSPGLYPH_X " to continue";
#elif defined(BBOS)
			const char* text;
			if (gGameView->m_GamepadIsEnabled)
				text = "press [X] to continue";
			else
				text = "touch to continue";
#else
			const char* text = "touch to continue";
#endif

			RenderText(Vec2F(0.0f, VIEW_SY - 120.0f), Vec2F((float)VIEW_SX, 0.0f), g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColor_Make(227, 227, 227, 255), TextAlignment_Center, TextAlignment_Center, true, text);
		}
		
		if (m_HintState != HintState_Disabled)
		{
			float t = 1.0f;
			if (m_HintState == HintState_AnimIn)
				t = m_HintTrigger.Progress_get();
			if (m_HintState == HintState_AnimOut)
				t = 1.0f - m_HintTrigger.Progress_get();
			
			Vec2F pos(0.0f, VIEW_SY - t * 33.0f);
			Vec2F size((float)VIEW_SX, 34.0f);
			RenderRect(pos, size, 1.0f, 1.0f, 1.0f, 1.0f, g_GameState->GetTexture(Textures::GAMEOVER_HINT_BACK));
			RenderText(pos, size, g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, m_Hint);
		}
		
		const FontMap* font = g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE);
		const char* text = "GAME OVER";
		const float textSx = font->m_Font.MeasureText(text);
		
		Mat3x2 matR;
		Mat3x2 matS;
		Mat3x2 matT;
		Mat3x2 matC;
		
		const float angle = sinf(g_GameState->m_TimeTracker_Global->Time_get() * 0.7f);
		matR.MakeRotation(angle);
		const float sx = (sinf(g_GameState->m_TimeTracker_Global->Time_get() * 2.345f) + 1.0f) * 0.5f * 2.0f + 0.5f;
		const float sy = (sinf(g_GameState->m_TimeTracker_Global->Time_get() * 3.456f) + 1.0f) * 0.5f * 2.0f + 0.5f;
		matS.MakeScaling(sx, sy);
		matT.MakeTranslation(Vec2F(VIEW_SX/2.0f, VIEW_SY/2.0f));
		matC.MakeTranslation(Vec2F(-textSx * 0.5f, -font->m_Font.m_Height * 0.5f));
		Mat3x2 mat = matT * matR * matS * matC;

		RenderText(mat, font, SpriteColor_MakeF(1.0f, 1.0f, 1.0f, (sinf(m_TouchTextTimer.Progress_get()) + 1.5f) / 2.5f), text);
							 
//		RenderText(Vec2F(0.0f, 160.0f), Vec2F(480.0f, 0.0f), g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, "GAME OVER");
	}
	
	// --------------------
	// View
	// --------------------
	
	int View_GameOver::RenderMask_get()
	{
		return RenderMask_HudInfo | RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground | RenderMask_WorldPrimary;
	}
	
	void View_GameOver::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Empty);
		
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_GAMEOVER);
		EventManager::I().Enable(EVENT_PRIO_INTERFACE_GAMEOVER);
		
		g_World->IsPaused_set(false);
		
		m_TouchTextTimer.Start(AnimTimerMode_TimeBased, false, 0.5f, AnimTimerRepeat_Mirror);
		m_RippleTimer.Restart();
		m_ExplosionTimer.Restart();
		m_TouchTrigger.Start(TOUCH_WAIT);
		m_TouchEnabled = false;
	}
	
	void View_GameOver::HandleFocusLost()
	{
		g_World->IsPaused_set(true);
		
		g_GameState->m_TouchDLG->Disable(USG::TOUCH_PRIO_GAMEOVER);
		EventManager::I().Disable(EVENT_PRIO_INTERFACE_GAMEOVER);
	}
	
	// --------------------
	// Touch related
	// --------------------
	
	bool View_GameOver::HandleTouchBegin(void* obj, const TouchInfo& touchInfo)
	{
		View_GameOver* self = (View_GameOver*)obj;
		
		if (!self->m_TouchEnabled)
			return false;

		NextView();
		
		return true;
	}
	
	bool View_GameOver::HandleTouchMove(void* obj, const TouchInfo& touchInfo)
	{
		return true;
	}
	
	bool View_GameOver::HandleTouchEnd(void* obj, const TouchInfo& touchInfo)
	{
		return true;
	}

	bool View_GameOver::OnEvent(Event& event)
	{
#if defined(PSP_UI)
		if (event.type == EVT_JOYBUTTON && (event.joy_button.button == INPUT_BUTTON_PSP_CROSS || event.joy_button.button == INPUT_BUTTON_PSP_CIRCLE) && event.joy_button.state)
		{
			NextView();
		}
#endif

		if (event.type == EVT_MENU_SELECT)
		{
			NextView();
		}

		return true;
	}
	
	static void NextView()
	{
#if defined(IPHONEOS) || defined(BBOS)
#if defined(IPHONEOS)
		bool isLoggedIn = g_gameCenter->IsLoggedIn();
#endif
#if defined(BBOS)
		bool isLoggedIn = true;
#endif
		if (isLoggedIn)
		{
			Game::View_ScoreSubmit* view = (Game::View_ScoreSubmit*)g_GameState->m_Views[::View_ScoreSubmit];
			view->Database_set(Game::ScoreDatabase_Global);
			g_GameState->ActiveView_set(::View_ScoreSubmit);
			//g_GameState->ActiveView_set(::View_ScoreAutoSubmit);
		}
		else
		{
			g_GameState->ActiveView_set(::View_ScoreEntry);
		}
#elif defined(PSP_UI)
		g_GameState->ActiveView_set(::View_ScoresPSP);
#else
		g_GameState->ActiveView_set(::View_ScoreEntry);
#endif
	}
}
