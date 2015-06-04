#include "Debugging.h"
#include "Util_Angle.h"

namespace Util
{
	bool AngleBetween(float angle, float baseAngle, float arc)
	{
		Assert(arc >= 0.0f);
		
		Rerange(angle);
		Rerange(baseAngle);
		
		float angle1 = baseAngle;
		float angle2 = baseAngle + arc;
		
		if (angle < angle1)
			angle += Calc::m2PI;
		
		return angle >= angle1 && angle <= angle2;
	}
}
