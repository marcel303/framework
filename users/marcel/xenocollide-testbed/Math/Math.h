/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <float.h>
#include <math.h>
#include <xmmintrin.h>

#define ASSERT(arg)

namespace XenoCollide
{
	enum ShuffleComponents
	{
		COMPONENT_DONTCARE = 0,
		COMPONENT_X = 0,
		COMPONENT_Y = 1,
		COMPONENT_Z = 2,
		COMPONENT_W = 3,
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Basic types

#include <stdint.h>

namespace XenoCollide
{
	typedef uint8_t				uint8;
	typedef uint16_t			uint16;
	typedef uint32_t			uint32;
	typedef uint64_t			uint64;
	typedef __m128				uint128;

	typedef int8_t				int8;
	typedef int16_t				int16;
	typedef int32_t				int32;
	typedef int64_t				int64;
	typedef __m128				int128;

	typedef float				float32;
	typedef double				float64;
	typedef long double			float80;

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Defined constants

	const float MAX_FLOAT = 3.40282346638528860e+38f;
	const float MIN_FLOAT = -MAX_FLOAT;
	const float EPS_FLOAT = 1.19209290e-07f;

	class Euler;
	class Matrix;
	class Quat;
	class Vector;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Math constants.
	//////////////////////////////////////////////////////////////////////////////////////////////

	const float32 PI = 3.1415926535897932384626433832795f;
	const float32 PI_2 = (PI / 2.0f);
	const float32 PI_180 = (PI / 180.0f);
	const float32 Vector_ZERO_EPSILON = 1.f / 16384.f;
	const float32 MIN_ZERO_EPSILON = 1.1754943508222875E-38;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Vector Class
	//////////////////////////////////////////////////////////////////////////////////////////////
	class Vector
	{

	public:

		Vector() { }
		Vector(const Vector& v);
		Vector(float32 x, float32 y, float32 z);
		Vector(float32 x, float32 y, float32 z, float32 w);
		Vector(uint128 u);
		explicit			Vector(const Quat& q);

		void				Set(float32 x, float32 y, float32 z);
		void				Set(float32 x, float32 y, float32 z, float32 w);
		void				Set(const Quat& q);

		float32& X();
		float32& Y();
		float32& Z();
		float32& W();
		float32& operator() (int i);

		float32				X() const;
		float32				Y() const;
		float32				Z() const;
		float32				W() const;
		float32				operator() (int i) const;

		float32				Len3() const;
		float32				Len3Squared() const;
		bool				IsZero3() const;
		Vector				UnitVec3() const;
		bool				CanNormalize3() const;

		void				Normalize3();
		void				Normalize4();

		float32				Len4() const;
		bool				IsZero4() const;

		Vector& operator=(const Vector& v)
		{
			vec4 = v.vec4;
			return *this;
		}

		Vector& operator%=(const Vector& v);
		Vector& operator*=(float32 s);
		Vector& operator/=(float32 s);
		Vector& operator+=(const Vector& v);
		Vector& operator-=(const Vector& v);

		uint128& Vec4();
		uint128				Vec4() const;

	protected:

		uint128 vec4;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Vector64 Class
	//////////////////////////////////////////////////////////////////////////////////////////////
	class Vector64
	{

	public:

		Vector64() { }
		Vector64(const Vector64& v);
		Vector64(float64 x, float64 y, float64 z);

		float64& X();
		float64& Y();
		float64& Z();
		//	float64&			W();
		//	float64&			operator() (int i);

		float64				X() const;
		float64				Y() const;
		float64				Z() const;
		//	float64				W() const;
		//	float64				operator() (int i) const;

		float64				Len3() const;
		//	float64				Len3Squared() const;						   
		//	bool				IsZero3() const;						
		//	Vector				UnitVec3() const;			   
		bool				CanNormalize3() const;

		void				Normalize3();
		//	void				Normalize4();

		//	float64				Len4() const;
		//	bool				IsZero4() const;

