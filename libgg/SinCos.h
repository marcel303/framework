#pragma once

#include <math.h>

struct SinCos
{
	double s;
	double c;
	
	double f;
	
	void init(const double frequency, const double amplitude)
	{
		f = 2.0 * M_PI * frequency;
		
		s = 0.0;
		c = amplitude;
	}

	void initWithTimeStep(const double frequency, const double amplitude, const double timeStep)
	{
		f = 2.0 * M_PI * frequency * timeStep;
		
		s = 0.0;
		c = amplitude;
	}
	
	void next()
	{
		s = s + f * c;
		c = c - f * s;
	}

	void next(const double dt)
	{
		s = s + f * dt * c;
		c = c - f * dt * s;
	}
};
