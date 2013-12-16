#if 0

#include "DebrisMgr.h"
#include "GameState.h"
#include "UsgResources.h"

namespace Game
{
	typedef struct ParticleData
	{
		float m_SpinSpeed;
		float m_Scale; // 0.5..1
		Vec2F m_Speed;
	};
	
	DebrisMgr::DebrisMgr()
	{
	}
	
	void DebrisMgr::Setup(int maxDebris)
	{
		// todo: make # particle settable in particle effect
	}
	
	void DebrisMgr::Update(float dt)
	{
		m_ParticleEffect.Update(dt);
	}
	
	void DebrisMgr::Render()
	{
		m_ParticleEffect.Render(&g_GameState->m_SpriteGfx);
	}
	
	// todo: speed of debris?
	
	void DebrisMgr::Spawn(const Vec2F& pos, DebrisSize size)
	{
		// todo: decide which graphics to use
		
		const int Shapes_Small[] =
		{
			Resources::DEBRIS_SMALL_01,
			Resources::DEBRIS_SMALL_02,
			Resources::DEBRIS_SMALL_03,
		};
		
		const int Shapes_Medium[] =
		{
			Resources::DEBRIS_MEDIUM_01,
			Resources::DEBRIS_MEDIUM_02,
			Resources::DEBRIS_MEDIUM_03,
		};
		
		const int Shapes_Large[] =
		{
			Resources::DEBRIS_LARGE_01,
			Resources::DEBRIS_LARGE_02,
			Resources::DEBRIS_LARGE_03,
		};
		
		// todo: decide amount of particles to spawn
		
		int amount = 0;
		const int* resources;
		
		switch (size)
		{
			case DebrisSize_One:
				amount = 1;
				resources = Shapes_Small;
				break;
			case DebrisSize_Small:
				amount = 2;
				resources = Shapes_Small;
				break;
			case DebrisSize_Medium:
				amount = 3;
				resources = Shapes_Medium;
				break;
			case DebrisSize_Large:
				amount = 5;
				resources = Shapes_Large;
				break;
			case DebrisSize_Massive:
				amount = 9;
				resources = Shapes_Large;
				break;
		}
		
		// todo: spawn particles
		
		for (int i = 0; i < amount; ++i)
		{
			int index = rand() % 3;
			VectorShape* shape = g_GameState->GetShape(resources[index]);
			
			Particle& particle = m_ParticleEffect.Allocate(0, shape, Update);
			
			particle.Setup(pos[0], pos[1], 20.0f, 1.0f, 1.0f, Calc::Random(0.0f, Calc::m2PI));
			
			ParticleData* data = (ParticleData*)particle.Data_get();
			
			data->m_SpinSpeed = Calc::Random(-1.0f, +1.0f);
			data->m_Scale = Calc::Random(-0.5f, 1.0f); // fixme.. use particle.m_Scale, render w/ scale
			data->m_Speed = Vec2F::FromAngle(Calc::Random(0.0f, Calc::m2PI)) * 10.0f;
		}
	}
	
	void DebrisMgr::Clear()
	{
		m_ParticleEffect.Clear();
	}
	
	void DebrisMgr::Update(Particle* particle, float dt)
	{
		ParticleData* data = (ParticleData*)particle->Data_get();
		
		particle->m_Angle += data->m_SpinSpeed * dt;
		particle->m_Position += data->m_Speed * dt;
	}
}

#endif