		Vector64& operator=(const Vector64& v)
		{
			x = v.x;
			y = v.y;
			z = v.z;
			return *this;
		}

		Vector64& operator%=(const Vector& v);
		Vector64& operator*=(float64 s);
		Vector64& operator/=(float64 s);
		Vector64& operator+=(const Vector& v);
		Vector64& operator-=(const Vector& v);

	protected:

		float64 x;
		float64 y;
		float64 z;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Euler Class
	//////////////////////////////////////////////////////////////////////////////////////////////
	class Euler : public Vector
	{

	public:

		Euler() {}
		Euler(float32 x, float32 y, float32 z) : Vector(x, y, z) {}
		explicit			Euler(const Vector& v) : Vector(v) {}
		explicit			Euler(const Matrix& m);
		explicit			Euler(const Quat& q);
		void				Set(const Matrix& m);
		void				Set(float32 x, float32 y, float32 z);

	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Quaternion Class
	//////////////////////////////////////////////////////////////////////////////////////////////
	class Quat : public Vector
	{

	public:

		Quat() {}
		explicit			Quat(const Vector& v) : Vector(v) {}
		explicit			Quat(const Matrix& m);
		Quat(float32 x, float32 y, float32 z, float32 w);
		Quat(const Vector& axis, float32 angle);
		Quat(const Vector& a, const Vector& b);

		void				Build(const Matrix& m);
		void				Build(const Vector& axis, float32 angle);
		void				Build(const Vector& a, const Vector& b);

		Quat& operator=(const Quat& q);
		Quat& operator*=(const Quat& q);

		Vector				Rotate(const Vector& v) const;
		void				ConvertToAxisAngle(Vector* axis, float32* angle) const;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Matrix Class
	//////////////////////////////////////////////////////////////////////////////////////////////
	class Matrix
	{

	public:

		Matrix();
		explicit			Matrix(const Quat& q);
		explicit			Matrix(const Euler& e);

		void				BuildIdentity();
		void				Build(const Quat& q);
		void				Build(const Euler& e);

		Matrix& operator=(const Matrix& m);
		Matrix& operator*=(float32 s);
		float32& operator()(int32 row, int32 col);
		float32				operator()(int32 row, int32 col) const;

		Vector& Col(int col);
		const Vector& Col(int col) const;

		Vector				GetXAxis() const;
		Vector				GetYAxis() const;
		Vector				GetZAxis() const;
		Vector				GetTrans() const;

		void				SetXAxis(const Vector& v);
		void				SetYAxis(const Vector& v);
		void				SetZAxis(const Vector& v);
		void				SetTrans(const Vector& v);

		void				ConvertToAxisAngle(Vector* axis, float32* angle) const;

		Vector v0;
		Vector v1;
		Vector v2;
		Vector v3;
	};

	// Quaternion exponent
	Quat QuatExp(const Quat& q);

	// Quaternion inverse
	Quat operator~(const Quat& q);

	// Matrix-Vector multiplication
	Vector Mul3x4(const Matrix& m, const Vector& v);

	// Matrix inverse
	void OrthonormalInverse3x4(Matrix* out, const Matrix& a);
	void GeneralInverse4x4(Matrix* out, const Matrix& a);

	inline float32 Max(float32 a, float32 b)
	{
		return (a > b ? a : b);
	}

	inline float32 Min(float32 a, float32 b)
	{
		return (a < b ? a : b);
	}

	inline float32 Abs(float32 a)
	{
		return (a >= 0.0f ? a : -a);
	}

	inline bool operator ==(const Vector& a, const Vector& b)
	{
		return (a.X() == b.X() && a.Y() == b.Y() && a.Z() == b.Z());
	}

	inline bool operator !=(const Vector& a, const Vector& b)
	{
		return !(a == b);
	}

	inline Vector operator +(const Vector& a, const Vector& b)
	{
		return Vector(_mm_add_ps(a.Vec4(), b.Vec4()));
	}

