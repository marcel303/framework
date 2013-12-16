#pragma once

#include "SIMD.h"

#define RESTRICTED __restrict

typedef int BOOL;
#define TRUE -1
#define FALSE 0

template<typename T>
class BaseVec
{
public:
	inline BaseVec()
	{
	}

	inline BaseVec(T x, T y, T z)
	{
		m_V[0] = x;
		m_V[1] = y;
		m_V[2] = z;
	}

	void SetZero()
	{
		m_V[0] = (T)0;
		m_V[1] = (T)0;
		m_V[2] = (T)0;
	}

	inline T LengthSq_get() const
	{
		return
			m_V[0] * m_V[0] +
			m_V[1] * m_V[1] +
			m_V[2] * m_V[2];
	}

	inline T Length_get() const
	{
		return sqrt(LengthSq_get());
	}

	inline void Normalize()
	{
		const T lengthRcp = 1.0f / Length_get();

		m_V[0] *= lengthRcp;
		m_V[1] *= lengthRcp;
		m_V[2] *= lengthRcp;
	}

	inline T operator*(const BaseVec<T>& v) const
	{
		return
			m_V[0] * v.m_V[0] +
			m_V[1] * v.m_V[1] +
			m_V[2] * v.m_V[2];
	}

	inline T& operator[](int index)
	{
		return m_V[index];
	}

	inline const T& operator[](int index) const
	{
		return m_V[index];
	}

	inline BaseVec<T> operator+(const BaseVec<T>& v) const
	{
		return BaseVec<T>(
			m_V[0] + v.m_V[0],
			m_V[1] + v.m_V[1],
			m_V[2] + v.m_V[2]);
	}

	inline BaseVec<T> operator-(const BaseVec<T>& v) const
	{
		return BaseVec<T>(
			m_V[0] - v.m_V[0],
			m_V[1] - v.m_V[1],
			m_V[2] - v.m_V[2]);
	}

	inline BaseVec<T> operator*(T v) const
	{
		return BaseVec<T>(
			m_V[0] * v,
			m_V[1] * v,
			m_V[2] * v);
	}

	inline BaseVec<T> operator/(T v) const
	{
		return BaseVec<T>(
			m_V[0] / v,
			m_V[1] / v,
			m_V[2] / v);
	}

	inline BaseVec<float> ToF() const
	{
		return BaseVec<float>(
			m_V[0],
			m_V[1],
			m_V[2]);
	}

	union
	{
		T m_V[4];
		struct
		{
			T m_X;
			T m_Y;
			T m_Z;
		};
	};
};

typedef BaseVec<int> VecI;
typedef BaseVec<float> VecF;

template<typename T>
class BasePlane
{
public:
	inline BasePlane()
	{
		m_Normal.SetZero();
		m_Distance = (T)0;
	}

	inline BasePlane(const BaseVec<T>& normal, T distance)
	{
		Initialize(normal, distance);
	}

	inline void Initialize(const BaseVec<T>& normal, T distance)
	{
		m_Normal = normal;
		m_Distance = distance;
	}

	inline T operator*(const BaseVec<T>& v) const
	{
		return v * m_Normal - m_Distance;
	}

	BaseVec<T> m_Normal;
	T m_Distance;
};

typedef BasePlane<int> PlaneI;
//typedef BasePlane<float> PlaneF;

class PlaneF
{
public:
	inline PlaneF()
	{
		m_Normal = m_Distance = SimdVec(VZERO);
	}

	inline SimdVec operator * (SimdVecArg v) const
	{
		SimdVec d = m_Normal.Dot3(v).Sub(m_Distance);
		return d;
		//return d.X();
	}

	SimdVec m_Normal;
	SimdVec m_Distance;
};

template <typename T>
class BaseSphere
{
public:
	void Initialize(
		BaseVec<T> pos,
		T radius)
	{
		m_Pos = pos;
		m_Radius = radius;
		m_RadiusSq = radius * radius;
	}

	BaseVec<T> m_Pos;
	T m_Radius;
	T m_RadiusSq;
};

typedef BaseSphere<float> SphereF;
typedef BaseSphere<int> SphereI;

// ASM stuff

#if 0

static _inline void fcossin (float a, float *c, float *s)
{
	_asm
	{
		fld a
		fsincos
		mov eax, c
		fstp dword ptr [eax]
		mov eax, s
		fstp dword ptr [eax]
	}
}

static _inline void dcossin (double a, double *c, double *s)
{
	_asm
	{
		fld a
		fsincos
		mov eax, c
		fstp qword ptr [eax]
		mov eax, s
		fstp qword ptr [eax]
	}
}

static _inline void ftol (float f, long *a)
{
	_asm
	{
		mov eax, a
		fld f
		fistp dword ptr [eax]
	}
}

static _inline void dtol (double d, long *a)
{
	_asm
	{
		mov eax, a
		fld qword ptr d
		fistp dword ptr [eax]
	}
}

	//WARNING: This ASM code requires >= PPRO
static _inline double dbound (double d, double dmin, double dmax)
{
	_asm
	{
		fld dmin
		fld d
		fucomi st, st(1)   ;if (d < dmin)
		fcmovb st, st(1)   ;    d = dmin;
		fld dmax
		fxch st(1)
		fucomi st, st(1)   ;if (d > dmax)
		fcmovnb st, st(1)  ;    d = dmax;
		fstp d
		fucompp
	}
	return(d);
}

static _inline long mulshr16 (long a, long d)
{
	_asm
	{
		mov eax, a
		mov edx, d
		imul edx
		shrd eax, edx, 16
	}
}

static _inline void copybuf (void *s, void *d, long c)
{
	_asm
	{
		push esi
		push edi
		mov esi, s
		mov edi, d
		mov ecx, c
		rep movsd
		pop edi
		pop esi
	}
}

static _inline void clearbuf (void *d, long c, long a)
{
	_asm
	{
		push edi
		mov edi, d
		mov ecx, c
		mov eax, a
		rep stosd
		pop edi
	}
}

#endif
