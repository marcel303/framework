#pragma once

#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

#if defined(__aarch64__)
	#include <arm_neon.h>
#endif

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

class Quat;

class Mat4x4
{
public:
	inline Mat4x4()
	{
	}
	
	explicit inline Mat4x4(const bool initializeToIdentity)
	{
		if (initializeToIdentity)
			MakeIdentity();
	}
	
	explicit inline Mat4x4(const float value)
	{
		MakeIdentity();
		
		m00 = value;
		m11 = value;
		m22 = value;
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
		MakeIdentity();

		m_v[INDEX(0, 0)] = x;
		m_v[INDEX(1, 1)] = y;
		m_v[INDEX(2, 2)] = z;
	}
	
	inline void MakeScaling(const Vec3 & scale)
	{
		MakeScaling(scale[0], scale[1], scale[2]);
	}
	
	void MakeRotationX(float angle, bool left = true);
	void MakeRotationY(float angle, bool left = true);
	void MakeRotationZ(float angle, bool left = true);
	
	inline void MakeTranslation(float x, float y, float z)
	{
		MakeIdentity();

		m_v[INDEX(3, 0)] = x;
		m_v[INDEX(3, 1)] = y;
		m_v[INDEX(3, 2)] = z;
	}
	
	inline void MakeTranslation(const Vec3 & position)
	{
		MakeTranslation(position[0], position[1], position[2]);
	}
	
	void MakePerspectiveLH(float fov, float aspect, float nearCP, float farCP);
	void MakePerspectiveGL(float fov, float aspect, float nearCP, float farCP);
	void MakeOrthoLH(float left, float right, float top, float bottom, float nearCP, float farCP);
	void MakeOrthoGL(float left, float right, float top, float bottom, float nearCP, float farCP);
	void MakeLookat(const Vec3& position, const Vec3& target, const Vec3& up);
	void MakeLookatInv(const Vec3& position, const Vec3& target, const Vec3& up);
	
	void SetTranslation(float x, float y, float z)
	{
		m_v[INDEX(3, 0)] = x;
		m_v[INDEX(3, 1)] = y;
		m_v[INDEX(3, 2)] = z;
	}
	
	void SetTranslation(const Vec3 & v)
	{
		SetTranslation(v[0], v[1], v[2]);
	}
	
	Vec3 GetTranslation() const
	{
		return Vec3(
			m_v[INDEX(3, 0)],
			m_v[INDEX(3, 1)],
			m_v[INDEX(3, 2)]);
	}
	
	Vec3 GetAxis(int index) const
	{
		return Vec3(
			m_v[INDEX(index, 0)],
			m_v[INDEX(index, 1)],
			m_v[INDEX(index, 2)]);
	}
	
	void SetAxis(int index, Vec3Arg axis)
	{
		m_v[INDEX(index, 0)] = axis[0];
		m_v[INDEX(index, 1)] = axis[1];
		m_v[INDEX(index, 2)] = axis[2];
	}
	
	const Vec4 & GetColumn(int index) const
	{
		return (Vec4&)m_v[INDEX(index, 0)];
	}
	
	inline Mat4x4 CalcTranspose() const
	{
		Mat4x4 r;
		
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				r(i, j) = (*this)(j, i);
		
		return r;
	}
	
	float CalcDet() const;
	Mat4x4 CalcInv() const;
	
	inline Vec2 Mul(const Vec2 & vec) const
	{
		Vec2 r;

		for (int i = 0; i < 2; ++i)
		{
			r[i] =
				m_v[INDEX(0, i)] * vec[0] +
				m_v[INDEX(1, i)] * vec[1];
		}

		return r;
	}