	inline Vector operator -(const Vector& a, const Vector& b)
	{
		return Vector(_mm_sub_ps(a.Vec4(), b.Vec4()));
	}

	inline Vector operator *(const Vector& a, float32 b)
	{
		return Vector(_mm_mul_ps(a.Vec4(), _mm_set_ps1(b)));
	}

	inline Vector operator *(float32 a, const Vector& b)
	{
		return Vector(_mm_mul_ps(_mm_set_ps1(a), b.Vec4()));
	}

	inline Vector operator /(const Vector& a, float32 b)
	{
		ASSERT(b != 0);
		return Vector(_mm_div_ps(a.Vec4(), _mm_set_ps1(b)));
	}

	inline Vector operator -(const Vector& a)
	{
		return -1.0f * a;
	}

#define MOVE_TO_X( COMPONENT ) _mm_shuffle_ps(ab, ab, _MM_SHUFFLE(COMPONENT_DONTCARE, COMPONENT_DONTCARE, COMPONENT_DONTCARE, COMPONENT))

	inline float32 _mm_extract_ps(const __m128& a, const int index)
	{
		return ((float*)&a)[index];
	}

	inline float32 operator *(const Vector& a, const Vector& b)
	{
		__m128 ab = _mm_mul_ps(a.Vec4(), b.Vec4());
		return _mm_extract_ps(_mm_add_ss(ab, _mm_add_ss(MOVE_TO_X(COMPONENT_Y), MOVE_TO_X(COMPONENT_Z))), COMPONENT_X);
	}

	inline Vector operator %(const Vector& a, const Vector& b)
	{
		return Vector(
			_mm_sub_ps(
				_mm_mul_ps(
					_mm_shuffle_ps(a.Vec4(), a.Vec4(), _MM_SHUFFLE(COMPONENT_W, COMPONENT_X, COMPONENT_Z, COMPONENT_Y)),
					_mm_shuffle_ps(b.Vec4(), b.Vec4(), _MM_SHUFFLE(COMPONENT_W, COMPONENT_Y, COMPONENT_X, COMPONENT_Z))),
				_mm_mul_ps(
					_mm_shuffle_ps(a.Vec4(), a.Vec4(), _MM_SHUFFLE(COMPONENT_W, COMPONENT_Y, COMPONENT_X, COMPONENT_Z)),
					_mm_shuffle_ps(b.Vec4(), b.Vec4(), _MM_SHUFFLE(COMPONENT_W, COMPONENT_X, COMPONENT_Z, COMPONENT_Y)))));
	}

	inline Vector CompAbs(const Vector& a)
	{
		return Vector(fabs(a.X()), fabs(a.Y()), fabs(a.Z()), fabs(a.W()));
	}

	inline Vector CompMul(const Vector& a, const Vector& b)
	{
		return Vector(_mm_mul_ps(a.Vec4(), b.Vec4()));
	}

	inline Vector CompMin(const Vector& a, const Vector& b)
	{
		return Vector(_mm_min_ps(a.Vec4(), b.Vec4()));
	}

	inline Vector CompMax(const Vector& a, const Vector& b)
	{
		return Vector(_mm_max_ps(a.Vec4(), b.Vec4()));
	}

	inline float32 DegToRad(float32 deg)
	{
		return deg * PI_180;
	}

	inline float32 RadToDeg(float32 rad)
	{
		return rad * (1.0f / PI_180);
	}

	inline float32 Sqrt(const float32 a)
	{
		ASSERT(a >= 0.f);
		return _mm_extract_ps(_mm_sqrt_ss(_mm_set_ps1(a)), 0);
	}
	inline float32 InvSqrt(const float32 a)
	{
		ASSERT(a > 0.f);
		static __m128 s_ssevec1 = _mm_set_ps1(1.f);
		return _mm_extract_ps(_mm_div_ss(s_ssevec1, _mm_sqrt_ss(_mm_set_ps1(a))), 0);
	}

	inline Euler::Euler(const Matrix& m)
	{
		Set(m);
	}

