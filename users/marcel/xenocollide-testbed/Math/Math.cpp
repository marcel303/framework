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


#include "stdafx.h"

#include "Math.h"

Vector Quat::Rotate(const Vector& v) const
{
	Quat qv(v);
	qv.W() = 0;
	return (Vector) (*this * qv * ~*this);
}

Quat QuatExp(const Quat& a)
{
	float32 theta = a.Len4();

	if (theta == 0.0f)
	{
		return Quat(0, 0, 0, 1);
	}

	float32 s = sinf(0.5f * theta);
	float32 c = cosf(0.5f * theta);

	Quat q = a * (s / theta);
	q.W() = c;

	return q;
}

void Quat::Build(const Vector& v1, const Vector& v2)
{
	if (v1.Len3Squared() > 0 && v2.Len3Squared() > 0)
	{
		Vector u1 = v1, u2 = v2;

		u1.Normalize3();
		u2.Normalize3();

		Vector mid = 0.5 * (u1 + u2);
		Vector axis = u1 % mid;

		*this = Quat(axis.X(), axis.Y(), axis.Z(), u1*mid);

		if (IsZero4())
		{
			if (Abs(u1.Z()) > 0.5f)
			{
				*this = Quat(u1.Z(), 0.f, -u1.X(), 0.f);
			}
			else
			{
				*this = Quat(u1.Y(), -u1.X(), 0.f, 0.f);
			}
		}

		Normalize4();
	}
	else
	{
		*this = Quat(0, 0, 0, 1);
	}
}

void Quat::Build(const Matrix& R)
{
	float32 trace = R(0,0) + R(1,1) + R(2,2);
    
	if (trace > 0)
	{
		X() = R(2,1) - R(1,2);
		Y() = R(0,2) - R(2,0);
		Z() = R(1,0) - R(0,1);
		W() = 1 + trace;
	}
	else
	{
		float32 mx = R(0,0);
		uint32 ix = 0;
	    
		if (R(1,1) > mx)
		{
			mx = R(1,1);
			ix = 1;
		}

		if (R(2,2) > mx)
		{
			mx = R(2,2);
			ix = 2;
		}

		switch ( ix )
		{

			case 0:

				X() = 1 + R(0,0) - R(1,1) - R(2,2);
				Y() = R(1,0) + R(0,1);
				Z() = R(2,0) + R(0,2);
				W() = R(2,1) - R(1,2);
				break;

			case 1:

				X() = R(0,1) + R(1,0);
				Y() = 1 - R(0,0) + R(1,1) - R(2,2);
				Z() = R(2,1) + R(1,2);
				W() = R(0,2) - R(2,0);
				break;

			case 2:

				X() = R(0,2) + R(2,0);
				Y() = R(1,2) + R(2,1);
				Z() = 1 - R(0,0) - R(1,1) + R(2,2);
				W() = R(1,0) - R(0,1);
				break;
		}
	}
    
	Normalize4();
}

void Quat::ConvertToAxisAngle(Vector* axis, float32* angle) const
{
	float32 halfAngle = acosf( W() );
	float32 s = sinf( halfAngle );

	*angle = 2 * halfAngle;

	if (s == 0)
	{
		*axis = Vector(0, 0, 0);
		return;
	}

	*axis = Vector(*this) / s;
}

bool Vector::IsZero3() const
{
	return ( X() < Vector_ZERO_EPSILON && X() > -Vector_ZERO_EPSILON &&
			 Y() < Vector_ZERO_EPSILON && Y() > -Vector_ZERO_EPSILON &&
			 Z() < Vector_ZERO_EPSILON && Z() > -Vector_ZERO_EPSILON );
}
bool Vector::IsZero4() const
{
	return ( IsZero3() && W() < Vector_ZERO_EPSILON && W() > -Vector_ZERO_EPSILON );
}

void Euler::Set(const Matrix& mat)
{
	const float *m = (float*) &mat;		// Make sure that the Matrix elements are up to date. (for Win32 version)

	float cy = Sqrt(m[0]*m[0] + m[1]*m[1]);
	if (cy > EPS_FLOAT*16)
	{
		Set( atan2f(m[6], m[10]), atan2f(-m[2], cy), atan2f(m[1], m[0]) );
	}
	else
	{
		Set( atan2f(-m[9], m[5]), atan2f(-m[2], cy), 0 );
	}
}

void Euler::Set(float32 x, float32 y, float32 z)
{
	Vector::Set(x, y, z);
}

Vector Mul3x4(const Matrix& a, const Vector& b)		
{
	__m128 b4 = b.Vec4();
	__m128 c1x = _mm_mul_ps(a.v0.Vec4(), _mm_shuffle_ps(b4, b4, _MM_SHUFFLE(0, 0, 0, 0)));
	__m128 c2y = _mm_mul_ps(a.v1.Vec4(), _mm_shuffle_ps(b4, b4, _MM_SHUFFLE(1, 1, 1, 1)));
	__m128 c3z = _mm_mul_ps(a.v2.Vec4(), _mm_shuffle_ps(b4, b4, _MM_SHUFFLE(2, 2, 2, 2)));
	return Vector(_mm_add_ps(c1x, _mm_add_ps(c2y, _mm_add_ps(c3z, a.v3.Vec4()))));
}

#if 0

