#pragma once

#include "libiphone_forward.h"
#include "Types.h"

class Particle
{
public:
	inline Particle()
	{
		life = 0.0f;
	}
	
	void Setup(const Vec2F& _p, const Vec2F& _s, float _life, float sx, float sy, const SpriteColor& c);
	
	inline bool IsAlive_get() const
	{
		return life > 0.0f;
	}
	
	Vec2F p;
	Vec2F q[4];
	Vec2F s;
	float life;
	float lifeRcp;
	int rgba;
};

class ParticleMgr
{
public:
	ParticleMgr(int capacity);
	~ParticleMgr();
	
	void Update(float dt);
	void Render();
	
	Particle* Allocate();
	
private:
	Particle* mParticles;
	int mParticleCount;
	int mParticleIndex;
	SpriteGfx* mGfx;
};

extern ParticleMgr* gParticleMgr;
