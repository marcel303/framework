#ifndef PARTICLESYS_H
#define PARTICLESYS_H
#pragma once

#include "Array.h"
#include "PolledTimer.h"
#include "ResIB.h"
#include "ResShader.h"
#include "ResVB.h"
#include "Vec3.h"

class Particle
{
public:
	Particle();

	void Update(float dt);

	bool IsDead() const;

	Vec3 m_position;
	Vec3 m_velocity;
	Vec3 m_force;
	float m_size;
	float m_life;
	float m_lifeSpan;
};

class ParticleSrc
{
public:
	ParticleSrc();
	virtual ~ParticleSrc();

	bool WantEmit();
	virtual void Emit(Particle* particle, const Vec3& position, float t);

	void Init(float speed, float size, float lifeSpan, float interval);

	float m_speed;
	float m_size;
	float m_lifeSpan;
	float m_interval;

private:
	PolledTimer m_emitTimer;
};

class ParticleSrcCone : public ParticleSrc
{
public:
	ParticleSrcCone();

	virtual void Emit(Particle* particle, const Vec3& position, float t);

	void Init(float speed, float size, float lifeSpan, float interval, Vec3 direction, float aperture);

	Vec3 m_direction;
	float m_aperture;
};

class ParticleMod
{
public:
	ParticleMod();

	virtual void Mod(Particle* particle, float t);

	void SetGravity(Vec3 gravity);

	Vec3 m_gravity;
};

class ParticleSys
{
public:
	ParticleSys();
	~ParticleSys();

	void Update(float dt);
	void Render();

	void Initialize(Vec3 position, int particleCount, bool respawn);

	bool AllDead();

//fixme private:
	void FillVB();

	ResVB m_vb;
	ResIB m_ib;

	Vec3 m_position;
	bool m_respawn;
	Array<Particle> m_particles;
	ParticleSrc* m_src;
	ParticleMod* m_mod;
	ShTex m_tex;
	ShShader m_shader;

	float m_t;
};

#endif
