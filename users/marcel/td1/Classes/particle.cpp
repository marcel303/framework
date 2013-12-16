#include "OpenGLCompat.h"
#include "Mat3x2.h"
#include "particle.h"
#include "SpriteGfx.h"

ParticleMgr* gParticleMgr = 0;

void Particle::Setup(const Vec2F& _p, const Vec2F& _s, float _life, float sx, float sy, const SpriteColor& c)
{
	p = _p;
	s = _s;
	life = _life;
	lifeRcp = 1.0f / _life;
	float angle = _s.ToAngle();
	Mat3x2 mat;
	mat.MakeRotation(-angle);
	q[0] = mat * Vec2F(-sx, -sy);
	q[1] = mat * Vec2F(+sx, -sy);
	q[2] = mat * Vec2F(+sx, +sy);
	q[3] = mat * Vec2F(-sx, +sy);
	rgba = c.rgba;
}

ParticleMgr::ParticleMgr(int capacity)
{
	mParticles = new Particle[capacity];
	mParticleCount = capacity;
	mParticleIndex = 0;
	
	mGfx = new SpriteGfx(1000, 3000);
}

ParticleMgr::~ParticleMgr()
{
	delete mGfx;
	mGfx = 0;
	
	delete[] mParticles;
	mParticles = 0;
}

void ParticleMgr::Update(float dt)
{
	for (int i = 0; i < mParticleCount; ++i)
	{
		Particle& p = mParticles[i];
		
		if (!p.IsAlive_get())
			continue;
		
		p.p += p.s * dt;
		p.life -= dt;
	}
}

void ParticleMgr::Render()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	for (int i = 0; i < mParticleCount; ++i)
	{
		Particle& p = mParticles[i];
		
		if (!p.IsAlive_get())
			continue;
		
		float t = p.life * p.lifeRcp;
//		SpriteColor c = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, t);
		SpriteColor c;
		c.rgba = p.rgba;
		c.v[3] = t * 255.0f;
//		int rgba = c.rgba;
		int rgba = c.rgba;
		
		mGfx->Reserve(4, 6);
		mGfx->WriteBegin();
		for (int j = 0; j < 4; ++j)
		{
			mGfx->WriteVertex(p.p[0] + p.q[j][0], p.p[1] + p.q[j][1], rgba, 0.0f, 0.0f);
		}
		mGfx->WriteIndex3(0, 1, 2);
		mGfx->WriteIndex3(0, 2, 3);
		mGfx->WriteEnd();
	}
	
	mGfx->Flush();
	
	glDisable(GL_BLEND);
}

Particle* ParticleMgr::Allocate()
{
	Particle* p = mParticles + mParticleIndex;
	
	mParticleIndex++;
	
	if (mParticleIndex == mParticleCount)
		mParticleIndex = 0;
	
	return p;
}