	inline Vec2 Mul4(const Vec2 & vec) const
	{
		Vec2 r;

		for (int i = 0; i < 2; ++i)
		{
			r[i] =
				m_v[INDEX(0, i)] * vec[0] +
				m_v[INDEX(1, i)] * vec[1] +
				m_v[INDEX(3, i)];
		}

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
	
#if defined(__aarch64__)
	inline Mat4x4 Mul(const Mat4x4 & mat) const
	{
		Mat4x4 result;
		
		const float * A = m_v;
		const float * B = mat.m_v;
		
		float * __restrict C = result.m_v;
		
		// these are the columns of A
		float32x4_t A0 = vld1q_f32(A);
		float32x4_t A1 = vld1q_f32(A+4);
		float32x4_t A2 = vld1q_f32(A+8);
		float32x4_t A3 = vld1q_f32(A+12);
		
		// Multiply accumulate in 4x1 blocks, i.e. each column in C
		float32x4_t B0 = vld1q_f32(B);
		float32x4_t C0 = vmovq_n_f32(0);
		C0 = vfmaq_laneq_f32(C0, A0, B0, 0);
		C0 = vfmaq_laneq_f32(C0, A1, B0, 1);
		C0 = vfmaq_laneq_f32(C0, A2, B0, 2);
		C0 = vfmaq_laneq_f32(C0, A3, B0, 3);
		vst1q_f32(C, C0);

		float32x4_t B1 = vld1q_f32(B+4);
		float32x4_t C1 = vmovq_n_f32(0);
		C1 = vfmaq_laneq_f32(C1, A0, B1, 0);
		C1 = vfmaq_laneq_f32(C1, A1, B1, 1);
		C1 = vfmaq_laneq_f32(C1, A2, B1, 2);
		C1 = vfmaq_laneq_f32(C1, A3, B1, 3);
		vst1q_f32(C+4, C1);

		float32x4_t B2 = vld1q_f32(B+8);
		float32x4_t C2 = vmovq_n_f32(0);
		C2 = vfmaq_laneq_f32(C2, A0, B2, 0);
		C2 = vfmaq_laneq_f32(C2, A1, B2, 1);
		C2 = vfmaq_laneq_f32(C2, A2, B2, 2);
		C2 = vfmaq_laneq_f32(C2, A3, B2, 3);
		vst1q_f32(C+8, C2);

		float32x4_t B3 = vld1q_f32(B+12);
		float32x4_t C3 = vmovq_n_f32(0);
		C3 = vfmaq_laneq_f32(C3, A0, B3, 0);
		C3 = vfmaq_laneq_f32(C3, A1, B3, 1);
		C3 = vfmaq_laneq_f32(C3, A2, B3, 2);
		C3 = vfmaq_laneq_f32(C3, A3, B3, 3);
		vst1q_f32(C+12, C3);
		
		return result;
	}
#else
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
#endif
	
	inline Mat4x4 operator*(const float v) const
	{
		Mat4x4 result;

		for (int i = 0; i < 16; ++i)
			result.m_v[i] = m_v[i] * v;

		return result;
	}

	inline Vec2 operator*(const Vec2 & vec) const
	{
		return Mul4(vec);
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

	inline Mat4x4 Translate(Vec3Arg vec) const
	{
		return Translate(vec[0], vec[1], vec[2]);
	}

	inline Mat4x4 Scale(const float scale) const
	{
		Mat4x4 t;
		t.MakeScaling(scale, scale, scale);
		return (*this) * t;
	}
	
	inline Mat4x4 Scale(const float x, const float y, const float z) const
	{
		Mat4x4 t;
		t.MakeScaling(x, y, z);
		return (*this) * t;
	}
	
	inline Mat4x4 Scale(Vec3Arg vec) const
	{
		return Scale(vec[0], vec[1], vec[2]);
	}

	inline Mat4x4 RotateX(const float angle, const bool left = true) const
	{
		Mat4x4 t;
		t.MakeRotationX(angle, left);
		return (*this) * t;
	}

	inline Mat4x4 RotateY(const float angle, const bool left = true) const
	{
		Mat4x4 t;
		t.MakeRotationY(angle, left);
		return (*this) * t;
	}

	inline Mat4x4 RotateZ(const float angle, const bool left = true) const
	{
		Mat4x4 t;
		t.MakeRotationZ(angle, left);
		return (*this) * t;
	}
	
	Mat4x4 Rotate(const Quat & q) const;
	Mat4x4 Rotate(const float angle, Vec3Arg axis) const;

	Mat4x4 Lookat(Vec3Arg position, Vec3Arg target, Vec3Arg up) const;
	
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
