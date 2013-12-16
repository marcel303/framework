#pragma once

#include "Reward.h"

#define SCORE_EFFECT_COUNT 20

namespace Game
{
	class ScoreEffect
	{
	public:
		ScoreEffect();
		void Initialize();
		
		void Setup(Reward reward, Vec2F position);
		
		void Update(float dt);
		void Render();
		
	private:
		Reward m_Reward;
		bool m_IsAlive;
		float m_LifeLeft;
		Vec2F m_Position;
		const VectorShape* m_Shape;
		
		friend class ScoreEffectMgr;
	};
	
	class ScoreEffectMgr
	{
	public:
		ScoreEffectMgr();
		void Initialize();
		
		void Update(float dt);
		void Render();
		
		void Spawn(Reward reward, Vec2F position);
		
	private:
		ScoreEffect m_Effects[SCORE_EFFECT_COUNT];
		int m_EffectIndex;
	};
}
