#include <math.h>
#include <stdlib.h>
#include "types2.h"

/*float Vec2F::DistanceTo(const Vec2F& _v) const
{
	float dx = _v.x - x;
	float dy = _v.y - y;

	return sqrtf(dx * dx + dy * dy);
}

Vec2F Vec2F::DirectionTo(const Vec2F& _v) const
{
	float dx = _v.x - x;
	float dy = _v.y - y;

	float distance = sqrtf(dx * dx + dy * dy);

	if (distance == 0.0f)
		return Vec2F();

	return Vec2F(dx / distance, dy / distance);
}

Vec2F Vec2F::FromAngle(float angle)
{
	Vec2F result;
	result.x = cosf(angle);
	result.y = sinf(angle);
	return result;
}

float Vec2F::ToAngle() const
{
	return atan2f(y, x);
}
*/

float Random(float min, float max)
{
	float t = (rand() & 4095) / 4095.0f;

	return min + (max - min) * t;
}
