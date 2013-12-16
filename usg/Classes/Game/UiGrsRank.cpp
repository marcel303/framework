#include "FontMap.h"
#include "GameState.h"
#include "Mat3x2.h"
#include "TempRender.h"
#include "UiGrsRank.h"
#include "UsgResources.h"

#define POS_Y 0.0f
//#define POS_Y 5.0f

namespace Game
{
	UiGrsRank::UiGrsRank()
	{
	}
	
	void UiGrsRank::Setup()
	{
		m_GrsScoreEffectTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		
		m_Rank = -1;
	}
	
	void UiGrsRank::Render()
	{
		const char* name = 0;
		
		if (m_Rank <= 10)
		{
			switch (m_Rank)
			{
#define CASE(n, s) \
	case n:\
		name = s; \
		break
					CASE(-1, "(n/a)");
					CASE(0, "1st!");
					CASE(1, "2nd!");
					CASE(2, "3rd!");
					CASE(3, "4th");
					CASE(4, "5th");
					CASE(5, "6th");
					CASE(6, "7th");
					CASE(7, "8th");
					CASE(8, "9th");
					CASE(9, "10th");
			}
		}
		
		SpriteColor color = SpriteColor_Make(255, 191, 127, 255);
		
		const FontMap* font = g_GameState->GetFont(Resources::FONT_USUZI_SMALL);
		
		char temp[16];
		
		if (name)
			sprintf(temp, "rank:%s", name);
		else
			sprintf(temp, "rank:#%d", m_Rank);
		
		float length = font->m_Font.MeasureText(temp);
		
		Mat3x2 mat;
		
		if (m_GrsScoreEffectTimer.IsRunning_get())
		{
			Mat3x2 matT;
			Mat3x2 matS;
			matT.MakeTranslation(Vec2F(VIEW_SX - 10 - length, POS_Y));
			float scale = 1.0f + m_GrsScoreEffectTimer.Progress_get() * 3.0f;
			matS.MakeScaling(scale, scale);
			mat = matT * matS;
			color = SpriteColor_BlendF(color, SpriteColor_MakeF(1.0f, 1.0f, 1.0f, 1.0f - m_GrsScoreEffectTimer.Progress_get()), m_GrsScoreEffectTimer.Progress_get());
		}
		else
		{
			mat.MakeTranslation(Vec2F(VIEW_SX - 10 - length, POS_Y));
		}
		
		RenderText(mat, font, color, temp);
	}
	
	void UiGrsRank::Rank_set(int value)
	{
		if (m_Rank != value)
			m_GrsScoreEffectTimer.Start(AnimTimerMode_TimeBased, true, 0.3f, AnimTimerRepeat_None);
		
		m_Rank = value;
	}
}
