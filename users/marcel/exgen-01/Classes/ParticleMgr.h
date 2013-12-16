#pragma once

#include "Particle.h"
#include "Surface.h"

class ParticleMgr
{
public:
	ParticleMgr();
	~ParticleMgr();
	void Setup(int count);
	
	void Update(double dt);
	void Render(Surface* surface, ColorRampCB ramp);
	
	inline int ParticleCount_get() const
	{
		return mParticleCount;
	}
	
	inline Particle& operator[](int index)
	{
		return mParticles[index];
	}
	
private:
	Particle* mParticles;
	int mParticleCount;
};