float32 Det4x4(const Matrix& a)
{
	static const char type[] = "0101^0101^0101^^1010^1010^1010";
	static const char perm[] = "123412314231243121342132413214321";

	float32 det = 0.0f;

	for (int32 i=0; type[i]; i++)
	{
		switch ( type[i] )
		{
			case '0':

				// Even permutation
				det += a(0, perm[i+0] - '0')
					 * a(1, perm[i+1] - '0')
					 * a(2, perm[i+2] - '0')
					 * a(3, perm[i+3] - '0');
				break;

			case '1':

				// Odd permutation
				det -= a(0, perm[i+0] - '0')
					 * a(1, perm[i+1] - '0')
					 * a(2, perm[i+2] - '0')
					 * a(3, perm[i+3] - '0');
				break;
		}
	}

	return det;
}

#endif

static float32 Cofactor(const Matrix& m, int32 row, int32 col)
{
	int32 r[3];
	int32 c[3];

	int32 rowCount = 0;
	int32 colCount = 0;

	for (int32 t = 0; t < 4; t++)
	{
		if (row != t) r[rowCount++] = t;
		if (col != t) c[colCount++] = t;
	}

	float32 det = m( r[0], c[0] ) * ( m(r[1], c[1]) * m(r[2], c[2]) - m(r[2], c[1]) * m(r[1], c[2]) )
		        - m( r[0], c[1] ) * ( m(r[1], c[2]) * m(r[2], c[0]) - m(r[2], c[2]) * m(r[1], c[0]) )
		        + m( r[0], c[2] ) * ( m(r[1], c[0]) * m(r[2], c[1]) - m(r[2], c[0]) * m(r[1], c[1]) );

	if ( (row + col) & 1 )
	{
		return -det;
	}

	return det;
}

void GeneralInverse4x4(Matrix* out, const Matrix& m)
{
	// Compute the adjoint of the input matrix
	Matrix adj;
	for (int32 i=0; i < 4; i++)
	{
		for (int32 j=0; j < 4; j++)
		{
			adj(j,i) = Cofactor(m, i, j);
		}
	}

	// Compute the determinant of the input matrix
	float32 det = m(0, 0) * adj(0, 0) 
		        - m(0, 1) * adj(1, 0)
		        + m(0, 2) * adj(2, 0)
		        - m(0, 3) * adj(3, 0);

	float32 invDet = 1.0f / det;

	// Convert the adjoint matrix into the inverse matrix
	adj.v0 *= invDet;
	adj.v1 *= invDet;
	adj.v2 *= invDet;
	adj.v3 *= invDet;

	// Return the result
	*out = adj;
}

void Matrix::Build(const Quat& q)
{
	float32 x = q.X();
	float32 y = q.Y();
	float32 z = q.Z();
	float32 w = q.W();

	float32 xx = x * x;
	float32 xy = x * y;
	float32 xz = x * z;

	float32 yy = y * y;
	float32 yz = y * z;

	float32 zz = z * z;

	float32 wx = w * x;
	float32 wy = w * y;
	float32 wz = w * z;
	
	v0.Set( 1 - 2 * (yy + zz),     2 * (xy + wz),     2 * (xz - wy),       0 );
	v1.Set(     2 * (xy - wz), 1 - 2 * (xx + zz),     2 * (yz + wx),       0 );
	v2.Set(     2 * (xz + wy),     2 * (yz - wx), 1 - 2 * (xx + yy),       0 );
	v3.Set(                 0,                 0,                 0,       1 );
}

void Matrix::Build(const Euler& euler)
{
	float32 cx = cosf(euler.X());
	float32 cy = cosf(euler.Y());
	float32 cz = cosf(euler.Z());

	float32 sx = sinf(euler.X());
	float32 sy = sinf(euler.Y());
	float32 sz = sinf(euler.Z());

	v0.Set(            cy*cz,            cy*sz,        -sy,     0 );
	v1.Set( sx*sy*cz - cx*sz, sx*sy*sz + cx*cz,      sx*cy,     0 );
	v2.Set( cx*sy*cz + sx*sz, cx*sy*sz - sx*cz,      cx*cy,     0 );
	v3.Set(                0,                0,          0,     1 );
}

void Quat::Build(const Vector& axis, float angle)
{
	ASSERT( axis.CanNormalize3() );

	float32 s = sinf(0.5f * angle);
	float32 c = cosf(0.5f * angle);

	vec4 = (s * axis.UnitVec3()).Vec4();
	W() = c;
}

void Vector::Normalize3()
{
	ASSERT(CanNormalize3());	
	__m128 xyzw2 = _mm_mul_ps(vec4, vec4);
	*this *= InvSqrt(_mm_add_ss(xyzw2, _mm_add_ss(L_SHUF_PASS(COMPONENT_Y), L_SHUF_PASS(COMPONENT_Z))).m128_f32[COMPONENT_X]);
}

void Vector::Normalize4()
{
	ASSERT(CanNormalize4());
	__m128 xyzw2 = _mm_mul_ps(vec4, vec4);
	*this *= InvSqrt(_mm_add_ss(xyzw2, _mm_add_ss(L_SHUF_PASS(COMPONENT_Y), _mm_add_ss(L_SHUF_PASS(COMPONENT_Z), L_SHUF_PASS(COMPONENT_W)))).m128_f32[COMPONENT_X]);
}

Vector Vector::UnitVec3() const
{
	Vector b(*this);
	b.Normalize3();
	return b;
}

bool Vector64::CanNormalize3() const
{
	float64 d = *this * *this;
	if (d > 0.00000000000000001) return true;
	return false;
}

void Vector64::Normalize3()
{
	float64 dd = *this * *this;
	float64 d = sqrt(dd);
	*this = *this * (1.0 / d);
}

float64 Vector64::Len3() const
{
	return sqrt( *this * *this );
}