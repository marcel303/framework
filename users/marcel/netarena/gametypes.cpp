#include "Debugging.h"
#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"

void CollisionShape::translate(float x, float y)
{
	for (int i = 0; i < numPoints; ++i)
	{
		points[i][0] += x;
		points[i][1] += y;
	}
}

float CollisionShape::projectedMax(Vec2Arg n) const
{
	Assert(numPoints != 0);

	float result = n * points[0];

	for (int i = 1; i < numPoints; ++i)
	{
		const float dot = n * points[i];

		if (dot < result)
			result = dot;
	}

	return result;
}

Vec2 CollisionShape::getSegmentNormal(int idx) const
{
	// todo : these could be precomputed

	const Vec2 & p1 = points[(idx + 0) % numPoints];
	const Vec2 & p2 = points[(idx + 1) % numPoints];

	const Vec2 d = (p2 - p1).CalcNormalized();
	const Vec2 n(+d[1], -d[0]);

	return n;
}

bool CollisionShape::checkCollision(const CollisionShape & other, Vec2Arg delta, float & contactDistance, Vec2 & contactNormal) const
{
	float minDistance = -FLT_MAX;
	Vec2 minNormal;
	bool flip = false;
	bool any = false;

	// perform SAT test

	// evaluate edges from the first shape

	for (int i = 0; i < numPoints; ++i)
	{
		const Vec2 & p = points[i];
		const Vec2 pn = getSegmentNormal(i);
		const float pd = pn * p;
		const float d = other.projectedMax(pn) - pd;

		if (d >= 0.f)
			return false;
		else
		{
			// is this a normal in the direction we're checking for?

			if (pn * delta == 0.f)
				continue;

			any = true;

			if (d > minDistance)
			{
				minDistance = d;
				minNormal = pn;
			}
		}
	}

	// evaluate edges from the second shape

	for (int i = 0; i < other.numPoints; ++i)
	{
		const Vec2 & p = other.points[i];
		const Vec2 pn = other.getSegmentNormal(i);
		const float pd = pn * p;
		const float d = projectedMax(pn) - pd;

		if (d >= 0.f)
			return false;
		else
		{
			// is this a normal in the direction we're checking for?

			if (pn * delta == 0.f)
				continue;

			any = true;

			if (d > minDistance)
			{
				minDistance = d;
				minNormal = -pn;

				flip = true;
			}
		}
	}

	if (flip && false)
	{
		minDistance = -minDistance;
		minNormal = -minNormal;
	}

	contactDistance = minDistance;
	contactNormal = minNormal;

	return any;
}

void CollisionShape::debugDraw() const
{
	for (int i = 0; i < numPoints; ++i)
	{
		const Vec2 p1 = points[(i + 0) % numPoints];
		const Vec2 p2 = points[(i + 1) % numPoints];
		drawLine(p1[0], p1[1], p2[0], p2[1]);

		const Vec2 m = (p1 + p2) / 2.f;
		const Vec2 n = getSegmentNormal(i);
		drawLine(m[0], m[1], m[0] + n[0] * BLOCK_SX/2.f, m[1] + n[1] * BLOCK_SX/2.f);
	}
}
