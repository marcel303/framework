#pragma once

#include "Vec3.h"

struct AngleAxis
{
	float angle = 0.f;
	
	Vec3 axis = Vec3(0.f, 1.f, 0.f);
};