#include "Calc.h"
#include "GameState.h"
#include "MissileTrail.h"
#include "ParticleEffect.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "UsgResources.h"

#if 0
#define FIRE_T1 0.0f
#define FIRE_T2 0.6f
#define SMOKE_T1 0.3f
#define SMOKE_T2 1.0f
#else
#define FIRE_T1 0.0f
#define FIRE_T2 1.0f
#define SMOKE_T1 2.0f
#define SMOKE_T2 3.0f
#endif

namespace Game
{	
	typedef struct ParticleData
	{
		bool m_DrawFire;
		bool m_DrawSmoke;
		float m_FireAlpha;
		float m_SmokeAlpha;
	} ParticleData;
	
	static void Particle_Setup(Particle* p, float x, float y, float life)
	{
		p->Setup(x, y, life, 0.0f, 0.0f, 0.0f);
	}
	
	static void Particle_Update(Particle* p, float dt)
	{
		ParticleData* data = (ParticleData*)p->Data_get();
		
		float t = 1.0f - p->m_Life * p->m_LifeBeginRcp;
		
		data->m_DrawFire = t >= FIRE_T1 && t <= FIRE_T2;
		data->m_DrawSmoke = t >= SMOKE_T1 && t <= SMOKE_T2;
		
		data->m_FireAlpha = (t - FIRE_T1) / (FIRE_T2 - FIRE_T1);
		data->m_SmokeAlpha = (t - SMOKE_T1) / (SMOKE_T2 - SMOKE_T1);
		
		if (data->m_DrawFire)
		{
			// update color based on progress
			
			float r = 1.0f - data->m_FireAlpha * 1.0f;
			float g = 1.0f - data->m_FireAlpha * 2.0f;
			float b = 1.0f - data->m_FireAlpha * 4.0f;
			float a = 1.0f - data->m_FireAlpha;
			
			r = Calc::Saturate(r);
			g = Calc::Saturate(g);
			b = Calc::Saturate(b);
			a = Calc::Saturate(a);
			
			p->m_Color = SpriteColor_MakeF(r, g, b, a);
		}
	}
	
	static void Particle_Render(const Particle& p)
	{
		const ParticleData* data = (const ParticleData*)p.Data_get();
		
		if (data->m_DrawFire)
		{
			g_GameState->Render(g_GameState->GetShape(Resources::MISSILE_TRAIL), p.m_Position, 0.0f, p.m_Color);
		}
		
		if (data->m_DrawSmoke)
		{
			SpriteColor color = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, (1.0f - data->m_SmokeAlpha) * 0.4f);
//			SpriteColor color = SpriteColor_MakeF(0.0f, 0.0f, 0.0f, (1.0f - data->m_SmokeAlpha) * 0.4f);
			
			g_GameState->Render(g_GameState->GetShape(Resources::MISSILE_TRAIL_GRAY), p.m_Position, 0.0f, color);
		}
	}
	
	//
	
	MissileTrail::MissileTrail()
	{
		Initialize();
	}
	
	MissileTrail::~MissileTrail()
	{
	}
	
	void MissileTrail::Initialize()
	{
		m_Traveller.Setup(10.0f, CallBack(this, HandleTravelEvent));
	}
	
	void MissileTrail::Setup(const Vec2F& pos)
	{
		m_Traveller.Begin(pos[0], pos[1]);
	}
	
	void MissileTrail::Update(float dt)
	{
	}
	
	void MissileTrail::HandleTravelEvent(void* obj, void* arg)
	{
		// note: self is unreliable
		
		TravelEvent* e = (TravelEvent*)arg;
	
		if (e->dx != 0.0f || e->dy != 0.0f)
		{
			const AtlasImageMap* image = g_GameState->GetTexture(Textures::EXPLOSION_LINE);

			Particle& p = g_GameState->m_ParticleEffect.Allocate(image->m_Info, 0, 0);
			
			Particle_RotMove_Setup(&p, e->x, e->y, 1.2f, 7.0f, 1.3f, Vec2F(e->dx, e->dy).ToAngle(), 0.0f, 0.0f, 0.0f);
		}

		Particle& p = g_GameState->m_ParticleEffect.Allocate(0, g_GameState->GetShape(Resources::MISSILE_TRAIL), Particle_Update);

		p.m_RenderCB = Particle_Render;
		
		Particle_Setup(&p, e->x, e->y, 0.5f);
	}
	
	void MissileTrail::Position_set(const Vec2F& position)
	{
		//m_Pos = position;
		m_Traveller.Update(position[0], position[1]);
	}
}
