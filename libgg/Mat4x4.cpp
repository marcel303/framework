#include "Mat4x4.h"
#include "Quat.h"
#include <math.h>

#define INDEX(x, y) ((x) * 4 + (y))

#define m00 m_v[INDEX(0, 0)]
#define m10 m_v[INDEX(1, 0)]
#define m20 m_v[INDEX(2, 0)]
#define m30 m_v[INDEX(3, 0)]
#define m01 m_v[INDEX(0, 1)]
#define m11 m_v[INDEX(1, 1)]
#define m21 m_v[INDEX(2, 1)]
#define m31 m_v[INDEX(3, 1)]
#define m02 m_v[INDEX(0, 2)]
#define m12 m_v[INDEX(1, 2)]
#define m22 m_v[INDEX(2, 2)]
#define m32 m_v[INDEX(3, 2)]
#define m03 m_v[INDEX(0, 3)]
#define m13 m_v[INDEX(1, 3)]
#define m23 m_v[INDEX(2, 3)]
#define m33 m_v[INDEX(3, 3)]

void Mat4x4::MakeRotationX(float angle, bool left)
{
	MakeIdentity();
	
	if (left)
	{
		m_v[INDEX(1, 1)] = +cosf(angle);
		m_v[INDEX(2, 1)] = +sinf(angle);
		m_v[INDEX(1, 2)] = -sinf(angle);
		m_v[INDEX(2, 2)] = +cosf(angle);
	}
	else
	{
		m_v[INDEX(1, 1)] = +cosf(angle);
		m_v[INDEX(2, 1)] = -sinf(angle);
		m_v[INDEX(1, 2)] = +sinf(angle);
		m_v[INDEX(2, 2)] = +cosf(angle);
	}
}

void Mat4x4::MakeRotationY(float angle, bool left)
{
	MakeIdentity();
	
	if (left)
	{
		m_v[INDEX(0, 0)] = +cosf(angle);
		m_v[INDEX(2, 0)] = -sinf(angle);
		m_v[INDEX(0, 2)] = +sinf(angle);
		m_v[INDEX(2, 2)] = +cosf(angle);
	}
	else
	{
		m_v[INDEX(0, 0)] = +cosf(angle);
		m_v[INDEX(2, 0)] = +sinf(angle);
		m_v[INDEX(0, 2)] = -sinf(angle);
		m_v[INDEX(2, 2)] = +cosf(angle);
	}
}

void Mat4x4::MakeRotationZ(float angle, bool left)
{
	MakeIdentity();
	
	if (left)
	{
		m_v[INDEX(0, 0)] = +cosf(angle);
		m_v[INDEX(1, 0)] = +sinf(angle);
		m_v[INDEX(0, 1)] = -sinf(angle);
		m_v[INDEX(1, 1)] = +cosf(angle);
	}
	else
	{
		m_v[INDEX(0, 0)] = +cosf(angle);
		m_v[INDEX(1, 0)] = -sinf(angle);
		m_v[INDEX(0, 1)] = +sinf(angle);
		m_v[INDEX(1, 1)] = +cosf(angle);
	}
}

void Mat4x4::MakePerspectiveLH(float fov, float aspect, float nearCP, float farCP)
{
	// left handed perspective matrix with clip space Z = (0, +1)
	
	const float scaleY = 1.0f / tanf(fov / 2.0f);
	const float scaleX = aspect * scaleY;
	const float l_33 = farCP / ( farCP - nearCP );
	const float l_43 = -nearCP * farCP / ( farCP - nearCP );
	
	m00 = scaleX; m01 = 0.0f;   m02 = 0.0f; m03 = 0.0f;
	m10 = 0.0f;   m11 = scaleY; m12 = 0.0f; m13 = 0.0f;
	m20 = 0.0f;   m21 = 0.0f;   m22 = l_33; m23 =+1.0f;
	m30 = 0.0f;   m31 = 0.0f;   m32 = l_43; m33 = 0.0f;
}

void Mat4x4::MakePerspectiveGL(float fov, float aspect, float nearCP, float farCP)
{
	// right handed perspective matrix with clip space Z = (-1, +1)
	
	const float scaleY = 1.0f / tanf(fov / 2.0f);
	const float scaleX = aspect * scaleY;
	const float l_33 = - (farCP + nearCP) / (nearCP - farCP);
	const float l_43 = 2.f * (farCP * nearCP) / (nearCP - farCP);
	
	m00 = scaleX; m01 = 0.0f;   m02 = 0.0f; m03 = 0.0f;
	m10 = 0.0f;   m11 = scaleY; m12 = 0.0f; m13 = 0.0f;
	m20 = 0.0f;   m21 = 0.0f;   m22 = l_33; m23 =+1.0f;
	m30 = 0.0f;   m31 = 0.0f;   m32 = l_43; m33 = 0.0f;
}

