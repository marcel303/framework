#ifndef MXPLANE_H
#define MXPLANE_H
#pragma once

#include "Vec3.h"

namespace Mx
{
	class Plane
	{
	public:
		inline Plane()
		{
			m_distance = 0.0f;
		}

		inline Plane(const Vec3& normal, float distance)
		{
			Setup(normal, distance);
		}

		inline void SetupCCW(const Vec3& p1, const Vec3& p2, const Vec3& p3)
		{
			Vec3 d1 = p2 - p1;
			Vec3 d2 = p3 - p2;

			m_normal = (d1 % d2).CalcNormalized();
			m_distance = p1 * m_normal;
		}

		inline void SetupCW(const Vec3& p1, const Vec3& p2, const Vec3& p3)
		{
			SetupCCW(p3, p2, p1);
		}

		inline void Setup(const Vec3& normal, float distance)
		{
			m_normal = normal;
			m_distance = distance;
		}

		inline float operator*(const Vec3& p) const
		{
			float d =
				p[0] * m_normal[0] +
				p[1] * m_normal[1] +
				p[2] * m_normal[2] -
				m_distance;

			return d;
		}

		inline Plane operator*(float s) const
		{
			return Plane(m_normal * s, m_distance * s);
		}

		inline Plane operator-() const
		{
			return Plane(-m_normal, -m_distance);
		}

		Vec3 m_normal;
		float m_distance;
	};
}

#endif
