#pragma once

#include <emmintrin.h>
#include <tmmintrin.h>
#include <xmmintrin.h>
#include "MemOps.h"

#define _MM_SHUFFLE_XYZW(x, y, z, w) _MM_SHUFFLE(w, z, y, x)

#if !defined(_MSC_VER)
	#define _MM_ACCESS(v, i) ((float*)&(v))[i]
	#define FORCEINLINE inline
	#define FORCEINLINE_BACK __attribute__((always_inline))
#else
	#define _MM_ACCESS(v, i) (v).m128_f32[i]
	#define FORCEINLINE __forceinline
	#define FORCEINLINE_BACK
#endif

ALIGN_CLASS(16) SimdVec;

#if !defined(_MSC_VER)
typedef __m128 vec128;
#else
typedef __declspec(align(16)) __m128 vec128;
#endif

typedef const SimdVec & SimdVecArg;
//typedef const SimdVec SimdVecArg;

enum VEC_ZERO_ENUM { VZERO };
enum VEC_ONE_ENUM { VONE };

#define VEC_ZERO SimdVec(VZERO)
#define VEC_ONE SimdVec(VONE)

ALIGN_CLASS(16) SimdVec
{
public:
	inline SimdVec();
	inline explicit SimdVec(vec128 v);
	inline explicit SimdVec(VEC_ZERO_ENUM preset);
	inline explicit SimdVec(VEC_ONE_ENUM preset);
	inline explicit SimdVec(SimdVecArg x, SimdVecArg y);
	inline explicit SimdVec(SimdVecArg x, SimdVecArg y, SimdVecArg z);
	inline explicit SimdVec(SimdVecArg x, SimdVecArg y, SimdVecArg z, SimdVecArg w);
	inline explicit SimdVec(float xyzw);
	inline explicit SimdVec(float x, float y);
	inline explicit SimdVec(float x, float y, float z);
	inline explicit SimdVec(float x, float y, float z, float w);
	inline explicit SimdVec(const float * xyzw);
	
	inline SimdVec Add(SimdVecArg v) const;
	inline SimdVec Sub(SimdVecArg v) const;
	inline SimdVec Mul(SimdVecArg v) const;
	inline SimdVec Div(SimdVecArg v) const;
	inline SimdVec Min(SimdVecArg v) const;
	inline SimdVec Max(SimdVecArg v) const;

	inline SimdVec MulAdd(SimdVecArg v1, SimdVecArg v2);

	inline SimdVec Inv() const;
	inline SimdVec Sqrt() const;
	inline SimdVec InvSqrt() const;
	inline SimdVec Neg() const;
	inline SimdVec Abs() const;
	
	inline SimdVec Saturate() const { return Max(VEC_ZERO).Min(VEC_ONE); }

	inline SimdVec Len1Sq() const;
	inline SimdVec Len2Sq() const;
	inline SimdVec Len3Sq() const;
	inline SimdVec Len4Sq() const;
	inline SimdVec Len1() const;
	inline SimdVec Len2() const;
	inline SimdVec Len3() const;
	inline SimdVec Len4() const;
	inline SimdVec UnitVec1() const;
	inline SimdVec UnitVec2() const;
	inline SimdVec UnitVec3() const;
	inline SimdVec UnitVec4() const;
	
	inline SimdVec Dot1(SimdVecArg v) const;
	inline SimdVec Dot2(SimdVecArg v) const;
	inline SimdVec Dot3(SimdVecArg v) const;
	inline SimdVec Dot4(SimdVecArg v) const;

	inline SimdVec Cross3(SimdVecArg v) const;

	inline SimdVec ReplicateX() const;
	inline SimdVec ReplicateY() const;
	inline SimdVec ReplicateZ() const;
	inline SimdVec ReplicateW() const;

	inline float & X();
	inline float & Y();
	inline float & Z();
	inline float & W();

	inline float X() const;
	inline float Y() const;
	inline float Z() const;
	inline float W() const;

	inline float & operator()(unsigned int index);
	inline float operator()(unsigned int index) const;
	
	inline vec128 Vec128() const;
	inline vec128 & Vec128();
	inline void Store(float * v) const;
	inline void SetZero();
	inline void SetAll(float xyzw);
	inline void Set2(float x, float y);
	inline void Set3(float x, float y, float z);
	inline void Set4(float x, float y, float z, float w);

	inline SimdVec Select(SimdVecArg v, bool x, bool y, bool z, bool w) const;
	inline SimdVec Permute(SimdVecArg v, unsigned int x, unsigned int y, unsigned int z, unsigned int w) const;

	inline SimdVec AND(SimdVecArg v) const;
	
	inline SimdVec CMP_EQ(SimdVecArg v) const;
	inline SimdVec CMP_NE(SimdVecArg v) const;
	inline SimdVec CMP_LT(SimdVecArg v) const;
	inline SimdVec CMP_LE(SimdVecArg v) const;
	inline SimdVec CMP_GT(SimdVecArg v) const;
	inline SimdVec CMP_GE(SimdVecArg v) const;
	
	inline bool ALL_EQ3(SimdVecArg v) const;
	inline bool ALL_NE3(SimdVecArg v) const;
	inline bool ALL_LT3(SimdVecArg v) const;
	inline bool ALL_LE3(SimdVecArg v) const;
	inline bool ALL_GT3(SimdVecArg v) const;
	inline bool ALL_GE3(SimdVecArg v) const;

	inline bool ANY_EQ3(SimdVecArg v) const;
	inline bool ANY_NE3(SimdVecArg v) const;
	inline bool ANY_LT3(SimdVecArg v) const;
	inline bool ANY_LE3(SimdVecArg v) const;
	inline bool ANY_GT3(SimdVecArg v) const;
	inline bool ANY_GE3(SimdVecArg v) const;

	inline bool ALL_EQ4(SimdVecArg v) const;
	inline bool ALL_NE4(SimdVecArg v) const;
	inline bool ALL_LT4(SimdVecArg v) const;
	inline bool ALL_LE4(SimdVecArg v) const;
	inline bool ALL_GT4(SimdVecArg v) const;
	inline bool ALL_GE4(SimdVecArg v) const;

	inline bool ANY_EQ4(SimdVecArg v) const;
	inline bool ANY_NE4(SimdVecArg v) const;
	inline bool ANY_LT4(SimdVecArg v) const;
	inline bool ANY_LE4(SimdVecArg v) const;
	inline bool ANY_GT4(SimdVecArg v) const;
	inline bool ANY_GE4(SimdVecArg v) const;
	
	inline int BITTEST() const
	{
		return _mm_movemask_ps(mVec128);
	}
	
private:
	union
	{
		vec128 mVec128;
		struct
		{
			float x, y, z, w;
		};
	};
};

