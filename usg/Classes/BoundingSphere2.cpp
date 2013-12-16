#include "BoundingSphere2.h"

BoundingSphere2::BoundingSphere2()
{
	Radius = 0.0f;
}

BoundingSphere2::BoundingSphere2(const Vec2F& pos, float radius)
{
	Pos = pos;
	Radius = radius;
}

XBOOL BoundingSphere2::HitTest(const Vec2F& pos) const
{
	const Vec2F delta = pos - Pos;

	const float lengthSq = delta.LengthSq_get();

	if (lengthSq > Radius * Radius)
		return XFALSE;

	return XTRUE;
}

XBOOL BoundingSphere2::HitTest(const BoundingSphere2& sphere) const
{
	Vec2F delta = sphere.Pos - Pos;
	
	const float lengthSq = delta.LengthSq_get();
	
	const float radius = Radius + sphere.Radius;
	
	return lengthSq <= radius * radius;
}

XBOOL BoundingSphere2::Intersect_LineSegment(const Vec2F& pos, const Vec2F& delta, float& out_T) const
{
	const Vec2F delta2 = Pos - pos;
	
	const float t = (delta2 * delta) / (delta * delta);

	if (t < 0.0f || t > 1.0f)
		return XFALSE;
	
	Vec2F delta3(delta[1], -delta[0]);
	
	delta3.Normalize();
	
	if (fabsf(delta3 * Pos - delta3 * pos) > Radius)
		return XFALSE;
	
	out_T = t;
	
	return XTRUE;
}
