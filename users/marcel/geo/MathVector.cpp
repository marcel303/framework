#include <math.h>
#include "MathVector.h"

Vector::Vector()
{

	v[0] = v[1] = v[2] = 0.0;
	v[3] = 1.0;

}

Vector::Vector(float v1, float v2, float v3, float v4)
{

	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	v[3] = v4;

}

Vector::Vector(const Vector& vector)
{

	v[0] = vector[0];
	v[1] = vector[1];
	v[2] = vector[2];
	v[3] = vector[3];

}

Vector::~Vector()
{

}

void Vector::Set(float v1, float v2, float v3, float v4)
{

	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	v[3] = v4;

}

float Vector::GetSize() const
{

	return sqrt(GetSize2());

}

float Vector::GetSize2() const
{

	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

}

void Vector::Normalize()
{

	const float size = GetSize();

	v[0] /= size;
	v[1] /= size;
	v[2] /= size;

}

void Vector::Copy(Vector vector)
{

	this->v[0] = vector[0];
	this->v[1] = vector[1];
	this->v[2] = vector[2];
	this->v[3] = vector[3];

}

Vector Vector::operator-() const
{

	Vector temp;
	
	temp[0] = -v[0];
	temp[1] = -v[1];
	temp[2] = -v[2];

	return temp;
	
}

Vector Vector::operator+(Vector vector) const
{

	Vector temp;

	temp[0] = v[0] + vector[0];
	temp[1] = v[1] + vector[1];
	temp[2] = v[2] + vector[2];

	return temp;

}

Vector Vector::operator-(Vector vector) const
{

	Vector temp;

	temp[0] = v[0] - vector[0];
	temp[1] = v[1] - vector[1];
	temp[2] = v[2] - vector[2];

	return temp;

}

Vector Vector::operator%(Vector vector) const
{

	Vector temp = *this;

	temp %= vector;

	return temp;

}

Vector Vector::operator^(Vector vector) const
{

	Vector temp;

	temp[0] = v[0] * vector[0];
	temp[1] = v[1] * vector[1];
	temp[2] = v[2] * vector[2];

	return temp;

}

Vector Vector::operator/(Vector vector) const
{

	Vector temp = *this;

	temp[0] = v[0] / vector[0];
	temp[1] = v[1] / vector[1];
	temp[2] = v[2] / vector[2];

	return temp;

}

void Vector::operator+=(Vector vector)
{

	v[0] += vector[0];
	v[1] += vector[1];
	v[2] += vector[2];

}

void Vector::operator-=(Vector vector)
{

	v[0] -= vector[0];
	v[1] -= vector[1];
	v[2] -= vector[2];

}

void Vector::operator%=(Vector vector)
{

	// 3x3 matrix. Calculate cross product using sarrus' rule:
	
	//   ax      ay      az
	//   bx      by      bz
	// (1,0,0) (0,1,0) (0,0,1)
	
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
	
	temp[0] = v[1] * vector[2] - v[2] * vector[1];
	temp[1] = v[2] * vector[0] - v[0] * vector[2];
	temp[2] = v[0] * vector[1] - v[1] * vector[0];
	
	v[0] = temp[0];
	v[1] = temp[1];
	v[2] = temp[2];

}

void Vector::operator^=(Vector vector)
{

	v[0] *= vector[0];
	v[1] *= vector[1];
	v[2] *= vector[2];

}

void Vector::operator/=(Vector vector)
{

	v[0] /= vector[0];
	v[1] /= vector[1];
	v[2] /= vector[2];

}

float Vector::operator*(Vector vector) const
{

	return v[0] * vector[0] + v[1] * vector[1] + v[2] * vector[2];

}

Vector Vector::operator*(float v) const
{

	Vector temp;

	temp[0] = this->v[0] * v;
	temp[1] = this->v[1] * v;
	temp[2] = this->v[2] * v;

	return temp;

}

Vector Vector::operator/(float v) const
{

	Vector temp;

	temp[0] = this->v[0] / v;
	temp[1] = this->v[1] / v;
	temp[2] = this->v[2] / v;

	return temp;

}

void Vector::operator*=(float v)
{

	this->v[0] *= v;
	this->v[1] *= v;
	this->v[2] *= v;

}

void Vector::operator/=(float v)
{

	this->v[0] /= v;
	this->v[1] /= v;
	this->v[2] /= v;

}

void Vector::operator=(const Vector& vector)
{

	v[0] = vector[0];
	v[1] = vector[1];
	v[2] = vector[2];
	v[3] = vector[3];

}

float& Vector::operator[](int index)
{

	return v[index];

}

float Vector::operator[](int index) const
{

	return v[index];

}

Vector::operator float*()
{

	return v;

}