	inline void Matrix::BuildIdentity()
	{
		v0 = Vector(1, 0, 0, 0);
		v1 = Vector(0, 1, 0, 0);
		v2 = Vector(0, 0, 1, 0);
		v3 = Vector(0, 0, 0, 1);
	}

	inline Matrix& Matrix::operator =(const Matrix& m)
	{
		v0 = m.v0;
		v1 = m.v1;
		v2 = m.v2;
		v3 = m.v3;
		return *this;
	}

	inline Matrix::Matrix()
	{
	}

	inline Matrix::Matrix(const Quat& q)
	{
		Build(q);
	}

	inline Matrix::Matrix(const Euler& e)
	{
		Build(e);
	}

	inline float32& Matrix::operator()(int32 row, int32 col)
	{
		ASSERT(row >= 0 && row <= 3);
		ASSERT(col >= 0 && col <= 3);
		return *((float32*)this + 4 * col + row);
	}

	inline Vector& Matrix::Col(int col)
	{
		ASSERT(col >= 0 && col <= 3);
		return *(&v0 + col);
	}

	inline float32 Matrix::operator()(int32 row, int32 col) const
	{
		ASSERT(row >= 0 && row <= 3);
		ASSERT(col >= 0 && col <= 3);
		return *((float32*)this + 4 * col + row);
	}
	inline const Vector& Matrix::Col(int col) const
	{
		ASSERT(col >= 0 && col <= 3);
		return *((Vector*)this + col);
	}

	inline void Matrix::SetTrans(const Vector& v)
	{
		v3.X() = v.X();
		v3.Y() = v.Y();
		v3.Z() = v.Z();
	}

	inline Matrix& Matrix::operator*=(float32 s)
	{
		v0 *= s;
		v1 *= s;
		v2 *= s;
		v3 *= s;
		return *this;
	}

	inline Vector operator*(const Matrix& m, const Vector& v)
	{
		return Vector(Mul3x4(m, v));
	}

	inline Quat::Quat(float x, float y, float z, float w)
	{
		vec4 = _mm_set_ps(w, z, y, x);
	}

	inline Quat::Quat(const Matrix& m)
	{
		Build(m);
	}

	inline Quat::Quat(const Vector& axis, float32 angle)
	{
		Build(axis, angle);
	}

	inline Quat::Quat(const Vector& a, const Vector& b)
	{
		Build(a, b);
	}

	inline Quat& Quat::operator =(const Quat& a)
	{
		vec4 = a.Vec4();
		return *this;
	}

	inline bool operator ==(const Quat& a, const Quat& b)
	{
		return (a.X() == b.X() && a.Y() == b.Y() && a.Z() == b.Z() && a.W() == b.W());
	}

	inline bool operator !=(const Quat& a, const Quat& b)
	{
		return !(a == b);
	}

	inline Quat operator +(const Quat& a, const Quat& b)
	{
		return Quat(_mm_add_ps(a.Vec4(), b.Vec4()));
	}

	inline Quat operator *(const Quat& a, const Quat& b)
	{
		Quat c;
		const float32 aW = a.W(), aX = a.X(), aY = a.Y(), aZ = a.Z();
		const float32 bW = b.W(), bX = b.X(), bY = b.Y(), bZ = b.Z();
		c.X() = (aW * bX) + (bW * aX) + (aY * bZ) - (aZ * bY);
		c.Y() = (aW * bY) + (bW * aY) + (aZ * bX) - (aX * bZ);
		c.Z() = (aW * bZ) + (bW * aZ) + (aX * bY) - (aY * bX);
		c.W() = (aW * bW) - ((aX * bX) + (aY * bY) + (aZ * bZ));
		return c;
	}

	inline Quat& Quat::operator *=(const Quat& q)
	{
		*this = (*this) * q;
		return *this;
	}

	inline Quat operator *(const Quat& a, float32 b)
	{
		return Quat(_mm_mul_ps(_mm_set_ps1(b), a.Vec4()));
	}

