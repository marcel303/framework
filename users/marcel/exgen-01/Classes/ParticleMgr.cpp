#include "ParticleMgr.h"

ParticleMgr::ParticleMgr()
{
	mParticles = 0;
	mParticleCount = 0;
}

ParticleMgr::~ParticleMgr()
{
	Setup(0);
}

void ParticleMgr::Setup(int count)
{
	delete[] mParticles;
	mParticles = 0;
	mParticleCount = 0;
	
	if (count > 0)
	{
		mParticles = new Particle[count];
		mParticleCount = count;
	}
}

void ParticleMgr::Update(double dt)
{
	for (int i = 0; i < mParticleCount; ++i)
	{
		if (mParticles[i].IsDead_get())
			continue;
		
		mParticles[i].mLife -= dt;
		
		mParticles[i].Update(dt);
	}
}

void ParticleMgr::Render(Surface* surface, ColorRampCB ramp)
{
	// todo: render particles using HQ drawing code
	
	for (int y = 0; y < surface->Sy_get(); ++y)
	{
		for (int x = 0; x < surface->Sx_get(); ++x)
		{
			double totalDensity = 0.0;
			
			for (int i = 0; i < mParticleCount; ++i)
			{
				if (mParticles[i].IsDead_get())
					continue;
				
				double density = mParticles[i].Density_get(&mParticles[i], x, y);
				
				if (density < 0.0)
					continue;
				
				totalDensity += density * mParticles[i].LifeFactor_get();
			}
			
			Pixel pixel = ramp(totalDensity);
			
			surface->Pixel_set(x, y, pixel);
		}
	}
}
