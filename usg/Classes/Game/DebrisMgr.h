#if 0

#pragma once

#include "ParticleEffect.h"

namespace Game
{
	enum DebrisSize
	{
		DebrisSize_One,
		DebrisSize_Small,
		DebrisSize_Medium,
		DebrisSize_Large,
		DebrisSize_Massive
	};
	
	class DebrisMgr
	{
	public:
		DebrisMgr();
		void Setup(int maxDebris);
		
		void Update(float dt);
		void Render();
		
		void Spawn(const Vec2F& pos, DebrisSize size);
		void Clear();
		
	private:
		static void Update(Particle* particle, float dt);
		
		ParticleEffect m_ParticleEffect;
	};
}

#endif
