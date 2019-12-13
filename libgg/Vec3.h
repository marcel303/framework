#pragma once

#if defined(__clang__)
	// use builtin for sqrtf
#else
	#include <math.h>
#endif

class Vec3
{
public:
	inline Vec3()
	{
		m_v[0] = m_v[1] = m_v[2] = 0.0f;
	}

	inline Vec3(float v1, float v2, float v3)
	{
		m_v[0] = v1;
		m_v[1] = v2;
		m_v[2] = v3;
	}
	
	inline void Set(float x, float y, float z)
	{
		m_v[0] = x;
		m_v[1] = y;
		m_v[2] = z;
	}
	
	inline void SetZero()
	{
		m_v[0] = m_v[1] = m_v[2] = 0.0f;
	}

	inline float CalcSize() const
	{
		const float sq =
			m_v[0] * m_v[0] +
			m_v[1] * m_v[1] +
			m_v[2] * m_v[2];
		
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
			m_v[1] * m_v[1] +
			m_v[2] * m_v[2];
		
		return sq;
	}

	inline Vec3 CalcNormalized() const
	{
		const float size = CalcSize();

		if (size == 0.0f)
		{
			return Vec3(1.0f, 0.0f, 0.0f);
		}
		else
		{
			Vec3 r;

			r[0] = m_v[0] / size;
			r[1] = m_v[1] / size;
			r[2] = m_v[2] / size;

			return r;
		}
	}

	inline void Normalize()
	{
		*this = CalcNormalized();
	}
	
	inline Vec3 Min(const Vec3 & other)
	{
		return Vec3(
			m_v[0] < other.m_v[0] ? m_v[0] : other.m_v[0],
			m_v[1] < other.m_v[1] ? m_v[1] : other.m_v[1],
			m_v[2] < other.m_v[2] ? m_v[2] : other.m_v[2]);
	}
	
	inline Vec3 Max(const Vec3 & other)
	{
		return Vec3(
			m_v[0] > other.m_v[0] ? m_v[0] : other.m_v[0],
			m_v[1] > other.m_v[1] ? m_v[1] : other.m_v[1],
			m_v[2] > other.m_v[2] ? m_v[2] : other.m_v[2]);
	}
	
	inline Vec3 Mul(const Vec3 & v) const
	{
		Vec3 r;

		r[0] = m_v[0] * v[0];
		r[1] = m_v[1] * v[1];
		r[2] = m_v[2] * v[2];
		
		return r;
	}
	
	inline Vec3 Div(const Vec3 & v) const
	{
		Vec3 r;

		r[0] = m_v[0] / v[0];
		r[1] = m_v[1] / v[1];
		r[2] = m_v[2] / v[2];
		
		return r;
	}

	inline Vec3 operator+(const Vec3 & v) const
	{
		Vec3 r;

		r[0] = m_v[0] + v[0];
		r[1] = m_v[1] + v[1];
		r[2] = m_v[2] + v[2];

		return r;
	}

	inline Vec3 operator-(const Vec3 & v) const
	{
		Vec3 r;

		r[0] = m_v[0] - v[0];
		r[1] = m_v[1] - v[1];
		r[2] = m_v[2] - v[2];

		return r;
	}

	inline float operator*(const Vec3 & v) const
	{
		return
			m_v[0] * v[0] +
			m_v[1] * v[1] +
			m_v[2] * v[2];
	}

	inline Vec3 operator%(const Vec3 & v) const
	{
		Vec3 r;

		r[0] =
			m_v[1] * v[2] -
			m_v[2] * v[1];
		r[1] =
			m_v[2] * v[0] -
			m_v[0] * v[2];
		r[2] =
			m_v[0] * v[1] -
			m_v[1] * v[0];

		return r;
	}

	inline Vec3 operator*(float s) const
	{
		Vec3 r;

		r[0] = m_v[0] * s;
		r[1] = m_v[1] * s;
		r[2] = m_v[2] * s;

		return r;
	}

	inline Vec3 operator/(float s) const
	{
		Vec3 r;

		r[0] = m_v[0] / s;
		r[1] = m_v[1] / s;
		r[2] = m_v[2] / s;

		return r;
	}

	inline void operator+=(const Vec3 & v)
	{
		m_v[0] += v.m_v[0];
		m_v[1] += v.m_v[1];
		m_v[2] += v.m_v[2];
	}

	inline void operator-=(const Vec3 & v)
	{
		m_v[0] -= v.m_v[0];
		m_v[1] -= v.m_v[1];
		m_v[2] -= v.m_v[2];
	}

	inline void operator*=(float v)
	{
		m_v[0] *= v;
		m_v[1] *= v;
		m_v[2] *= v;
	}

	inline void operator/=(float v)
	{
		m_v[0] /= v;
		m_v[1] /= v;
		m_v[2] /= v;
	}

	inline Vec3 operator-() const
	{
		Vec3 r;

		r[0] = -m_v[0];
		r[1] = -m_v[1];
		r[2] = -m_v[2];

		return r;
	}
	
	inline bool operator==(const Vec3 & other) const
	{
		return
			m_v[0] == other.m_v[0] &&
			m_v[1] == other.m_v[1] &&
			m_v[2] == other.m_v[2];
	}
	
	inline bool operator!=(const Vec3 & other) const
	{
		return
			m_v[0] != other.m_v[0] ||
			m_v[1] != other.m_v[1] ||
			m_v[2] != other.m_v[2];
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
	float m_v[3];
};

typedef const Vec3 & Vec3Arg;
