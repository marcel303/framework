#include "Types.h"
#include "Vec3.h"

class TiltReader
{
public:
	TiltReader();
	
	Vec3 GravityVector_get() const;
	Vec2F DirectionXY_get() const;
	
	void* mReader;
};
