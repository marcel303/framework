#pragma once

#include "Calc.h"

namespace Util
{
	inline float Rerange(float angle)
	{
		angle = fmodf(angle, Calc::m2PI);
		
		if (angle < 0.0f)
			angle += Calc::m2PI;
		
		return angle;
	}
	
	bool AngleBetween(float angle, float baseAngle, float arc);
}