void Mat4x4::MakeOrthoLH(float left, float right, float top, float bottom, float nearCP, float farCP)
{
	// left handed orthographic matrix with clip space Z = (0, +1)
	
	const float rl = 2.0f / (right - left);
	const float tb = 2.0f / (top - bottom);
	const float fn = 1.0f / (farCP - nearCP);
	
	const float tx = (right + left) / (left - right);
	const float ty = (top + bottom) / (bottom - top);
	const float tz = nearCP / (nearCP - farCP);
	
	m00 = rl;   m01 = 0.0f; m02 = 0.0f; m03 = 0.0f;
	m10 = 0.0f; m11 = tb;   m12 = 0.0f; m13 = 0.0f;
	m20 = 0.0f; m21 = 0.0f; m22 = fn;   m23 = 0.0f;
	m30 = tx;   m31 = ty;   m32 = tz;   m33 = 1.0f;
}

void Mat4x4::MakeOrthoGL(float left, float right, float top, float bottom, float nearCP, float farCP)
{
	// right handed orthographic matrix with clip space Z = (-1, +1)
	
	const float rl = 2.0f / (right - left);
	const float tb = 2.0f / (top - bottom);
	const float fn = 2.0f / (farCP - nearCP);
	
	const float tx = (right + left) / (left - right);
	const float ty = (top + bottom) / (bottom - top);
	const float tz = (farCP + nearCP) / (nearCP - farCP);
	
	m00 = rl;   m01 = 0.0f; m02 = 0.0f; m03 = 0.0f;
	m10 = 0.0f; m11 = tb;   m12 = 0.0f; m13 = 0.0f;
	m20 = 0.0f; m21 = 0.0f; m22 = fn;   m23 = 0.0f;
	m30 = tx;   m31 = ty;   m32 = tz;   m33 = 1.0f;
}

void Mat4x4::MakeLookat(const Vec3& position, const Vec3& target, const Vec3& up)
{
	Mat4x4 orient;
	
	orient.MakeIdentity();
	
	Vec3 axisZ = target - position;
	
	if (axisZ.CalcSizeSq() == 0.0f || (axisZ % up).CalcSizeSq() == 0.0f)
		axisZ = Vec3(up[1], up[2], up[0]);
	
	Vec3 axisX = up % axisZ;
	Vec3 axisY = axisZ % axisX;
	
	axisX = axisX.CalcNormalized();
	axisY = axisY.CalcNormalized();
	axisZ = axisZ.CalcNormalized();
	
	for (int i = 0; i < 3; ++i)
	{
		orient(i, 0) = axisX[i];
		orient(i, 1) = axisY[i];
		orient(i, 2) = axisZ[i];
	}
	
	MakeTranslation(-position);
	
	*this = orient * (*this);
}

void Mat4x4::MakeLookatInv(const Vec3& position, const Vec3& target, const Vec3& up)
{
	Vec3 axisZ = target - position;
	
	if (axisZ.CalcSizeSq() == 0.0f || (axisZ % up).CalcSizeSq() == 0.0f)
		axisZ = Vec3(up[1], up[2], up[0]);
	
	Vec3 axisX = up % axisZ;
	Vec3 axisY = axisZ % axisX;
	
	axisX = axisX.CalcNormalized();
	axisY = axisY.CalcNormalized();
	axisZ = axisZ.CalcNormalized();
	
	for (int i = 0; i < 3; ++i)
	{
		m_v[INDEX(0, i)] = axisX[i];
		m_v[INDEX(1, i)] = axisY[i];
		m_v[INDEX(2, i)] = axisZ[i];
	}
	
	m_v[INDEX(0, 3)] = 0.f;
	m_v[INDEX(1, 3)] = 0.f;
	m_v[INDEX(2, 3)] = 0.f;
	
	m_v[INDEX(3, 0)] = position[0];
	m_v[INDEX(3, 1)] = position[1];
	m_v[INDEX(3, 2)] = position[2];
	m_v[INDEX(3, 3)] = 1.f;
}

float Mat4x4::CalcDet() const
{
	float r;
	
	r =
		(m03 * m12 * m21 * m30) - (m02 * m13 * m21 * m30) - (m03 * m11 * m22 * m30) + (m01 * m13 * m22 * m30) +
		(m02 * m11 * m23 * m30) - (m01 * m12 * m23 * m30) - (m03 * m12 * m20 * m31) + (m02 * m13 * m20 * m31) +
		(m03 * m10 * m22 * m31) - (m00 * m13 * m22 * m31) - (m02 * m10 * m23 * m31) + (m00 * m12 * m23 * m31) +
		(m03 * m11 * m20 * m32) - (m01 * m13 * m20 * m32) - (m03 * m10 * m21 * m32) + (m00 * m13 * m21 * m32) +
		(m01 * m10 * m23 * m32) - (m00 * m11 * m23 * m32) - (m02 * m11 * m20 * m33) + (m01 * m12 * m20 * m33) +
		(m02 * m10 * m21 * m33) - (m00 * m12 * m21 * m33) - (m01 * m10 * m22 * m33) + (m00 * m11 * m22 * m33);
	
	return r;
}