	inline Quat operator *(float32 a, const Quat& b)
	{
		return Quat(_mm_mul_ps(_mm_set_ps1(a), b.Vec4()));
	}

	inline Quat operator /(const Quat& a, float32 b)
	{
		ASSERT(b != 0);
		return Quat(_mm_div_ps(a.Vec4(), _mm_set_ps1(b)));
	}

	inline Quat operator ~(const Quat& a)
	{
		Quat q = -1.0f * a;
		q.W() = a.W();
		return q;
	}

	inline Vector::Vector(uint128 u)
	{
		vec4 = u;
	}

	inline Vector::Vector(float32 x, float32 y, float32 z, float32 w)
	{
		Set(x, y, z, w);
	}

	inline float32& Vector::X()
	{
		return (*this)(COMPONENT_X);
	}

	inline float32& Vector::Y()
	{
		return (*this)(COMPONENT_Y);
	}

	inline float32& Vector::Z()
	{
		return (*this)(COMPONENT_Z);
	}

	inline float32& Vector::W()
	{
		return (*this)(COMPONENT_W);
	}

	inline float32& Vector::operator () (int i)
	{
		return ((float32*)this)[i];
	}

	inline float32 Vector::X() const
	{
		return (*this)(COMPONENT_X);
	}

	inline float32 Vector::Y() const
	{
		return (*this)(COMPONENT_Y);
	}

	inline float32 Vector::Z() const
	{
		return (*this)(COMPONENT_Z);
	}

	inline float32 Vector::W() const
	{
		return (*this)(COMPONENT_W);
	}

	inline float32 Vector::operator () (int i) const
	{
		ASSERT(i >= 0 && i <= 3);
		return ((const float32*)this)[i];
	}

	inline uint128 Vector::Vec4() const
	{
		return vec4;
	}

	inline uint128& Vector::Vec4()
	{
		return vec4;
	}

	inline void Vector::Set(float32 x, float32 y, float32 z)
	{
		vec4 = _mm_set_ps(0.f, z, y, x);
	}

	inline void Vector::Set(float32 x, float32 y, float32 z, float32 w)
	{
		vec4 = _mm_set_ps(w, z, y, x);
	}

	inline Vector& Vector::operator *=(float32 s)
	{
		*this = (*this) * s;
		return *this;
	}

	inline Vector& Vector::operator /=(float32 s)
	{
		*this = (*this) / s;
		return *this;
	}

	inline Vector& Vector::operator +=(const Vector& v)
	{
		*this = (*this) + v;
		return *this;
	}

	inline Vector& Vector::operator -=(const Vector& v)
	{
		*this = (*this) - v;
		return *this;
	}

	inline bool Vector::CanNormalize3() const
	{
		float32 len = Len3Squared();
		return (len > MIN_ZERO_EPSILON);
	}

	inline Vector::Vector(float32 x, float32 y, float32 z)
	{
		Set(x, y, z);
	}

	inline Vector::Vector(const Vector& v)
	{
		vec4 = v.Vec4();
	}

	inline Vector::Vector(const Quat& q)
	{
		vec4 = q.Vec4();
	}

	inline void Vector::Set(const Quat& q)
	{
		vec4 = q.Vec4();
	}

	inline Vector& Vector::operator %=(const Vector& v)
	{
		*this = (*this) % v;
		return *this;
	}

#define L_SHUF_PASS(CI_1) _mm_shuffle_ps(xyzw2, xyzw2, _MM_SHUFFLE(COMPONENT_DONTCARE, COMPONENT_DONTCARE, COMPONENT_DONTCARE, CI_1))

	inline float Vector::Len3() const
	{
		__m128 xyzw2 = _mm_mul_ps(vec4, vec4);
		return _mm_extract_ps(_mm_sqrt_ss(_mm_add_ss(xyzw2, _mm_add_ss(L_SHUF_PASS(COMPONENT_Y), L_SHUF_PASS(COMPONENT_Z)))), COMPONENT_X);
	}