#define SSE_CMP_MASK3(x) ((x) & 7)
#define SSE_CMP_TEST3(x) ((x) == 7)
#define SSE_CMP_MASK4(x) (x)
#define SSE_CMP_TEST4(x) ((x) == 15)
#define SSE_TEST_ALL3(x) SSE_CMP_TEST3(SSE_CMP_MASK3(_mm_movemask_ps(x)))
#define SSE_TEST_ALL4(x) SSE_CMP_TEST4(SSE_CMP_MASK4(_mm_movemask_ps(x)))
#define SSE_TEST_ANY3(x) (SSE_CMP_MASK3(_mm_movemask_ps(x)) != 0)
#define SSE_TEST_ANY4(x) (SSE_CMP_MASK4(_mm_movemask_ps(x)) != 0)

#define _MM_SHUFFLE_XYZW(x, y, z, w) _MM_SHUFFLE(w, z, y, x)

inline void Normalize3(SimdVec & x, SimdVec & y, SimdVec & z)
{
	const SimdVec lengthSq = x.Mul(x).Add(y.Mul(y)).Add(z.Mul(z));
	const SimdVec lengthRcp = lengthSq.InvSqrt();
	x = x.Mul(lengthRcp);
	y = y.Mul(lengthRcp);
	z = z.Mul(lengthRcp);
}

#pragma region construction

inline SimdVec::SimdVec()
{
}

inline SimdVec::SimdVec(vec128 v)
	: mVec128(v)
{
}

/*inline SimdVec::SimdVec(SimdVecArg v) : mVec128(v.mVec128)
{
}*/

inline SimdVec::SimdVec(VEC_ZERO_ENUM preset)
	: mVec128(_mm_setzero_ps())
{
}

inline SimdVec::SimdVec(VEC_ONE_ENUM preset)
	: mVec128(_mm_set_ps1(1.0f))
{
}

