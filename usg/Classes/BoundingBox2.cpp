#include "BoundingBox2.h"

BoundingBox2::BoundingBox2()
{
	IsEmpty = XTRUE;
}

BoundingBox2::BoundingBox2(const Vec2F& min, const Vec2F& max)
{
	IsEmpty = XFALSE;

	Min = min;
	Max = max;
}

void BoundingBox2::Merge(const Vec2F& p)
{
	if (IsEmpty)
	{
		IsEmpty = XFALSE;

		Min = p;
		Max = p;
	}
	else
	{
		for (int i = 0; i < 2; ++i)
		{
			if (p[i] < Min[i])
				Min[i] = p[i];
			if (p[i] > Max[i])
				Max[i] = p[i];
		}
	}
}

void BoundingBox2::Merge(const BoundingBox2& bb)
{
	Merge(bb.Min);
	Merge(bb.Max);
}

XBOOL BoundingBox2::Inside(const Vec2F& p) const
{
	for (int i = 0; i < 2; ++i)
	{
		if (p[i] < Min[i])
			return XFALSE;
		if (p[i] > Max[i])
			return XFALSE;
	}

	return XTRUE;
}
