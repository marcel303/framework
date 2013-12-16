#include "GameState.h"
#include "ScoreEffect.h"
#include "StringBuilder.h"
#include "TempRender.h"
#include "UsgResources.h"

namespace Game
{
	ScoreEffect::ScoreEffect()
	{
		Initialize();
	}
	
	void ScoreEffect::Initialize()
	{
		m_IsAlive = false;
	}
	
	void ScoreEffect::Setup(Reward reward, Vec2F position)
	{
		m_Reward = reward;
		m_LifeLeft = 3.5f;
		m_IsAlive = true;
		m_Position = position;
		
		switch (reward.m_Type)
		{
			case RewardType_ScoreBronze:
				m_Shape = g_GameState->GetShape(Resources::SCORE_BRONZE);
				break;
			case RewardType_ScoreSilver:
				m_Shape = g_GameState->GetShape(Resources::SCORE_SILVER);
				break;
			case RewardType_ScoreGold:
				m_Shape = g_GameState->GetShape(Resources::SCORE_GOLD);
				break;
			case RewardType_ScorePlatinum:
				m_Shape = g_GameState->GetShape(Resources::SCORE_PLATINUM);
				break;
				
			default:
				m_Shape = 0;
		}
	}
	
	void ScoreEffect::Update(float dt)
	{
		// update position
		
		m_Position[1] -= dt * 25.0f;
		
		// update life
		
		m_LifeLeft -= dt;
		
		if (m_LifeLeft <= 0.0f)
			m_IsAlive = false;
	}
	
	void ScoreEffect::Render()
	{
		float a = m_LifeLeft;
		
		if (a > 1.0f)
			a = 1.0f;
		
		SpriteColor color = SpriteColor_MakeF(0.0f, 0.0f, 0.0f, a);
		
		if (m_Shape)
		{
			g_GameState->Render(m_Shape, m_Position, 0.0f, color);
		}
		else 
		{
			StringBuilder<32> sb;
			sb.AppendFormat("+%d", m_Reward.m_Value);
			RenderText(m_Position, Vec2F(0.0f, 0.0f), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), color, TextAlignment_Center, TextAlignment_Center, true, sb.ToString());
		}
	}
	
	//

	ScoreEffectMgr::ScoreEffectMgr()
	{
		Initialize();
	}
	
	void ScoreEffectMgr::Initialize()
	{
		m_EffectIndex = 0;
	}
	
	void ScoreEffectMgr::Update(float dt)
	{
		for (int i = 0; i < SCORE_EFFECT_COUNT; ++i)
			if (m_Effects[i].m_IsAlive)
				m_Effects[i].Update(dt);
	}
	
	void ScoreEffectMgr::Render()
	{
		for (int i = 0; i < SCORE_EFFECT_COUNT; ++i)
			if (m_Effects[i].m_IsAlive)
				m_Effects[i].Render();
	}
	
	void ScoreEffectMgr::Spawn(Reward reward, Vec2F position)
	{
		if (reward.m_Type == RewardType_None)
			return;
		
		ScoreEffect& effect = m_Effects[m_EffectIndex];
		
		effect.Setup(reward, position);
		
		m_EffectIndex++;
		
		if (m_EffectIndex == SCORE_EFFECT_COUNT)
			m_EffectIndex = 0;
	}
}