inline SimdVec::SimdVec(SimdVecArg x, SimdVecArg y)
{
	mVec128 = _mm_unpacklo_ps(x.mVec128, y.mVec128);
}

inline SimdVec::SimdVec(SimdVecArg x, SimdVecArg y, SimdVecArg z)
{
	vec128 xynn = _mm_unpacklo_ps(x.mVec128, y.mVec128);
	mVec128 = _mm_shuffle_ps(xynn, z.mVec128, _MM_SHUFFLE_XYZW(0,1,2,3));
}

inline SimdVec::SimdVec(SimdVecArg x, SimdVecArg y, SimdVecArg z, SimdVecArg w)
{
	vec128 xynn = _mm_unpacklo_ps(x.mVec128, y.mVec128);
	vec128 nnzw = _mm_unpacklo_ps(z.mVec128, w.mVec128);
	mVec128 = _mm_shuffle_ps(xynn, nnzw, _MM_SHUFFLE_XYZW(0,1,2,3));
}

inline SimdVec::SimdVec(float xyzw)
	: mVec128(_mm_set_ps1(xyzw))
{
}

inline SimdVec::SimdVec(float x, float y)
	: mVec128(_mm_set_ps(0.0f, 0.0f, y, x))
{
}

inline SimdVec::SimdVec(float x, float y, float z)
	: mVec128(_mm_set_ps(0.0f, z, y, x))
{
}

inline SimdVec::SimdVec(float x, float y, float z, float w)
	: mVec128(_mm_set_ps(w, z, y, x))
{
}

inline SimdVec::SimdVec(const float * xyzw)
	: mVec128(_mm_loadu_ps(xyzw))
{
}

#pragma endregion

#pragma region arithmetic

