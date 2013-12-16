#pragma once

#include "Types.h"

/*class Vec2F
{
public:
	Vec2F()
	{
		v[0] = v[1] = 0.0f;
	}

	Vec2F(float _x, float _y) :
		x(_x),
		y(_y)
	{
	}

	float DistanceTo(const Vec2F& v) const;
	Vec2F DirectionTo(const Vec2F& v) const;

	static Vec2F FromAngle(float angle);
	float ToAngle() const;

	union
	{
		struct
		{
			float x;
			float y;
		};
		float v[2];
	};
};*/

float Random(float min, float max);
