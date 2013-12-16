#pragma once

#include "BoundingBox2.h"
#include "Types.h"

class BoundingSphere2
{
public:
	BoundingSphere2();
	BoundingSphere2(const Vec2F& pos, float radius);
	
	inline void Set(const Vec2F& pos, float radius)
	{
		Pos = pos;
		Radius = radius;
	}
	
	XBOOL HitTest(const Vec2F& pos) const;
	XBOOL HitTest(const BoundingSphere2& sphere) const;
	XBOOL Intersect_LineSegment(const Vec2F& pos, const Vec2F& delta, float& out_T) const;
	
	Vec2F Pos;
	float Radius;
};
