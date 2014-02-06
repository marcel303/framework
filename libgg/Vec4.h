#pragma once

#include <cmath>
#include "Vec2.h"
#include "Vec3.h"

class Vec4
{
public:
	inline Vec4()
	{
		m_v[0] = m_v[1] = m_v[2] = m_v[3] = 0.0f;
	}

	inline Vec4(float x, float y, float z, float w)
	{
		m_v[0] = x;
		m_v[1] = y;
		m_v[2] = z;
		m_v[3] = w;
	}

	inline Vec4(const Vec3 & xyz, float w)
	{
		m_v[0] = xyz[0];
		m_v[1] = xyz[1];
		m_v[2] = xyz[2];
		m_v[3] = w;
	}

	inline float CalcSize() const
	{
		const float sq =
			m_v[0] * m_v[0] +
			m_v[1] * m_v[1] +
			m_v[2] * m_v[2] +
			m_v[3] * m_v[3];
		
		return std::sqrt(sq);
	}

	inline float CalcSizeSq() const
	{
		const float sq =
			m_v[0] * m_v[0] +
			m_v[1] * m_v[1] +
			m_v[2] * m_v[2] +
			m_v[3] * m_v[3];
		
		return sq;
	}

	inline Vec4 CalcNormalized() const
	{
		const float size = CalcSize();

		if (size == 0.0f)
		{
			return Vec4(1.0f, 0.0f, 0.0f, 0.0f);
		}
		else
		{
			Vec4 r;

			r[0] = m_v[0] / size;
			r[1] = m_v[1] / size;
			r[2] = m_v[2] / size;
			r[3] = m_v[3] / size;

			return r;
		}
	}

	inline void Normalize()
	{
		*this = CalcNormalized();
	}

	inline const Vec3 XYZ() const
	{
		Vec3 r;

		r[0] = m_v[0];
		r[1] = m_v[1];
		r[2] = m_v[2];

		return r;
	}

	inline Vec4 operator+(const Vec4 & v) const
	{
		Vec4 r;

		r[0] = m_v[0] + v[0];
		r[1] = m_v[1] + v[1];
		r[2] = m_v[2] + v[2];
		r[3] = m_v[3] + v[3];

		return r;
	}

	inline Vec4 operator-(const Vec4 & v) const
	{
		Vec4 r;

		r[0] = m_v[0] - v[0];
		r[1] = m_v[1] - v[1];
		r[2] = m_v[2] - v[2];
		r[3] = m_v[3] - v[3];

		return r;
	}

	inline float operator*(const Vec4 & v) const
	{
		return
			m_v[0] * v[0] +
			m_v[1] * v[1] +
			m_v[2] * v[2] +
			m_v[3] * v[3];
	}

	inline Vec4 operator*(float s) const
	{
		Vec4 r;

		r[0] = m_v[0] * s;
		r[1] = m_v[1] * s;
		r[2] = m_v[2] * s;
		r[3] = m_v[3] * s;

		return r;
	}

	inline Vec4 operator/(float s) const
	{
		Vec4 r;

		r[0] = m_v[0] / s;
		r[1] = m_v[1] / s;
		r[2] = m_v[2] / s;
		r[3] = m_v[3] / s;

		return r;
	}

	inline void operator+=(const Vec4 & v)
	{
		m_v[0] += v.m_v[0];
		m_v[1] += v.m_v[1];
		m_v[2] += v.m_v[2];
		m_v[3] += v.m_v[3];
	}

	inline void operator-=(const Vec4 & v)
	{
		m_v[0] -= v.m_v[0];
		m_v[1] -= v.m_v[1];
		m_v[2] -= v.m_v[2];
		m_v[3] -= v.m_v[3];
	}

	inline void operator*=(float v)
	{
		m_v[0] *= v;
		m_v[1] *= v;
		m_v[2] *= v;
		m_v[3] *= v;
	}

	inline void operator/=(float v)
	{
		m_v[0] /= v;
		m_v[1] /= v;
		m_v[2] /= v;
		m_v[3] /= v;
	}

	inline Vec4 operator-() const
	{
		Vec4 r;

		r[0] = -m_v[0];
		r[1] = -m_v[1];
		r[2] = -m_v[2];
		r[3] = -m_v[3];

		return r;
	}

	inline Vec4 & operator=(const Vec2 & v)
	{
		m_v[0] = v[0];
		m_v[1] = v[1];
		m_v[2] = 0.0f;
		m_v[3] = 0.0f;

		return *this;
	}

	inline Vec4 & operator=(const Vec3 & v)
	{
		m_v[0] = v[0];
		m_v[1] = v[1];
		m_v[2] = v[2];
		m_v[3] = 0.0f;

		return *this;
	}

	inline float & operator[](int index)
	{
		return m_v[index];
	}

	inline const float & operator[](int index) const
	{
		return m_v[index];
	}

private:
	float m_v[4];
};
