#pragma once

#include "Particle.h"

namespace DT_Linear
{
	void Setup(Particle* p, double linearFactor);
	double Density_get(Particle* p, double x, double y);
}
