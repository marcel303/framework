#pragma once

#include <math.h>
#include "Debugging.h"
#include "SIMD.h"

ALIGN_CLASS(16) SimdMat4x4
{
public:
	inline SimdMat4x4()
	{
	}

	inline SimdMat4x4(
		SimdVecArg row0,
		SimdVecArg row1,
		SimdVecArg row2,
		SimdVecArg row3)
	{
		m_rows[0] = row0;
		m_rows[1] = row1;
		m_rows[2] = row2;
		m_rows[3] = row3;
	}

	inline SimdMat4x4(
		float v00, float v10, float v20, float v30,
		float v01, float v11, float v21, float v31,
		float v02, float v12, float v22, float v32,
		float v03, float v13, float v23, float v33)
	{
		m_rows[0].Set4(v00, v10, v20, v30);
		m_rows[1].Set4(v01, v11, v21, v31);
		m_rows[2].Set4(v02, v12, v22, v32);
		m_rows[3].Set4(v03, v13, v23, v33);
	}

	inline void MakeIdentity()
	{
		m_rows[0].Set4(1.0f, 0.0f, 0.0f, 0.0f);
		m_rows[1].Set4(0.0f, 1.0f, 0.0f, 0.0f);
		m_rows[2].Set4(0.0f, 0.0f, 1.0f, 0.0f);
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline void MakeScaling3(SimdVecArg x, SimdVecArg y, SimdVecArg z)
	{
		m_rows[0] = SimdVec(x(0), 0.0f, 0.0f, 0.0f);
		m_rows[1] = SimdVec(0.0f, y(0), 0.0f, 0.0f);
		m_rows[2] = SimdVec(0.0f, 0.0f, z(0), 0.0f);
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline void MakeScaling3(SimdVecArg scale)
	{
		m_rows[0] = SimdVec(scale(0), 0.0f, 0.0f, 0.0f);
		m_rows[1] = SimdVec(0.0f, scale(1), 0.0f, 0.0f);
		m_rows[2] = SimdVec(0.0f, 0.0f, scale(2), 0.0f);
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline void MakeScaling3f(float x, float y, float z)
	{
		m_rows[0] = SimdVec(x, 0.0f, 0.0f, 0.0f);
		m_rows[1] = SimdVec(0.0f, y, 0.0f, 0.0f);
		m_rows[2] = SimdVec(0.0f, 0.0f, z, 0.0f);
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline void MakeRotationX(float angle)
	{
		m_rows[0].Set4(1.0f, 0.0f, 0.0f, 0.0f);
		m_rows[1].Set4(0.0f, +cosf(angle), +sinf(angle), 0.0f);
		m_rows[2].Set4(0.0f, -sinf(angle), +cosf(angle), 0.0f);
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline void MakeRotationY(float angle)
	{
		m_rows[0].Set4(+cosf(angle), 0.0f, -sinf(angle), 0.0f);
		m_rows[1].Set4(0.0f, 1.0f, 0.0f, 0.0f);
		m_rows[2].Set4(+sinf(angle), 0.0f, +cosf(angle), 0.0f);
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline void MakeRotationZ(float angle)
	{
		m_rows[0].Set4(+cosf(angle), +sinf(angle), 0.0f, 0.0f);
		m_rows[1].Set4(-sinf(angle), +cosf(angle), 0.0f, 0.0f);
		m_rows[2].Set4(0.0f, 0.0f, 1.0f, 0.0f);
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline void MakeTranslation(SimdVecArg x, SimdVecArg y, SimdVecArg z)
	{
		m_rows[0].Set4(1.0f, 0.0f, 0.0f, x(0));
		m_rows[1].Set4(0.0f, 1.0f, 0.0f, y(0));
		m_rows[2].Set4(0.0f, 0.0f, 1.0f, z(0));
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline void MakeTranslation(SimdVecArg position)
	{
		m_rows[0].Set4(1.0f, 0.0f, 0.0f, position.X());
		m_rows[1].Set4(0.0f, 1.0f, 0.0f, position.Y());
		m_rows[2].Set4(0.0f, 0.0f, 1.0f, position.Z());
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline void MakeTranslation(float x, float y, float z)
	{
		m_rows[0].Set4(1.0f, 0.0f, 0.0f, x);
		m_rows[1].Set4(0.0f, 1.0f, 0.0f, y);
		m_rows[2].Set4(0.0f, 0.0f, 1.0f, z);
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	void MakePerspectiveLH(float fov, float aspect, float nearCP, float farCP)
	{
		const float scaleY = 1.0f / tanf(fov / 2.0f);
		const float scaleX = aspect * scaleY;
		const float l_33 = farCP / ( farCP - nearCP );
		const float l_34 = -nearCP * farCP / ( farCP - nearCP );

		m_rows[0].Set4(scaleX, 0.0f, 0.0f, 0.0f);
		m_rows[1].Set4(0.0f, scaleY, 0.0f, 0.0f);
		m_rows[2].Set4(0.0f, 0.0f, l_33, 1.0f);
		m_rows[3].Set4(0.0f, 0.0f, l_34, 0.0f);
	}

	void MakeOrthoLH(float left, float right, float top, float bottom, float nearCP, float farCP)
	{
		const float rl = 2.0f / (right - left);
		const float tb = 2.0f / (top - bottom);
		const float fn = 1.0f / (farCP - nearCP);

		const float tx = (right + left) / (left - right);
		const float ty = (top + bottom) / (bottom - top);
		const float tz = nearCP / (nearCP - farCP);

		m_rows[0].Set4(rl, 0.0f, 0.0f, 0.0f);
		m_rows[1].Set4(0.0f, tb, 0.0f, 0.0f);
		m_rows[2].Set4(0.0f, 0.0f, fn, 0.0f);
		m_rows[3].Set4(tx, ty, tz, 1.0f);
	}

	void MakeLookat(SimdVecArg position, SimdVecArg target, SimdVecArg up)
	{
		SimdVec axisZ = target.Sub(position);

		if (axisZ.Len3Sq().ALL_EQ4(VEC_ZERO))
			axisZ.Set3(up(1), up(2), up(0));

		SimdVec axisX = up.Cross3(axisZ);
		SimdVec axisY = axisZ.Cross3(axisX);

		axisX = axisX.UnitVec3();
		axisY = axisY.UnitVec3();
		axisZ = axisZ.UnitVec3();

		SimdVec translation = position.Neg();

		m_rows[0] = axisX.Permute(translation, 0,1,2,4);
		m_rows[1] = axisY.Permute(translation, 0,1,2,5);
		m_rows[2] = axisZ.Permute(translation, 0,1,2,6);
		m_rows[3].Set4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline SimdMat4x4 CalcTranspose() const
	{
#if 1
		SimdMat4x4 r;
		
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				r(i, j) = (*this)(j, i);
		
		return r;
#else
		SimdMat4x4 r = *this;

		_MM_TRANSPOSE4_PS(
			r.m_rows[0].Vec128(),
			r.m_rows[1].Vec128(),
			r.m_rows[2].Vec128(),
			r.m_rows[3].Vec128());

		return r;
#endif
	}

#if 1
#define m00 m_rows[0].X()
#define m01 m_rows[0].Y()
#define m02 m_rows[0].Z()
#define m03 m_rows[0].W()
#define m10 m_rows[1].X()
#define m11 m_rows[1].Y()
#define m12 m_rows[1].Z()
#define m13 m_rows[1].W()
#define m20 m_rows[2].X()
#define m21 m_rows[2].Y()
#define m22 m_rows[2].Z()
#define m23 m_rows[2].W()
#define m30 m_rows[3].X()
#define m31 m_rows[3].Y()
#define m32 m_rows[3].Z()
#define m33 m_rows[3].W()

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

	SimdMat4x4 CalcInv() const
	{
		SimdMat4x4 r;

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

		const SimdVec det(CalcDet());

		for (int i = 0; i < 4; ++i)
			r.m_rows[i] = r.m_rows[i].Div(det);

		return r;
	}

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
#endif

	FORCEINLINE SimdVec Mul3x3(SimdVecArg vec) const
	{
		return SimdVec(
			m_rows[0].Dot3(vec),
			m_rows[1].Dot3(vec),
			m_rows[2].Dot3(vec));
	}

	FORCEINLINE SimdVec Mul3x4(SimdVecArg vec) const
	{
		SimdVec vec4 = vec.Select(VEC_ONE, 0,0,0,1);

		return SimdVec(
			m_rows[0].Dot4(vec4),
			m_rows[1].Dot4(vec4),
			m_rows[2].Dot4(vec4));
	}

	FORCEINLINE SimdVec Mul4x4(SimdVecArg vec) const
	{
		return SimdVec(
			m_rows[0].Dot4(vec),
			m_rows[1].Dot4(vec),
			m_rows[2].Dot4(vec),
			m_rows[3].Dot4(vec));
	}

	inline void Mul(const SimdMat4x4 & mat, SimdMat4x4 & rResult) const
	{
		Assert(&rResult != this);
		Assert(&rResult != &mat);

	#if 0
		for (int x = 0; x < 4; ++x)
		{
			for (int y = 0; y < 4; ++y)
			{
				float v = 0.0f;

				for (int i = 0; i < 4; ++i)
					v += (*this)(i, y) * mat(x, i);

				rResult(x, y) = v;
			}
		}
	#else
		// todo : test
		const SimdVec row1 = mat.m_rows[0];
		const SimdVec row2 = mat.m_rows[1];
		const SimdVec row3 = mat.m_rows[2];
		const SimdVec row4 = mat.m_rows[3];

		for (int i = 0; i < 4; ++i)
		{
			const SimdVec brod1 = m_rows[i].ReplicateX();
			const SimdVec brod2 = m_rows[i].ReplicateY();
			const SimdVec brod3 = m_rows[i].ReplicateZ();
			const SimdVec brod4 = m_rows[i].ReplicateW();

			rResult.m_rows[i] =
				(
					brod1.Mul(row1).Add(brod2.Mul(row2))
				).Add
				(
					brod3.Mul(row3).Add(brod4.Mul(row4))
				);
		}
	#endif
	}

	inline SimdMat4x4 operator*(const SimdMat4x4 & mat) const
	{
		SimdMat4x4 temp;

		Mul(mat, temp);

		return temp;
	}

	inline void operator*=(const SimdMat4x4 & mat)
	{
		SimdMat4x4 temp;

		Mul(mat, temp);

		*this = temp;
	}

	FORCEINLINE float & operator()(int x, int y)
	{
			return m_rows[y](x);
	}

	FORCEINLINE const float operator()(int x, int y) const
	{
		return m_rows[y](x);
	}

	SimdVec m_rows[4];
};
