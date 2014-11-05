#ifndef AABB_H
#define AABB_H
#pragma once

#include "Mat4x4.h"
#include "Vec3.h"

class AABB
{
public:
	inline AABB()
	{
	}

	inline AABB(const Vec3& min, const Vec3& max)
	{
		Set(min, max);
	}

	void Set(const Vec3& min, const Vec3& max)
	{
		m_min = min;
		m_max = max;
	}

	// NOTE: out_points is an array of 8 vectors.
	void Transform(const Mat4x4& mat, Vec3* out_points) const
	{
		const Vec3 v[2] = { m_min, m_max };

		int index = 0;

		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				for (int k = 0; k < 2; ++k)
				{
					Vec3 temp(
						v[i][0],
						v[j][1],
						v[k][2]);

					out_points[index] = mat.Mul4(temp);

					++index;
				}
			}
		}
	}

	inline AABB operator+(const AABB& other)
	{
		AABB r = *this;

		r += other;

		return r;
	}

	inline void operator+=(const AABB& other)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (other.m_min[i] < m_min[i])
				m_min[i] = other.m_min[i];
			if (other.m_max[i] > m_max[i])
				m_max[i] = other.m_max[i];
		}
	}

	inline void operator+=(const Vec3& v)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (v[i] < m_min[i])
				m_min[i] = v[i];
			if (v[i] > m_max[i])
				m_max[i] = v[i];
		}
	}

	Vec3 m_min;
	Vec3 m_max;
};

#endif
