#include "calibration.h"
#include <math.h>

void drawCalibrationImage_rectangle(LaserPoint * points, const int numPoints)
{
	const LaserPoint corners[4] =
	{
		{ -1.f, -1.f, 1.f, 0.f, 0.f },
		{ +1.f, -1.f, 1.f, 1.f, 0.f },
		{ +1.f, +1.f, 1.f, 0.f, 1.f },
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

void drawCalibrationImage_rectanglePoints(LaserPoint * points, const int numPoints)
{
	const LaserPoint corners[4] =
	{
		{ -1.f, -1.f, 1.f, 0.f, 0.f },
		{ +1.f, -1.f, 1.f, 1.f, 0.f },
		{ +1.f, +1.f, 1.f, 0.f, 1.f },
		{ -1.f, +1.f, 1.f, 1.f, 1.f },
	};
	
	const int filler = numPoints * 6 / 10;
	
	for (int i = 0; i < filler; ++i)
	{
		const float t = i / float(filler - 1);
		
		points[i].x = cosf(t * float(M_PI) * 2.f) * .5f;
		points[i].y = sinf(t * float(M_PI) * 2.f) * .5f;
		
		points[i].r = 1.f;
		points[i].g = 0.f;
		points[i].b = 0.f;
	}
	
	const int remainder = numPoints - filler;
	
	for (int i = filler; i < numPoints; ++i)
	{
		const int c = 4 * (i - filler) / remainder;
		
		points[i] = corners[c];
	}
}

void drawCalibrationImage_line_vscroll(LaserPoint * points, const int numPoints, const float phase)
{
	for (int i = 0; i < numPoints; ++i)
	{
		points[i].x = -1.f + 2.f * i / float(numPoints);
		points[i].y = -1.f + 2.f * fmodf(phase, 1.f);
		points[i].r = points[i].g = points[i].b = 1.f;
	}
}

void drawCalibrationImage_line_hscroll(LaserPoint * points, const int numPoints, const float phase)
{
	for (int i = 0; i < numPoints; ++i)
	{
		points[i].x = -1.f + 2.f * fmodf(phase, 1.f);
		points[i].y = -1.f + 2.f * i / float(numPoints);
		points[i].r = points[i].g = points[i].b = 1.f;
	}
}
