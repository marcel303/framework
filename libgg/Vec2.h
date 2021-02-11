#pragma once

#if defined(__clang__)
	// use builtin for sqrtf
#else
	#include <math.h>
#endif

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
	
	explicit
	inline Vec2(float v)
	{
		m_v[0] = m_v[1] = v;
	}

	inline void Set(float x, float y)
	{
		m_v[0] = x;
		m_v[1] = y;
	}

	inline void SetZero()
	{
		m_v[0] = m_v[1] = 0.f;
	}

	inline float CalcSize() const
	{
		const float sq =
			m_v[0] * m_v[0] +
			m_v[1] * m_v[1];
		
	#if defined(__clang__)
		return __builtin_sqrtf(sq);
	#else
		return sqrtf(sq);
	#endif
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

	inline Vec2 Min(const Vec2 & v) const
	{
		return Vec2(
			m_v[0] < v[0] ? m_v[0] : v[0],
			m_v[1] < v[1] ? m_v[1] : v[1]);
	}

	inline Vec2 Max(const Vec2 & v) const
	{
		return Vec2(
			m_v[0] > v[0] ? m_v[0] : v[0],
			m_v[1] > v[1] ? m_v[1] : v[1]);
	}

	inline Vec2 Mul(const Vec2 & v) const
	{
		Vec2 r;

		r[0] = m_v[0] * v[0];
		r[1] = m_v[1] * v[1];
		
		return r;
	}
	
	inline Vec2 Div(const Vec2 & v) const
	{
		Vec2 r;

		r[0] = m_v[0] / v[0];
		r[1] = m_v[1] / v[1];
		
		return r;
	}

	inline Vec2 Abs() const
	{
		Vec2 r;

	#if defined(__clang__)
		r[0] = __builtin_fabsf(m_v[0]);
		r[1] = __builtin_fabsf(m_v[1]);
	#else
		r[0] = fabsf(m_v[0]);
		r[1] = fabsf(m_v[1]);
	#endif

		return r;
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

	inline Vec2 operator^(Vec2 v) const
	{
		return Vec2(
			m_v[0] * v[0],
			m_v[1] * v[1]);
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

	inline bool operator==(const Vec2 & other) const
	{
		return
			m_v[0] == other.m_v[0] &&
			m_v[1] == other.m_v[1];
	}

	inline bool operator!=(const Vec2 & other) const
	{
		return
			m_v[0] != other.m_v[0] ||
			m_v[1] != other.m_v[1];
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

typedef const Vec2 & Vec2Arg;
