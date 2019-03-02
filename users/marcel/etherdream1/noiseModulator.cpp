#include "noiseModulator.h"
//#include <glm/gtc/noise.hpp>
#include <math.h>
//#include <mathutils.h>

#include <stdio.h>

void NoiseModulator::tick(const float x, const float dt)
{
	if (dt <= 0.f)
		return;
	
	const float retain = powf(1.f - followFactor, dt * 100.f);

	x_desired = x;
	
	x1 = x_current;
	x_current = x_desired * (1.f - retain) + x_current * retain;
	x2 = x_current;
	
	time_phase = fmodf(time_phase + dt * noiseFrequency_time, 1.f);
	
	velocity = fabsf(x2 - x1) / dt;
	//printf("velocity: %g\n", velocity);
}

static float isInside(const float x, const float in_x1, const float in_x2)
{
	const float x1 = fminf(in_x1, in_x2);
	const float x2 = fmaxf(in_x1, in_x2);
	
	return x >= x1 && x <= x2;
}

static float distanceBetween(const float x1, const float x2)
{
	return fabsf(x2 - x1);
}

float NoiseModulator::calculateForce(const float in_x, const float in_y) const
{
	float dx = isInside(in_x, x1, x2)
		? 0.f
		: fminf(distanceBetween(in_x, x1), distanceBetween(in_x, x2));
	
	dx *= falloff_strength;
	
	float falloff = 1.f / fmaxf(dx * dx, .000001f);
	falloff *= 1.f / sqrtf(1.f + falloff * falloff);
	
	float valueAtX = 0.f;
	
	//if (mode == kNoiseModulatorMode_Simplex)
	//	valueAtX = glm::simplex(glm::vec2(in_x * noiseFrequency, 0.f));
	//else if (mode == kNoiseModulatorMode_Random)
	//	valueAtX = nap::math::random<float>(-1.f, +1.f);
	//else
		valueAtX = sinf((in_x * noiseFrequency_spat + time_phase) * 2.f * float(M_PI));
	
	if (multiply_strength_with_velocity)
		valueAtX *= velocity;
		
	return valueAtX * falloff * strength;
}
