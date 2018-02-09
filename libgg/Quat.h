#pragma once

#include "Vec3.h"
#include "Vec4.h"

class Mat4x4;
class Quat;

class Quat
{
public:
	Quat();
	Quat(const Quat & quat);
	Quat(float x, float y, float z, float w);

	float calcSize() const;
	void normalize();
	Quat calcConjugate() const;
	void fromAxisAngle(Vec3 axis, float angle);
	void fromMatrix(const Mat4x4 & matrix);
	Mat4x4 toMatrix() const;
	void toMatrix3x3(Mat4x4 & matrix) const; // fill only the 3x3 rotation part
	void toAxisAngle(Vec3 & out_axis, float & out_angle) const;
	float calcDot(const Quat & quat) const;
	void makeIdentity();
	Quat calcInverse() const;
	
	Quat calcExp() const;
	Quat calcLog() const;

	Quat slerp(const Quat & quat, float t) const;
	Quat nlerp(const Quat & quat, float t) const;

	Quat operator-() const;
	Quat& operator=(const Quat & quat);

	Quat operator*(const Quat & quat) const;
	Quat operator-(const Quat & quat) const;
	Quat operator+(const Quat & quat) const;
	Quat operator/(float v) const;
	Quat operator*(float v) const;

	Quat& operator*=(const Quat & quat);
	Quat& operator-=(const Quat & quat);
	Quat& operator+=(const Quat & quat);
	Quat& operator/=(float v);
	Quat& operator*=(float v);

	operator Mat4x4() const;
	float &     operator[](int index);
	const float operator[](int index) const;

	Vec3 m_xyz;
	float m_w;
};
