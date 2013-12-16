#include "CR_Fire.h"

namespace CR_Fire
{
	inline double MakeDiscrete(double density, double steps)
	{
		double dps = 1.0 / steps;
		
		if (density <= dps)
			return 0.0;
		
		return ((int)(density / dps) + 1) * dps;
	}
	
	Pixel ColorRamp(double density)
	{
		density = sqrt(density);
		density = MakeDiscrete(density, 5.0);
//		density = sin(density * 1.0);
		
		double r = density * 4.0;
		double g = density * 0.5;
		double b = density * 0.2;
		double a = density;
		
//		if (density >= 0.2)
//			r = 1.0;
		
		return Pixel(r, g, b, a);
	}
}
