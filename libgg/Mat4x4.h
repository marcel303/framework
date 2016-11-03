#pragma once

#include <cmath>
#include "Vec3.h"
#include "Vec4.h"

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

class Mat4x4
{
public:
	inline Mat4x4()
	{
	}
	

	inline Mat4x4(const bool initializeToIdentity)
	{
		if (initializeToIdentity)
			MakeIdentity();
	}

	inline Mat4x4(
		float v00, float v10, float v20, float v30,
		float v01, float v11, float v21, float v31,
		float v02, float v12, float v22, float v32,
		float v03, float v13, float v23, float v33)
	{
		m00 = v00; m10 = v10; m20 = v20; m30 = v30;
		m01 = v01; m11 = v11; m21 = v21; m31 = v31;
		m02 = v02; m12 = v12; m22 = v22; m32 = v32;
		m03 = v03; m13 = v13; m23 = v23; m33 = v33;
	}
	
	inline void MakeIdentity()
	{
		m00 = 1.0f; m10 = 0.0f; m20 = 0.0f; m30 = 0.0f;
		m01 = 0.0f; m11 = 1.0f; m21 = 0.0f; m31 = 0.0f;
		m02 = 0.0f; m12 = 0.0f; m22 = 1.0f; m32 = 0.0f;
		m03 = 0.0f; m13 = 0.0f; m23 = 0.0f; m33 = 1.0f;
	}
	
	inline void MakeScaling(float x, float y, float z)
	{
		MakeScaling(Vec3(x, y, z));
	}
	
	inline void MakeScaling(const Vec3 & scale)
	{
		MakeIdentity();
		
		m_v[INDEX(0, 0)] = scale[0];
		m_v[INDEX(1, 1)] = scale[1];
		m_v[INDEX(2, 2)] = scale[2];
	}
	
	inline void MakeRotationX(float angle, bool left = true)
	{
		MakeIdentity();
		
		if (left)
		{
			m_v[INDEX(1, 1)] = +std::cosf(angle);
			m_v[INDEX(2, 1)] = +std::sinf(angle);
			m_v[INDEX(1, 2)] = -std::sinf(angle);
			m_v[INDEX(2, 2)] = +std::cosf(angle);
		}
		else
		{
			m_v[INDEX(1, 1)] = +std::cosf(angle);
			m_v[INDEX(2, 1)] = -std::sinf(angle);
			m_v[INDEX(1, 2)] = +std::sinf(angle);
			m_v[INDEX(2, 2)] = +std::cosf(angle);
		}
	}
	
	inline void MakeRotationY(float angle, bool left = true)
	{
		MakeIdentity();
		
		if (left)
		{
			m_v[INDEX(0, 0)] = +std::cosf(angle);
			m_v[INDEX(2, 0)] = -std::sinf(angle);
			m_v[INDEX(0, 2)] = +std::sinf(angle);
			m_v[INDEX(2, 2)] = +std::cosf(angle);
		}
		else
		{
			m_v[INDEX(0, 0)] = +std::cosf(angle);
			m_v[INDEX(2, 0)] = +std::sinf(angle);
			m_v[INDEX(0, 2)] = -std::sinf(angle);
			m_v[INDEX(2, 2)] = +std::cosf(angle);
		}
	}
	
	inline void MakeRotationZ(float angle, bool left = true)
	{
		MakeIdentity();
		
		if (left)
		{
			m_v[INDEX(0, 0)] = +std::cosf(angle);
			m_v[INDEX(1, 0)] = +std::sinf(angle);
			m_v[INDEX(0, 1)] = -std::sinf(angle);
			m_v[INDEX(1, 1)] = +std::cosf(angle);
		}
		else
		{
			m_v[INDEX(0, 0)] = +std::cosf(angle);
			m_v[INDEX(1, 0)] = -std::sinf(angle);
			m_v[INDEX(0, 1)] = +std::sinf(angle);
			m_v[INDEX(1, 1)] = +std::cosf(angle);
		}
	}
	
	inline void MakeTranslation(float x, float y, float z)
	{
		MakeTranslation(Vec3(x, y, z));
	}
	
	inline void MakeTranslation(const Vec3 & position)
	{
		MakeIdentity();
		
		m_v[INDEX(3, 0)] = position[0];
		m_v[INDEX(3, 1)] = position[1];
		m_v[INDEX(3, 2)] = position[2];
	}
	
	void MakePerspectiveLH(float fov, float aspect, float nearCP, float farCP)
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
	
	void MakePerspectiveGL(float fov, float aspect, float nearCP, float farCP)
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
	
