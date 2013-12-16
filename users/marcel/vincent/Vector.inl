#pragma once

#include <math.h>
#include "Vector.h"

inline Vector::Vector()
{
	v[0] = v[1] = v[2] = 0.0f;
	v[3] = 1.0f;
}

inline Vector::Vector(float v1, float v2, float v3, float v4)
{
	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	v[3] = v4;
}

inline Vector::Vector(const Vector& vector)
{
	v[0] = vector[0];
	v[1] = vector[1];
	v[2] = vector[2];
	v[3] = vector[3];
}

inline Vector::~Vector()
{
}

inline void Vector::Set(float v1, float v2, float v3, float v4)
{
	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	v[3] = v4;
}

inline void Vector::Copy(const Vector& vector)
{
	v[0] = vector[0];
	v[1] = vector[1];
	v[2] = vector[2];
	v[3] = vector[3];
}

inline float Vector::GetSize() const
{
	return sqrtf(GetSquaredSize());
}

inline float Vector::GetSquaredSize() const
{
	// Return squared size.
	
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

inline void Vector::Normalize()
{
	const float size_1 = 1.0f / GetSize();

	v[0] *= size_1;
	v[1] *= size_1;
	v[2] *= size_1;
}

inline Vector Vector::operator-() const
{
	Vector temp;
	
	temp[0] = -v[0];
	temp[1] = -v[1];
	temp[2] = -v[2];

	return temp;
}

inline Vector Vector::operator+(const Vector& vector) const
{
	Vector temp;

	temp[0] = v[0] + vector[0];
	temp[1] = v[1] + vector[1];
	temp[2] = v[2] + vector[2];

	return temp;
}

inline Vector Vector::operator-(const Vector& vector) const
{
	Vector temp;

	temp[0] = v[0] - vector[0];
	temp[1] = v[1] - vector[1];
	temp[2] = v[2] - vector[2];

	return temp;
}

inline Vector Vector::operator%(const Vector& vector) const
{
	// Calculate cross product on a temp variable and return it.

	Vector temp = *this;

	temp %= vector;

	return temp;
}

inline Vector Vector::operator^(const Vector& vector) const
{
	Vector temp;

	temp[0] = v[0] * vector[0];
	temp[1] = v[1] * vector[1];
	temp[2] = v[2] * vector[2];

	return temp;
}

inline Vector Vector::operator/(const Vector& vector) const
{
	Vector temp = *this;

	temp[0] = v[0] / vector[0];
	temp[1] = v[1] / vector[1];
	temp[2] = v[2] / vector[2];

	return temp;
}

inline void Vector::operator+=(const Vector& vector)
{
	v[0] += vector[0];
	v[1] += vector[1];
	v[2] += vector[2];
}

inline void Vector::operator-=(const Vector& vector)
{
	v[0] -= vector[0];
	v[1] -= vector[1];
	v[2] -= vector[2];
}

inline void Vector::operator%=(const Vector& vector)
{
	// 3x3 matrix. Calculate cross product using sarrus' rule:
	
	// |  ax      ay      az     |
	// |  bx      by      bz     |
	// | (1,0,0) (0,1,0) (0,0,1) |
	
	// ->
	
	// ax * by * "z" +
	// ay * bz * "x" +
	// az * bx * "y" -
	// az * by * "x" -
	// ax * bz * "y" -
	// ay * bx * "z"
	
	// ->
	
	// ay * bz - az * by = x
	// az * bx - ax * bz = y
	// ax * by - ay * bx = z
	
	float temp[3];
	
	// Cross.

	temp[0] = v[1] * vector[2] - v[2] * vector[1];
	temp[1] = v[2] * vector[0] - v[0] * vector[2];
	temp[2] = v[0] * vector[1] - v[1] * vector[0];
	
	v[0] = temp[0];
	v[1] = temp[1];
	v[2] = temp[2];
}

inline void Vector::operator^=(const Vector& vector)
{
	v[0] *= vector[0];
	v[1] *= vector[1];
	v[2] *= vector[2];
}

inline void Vector::operator/=(const Vector& vector)
{
	v[0] /= vector[0];
	v[1] /= vector[1];
	v[2] /= vector[2];
}

inline float Vector::operator*(const Vector& vector) const
{
	return v[0] * vector[0] + v[1] * vector[1] + v[2] * vector[2];
}

inline Vector Vector::operator*(float _v) const
{
	Vector temp;

	temp[0] = this->v[0] * _v;
	temp[1] = this->v[1] * _v;
	temp[2] = this->v[2] * _v;
	temp[3] = this->v[3];

	return temp;
}

inline Vector Vector::operator/(float _v) const
{
	Vector temp;

	temp[0] = this->v[0] / _v;
	temp[1] = this->v[1] / _v;
	temp[2] = this->v[2] / _v;
	temp[3] = this->v[3];

	return temp;
}

inline void Vector::operator*=(float _v)
{
	this->v[0] *= _v;
	this->v[1] *= _v;
	this->v[2] *= _v;
}

inline void Vector::operator/=(float _v)
{
	this->v[0] /= _v;
	this->v[1] /= _v;
	this->v[2] /= _v;
}

const inline Vector& Vector::operator=(const Vector& vector)
{
	v[0] = vector[0];
	v[1] = vector[1];
	v[2] = vector[2];
	v[3] = vector[3];

	return *this;
}

inline bool Vector::operator==(const Vector& vector) const
{
	return (vector - (*this)).GetSquaredSize() == 0;
}

inline bool Vector::operator<(const Vector& vector) const
{
	for (int i = 0; i < 4; ++i)
		if (values[i] < vector.values[i])
			return true;
		else if (vector.values[i] < values[i])
			return false;

	return false;
}

inline float& Vector::operator[](size_t index)
{
	return v[index];
}

inline float& Vector::operator[](int index)
{
	return v[index];
}

const inline float& Vector::operator[](size_t index) const
{
	return v[index];
}

const inline float& Vector::operator[](int index) const
{
	return v[index];
}

inline Vector::operator float*()
{
	return v;
}
