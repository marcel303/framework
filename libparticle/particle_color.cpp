#include "particle.h"

#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include "ui.h" // srgb <-> linear

#include <string.h> // memset

ParticleColor::ParticleColor()
{
	memset(this, 0, sizeof(ParticleColor));
}

ParticleColor::ParticleColor(float r, float g, float b, float a)
{
	rgba[0] = r;
	rgba[1] = g;
	rgba[2] = b;
	rgba[3] = a;
}

void ParticleColor::set(float r, float g, float b, float a)
{
	rgba[0] = r;
	rgba[1] = g;
	rgba[2] = b;
	rgba[3] = a;
}

void ParticleColor::modulateWith(const ParticleColor & other)
{
	for (int i = 0; i < 4; ++i)
		rgba[i] *= other.rgba[i];
}

void ParticleColor::interpolateBetween(const ParticleColor & v1, const ParticleColor & v2, const float t)
{
	const float t1 = 1.f - t;
	const float t2 =       t;
	for (int i = 0; i < 4; ++i)
		rgba[i] = v1.rgba[i] * t1 + v2.rgba[i] * t2;
}

void ParticleColor::interpolateBetweenLinear(const ParticleColor & v1, const ParticleColor & v2, const float t)
{
	const float t1 = 1.f - t;
	const float t2 =       t;
	
	float linear1[3];
	float linear2[3];
	float linear[3];
	
	srgbToLinear(v1.rgba[0], v1.rgba[1], v1.rgba[2], linear1[0], linear1[1], linear1[2]);
	srgbToLinear(v2.rgba[0], v2.rgba[1], v2.rgba[2], linear2[0], linear2[1], linear2[2]);
	
	for (int i = 0; i < 3; ++i)
		linear[i] = linear1[i] * t1 + linear2[i] * t2;
	for (int i = 3; i < 4; ++i)
		rgba[i] = v1.rgba[i] * t1 + v2.rgba[i] * t2;
	
	linearToSrgb(linear[0], linear[1], linear[2], rgba[0], rgba[1], rgba[2]);
}

void ParticleColor::save(tinyxml2::XMLPrinter * printer) const
{
	printer->PushAttribute("r", rgba[0]);
	printer->PushAttribute("g", rgba[1]);
	printer->PushAttribute("b", rgba[2]);
	printer->PushAttribute("a", rgba[3]);
}

void ParticleColor::load(const tinyxml2::XMLElement * elem)
{
	*this = ParticleColor();

	rgba[0] = floatAttrib(elem, "r", rgba[0]);
	rgba[1] = floatAttrib(elem, "g", rgba[1]);
	rgba[2] = floatAttrib(elem, "b", rgba[2]);
	rgba[3] = floatAttrib(elem, "a", rgba[3]);
}
