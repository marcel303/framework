#include "calibration.h"
#include <math.h>

void drawCalibrationImage(LaserPoint * points, const int numPoints)
{
	const LaserPoint corners[4] =
	{
		{ -1.f, -1.f, 1.f, 1.f, 1.f },
		{ +1.f, -1.f, 1.f, 1.f, 1.f },
		{ +1.f, +1.f, 1.f, 1.f, 1.f },
		{ -1.f, +1.f, 1.f, 1.f, 1.f },
	};
	
	for (int i = 0; i < numPoints; ++i)
	{
		const float c = 4.f * i / float(numPoints);
		
		const int c1 = (int)floorf(c);
		const int c2 = (c1 + 1) % 4;
		
		const float t = c - c1;
		
		corners[c1].interpTo(corners[c2], t, points[i]);
	}
}
