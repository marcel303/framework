#pragma once

#include "TypesBase.h"
#include "Types.h"

// todo: copy from prototype (?)

class BoundingBox2
{
public:
	BoundingBox2();
	BoundingBox2(const Vec2F& min, const Vec2F& max);
	
	void Merge(const Vec2F& p);
	void Merge(const BoundingBox2& bb);
	
	XBOOL Inside(const Vec2F& p) const;

	inline Vec2F Size_get() const
	{
		return Max - Min;
	}

	XBOOL IsEmpty;
	Vec2F Min;
	Vec2F Max;
};
