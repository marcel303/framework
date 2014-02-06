#pragma once

#include <cmath>

class Vec2
{
public:
	inline Vec2()
	{
		m_v[0] = m_v[1] = 0.0f;
	}

	inline Vec2(float x, float y)
	{
		m_v[0] = x;
		m_v[1] = y;
	}

	inline float CalcSize() const
	{
		const float sq =
			m_v[0] * m_v[0] +
			m_v[1] * m_v[1];
		
		return std::sqrt(sq);
	}

	inline float CalcSizeSq() const
	{
		const float sq =
			m_v[0] * m_v[0] +
			m_v[1] * m_v[1];
		
		return sq;
	}

	inline Vec2 CalcNormalized() const
	{
		const float size = CalcSize();

		if (size == 0.0f)
		{
			return Vec2(1.0f, 0.0f);
		}
		else
		{
			Vec2 r;

			r[0] = m_v[0] / size;
			r[1] = m_v[1] / size;
				
			return r;
		}
	}

	inline void Normalize()
	{
		*this = CalcNormalized();
	}

	inline Vec2 operator+(const Vec2 & v) const
	{
		Vec2 r;

		r[0] = m_v[0] + v[0];
		r[1] = m_v[1] + v[1];

		return r;
	}

	inline Vec2 operator-(const Vec2 & v) const
	{
		Vec2 r;

		r[0] = m_v[0] - v[0];
		r[1] = m_v[1] - v[1];

		return r;
	}

	inline float operator*(Vec2 v) const
	{
		return
			m_v[0] * v[0] +
			m_v[1] * v[1];
	}

	inline Vec2 operator*(float s) const
	{
		Vec2 r;

		r[0] = m_v[0] * s;
		r[1] = m_v[1] * s;

		return r;
	}

	inline Vec2 operator/(float s) const
	{
		Vec2 r;

		r[0] = m_v[0] / s;
		r[1] = m_v[1] / s;

		return r;
	}

	inline void operator+=(const Vec2 & v)
	{
		m_v[0] += v.m_v[0];
		m_v[1] += v.m_v[1];
	}

	inline void operator-=(const Vec2 & v)
	{
		m_v[0] -= v.m_v[0];
		m_v[1] -= v.m_v[1];
	}

	inline void operator*=(float v)
	{
		m_v[0] *= v;
		m_v[1] *= v;
	}

	inline void operator/=(float v)
	{
		m_v[0] /= v;
		m_v[1] /= v;
	}

	inline Vec2 operator-() const
	{
		Vec2 r;

		r[0] = -m_v[0];
		r[1] = -m_v[1];

		return r;
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
	float m_v[2];
};
