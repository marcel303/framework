#include "Atlas_ImageInfo.h"
#include "Calc.h"
#include "Entity.h"
#include "ParticleGenerator.h"

void ParticleGenerator::GenerateRandomExplosion(ParticleEffect& e, const Vec2F& pos, float minSpeed, float maxSpeed, float life, int particleCount, const Atlas_ImageInfo* image, float imageScale, ParticleUpdateCB updateCB)
{
	for (int i = 0; i < particleCount; ++i)
	{
		const float angle = Calc::Random(0.0f, Calc::m2PI);
		const float speed = Calc::Random(minSpeed, maxSpeed);
		
		Particle& p = e.Allocate(image, 0, updateCB);
		
		Particle_Default_Setup(&p, pos[0], pos[1], life, image->m_ImageSize[0] * imageScale, image->m_ImageSize[1] * imageScale, angle, speed);
	}
}

void ParticleGenerator::GenerateCircularExplosion(ParticleEffect& e, const Vec2F& pos, float speed, float life, float size, int particleCount, const Atlas_ImageInfo* image, ParticleUpdateCB updateCB)
{
	for (int i = 0; i < particleCount; ++i)
	{
		const float angle = Calc::Random(0.0f, Calc::m2PI);
		
		Particle& p = e.Allocate(image, 0, updateCB);
		
		Particle_Default_Setup(&p, pos[0], pos[1], life, size, size, angle, speed);
	}
}

void ParticleGenerator::GenerateCircularUniformExplosion(ParticleEffect& e, const Vec2F& pos, float baseAngle, float speed, float life, int particleCount, const Atlas_ImageInfo* image, float imageScale, ParticleUpdateCB updateCB)
{
	const float step = Calc::m2PI / particleCount;
	
	for (int i = 0; i < particleCount; ++i)
	{
		const float angle = baseAngle + step * i;
		
		Particle& p = e.Allocate(image, 0, updateCB);
		
		Particle_Default_Setup(&p, pos[0], pos[1], life, image->m_ImageSize[0] * imageScale, image->m_ImageSize[1] * imageScale, angle, speed);
	}
}

void ParticleGenerator::GenerateSparks(ParticleEffect& e, const Vec2F& pos, const Vec2F& dir, float spread, float speed, float life, float size, int particleCount, const Atlas_ImageInfo* image, ParticleUpdateCB updateCB)
{
	const float baseAngle = Vec2F::ToAngle(dir);
	
	spread *= 0.5f;
	
	for (int i = 0; i < particleCount; ++i)
	{
		const float angle = baseAngle + Calc::Random(-spread, +spread);
		
		Particle& p = e.Allocate(image, 0, updateCB);

		Particle_Default_Setup(&p, pos[0], pos[1], life, size, size, angle, speed);
	}
}

class WarpData
{
public:
	CD_TYPE m_Id;
	Vec2F m_Speed;
	float m_Warp;
	float m_Falloff;
};

static void UpdateWarp(Particle* particle, float dt)
{
	WarpData* data = (WarpData*)particle->Data_get();
		
	if (!Game::g_EntityInfo[data->m_Id].alive)
		particle->m_Life = 0.0f;
	else
	{		
		const Vec2F delta = Game::g_EntityInfo[data->m_Id].position - particle->m_Position;
		
		const float force = delta.Length_get() * data->m_Warp;
		
		data->m_Speed += delta.Normal() * force * dt;
		data->m_Speed *= data->m_Falloff;
		
		particle->m_Position += data->m_Speed * dt;
		
		const float v = particle->m_Life * particle->m_LifeBeginRcp;
			
		const int c = (int)(v * 255.0f);
		
		particle->m_Color.v[3] = c;// = SpriteColor_Make(c, c, c, c);
	}
}

void ParticleGenerator::GenerateWarp(ParticleEffect& e, const Vec2F& pos, const Vec2F& dir, bool oriented, float spread, float speed1, float speed2, float warp, float falloff, float life, float size, int particleCount, const Atlas_ImageInfo* image, CD_TYPE entityId)
{
	const float baseAngle = Vec2F::ToAngle(dir);
	
	spread *= 0.5f;
	
	for (int i = 0; i < particleCount; ++i)
	{
		const float angle = baseAngle + Calc::Random(-spread, +spread);
		float speed = Calc::Random(speed1, speed2);
		
		Particle& p = e.Allocate(image, 0, UpdateWarp);

		p.Setup(pos[0], pos[1], life, size, size, oriented ? angle : 0.0f);
		
		WarpData* data = (WarpData*)p.Data_get();
		
		data->m_Id = entityId;
		data->m_Speed = Vec2F::FromAngle(angle) * speed;
		data->m_Warp = warp;
		data->m_Falloff = powf(falloff, 1.0f/60.0f);
	}
}