	void MakeOrthoLH(float left, float right, float top, float bottom, float nearCP, float farCP)
	{
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
	
	void MakeLookat(const Vec3& position, const Vec3& target, const Vec3& up)
	{
		Mat4x4 orient;
		
		orient.MakeIdentity();
		
		Vec3 axisZ = target - position;
		
		if (axisZ.CalcSizeSq() == 0.0f)
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
	
	void SetTranslation(float x, float y, float z)
	{
		SetTranslation(Vec3(x, y, z));
	}
	
	void SetTranslation(const Vec3 & v)
	{
		m_v[INDEX(3, 0)] = v[0];
		m_v[INDEX(3, 1)] = v[1];
		m_v[INDEX(3, 2)] = v[2];
	}
	
	Vec3 GetTranslation() const
	{
		return Vec3(
			m_v[INDEX(3, 0)],
			m_v[INDEX(3, 1)],
			m_v[INDEX(3, 2)]);
	}
	
	inline Mat4x4 CalcTranspose() const
	{
		Mat4x4 r;
		
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				r(i, j) = (*this)(j, i);
		
		return r;
	}
	
	float CalcDet() const
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
	
	Mat4x4 CalcInv() const
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
	
	inline Vec3 Mul(const Vec3 & vec) const
	{
		Vec3 r;
		
		for (int i = 0; i < 3; ++i)
		{
			r[i] =
				m_v[INDEX(0, i)] * vec[0] +
				m_v[INDEX(1, i)] * vec[1] +
				m_v[INDEX(2, i)] * vec[2];
		}
		
		return r;
	}
	
	inline Vec3 Mul3(const Vec3 & vec) const
	{
		return Mul(vec);
	}
	
	inline Vec3 Mul4(const Vec3 & vec) const
	{
		Vec3 r;
		
		for (int i = 0; i < 3; ++i)
		{
			r[i] =
				m_v[INDEX(0, i)] * vec[0] +
				m_v[INDEX(1, i)] * vec[1] +
				m_v[INDEX(2, i)] * vec[2] +
				m_v[INDEX(3, i)];
		}
		
		return r;
	}
	
	inline Vec4 Mul(const Vec4 & vec) const
	{
		Vec4 r;
		
		for (int i = 0; i < 4; ++i)
		{
			r[i] =
				m_v[INDEX(0, i)] * vec[0] +
				m_v[INDEX(1, i)] * vec[1] +
				m_v[INDEX(2, i)] * vec[2] +
				m_v[INDEX(3, i)] * vec[3];
		}
		
		return r;
	}
	
	inline Mat4x4 Mul(const Mat4x4 & mat) const
	{
		Mat4x4 r;
		
		for (int x = 0; x < 4; ++x)
		{
			for (int y = 0; y < 4; ++y)
			{
				float v = 0.0f;
				
				for (int i = 0; i < 4; ++i)
					v += m_v[INDEX(i, y)] * mat.m_v[INDEX(x, i)];
				
				r.m_v[INDEX(x, y)] = v;
			}
		}
		
		return r;
	}
	
	inline Mat4x4 operator*(const float v) const
	{
		Mat4x4 result;

		for (int i = 0; i < 16; ++i)
			result.m_v[i] = m_v[i] * v;

		return result;
	}

	inline Vec3 operator*(const Vec3 & vec) const
	{
		return Mul4(vec);
	}
	
	inline Vec4 operator*(const Vec4 & vec) const
	{
		return Mul(vec);
	}
	
	inline Mat4x4 operator*(const Mat4x4 & mat) const
	{
		return Mul(mat);
	}

	inline Mat4x4 operator+(const Mat4x4 & mat) const
	{
		Mat4x4 result;

		for (int i = 0; i < 16; ++i)
			result.m_v[i] = m_v[i] + mat.m_v[i];

		return result;
	}

	inline void operator*=(const float v)
	{
		for (int i = 0; i < 16; ++i)
			m_v[i] *= v;
	}
	
	inline void operator*=(const Mat4x4 & mat)
	{
		(*this) = Mul(mat);
	}
	
	inline float & operator()(int x, int y)
	{
		return m_v[INDEX(x, y)];
	}
	
	inline const float & operator()(int x, int y) const
	{
		return m_v[INDEX(x, y)];
	}
	
	//

	inline Mat4x4 Translate(const float x, const float y, const float z) const
	{
		Mat4x4 t;
		t.MakeTranslation(x, y, z);
		return (*this) * t;
	}

	inline Mat4x4 Scale(const float x, const float y, const float z) const
	{
		Mat4x4 t;
		t.MakeScaling(x, y, z);
		return (*this) * t;
	}

	inline Mat4x4 RotateZ(const float angle, const bool left = true) const
	{
		Mat4x4 t;
		t.MakeRotationZ(angle, left);
		return (*this) * t;
	}

	inline Mat4x4 Invert() const
	{
		return CalcInv();
	}

	float m_v[16];
};

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