Mat4x4 Mat4x4::CalcInv() const
{
	Mat4x4 r;
	
	r.m00 = (m12 * m23 * m31) - (m13 * m22 * m31) + (m13 * m21*m32) - (m11 * m23 * m32) - (m12 * m21 * m33) + (m11 * m22 * m33);
	r.m01 = (m03 * m22 * m31) - (m02 * m23 * m31) - (m03 * m21*m32) + (m01 * m23 * m32) + (m02 * m21 * m33) - (m01 * m22 * m33);
	r.m02 = (m02 * m13 * m31) - (m03 * m12 * m31) + (m03 * m11*m32) - (m01 * m13 * m32) - (m02 * m11 * m33) + (m01 * m12 * m33);
	r.m03 = (m03 * m12 * m21) - (m02 * m13 * m21) - (m03 * m11*m22) + (m01 * m13 * m22) + (m02 * m11 * m23) - (m01 * m12 * m23);
	r.m10 = (m13 * m22 * m30) - (m12 * m23 * m30) - (m13 * m20*m32) + (m10 * m23 * m32) + (m12 * m20 * m33) - (m10 * m22 * m33);
	r.m11 = (m02 * m23 * m30) - (m03 * m22 * m30) + (m03 * m20*m32) - (m00 * m23 * m32) - (m02 * m20 * m33) + (m00 * m22 * m33);
	r.m12 = (m03 * m12 * m30) - (m02 * m13 * m30) - (m03 * m10*m32) + (m00 * m13 * m32) + (m02 * m10 * m33) - (m00 * m12 * m33);
	r.m13 = (m02 * m13 * m20) - (m03 * m12 * m20) + (m03 * m10*m22) - (m00 * m13 * m22) - (m02 * m10 * m23) + (m00 * m12 * m23);
	r.m20 = (m11 * m23 * m30) - (m13 * m21 * m30) + (m13 * m20*m31) - (m10 * m23 * m31) - (m11 * m20 * m33) + (m10 * m21 * m33);
	r.m21 = (m03 * m21 * m30) - (m01 * m23 * m30) - (m03 * m20*m31) + (m00 * m23 * m31) + (m01 * m20 * m33) - (m00 * m21 * m33);
	r.m22 = (m01 * m13 * m30) - (m03 * m11 * m30) + (m03 * m10*m31) - (m00 * m13 * m31) - (m01 * m10 * m33) + (m00 * m11 * m33);
	r.m23 = (m03 * m11 * m20) - (m01 * m13 * m20) - (m03 * m10*m21) + (m00 * m13 * m21) + (m01 * m10 * m23) - (m00 * m11 * m23);
	r.m30 = (m12 * m21 * m30) - (m11 * m22 * m30) - (m12 * m20*m31) + (m10 * m22 * m31) + (m11 * m20 * m32) - (m10 * m21 * m32);
	r.m31 = (m01 * m22 * m30) - (m02 * m21 * m30) + (m02 * m20*m31) - (m00 * m22 * m31) - (m01 * m20 * m32) + (m00 * m21 * m32);
	r.m32 = (m02 * m11 * m30) - (m01 * m12 * m30) - (m02 * m10*m31) + (m00 * m12 * m31) + (m01 * m10 * m32) - (m00 * m11 * m32);
	r.m33 = (m01 * m12 * m20) - (m02 * m11 * m20) + (m02 * m10*m21) - (m00 * m12 * m21) - (m01 * m10 * m22) + (m00 * m11 * m22);
	
	const float scale = 1.0f / CalcDet();
	
	for (int i = 0; i < 16; ++i)
		r.m_v[i] *= scale;
	
	return r;
}

Mat4x4 Mat4x4::Rotate(const Quat & q) const
{
	Mat4x4 t = q.toMatrix();
	return (*this) * t;
}

Mat4x4 Mat4x4::Rotate(const float angle, Vec3Arg axis) const
{
	Quat q;
	q.fromAngleAxis(angle, axis);
	
	return Rotate(q);
}

Mat4x4 Mat4x4::Lookat(Vec3Arg position, Vec3Arg target, Vec3Arg up) const
{
	Mat4x4 lookat;
	lookat.MakeLookatInv(position, target, up);
	return (*this) * lookat;
}

#undef INDEX

#undef m00
#undef m10
#undef m20
#undef m30
#undef m01
#undef m11
#undef m21
#undef m31
#undef m02
#undef m12
#undef m22
#undef m32
#undef m03
#undef m13
#undef m23
#undef m33
