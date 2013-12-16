#include "Calc.h"
#include "GameState.h"
#include "Mat3x2.h"
#include "TempRender.h"
#include "Textures.h"
#include "Timer.h"
#include "UsgResources.h"
#include "View_ScoreAutoSubmit.h"

#define FADEIN_TIME 2.0f
#define FADEOUT_TIME 1.5f
#define SKIP_TIME 3.5f

namespace Game
{
	View_ScoreAutoSubmit::View_ScoreAutoSubmit()
	{
		m_fadeInTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_fadeOutTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_skipTimer.Initialize(g_GameState->m_TimeTracker_Global);
	}
	
	View_ScoreAutoSubmit::~View_ScoreAutoSubmit()
	{
		
	}
	
	void View_ScoreAutoSubmit::Update(float dt)
	{
		if (m_skipTimer.Read())
		{
			// todo: enable skip button
			
			LOG_DBG("auto submit score: skip timer triggered", 0);
		}
		
		if (m_state == State_Submit)
		{
		}
		else if (m_state == State_SubmitComplete)
		{
			if (!m_fadeInTimer.IsRunning_get())
			{
				// start fade out
				
				m_fadeOutTimer.Start(AnimTimerMode_TimeBased, false, 1.5f, AnimTimerRepeat_None);
				
				m_state = State_FadeOut;
				
				LOG_DBG("auto submit view: state = State_FadeOut", 0);
			}
		}
		else if (m_state == State_FadeOut)
		{
			if (!m_fadeOutTimer.IsRunning_get())
			{
				m_state = State_Done;
				
				g_GameState->ActiveView_set(::View_Scores);
				
				LOG_DBG("auto submit view: state = State_Done", 0);
			}
		}
		else if (m_state == State_Done)
		{
			// nop
		}
		else
		{
			Assert(false);
		}
	}
	
	void View_ScoreAutoSubmit::Render()
	{
		const AtlasImageMap * backImage = g_GameState->GetTexture(Textures::COLOR_WHITE);
		
		RenderRect(Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY), 0.0f, 0.0f, 0.0f, backImage);
		
		if (m_state == State_Submit || m_state == State_SubmitComplete)
		{
			// render fade in
			
			float t = m_fadeInTimer.IsRunning_get() ? m_fadeInTimer.Progress_get() : 1.0f;
			
			float x = Calc::LerpSat(60.0f, 20.0f, t);
			float a = Calc::Min(1.0f, t * 1.0f);
			
			Mat3x2 matT;
			matT.MakeTranslation(Vec2F(x, VIEW_SY / 2.0f - 22.0f));
			Mat3x2 mat = matT;
			SpriteColor color = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, a);
			
			RenderText(mat, g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), color, TextAlignment_Left, TextAlignment_Center, "Submitting Highscore..");
		}
		else if (m_state == State_FadeOut)
		{
			// render fade out
		}
		else if (m_state == State_Done)
		{
			// render black screen
		}
		else
		{
			Assert(false);
		}
	}
	
	void View_ScoreAutoSubmit::HandleFocus()
	{
		m_state = State_Submit;
		
		// submit score
		
		// start fade in
		
		m_fadeInTimer.Start(AnimTimerMode_TimeBased, false, FADEIN_TIME, AnimTimerRepeat_None);
		m_skipTimer.Start(SKIP_TIME);
		
		LOG_DBG("auto submit view: state = State_Submit", 0);
	}
	
	int View_ScoreAutoSubmit::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles;
	}
	
	float View_ScoreAutoSubmit::FadeTime_get()
	{
		return 0.0f;
	}
	
	void View_ScoreAutoSubmit::Handle_ScoreSubmitComplete(void * obj, void * arg)
	{
		View_ScoreAutoSubmit* self = (View_ScoreAutoSubmit*)obj;
		
		self->m_state = State_SubmitComplete;
		
		LOG_DBG("auto submit view: state = State_SubmitComplete", 0);
	}
	
	void View_ScoreAutoSubmit::Handle_ScoreSubmitFailed(void * obj, void * arg)
	{
		//View_ScoreAutoSubmit* self = (View_ScoreAutoSubmit*)obj;	
	}
	
	void View_ScoreAutoSubmit::Handle_Dismiss(void * obj, void * arg)
	{
		//View_ScoreAutoSubmit* self = (View_ScoreAutoSubmit*)obj;
		
		g_GameState->ActiveView_set(::View_Main);
	}
}