inline SimdVec SimdVec::Add(SimdVecArg v) const
{
	return SimdVec(_mm_add_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::Sub(SimdVecArg v) const
{
	return SimdVec(_mm_sub_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::Mul(SimdVecArg v) const
{
	return SimdVec(_mm_mul_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::Div(SimdVecArg v) const
{
	return SimdVec(_mm_div_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::Min(SimdVecArg v) const
{
	return SimdVec(_mm_min_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::Max(SimdVecArg v) const
{
	return SimdVec(_mm_max_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::MulAdd(SimdVecArg v1, SimdVecArg v2)
{
	return SimdVec(_mm_add_ps(mVec128, _mm_mul_ps(v1.mVec128, v2.mVec128)));
}

inline SimdVec SimdVec::Inv() const
{
	return SimdVec(_mm_rcp_ps(mVec128));
}

inline SimdVec SimdVec::Sqrt() const
{
	return SimdVec(_mm_sqrt_ps(mVec128));
}

inline SimdVec SimdVec::InvSqrt() const
{
	return SimdVec(_mm_rsqrt_ps(mVec128));
}

inline SimdVec SimdVec::Neg() const
{
	return SimdVec(_mm_sub_ps(_mm_setzero_ps(), mVec128));
}

inline SimdVec SimdVec::Abs() const
{
	return SimdVec(_mm_max_ps(mVec128, _mm_sub_ps(_mm_setzero_ps(), mVec128)));
}

#pragma endregion

#pragma region logic

#pragma endregion

#pragma region length

inline SimdVec SimdVec::Len1Sq() const
{
	SimdVec dot = Mul(*this);
	SimdVec x = dot.ReplicateX();
	return x;
}

inline SimdVec SimdVec::Len2Sq() const
{
#if 1
	return Dot2(*this);
#else
	SimdVec dot = Mul(*this);
	SimdVec x = dot.ReplicateX();
	SimdVec y = dot.ReplicateY();
	return x.Add(y);
#endif
}

inline SimdVec SimdVec::Len3Sq() const
{
#if 1
	return Dot3(*this);
#else
	SimdVec dot = Mul(*this);
	SimdVec x = dot.ReplicateX();
	SimdVec y = dot.ReplicateY();
	SimdVec z = dot.ReplicateZ();
	return x.Add(y.Add(z));
#endif
}

inline SimdVec SimdVec::Len4Sq() const
{
#if 1
	return Dot4(*this);
#else
	SimdVec dot = Mul(*this);
	SimdVec x = dot.ReplicateX();
	SimdVec y = dot.ReplicateY();
	SimdVec z = dot.ReplicateZ();
	SimdVec w = dot.ReplicateW();
	return x.Add(y.Add(z.Add(w)));
#endif
}

inline SimdVec SimdVec::Len1() const
{
	return Abs().ReplicateX();
}

inline SimdVec SimdVec::Len2() const
{
	return Len2Sq().Sqrt();
}

inline SimdVec SimdVec::Len3() const
{
	return Len3Sq().Sqrt();
}

inline SimdVec SimdVec::Len4() const
{
	return Len4Sq().Sqrt();
}

#pragma endregion

#pragma region unit vector

inline SimdVec SimdVec::UnitVec1() const
{
	return Div(Len1());
}

inline SimdVec SimdVec::UnitVec2() const
{
	return Div(Len2());
}

inline SimdVec SimdVec::UnitVec3() const
{
	return Div(Len3());
}

inline SimdVec SimdVec::UnitVec4() const
{
	return Div(Len4());
}

#pragma endregion

#pragma region horizontal dot

inline SimdVec SimdVec::Dot1(SimdVecArg v) const
{
	SimdVec dot = Mul(v);
	SimdVec x = dot.ReplicateX();
	return x;
}

inline SimdVec SimdVec::Dot2(SimdVecArg v) const
{
	SimdVec dot = Mul(v);
	SimdVec x = dot.ReplicateX();
	SimdVec y = dot.ReplicateY();
	return x.Add(y);
}

inline SimdVec SimdVec::Dot3(SimdVecArg v) const
{
	vec128 dot = _mm_mul_ps(mVec128, v.mVec128);
	vec128 xnnn = dot;
	vec128 ynnn = _mm_shuffle_ps(dot, dot, _MM_SHUFFLE_XYZW(1,1,1,1));
	vec128 znnn = _mm_shuffle_ps(dot, dot, _MM_SHUFFLE_XYZW(2,2,2,2));
	vec128 temp = _mm_add_ss(xnnn, _mm_add_ss(ynnn, znnn));
	return SimdVec(_mm_shuffle_ps(temp, temp, _MM_SHUFFLE_XYZW(0,0,0,0)));
}

inline SimdVec SimdVec::Dot4(SimdVecArg v) const
{
#if defined(_MSC_VER)
	return SimdVec(_mm_dp_ps(mVec128, v.mVec128, 0xff));
#elif 0
	vec128 dot = _mm_mul_ps(mVec128, v.mVec128); // |  x  |  y  |  z  |  w  |
	vec128 temp1 = _mm_hadd_ps(dot, dot);        // | x+y | z+w | x+y | z+w |
	vec128 temp2 = _mm_hadd_ps(temp1, temp1);    // | x+y+z+w | ............|
	return SimdVec(_mm_shuffle_ps(temp2, temp2, _MM_SHUFFLE_XYZW(0,0,0,0)));
#else
	vec128 dot = _mm_mul_ps(mVec128, v.mVec128);
	vec128 temp1 = _mm_unpacklo_ps(dot, dot);                               // |  x  |  x  |. y  |  y  |
	vec128 temp2 = _mm_unpackhi_ps(dot, dot);                               // |  z  |  z  |  w..|..w  |
	vec128 temp3 = _mm_add_ps(temp1, temp2);                                // | x+z | ... |.y+w |     |
	vec128 temp4 = _mm_shuffle_ps(temp3, temp3, _MM_SHUFFLE_XYZW(2,0,0,0)); // | y+w | ................|
	vec128 temp5 = _mm_add_ss(temp3, temp4);
	return SimdVec(_mm_shuffle_ps(temp5, temp5, _MM_SHUFFLE_XYZW(0,0,0,0)));
#endif
}

#pragma endregion

#pragma region horizontal cross

inline SimdVec SimdVec::Cross3(SimdVecArg v) const
{
	vec128 a1 = _mm_shuffle_ps(mVec128,   mVec128,   _MM_SHUFFLE_XYZW(1, 2, 0, 0));
	vec128 a2 = _mm_shuffle_ps(v.mVec128, v.mVec128, _MM_SHUFFLE_XYZW(2, 0, 1, 0));
	vec128 b1 = _mm_shuffle_ps(mVec128,   mVec128,   _MM_SHUFFLE_XYZW(2, 0, 1, 0));
	vec128 b2 = _mm_shuffle_ps(v.mVec128, v.mVec128, _MM_SHUFFLE_XYZW(1, 2, 0, 0));
	vec128 a = _mm_mul_ps(a1, a2);
	vec128 b = _mm_mul_ps(b1, b2);
	return SimdVec(_mm_sub_ps(a, b));

#if 0
	r[0] =
		m_v[1] * v[2] -
		m_v[2] * v[1];
	r[1] =
		m_v[2] * v[0] -
		m_v[0] * v[2];
	r[2] =
		m_v[0] * v[1] -
		m_v[1] * v[0];
#endif
}

#pragma endregion

#pragma region element replication

inline SimdVec SimdVec::ReplicateX() const
{
	return SimdVec(_mm_shuffle_ps(mVec128, mVec128, _MM_SHUFFLE(0, 0, 0, 0)));
}

inline SimdVec SimdVec::ReplicateY() const
{
	return SimdVec(_mm_shuffle_ps(mVec128, mVec128, _MM_SHUFFLE(1, 1, 1, 1)));
}

inline SimdVec SimdVec::ReplicateZ() const
{
	return SimdVec(_mm_shuffle_ps(mVec128, mVec128, _MM_SHUFFLE(2, 2, 2, 2)));
}

inline SimdVec SimdVec::ReplicateW() const
{
	return SimdVec(_mm_shuffle_ps(mVec128, mVec128, _MM_SHUFFLE(3, 3, 3, 3)));
}

#pragma endregion

#pragma region direct element access

inline float & SimdVec::X()
{
	return _MM_ACCESS(mVec128, 0);
}

inline float & SimdVec::Y()
{
	return _MM_ACCESS(mVec128, 1);
}

inline float & SimdVec::Z()
{
	return _MM_ACCESS(mVec128, 2);
}

inline float & SimdVec::W()
{
	return _MM_ACCESS(mVec128, 3);
}

inline float SimdVec::X() const
{
	return _MM_ACCESS(mVec128, 0);
}

inline float SimdVec::Y() const
{
	return _MM_ACCESS(mVec128, 1);
}

inline float SimdVec::Z() const
{
	return _MM_ACCESS(mVec128, 2);
}

inline float SimdVec::W() const
{
	return _MM_ACCESS(mVec128, 3);
}

inline float & SimdVec::operator()(unsigned int index)
{
	return reinterpret_cast<float *>(&mVec128)[index];
}

inline float SimdVec::operator()(unsigned int index) const
{
	return reinterpret_cast<const float *>(&mVec128)[index];
}

#pragma endregion

inline vec128 SimdVec::Vec128() const
{
	return mVec128;
}
inline vec128 & SimdVec::Vec128()
{
	return mVec128;
}

inline void SimdVec::Store(float * v) const
{
	_mm_storeu_ps(v, mVec128);
}

inline void SimdVec::SetZero()
{
	mVec128 = _mm_setzero_ps();
}

inline void SimdVec::SetAll(float xyzw)
{
	mVec128 = _mm_set_ps1(xyzw);
}

inline void SimdVec::Set2(float x, float y)
{
	mVec128 = _mm_set_ps(0.0f, 0.0f, y, x);
}

inline void SimdVec::Set3(float x, float y, float z)
{
	mVec128 = _mm_set_ps(0.0f, z, y, x);
}

inline void SimdVec::Set4(float x, float y, float z, float w)
{
	mVec128 = _mm_set_ps(w, z, y, x);
}

inline SimdVec SimdVec::Select(SimdVecArg v, bool x, bool y, bool z, bool w) const
{
	return SimdVec(
		x ? v.ReplicateX() : ReplicateX(),
		y ? v.ReplicateY() : ReplicateY(),
		z ? v.ReplicateZ() : ReplicateZ(),
		w ? v.ReplicateW() : ReplicateW());
}

inline SimdVec SimdVec::Permute(SimdVecArg v, unsigned int x, unsigned int y, unsigned int z, unsigned int w) const
{
	return SimdVec(
		x < 4 ? (*this)(x) : v(x - 4),
		y < 4 ? (*this)(y) : v(y - 4),
		z < 4 ? (*this)(z) : v(z - 4),
		w < 4 ? (*this)(w) : v(w - 4));
}

inline SimdVec SimdVec::AND(SimdVecArg v) const
{
	return SimdVec(_mm_and_ps(mVec128, v.mVec128));
}

#pragma region element comparision

inline SimdVec SimdVec::CMP_EQ(SimdVecArg v) const
{
	return SimdVec(_mm_cmpeq_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::CMP_NE(SimdVecArg v) const
{
	return SimdVec(_mm_cmpneq_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::CMP_LT(SimdVecArg v) const
{
	return SimdVec(_mm_cmplt_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::CMP_LE(SimdVecArg v) const
{
	return SimdVec(_mm_cmple_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::CMP_GT(SimdVecArg v) const
{
	return SimdVec(_mm_cmpgt_ps(mVec128, v.mVec128));
}

inline SimdVec SimdVec::CMP_GE(SimdVecArg v) const
{
	return SimdVec(_mm_cmpge_ps(mVec128, v.mVec128));
}

#pragma endregion

#pragma region 3 element ALL comparision

inline bool SimdVec::ALL_EQ3(SimdVecArg v) const
{
	return SSE_TEST_ALL3(_mm_cmpeq_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_NE3(SimdVecArg v) const
{
	return SSE_TEST_ALL3(_mm_cmpneq_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_LT3(SimdVecArg v) const
{
	return SSE_TEST_ALL3(_mm_cmplt_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_LE3(SimdVecArg v) const
{
	return SSE_TEST_ALL3(_mm_cmple_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_GT3(SimdVecArg v) const
{
	return SSE_TEST_ALL3(_mm_cmpgt_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_GE3(SimdVecArg v) const
{
	return SSE_TEST_ALL3(_mm_cmpge_ps(mVec128, v.mVec128));
}

#pragma endregion

#pragma region 3 element ANY comparision

inline bool SimdVec::ANY_EQ3(SimdVecArg v) const
{
	return SSE_TEST_ANY3(_mm_cmpeq_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_NE3(SimdVecArg v) const
{
	return SSE_TEST_ANY3(_mm_cmpneq_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_LT3(SimdVecArg v) const
{
	return SSE_TEST_ANY3(_mm_cmplt_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_LE3(SimdVecArg v) const
{
	return SSE_TEST_ANY3(_mm_cmple_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_GT3(SimdVecArg v) const
{
	return SSE_TEST_ANY3(_mm_cmpgt_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_GE3(SimdVecArg v) const
{
	return SSE_TEST_ANY3(_mm_cmpge_ps(mVec128, v.mVec128));
}

#pragma endregion

#pragma region 4 element ALL comparision

inline bool SimdVec::ALL_EQ4(SimdVecArg v) const
{
	return SSE_TEST_ALL4(_mm_cmpeq_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_NE4(SimdVecArg v) const
{
	return SSE_TEST_ALL4(_mm_cmpneq_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_LT4(SimdVecArg v) const
{
	return SSE_TEST_ALL4(_mm_cmplt_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_LE4(SimdVecArg v) const
{
	return SSE_TEST_ALL4(_mm_cmple_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_GT4(SimdVecArg v) const
{
	return SSE_TEST_ALL4(_mm_cmpgt_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ALL_GE4(SimdVecArg v) const
{
	return SSE_TEST_ALL4(_mm_cmpge_ps(mVec128, v.mVec128));
}

#pragma endregion

#pragma region 4 element ANY comparision

inline bool SimdVec::ANY_EQ4(SimdVecArg v) const
{
	return SSE_TEST_ANY4(_mm_cmpeq_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_NE4(SimdVecArg v) const
{
	return SSE_TEST_ANY4(_mm_cmpneq_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_LT4(SimdVecArg v) const
{
	return SSE_TEST_ANY4(_mm_cmplt_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_LE4(SimdVecArg v) const
{
	return SSE_TEST_ANY4(_mm_cmple_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_GT4(SimdVecArg v) const
{
	return SSE_TEST_ANY4(_mm_cmpgt_ps(mVec128, v.mVec128));
}

inline bool SimdVec::ANY_GE4(SimdVecArg v) const
{
	return SSE_TEST_ANY4(_mm_cmpge_ps(mVec128, v.mVec128));
}

//

const static SimdVec SIMD_VEC_ZERO(_mm_setzero_ps());

#pragma endregion

