#pragma once

#include <math.h>

class Vec3
{
public:
	inline Vec3()
	{
	}
	
	inline Vec3(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
	
	inline float LengthSq_get() const
	{
		return x * x + y * y + z * z;
	}
	
	inline float Length_get() const
	{
		return sqrtf(LengthSq_get());
	}
	
	inline void Normalize()
	{
		float lengthRcp = 1.0f / Length_get();
		
		x *= lengthRcp;
		y *= lengthRcp;
		z *= lengthRcp;
	}
	
	inline float& operator[](int index)
	{
		return v[index];
	}
	
	inline Vec3 operator%(const Vec3& v) const
	{
		return Vec3(
			y * v.y - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x);
	}
	
	inline Vec3 operator+(const Vec3& v) const
	{
		return Vec3(
			x + v.x,
			y + v.y,
			z + v.z);
	}
	
	inline Vec3 operator-(const Vec3& v) const
	{
		return Vec3(
			x - v.x,
			y - v.y,
			z - v.z);
	}
	
	inline Vec3 operator*(float v) const
	{
		return Vec3(
			x * v,
			y * v,
			z * v);
	}
	
	inline void operator*=(float v)
	{
		x *= v;
		y *= v;
		z *= v;
	}
	
	union
	{
		struct
		{
			float x;
			float y;
			float z;
		};
		float v[3];
	};
};