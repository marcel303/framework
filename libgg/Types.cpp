#include "Types.h"

static inline float IntersectLines(Vec2F p1, const Vec2F& d1, Vec2F p2, const Vec2F& d2)
{
	const Vec2F planeNormal(-d2[1], +d2[0]);
	const float planeDistance = planeNormal * p2;
	
	const float d = planeNormal * p1 - planeDistance;
	const float dd = planeNormal * d1;
	
	if (dd == 0.0f)
		return -1.0f;
	
	// d + dd * t = 0 -> t = (0 - d) / dd
	
	const float t = -d / dd;
	
	if (t < 0.0f)
		return -1.0f;
	else
		return t;
}

Vec2F Intersect_Rect(const RectF& rect, const Vec2F& pos, const Vec2F& dir, float& out_T)
{
	Vec2F result;
	
	const Vec2F lines[4][2] =
	{
		{
			Vec2F(rect.m_Position),
			Vec2F(1.0f, 0.0f)
		},
		{
			Vec2F(rect.m_Position) + Vec2F(rect.m_Size.x, 0.0f),
			Vec2F(0.0f, 1.0f)
		},
		{
			Vec2F(rect.m_Position) + Vec2F(rect.m_Size.x, rect.m_Size.y),
			Vec2F(-1.0f, 0.0f)
		},
		{
			Vec2F(rect.m_Position) + Vec2F(0.0f, rect.m_Size.y),
			Vec2F(0.0f, -1.0f)
		}
	};
	
	float bestT = -1.0f;
	
	for (int i = 0; i < 4; ++i)
	{
		const Vec2F& pos2 = lines[i][0];
		const Vec2F& dir2 = lines[i][1];
		
		float t = IntersectLines(pos, dir, pos2, dir2);
		
		if (t >= 0.0f && (t < bestT || bestT < 0.0f))
		{
			bestT = t;
		}
	}
	
	if (bestT < 0.0f)
		bestT = 0.0f;
	
	result = pos + dir * bestT;
	
	out_T = bestT;
	
	return result;
}
