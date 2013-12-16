#pragma once

#include "Particle.h"

class AC_Vortex : public Modifier
{
public:
	void Setup(double cx, double cy, double a);
	virtual void Update(Particle* p, double dt);
	
private:
	Vec mCenter;
	double mAccel;
};