	inline float Vector::Len3Squared() const
	{
		__m128 xyzw2 = _mm_mul_ps(vec4, vec4);
		return _mm_extract_ps(_mm_add_ss(xyzw2, _mm_add_ss(L_SHUF_PASS(COMPONENT_Y), L_SHUF_PASS(COMPONENT_Z))), COMPONENT_X);
	}

	inline float Vector::Len4() const
	{
		__m128 xyzw2 = _mm_mul_ps(vec4, vec4);
		return _mm_extract_ps(_mm_sqrt_ss(_mm_add_ss(xyzw2, _mm_add_ss(L_SHUF_PASS(COMPONENT_Y), _mm_add_ss(L_SHUF_PASS(COMPONENT_Z), L_SHUF_PASS(COMPONENT_W))))), COMPONENT_X);
	}

	inline float64 Vector64::X() const
	{
		return x;
	}

	inline float64 Vector64::Y() const
	{
		return y;
	}

	inline float64 Vector64::Z() const
	{
		return z;
	}

	inline float64& Vector64::X()
	{
		return x;
	}

	inline float64& Vector64::Y()
	{
		return y;
	}

	inline float64& Vector64::Z()
	{
		return z;
	}

	inline Vector64 operator+ (const Vector64& a, const Vector64& b)
	{
		return Vector64(a.X() + b.X(), a.Y() + b.Y(), a.Z() + b.Z());
	}

	inline Vector64 operator- (const Vector64& a)
	{
		return Vector64(-a.X(), -a.Y(), -a.Z());
	}

	inline Vector64 operator- (const Vector64& a, const Vector64& b)
	{
		return Vector64(a.X() - b.X(), a.Y() - b.Y(), a.Z() - b.Z());
	}

	inline Vector64 operator* (const Vector64& a, float64 b)
	{
		return Vector64(a.X() * b, a.Y() * b, a.Z() * b);
	}

	inline float64 operator* (const Vector64& a, const Vector64& b)
	{
		return (a.X() * b.X()) + (a.Y() * b.Y()) + (a.Z() * b.Z());
	}

	inline Vector64 operator% (const Vector64& a, const Vector64& b)
	{
		return Vector64
		(
			a.Y() * b.Z() - a.Z() * b.Y(),
			a.Z() * b.X() - a.X() * b.Z(),
			a.X() * b.Y() - a.Y() * b.X()
		);
	}

	inline bool operator== (const Vector64& a, const Vector64& b)
	{
		return (a.X() == b.X() && a.Y() == b.Y() && a.Z() == b.Z());
	}

	inline bool operator!= (const Vector64& a, const Vector64& b)
	{
		return (a.X() != b.X() || a.Y() != b.Y() || a.Z() != b.Z());
	}

	inline float64 Abs(float64 a)
	{
		return (a >= 0.0 ? a : -a);
	}

	inline Vector64 CompAbs(const Vector64& a)
	{
		return Vector64(Abs(a.X()), Abs(a.Y()), Abs(a.Z()));
	}

	inline Vector64 CompMin(const Vector64& a, const Vector64& b)
	{
		Vector64 result;
		result.X() = (a.X() < b.X() ? a.X() : b.X());
		result.Y() = (a.Y() < b.Y() ? a.Y() : b.Y());
		result.Z() = (a.Z() < b.Z() ? a.Z() : b.Z());
		return result;
	}

	inline Vector64 CompMax(const Vector64& a, const Vector64& b)
	{
		Vector64 result;
		result.X() = (a.X() > b.X() ? a.X() : b.X());
		result.Y() = (a.Y() > b.Y() ? a.Y() : b.Y());
		result.Z() = (a.Z() > b.Z() ? a.Z() : b.Z());
		return result;
	}

	inline Vector64::Vector64(float64 _x, float64 _y, float64 _z)
		: x(_x)
		, y(_y)
		, z(_z)
	{
	}

	inline Vector64::Vector64(const Vector64& a)
	{
		x = a.x;
		y = a.y;
		z = a.z;
	}
}
