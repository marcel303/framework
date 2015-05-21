#include "Debugging.h"
#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"

const char * g_gameStateNames[kGameState_COUNT] =
{
	"mainMenus",
	"connecting",
	"onlineMenus",
	"newGame",
	"roundComplete"
};

const char * g_playerSpecialNames[kPlayerSpecial_COUNT] =
{
	"none",
	"RocketPunch",
	"DoubleSidedMelee",
	"StompAttack",
	"Shield",
	"Invisibility",
	"Jetpack",
	"Zweihander",
	"Axe",
	"Pipebomb",
	"Grapple"
};

Curve defaultCurve(0.f, 1.f);
Curve pipebombBlastCurve;
Curve grenadeBlastCurve;
Curve jetpackAnalogCurveX;
Curve jetpackAnalogCurveY;
Curve gravityWellFalloffCurve;

//

const CollisionShape & CollisionShape::operator=(const CollisionBox & box)
{
	Vec2 p1(box.min[0], box.min[1]);
	Vec2 p2(box.max[0], box.min[1]);
	Vec2 p3(box.max[0], box.max[1]);
	Vec2 p4(box.min[0], box.max[1]);

	set(p1, p2, p3, p4);

	return *this;
}

void CollisionShape::fixup()
{
	Assert(numPoints > 0);
	Vec2 center;
	for (int i = 0; i < numPoints; ++i)
		center += points[i];
	center /= numPoints;
	const Vec2 n = getSegmentNormal(0);
	const float d1 = n * points[0];
	const float d2 = n * center - d1;
	if (d2 > 0.f)
	{
		for (int i = 0; i < numPoints/2; ++i)
			std::swap(points[i], points[numPoints - 1 - i]);
	}
}

void CollisionShape::translate(float x, float y)
{
	for (int i = 0; i < numPoints; ++i)
	{
		points[i][0] += x;
		points[i][1] += y;
	}
}

void CollisionShape::getMinMax(Vec2 & min, Vec2 & max) const
{
	Assert(numPoints != 0);

	min = points[0];
	max = points[0];

	for (int i = 1; i < numPoints; ++i)
	{
		min = min.Min(points[i]);
		max = max.Max(points[i]);
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

bool CollisionShape::intersects(const CollisionShape & other) const
{
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
	}

	return true;
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

void CollisionShape::debugDraw(bool drawNormals) const
{
	for (int i = 0; i < numPoints; ++i)
	{
		const Vec2 p1 = points[(i + 0) % numPoints];
		const Vec2 p2 = points[(i + 1) % numPoints];
		drawLine(p1[0], p1[1], p2[0], p2[1]);

		if (drawNormals)
		{
			const Vec2 m = (p1 + p2) / 2.f;
			const Vec2 n = getSegmentNormal(i);
			drawLine(m[0], m[1], m[0] + n[0] * BLOCK_SX/2.f, m[1] + n[1] * BLOCK_SX/2.f);
		}
	}
}

//

Curve::Curve()
{
	memset(this, 0, sizeof(Curve));
}

Curve::Curve(float min, float max)
{
	makeLinear(min, max);
}

void Curve::makeLinear(float v1, float v2)
{
	for (int i = 0; i < 32; ++i)
	{
		const float u = i / 31.f;
		const float v = 1.f - u;
		value[i] = v1 * v + v2 * u;
	}
}

float Curve::eval(float t) const
{
	t = t < 0.f ? 0.f : t > 1.f ? 1.f : t;
	const float s = t * 31.f;
	const int index1 = (int)s;
	const int index2 = (index1 + 1) & 31;
	const float u = s - index1;
	const float v = 1.f - u;
	const float v1 = value[index1];
	const float v2 = value[index2];
	return v1 * v + v2 * u;
}
