#pragma once

#include <exception>
#include <math.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "Exception.h"
#include "Types.h"

// --------------------

class Triangle
{
public:
	PointF m_Points[3];
};

// --------------------

class Poly
{
public:
	std::vector<PointI> m_Points;
};

// --------------------

class Circle
{
public:
	Circle()
	{
		x = 0;
		y = 0;
		r = 0;
	}
	
	inline float CalcDistance(const PointF& point) const
	{
		float dx = point.x - x;
		float dy = point.y - y;

		float d = sqrtf(dx * dx + dy * dy) - r;

		return fabsf(d);
	}

	int x;
	int y;
	int r;
};

// --------------------

class CurvePoint
{
public:
	int x;
	int y;
	int tx1;
	int ty1;
	int tx2;
	int ty2;
};

class Curve
{
public:
	Curve()
	{
		m_IsClosed = false;
	}

	std::vector<CurvePoint> m_Points;
	bool m_IsClosed;
};

// --------------------

class Tag
{
public:
	int x;
	int y;
};

// --------------------

class HullLine
{
public:
	float Distance(const PointF& point) const
	{
#if 0
		if (m_Plane1.Distance(point) < 0.0f)
			return m_Point1.Distance(point);
		if (m_Plane2.Distance(point) < 0.0f)
			return m_Point2.Distance(point);
#endif

		return fabsf(m_Plane.Distance(point));
	}

	float DistanceSq(const PointF& point) const
	{
#if 1
		if (m_Plane1.Distance(point) < 0.0f)
			return m_Point1.DistanceSq(point);
		if (m_Plane2.Distance(point) < 0.0f)
			return m_Point2.DistanceSq(point);
#endif

		float d = fabsf(m_Plane.Distance(point));

		return d * d;
	}

	PointF m_Point1;
	PointF m_Point2;
	PlaneF m_Plane1;
	PlaneF m_Plane2;
	PlaneF m_Plane;
};

// --------------------

class Hull
{
public:
	void Construct(const std::vector<PointI>& points)
	{
		for (size_t i = 0; i < points.size(); ++i)
		{
			size_t index1 = (i + 0) % points.size();
			size_t index2 = (i + 1) % points.size();

			PointF p1;
			PointF p2;

			p1.x = (float)points[index1].x;
			p1.y = (float)points[index1].y;
			p2.x = (float)points[index2].x;
			p2.y = (float)points[index2].y;

			float dx = p2.x - p1.x;
			float dy = p2.y - p1.y;

			float ds = sqrtf(dx * dx + dy * dy);

			PointF pn;
			pn.x = -dy / ds;
			pn.y = +dx / ds;
			float pd = pn.x * p1.x + pn.y * p1.y;

			HullLine line;

			line.m_Point1 = p1;
			line.m_Point2 = p2;
			line.m_Plane1 = PlaneF::FromPoints(p1, p2);
			line.m_Plane2 = PlaneF::FromPoints(p2, p1);
			line.m_Plane.m_Normal = pn;
			line.m_Plane.m_Distance = pd;

			m_Lines.push_back(line);
		}
	}

	float CalcDistance(const PointF& point) const
	{
		float dSqMin = 1000000.0f;

		for (size_t i = 0; i < m_Lines.size(); ++i)
		{
			float dSq = m_Lines[i].DistanceSq(point);

			if (dSq < dSqMin)
				dSqMin = dSq;
		}

		return sqrtf(dSqMin);
	}

	static inline float Min(float v1, float v2)
	{
		return v1 < v2 ? v1 : v2;
	}

	static inline float Max(float v1, float v2)
	{
		return v1 > v2 ? v1 : v2;
	}

	bool IsConvex() const
	{
		const float EPS = 0.01f;

		for (size_t i = 0; i < m_Lines.size(); ++i)
		{
			float min = 0.0f;
			float max = 0.0f;

			for (size_t j = 0; j < m_Lines.size(); ++j)
			{
				if (i == j)
					continue;

				min = Min(min, m_Lines[i].m_Plane.Distance(m_Lines[j].m_Point1));
				min = Min(min, m_Lines[i].m_Plane.Distance(m_Lines[j].m_Point2));
				max = Max(max, m_Lines[i].m_Plane.Distance(m_Lines[j].m_Point1));
				max = Max(max, m_Lines[i].m_Plane.Distance(m_Lines[j].m_Point2));
			}

			if (min < -EPS && max > +EPS)
				return false;
		}

		return true;
	}

	std::vector<HullLine> m_Lines;
};
