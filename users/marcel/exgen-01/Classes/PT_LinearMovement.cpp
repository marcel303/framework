#include "PT_LinearMovement.h"

void AC_Vortex::Setup(double cx, double cy, double a)
{
	mCenter[0] = cx;
	mCenter[1] = cy;
	mAccel = a;
}
 
void AC_Vortex::Update(Particle* p, double dt)
{
	Vec s = p->mPosition - mCenter;
	double l = s.L();
	Vec d = s.D();
	Vec n = d.N();
	
	p->mForce.AddS(d, -mAccel * l);
	p->mForce.AddS(n, +mAccel * 10.0);
}
