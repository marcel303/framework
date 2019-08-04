#include "edreamUtils.h"
#include "etherdream.h"
#include "laserTypes.h"

static uint16_t unormFloatToU16(float in_v)
{
	if (in_v < 0.f)
		in_v = 0.f;
	else if (in_v > 1.f)
		in_v = 1.f;
	
	return uint16_t(in_v * ((1 << 16) - 1));
}

static uint16_t snormFloatToS16(float in_v)
{
	if (in_v < -1.f)
		in_v = -1.f;
	else if (in_v > +1.f)
		in_v = +1.f;
	
	return int16_t(in_v * ((1 << 15) - 1));
}

void convertLaserImageToEtherdream(
	const LaserPoint * points,
	const int numPoints,
	etherdream_point * out_points)
{
	for (int i = 0; i < numPoints; ++i)
	{
		auto & src = points[i];
		auto & dst = out_points[i];
		
		dst.x = snormFloatToS16(+src.x);
		dst.y = snormFloatToS16(-src.y);
		
		dst.r = unormFloatToU16(src.r);
		dst.g = unormFloatToU16(src.g);
		dst.b = unormFloatToU16(src.b);
		
		dst.i = uint16_t(-1);
		dst.u1 = 0; // unused
		dst.u2 = 0;
	}
}
