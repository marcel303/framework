#include "Particle.h"

Particle::Particle()
{
	mPosition[0] = 0.0;
	mPosition[1] = 0.0;
	mLife = 0.0;
	
	Density_get = 0;
}

void Particle::Setup(double x, double y, double vx, double vy, double life)
{
	mPosition[0] = x;
	mPosition[1] = y;
	mSpeed[0] = vx;
	mSpeed[1] = vy;
	mForce[0] = 0.0;
	mForce[1] = 0.0;
	mLife = life;
	mLifeRcp = 1.0 / life;
}

void Particle::Update(double dt)
{
	mModifierList.Update(this, dt);
	
	mSpeed[0] += mForce[0] * dt;
	mSpeed[1] += mForce[1] * dt;
	
	mPosition[0] += mSpeed[0] * dt;
	mPosition[1] += mSpeed[1] * dt;
	
	mSpeed[0] *= pow(0.95, dt);
	mSpeed[1] *= pow(0.95, dt);
	
	mForce[0] = 0.0;
	mForce[1] = 0.0;
}
