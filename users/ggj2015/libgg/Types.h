#pragma once

#include <math.h>
#include "Debugging.h"
#include "Exception.h"
#include "TypesBase.h"

#ifndef PSP
#define USE_PSPVEC 0
#endif

#if defined(IPHONEOS)
#include <CoreGraphics/CGGeometry.h>
#define PACKED __attribute__ ((packed))
#elif defined(MACOS)
#define PACKED __attribute__ ((packed))
#elif defined(WIN32)
#define PACKED
#elif defined(PSP)
#define PACKED __attribute__ ((packed))
#elif defined(BBOS)
#define PACKED __attribute__ ((packed))
#endif

#if USE_PSPVEC
#include <libvfpu.h>
typedef int V4 __attribute__((mode(VF), aligned(16)));
#endif

// --------------------

template <class T>
class BasePoint
{
public:
	inline BasePoint()
	{
		x = y = (T)0;
	}

	inline BasePoint(T _x, T _y)
	{
		x = _x;
		y = _y;
	}

	inline BasePoint<T> Add(const BasePoint<T>& p) const
	{
		return BasePoint(m_V[0] + p[0], m_V[1] + p[1]);
	}

	inline T& operator[](int index)
	{
#ifdef DEBUG
		if (index < 0 || index > 1)
			throw ExceptionVA("index out of range");
#endif
		
		return m_V[index];
	}

	inline T operator[](int index) const
	{
#ifdef DEBUG
		if (index < 0 || index > 1)
			throw ExceptionVA("index out of range");
#endif
		
		return m_V[index];
	}

#if 1
	inline BasePoint<float> UnitDelta(const BasePoint<float>& point) const
	{
		const float dx = point.x - x;
		const float dy = point.y - y;

		const float ds = sqrtf(dx * dx + dy * dy);

		return BasePoint<float>(dx / ds, dy / ds);
	}

	inline float Distance(const BasePoint<float>& point) const
	{
		const float dx = point.x - x;
		const float dy = point.y - y;

		return sqrtf(dx * dx + dy * dy);
	}

	inline T DistanceSq(const BasePoint<T>& point) const
	{
		const T dx = point.x - x;
		const T dy = point.y - y;

		return dx * dx + dy * dy;
	}

	inline BasePoint<T> Subtract(const BasePoint<T>& point) const
	{
		return BasePoint<T>(x - point.x, y - point.y);
	}

	inline float Dot(const BasePoint<float>& point) const
	{
		return x * point.x + y * point.y;
	}
	
	inline BasePoint<T> Scale(T s) const
	{
		return BasePoint<T>(x * s, y * s);
	}
#endif
	
	union
	{
		struct
		{
			T x;
			T y;
		};
		
		T m_V[2];
	};
};

// --------------------

typedef BasePoint<int> PointI;
typedef BasePoint<float> PointF;

// --------------------

template<typename T>
class BaseVec
{
public:
	inline BaseVec() :
		x(0), y(0)
	{
	}

	inline BaseVec(T _x, T _y) :
		x(_x), y(_y)
	{
	}
	
	inline BaseVec(const BasePoint<T>& p)
	{
		m_V[0] = p.x;
		m_V[1] = p.y;
	}
	
#ifdef IPHONEOS
	inline BaseVec(const CGPoint& p)
	{
		m_V[0] = (T)p.x;
		m_V[1] = (T)p.y;
	}
	
	inline BaseVec(const CGSize& s)
	{
		m_V[0] = (T)s.width;
		m_V[1] = (T)s.height;
	}
#endif

#if USE_PSPVEC
	inline BaseVec(const V4& _v)
	{
		v4 = _v;
	}
#endif

	void SetZero()
	{
		m_V[0] = (T)0;
		m_V[1] = (T)0;
	}
	
	void Set(T _x, T _y)
	{
		m_V[0] = _x;
		m_V[1] = _y;
	}

	inline T LengthSq_get() const
	{
		return
			m_V[0] * m_V[0] +
			m_V[1] * m_V[1];
	}

	inline T Length_get() const
	{
		return (T)sqrtf((float)LengthSq_get());
	}

	inline void Normalize()
	{
		const float length = (float)Length_get();
		
		if (length == 0.0f)
		{
			m_V[0] = (T)1;
			m_V[1] = (T)0;
			return;
		}
		
		const float lengthRcp = 1.0f / length;

		m_V[0] *= lengthRcp;
		m_V[1] *= lengthRcp;
	}
	
	inline BaseVec<T> Delta(const BaseVec<T>& v) const
	{
		return v - *this;
	}
	
	inline BaseVec<float> UnitDelta(const BaseVec<float>& v) const
	{
		BaseVec<float> delta = v - *this;
		
		delta.Normalize();
		
		return delta;
	}
	
	inline BaseVec<float> Normal() const
	{
		BaseVec<float> v = *this;
		
		v.Normalize();
		
		return v;
	}

	inline float Distance(const BaseVec<float>& v) const
	{
		BaseVec<float> delta = v - *this;
		
		return delta.Length_get();
	}
	
	inline float DistanceTo(const BaseVec<float>& v) const
	{
		return Distance(v);
	}

	inline T DistanceSq(const BaseVec<T>& v) const
	{
		BaseVec<T> delta = v - *this;
		
		return delta.LengthSq_get();
	}
	
	inline const BaseVec<float> DirectionTo(const BaseVec<float>& v) const
	{
		float dx = v.m_V[0] - m_V[0];
		float dy = v.m_V[1] - m_V[1];

		float distance = sqrtf(dx * dx + dy * dy);

		if (distance == 0.0f)
			return BaseVec<float>();

		return BaseVec<float>(dx / distance, dy / distance);
	}

	inline BaseVec<T> LerpTo(const BaseVec<T>& v, float t) const
	{
		BaseVec<T> delta = v - *this;
		
		return *this + delta * t;
	}
	
	inline float ToAngle() const
	{
#if USE_PSPVEC
		return sceVfpuScalarAtan2(m_V[1], m_V[0]);
#else
		if (m_V[0] == 0.0f && m_V[1] == 0.0f)
			return 0.0f;
		
		return atan2f(m_V[1], m_V[0]);
#endif
	}

	inline static float ToAngle(const BaseVec<float>& v)
	{
		return v.ToAngle();
	}
	
	inline static BaseVec<float> FromAngle(float angle)
	{
#if USE_PSPVEC
		return BaseVec<float>(sceVfpuScalarCos(angle), sceVfpuScalarSin(angle));
#else
		return BaseVec<float>(cosf(angle), sinf(angle));
#endif
	}
	
	inline static float AngleBetween(const BaseVec<float>& v1, const BaseVec<float>& v2)
	{	
#if USE_PSPVEC
		ScePspFVector2 delta;
		sceVfpuVector2Sub(&delta, (const ScePspFVector2*)(&v1), (const ScePspFVector2*)(&v2));
		return sceVfpuScalarAtan2(delta[0], delta[1]);
#else
		const BaseVec<float> delta = v2 - v1;

		return atan2f(delta[1], delta[0]);
#endif
	}
	

	// From BasePoint<T>
	
	inline BaseVec<T> Add(const BaseVec<T>& p) const
	{
		return BaseVec(m_V[0] + p[0], m_V[1] + p[1]);
	}

	inline BaseVec<T> Subtract(const BaseVec<T>& point) const
	{
		return BaseVec<T>(x - point.x, y - point.y);
	}

	inline float Dot(const BaseVec<float>& point) const
	{
#if USE_PSPVEC
		return sceVfpuVector2InnerProduct((ScePspFVector2*)(this), (ScePspFVector2*)(&point));
#else
		return x * point.x + y * point.y;
#endif
	}

	inline BaseVec<T> Scale(T s) const
	{
		return BaseVec<T>(x * s, y * s);
	}

	// --------------------
	// Operators
	// --------------------
	
	inline T operator*(const BaseVec<T>& v) const
	{
		return
			m_V[0] * v.m_V[0] +
			m_V[1] * v.m_V[1];
	}

	inline T& operator[](int index)
	{
#ifdef DEBUG
		if (index < 0 || index > 1)
			throw ExceptionVA("index out of range");
#endif
		
		return m_V[index];
	}

	inline const T& operator[](int index) const
	{
#ifdef DEBUG
		if (index < 0 || index > 1)
			throw ExceptionVA("index out of range");
#endif
		
		return m_V[index];
	}

	inline BaseVec<T> operator+(const BaseVec<T>& v) const
	{
		return BaseVec<T>(
			m_V[0] + v.m_V[0],
			m_V[1] + v.m_V[1]);
	}

	inline BaseVec<T> operator-(const BaseVec<T>& v) const
	{
		return BaseVec<T>(
			m_V[0] - v.m_V[0],
			m_V[1] - v.m_V[1]);
	}
	
	inline BaseVec<T> operator+(T v) const
	{
		return BaseVec<T>(
			m_V[0] + v,
			m_V[1] + v);
	}
	
	inline BaseVec<T> operator-(T v) const
	{
		return BaseVec<T>(
			m_V[0] - v,
			m_V[1] - v);
	}
	
	
	inline BaseVec<T> operator*(T v) const
	{
		return BaseVec<T>(
			m_V[0] * v,
			m_V[1] * v);
	}

	inline BaseVec<T> operator/(T v) const
	{
		return BaseVec<T>(
			m_V[0] / v,
			m_V[1] / v);
	}
	
	inline BaseVec<T> operator^(const BaseVec<T>& other) const
	{
		return BaseVec<T>(m_V[0] * other[0], m_V[1] * other[1]);
	}

	inline void operator-=(const BaseVec<T>& other)
	{
		m_V[0] -= other[0];
		m_V[1] -= other[1];
	}

	inline void operator+=(const BaseVec<T>& other)
	{
		m_V[0] += other[0];
		m_V[1] += other[1];
	}

	inline void operator*=(T v)
	{
		m_V[0] *= v;
		m_V[1] *= v;
	}
	
	inline void operator/=(T v)
	{
		m_V[0] /= v;
		m_V[1] /= v;
	}
	
	inline BaseVec<T> operator-() const
	{
		return BaseVec<T>(
			-m_V[0],
			-m_V[1]);
	}
	
	inline const BaseVec<T>& operator+() const
	{
		return *this;
	}

	inline BaseVec<float> ToF() const
	{
		return BaseVec<float>(
			(float)m_V[0],
			(float)m_V[1]);
	}
	
#ifdef IPHONEOS
	CGPoint ToCgPoint() const
	{
		return CGPointMake(x, y);
	}
#endif

	union
	{
		T m_V[2];
		struct
		{
			T m_X;
			T m_Y;
		};
		struct
		{
			T x;
			T y;
		};
#if USE_PSPVEC
		V4 v4;
#endif
	};
};

template <>
inline BaseVec<float> BaseVec<float>::ToF() const
{
	return *this;
}

// --------------------

typedef BaseVec<int> Vec2I;
typedef BaseVec<float> Vec2F;

// --------------------
// PSP optimized Vec2F

#if USE_PSPVEC && 0

#define BV BaseVec<float>

inline BV::BaseVec()
{
	_asm
	{
		vzero.p v4;
	};
}

inline void BV::SetZero()
{
	_asm
	{
		vzero.p v4;
	};
}

inline float BV::LengthSq_get() const
{
	float result;
	_asm
	{
		vdot.p s100, v4, v4;
		sv.s s100, &result;
	};
	return result;
}

inline float BV::Length_get() const
{
	float result;
	_asm
	{
		vdot.p s100, v4, v4;
		vsqrt.s s100, s100;
		sv.s s100, &result;
	};
	return result;
}

inline void BV::Normalize()
{
	_asm
	{
		vdot.p s100, v4, v4;
		vzero.s s101;
		vcmp.s EZ, s100;
		vone.s s102;
		vrsq.s s100, s100;
		vscl.p c000[-1:1,-1:1], v4, s100;
		vcmovt.s s000, s101, 0
		vcmovt.s s001, s102, 0
		sv.p c000, &v4;
	};
}
	
inline BV BV::Delta(const BV& v) const
{
	V4 result;
	_asm
	{
		vsub.p result, v4, v.v4;
	};
	return BV(result);
}

inline float BV::Distance(const BV& v) const
{
	float result;
	_asm
	{
		vsub.p c000, v4, v.v4;
		vdot.p s100, c000, c000;
		vsqrt.s s100, s100;
		sv.s s100, &result;
	};
	return result;
}

inline float BV::DistanceSq(const BV& v) const
{
	float result;
	_asm
	{
		vsub.p c000, v4, v.v4;
		vdot.p s100, c000, c000;
		sv.s s100, &result;
	};
	return result;
}

inline BV BV::LerpTo(const BV& v, float t) const
{
#if 1
	BV delta = v - *this;
		
	return *this + delta * t;
#else
	V4 result;
	_asm
	{
		lv.s s000, &t;
		vsub.p c010, v.v4, v4;
		vscl.p c020, c010, s000;
		vadd.p c030, c020, v4;
		sv.p c030, &result;
	};
	return BV(result);
#endif
}

inline float BV::ToAngle() const
{
	return sceVfpuScalarAtan2(m_V[1], m_V[0]);
	//return atan2f(m_V[1], m_V[0]);
}
	
inline BV BV::FromAngle(float angle)
{
	return BV(sceVfpuScalarCos(angle), sceVfpuScalarSin(angle));
	//return Vec2F4(cosf(angle), sinf(angle));
}
	
inline float BV::AngleBetween(const BV& v1, const BV& v2)
{	
	ScePspFVector2 delta;
	sceVfpuVector2Sub(&delta, (const ScePspFVector2*)(&v1), (const ScePspFVector2*)(&v2));
	return sceVfpuScalarAtan2(delta.x, delta.y);
}	

// From BasePoint<float>

inline BV BV::Add(const BV& p) const
{
	V4 result;
	_asm
	{
		vadd.p result, v4, p.v4;
	};
	return BV(result);
}

inline BV BV::Subtract(const BV& point) const
{
	V4 result;
	_asm
	{
		vsub.p result, v4, point.v4;
	};
	return BV(result);
}

inline float BV::Dot(const BV& point) const
{
	float result;
	_asm
	{
		vdot.p s100, v4, point.v4;
		sv.s s100, &result;
	};
	return result;
}

inline BV BV::Scale(float s) const
{
	V4 result;
	_asm
	{
		lv.s s100, &s;
		vscl.p c000, v4, s100;
		sv.p c000, &result;
	};
	return BV(result);
}

// --------------------
// Operators
// --------------------

inline BV BV::operator+(float v) const
{
	V4 result;
	_asm
	{
		lv.s s100, &v;
		lv.s s101, &v;
		vadd.p result, v4, c100;
	};
	return BV(result);
}

inline BV BV::operator-(float v) const
{
	V4 result;
	_asm
	{
		lv.s s100, &v;
		lv.s s101, &v;
		vsub.p result, v4, c100;
	};
	return BV(result);
}

inline BV BV::operator*(float v) const
{
	return Scale(v);
}

inline BV BV::operator/(float v) const
{
#if 1
	return BV(m_V[0] / v, m_V[1] / v);
#else
	V4 result;
	_asm
	{
		lv.s s010, &v;
		lv.s s011, &v;
		vdiv.p c000, v4, c010;
		sv.p c000, &result;
	};
	return Vec2F4(result);
#endif
}

inline BV BV::operator^(const BV& other) const
{
	V4 result;
	_asm
	{
		vmul.p result, v4, other.v4;
	};
	return BV(result);
}

inline void BV::operator-=(const BV& other)
{
	_asm
	{
		vsub.p v4, v4, other.v4;
	};
}

inline void BV::operator+=(const BV& other)
{
	_asm
	{
		vadd.p v4, v4, other.v4;
	};
}

inline void BV::operator*=(float v)
{
	_asm
	{
		lv.s s100, &v;
		vscl.p v4, v4, s100;
	};
}

inline void BV::operator/=(float v)
{
	_asm
	{
		lv.s s100, &v;
		vrcp.s s100, s100;
		vscl.p v4, v4, s100;
	};
}

inline BV BV::operator-() const
{
	V4 result;
	_asm
	{
		vneg.p result, v4;
	};
	return BV(result);
}

inline const BV& BV::operator+() const
{
	return *this;
}

#endif

template <typename T>
class BaseRect
{
public:
	BaseRect()
	{
	}

	BaseRect(const BaseVec<T>& position, const BaseVec<T>& size)
	{
		Setup(position, size);
	}

	void Setup(const BaseVec<T>& position, const BaseVec<T>& size)
	{
		m_Position = position;
		m_Size = size;
	}

	bool IsInside(const BaseVec<T>& p) const
	{
		for (int i = 0; i < 2; ++i)
		{
			if (p[i] < m_Position[i])
				return false;
			if (p[i] > m_Position[i] + m_Size[i])
				return false;
		}

		return true;
	}

	inline T Width_get() const
	{
		return m_Size[0];
	}

	inline T Height_get() const
	{
		return m_Size[1];
	}

	inline BaseVec<T> Min_get() const
	{
		return m_Position;
	}

	inline BaseVec<T> Max_get() const
	{
		return m_Position + m_Size;
	}
	
	inline T Area_get() const
	{
		return m_Size[0] * m_Size[1];
	}
	
	inline BaseVec<T> Clamp(BaseVec<T> v) const
	{
		if (v[0] < m_Position[0])
			v[0] = m_Position[0];
		if (v[1] < m_Position[1])
			v[1] = m_Position[1];
		if (v[0] > m_Position[0] + m_Size[0])
			v[0] = m_Position[0] + m_Size[0];
		if (v[1] > m_Position[1] + m_Size[1])
			v[1] = m_Position[1] + m_Size[1];
		
		return v;
	}

	//
	
	BaseVec<T> m_Position;
	BaseVec<T> m_Size;
};

typedef BaseRect<int> RectI;
typedef BaseRect<float> RectF;

Vec2F Intersect_Rect(const RectF& rect, const Vec2F& pos, const Vec2F& dir, float& out_T);

#if USE_PSPVEC
inline BaseVec<float> BaseRect<float>::Clamp(BaseVec<float> v) const
{
	v[0] = sceVfpuScalarMin(sceVfpuScalarMax(v[0], m_Position[0]), m_Position[0] + m_Size[0]);
	v[1] = sceVfpuScalarMin(sceVfpuScalarMax(v[1], m_Position[1]), m_Position[1] + m_Size[1]);
	return v;
}
#endif

// --------------------

#define AREA_UNSET -1337

template <class T>
class BaseArea
{
public:
	inline BaseArea()
	{
		m_Min[0] = AREA_UNSET;
#ifdef DEBUG
		m_Min[1] = AREA_UNSET;
		m_Max[0] = AREA_UNSET;
		m_Max[1] = AREA_UNSET;
#endif
	}

	inline BaseArea(const BaseVec<T>& min, const BaseVec<T>& max)
	{
		//Assert(min[0] <= max[0]);
		//Assert(min[1] <= max[1]);
		
		m_Min[0] = min[0];
		m_Min[1] = min[1];
		m_Max[0] = max[0];
		m_Max[1] = max[1];
	}

	inline void Reset()
	{
		m_Min[0] = AREA_UNSET;
#ifdef DEBUG
		m_Min[1] = AREA_UNSET;
		m_Max[0] = AREA_UNSET;
		m_Max[1] = AREA_UNSET;
#endif
	}
	
	inline void Merge(const BaseArea<T>& v)
	{
		Assert(v.IsSet_get());
		
		if (IsSet_get())
		{
			// merge
			
			for (int i = 0; i < 2; ++i)
			{
				if (v.m_Min[i] < m_Min[i])
					m_Min[i] = v.m_Min[i];
				if (v.m_Max[i] > m_Max[i])
					m_Max[i] = v.m_Max[i];
			}
		}
		else
		{
			// copy
			
			*this = v;
		}
	}

	inline bool Clip(const BaseArea<T>& v)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (m_Min[i] < v.m_Min[i])
				m_Min[i] = v.m_Min[i];
			if (m_Max[i] > v.m_Max[i])
				m_Max[i] = v.m_Max[i];

			if (m_Min[i] > m_Max[i])
				return false;
		}

		return true;
	}

	inline int Area_get() const
	{
		const int sx = m_Max[0] - m_Min[0] + 1;
		const int sy = m_Max[1] - m_Min[1] + 1;

		return sx * sy;
	}

/*	inline bool HasArea_get() const
	{
		const int sx = m_Max[0] - m_Min[0] + 1;
		const int sy = m_Max[1] - m_Min[1] + 1;
		
		return sx >= 1 && sy >= 1;
	}*/
	
	inline bool IsSet_get() const
	{
		return m_Min[0] != AREA_UNSET;
	}
	
	RectI ToRectI() const
	{
		Assert(IsSet_get());
		
		const int x = m_Min[0];
		const int y = m_Min[1];
		const int sx = m_Max[0] - m_Min[0] + 1;
		const int sy = m_Max[1] - m_Min[1] + 1;
		
		return RectI(Vec2I(x, y), Vec2I(sx, sy));
	}
	
	union
	{
		struct
		{
			T m_Min[2];
			T m_Max[2];
		};
		struct
		{
			T x1, y1;
			T x2, y2;
		};
	};
	
//	bool mIsSet;
};

typedef BaseArea<int> AreaI;
typedef BaseArea<float> AreaF;

// --------------------

#define BBOX_UNSET -1000000

class BBoxI
{
public:
	BBoxI()
	{
		Initialize();
	}

	void Initialize()
	{
		m_Min = PointI(BBOX_UNSET, BBOX_UNSET);
		m_Max = PointI(BBOX_UNSET, BBOX_UNSET);
	}

	int IsInside(int x, int y) const
	{
		return 
			x >= m_Min.x &&
			y >= m_Min.y && 
			x <= m_Max.x &&
			y <= m_Max.y;
	}

	void Merge(const BBoxI& bbox)
	{
		if (bbox.m_Min.x == BBOX_UNSET)
			return;

		Merge(bbox.m_Min);
		Merge(bbox.m_Max);
	}

	void Merge(const Vec2I& point)
	{
		if (m_Min.x != BBOX_UNSET)
		{
			if (point.x < m_Min.x)
				m_Min.x = point.x;
			if (point.y < m_Min.y)
				m_Min.y = point.y;
			if (point.x > m_Max.x)
				m_Max.x = point.x;
			if (point.y > m_Max.y)
				m_Max.y = point.y;
		}
		else
		{
			m_Min = point;
			m_Max = point;
		}
	}
	
	void Merge(const RectI& rect)
	{
		Merge(rect.m_Position);
		Merge(PointI(rect.m_Position[0] + rect.m_Size[0] - 1, rect.m_Position[1] + rect.m_Size[1] - 1));
	}

	RectI ToRect() const
	{
		RectI result;

		if (m_Min.x != BBOX_UNSET)
		{
			result.m_Position = m_Min;
			result.m_Size[0] = m_Max[0] - m_Min[0] + 1;
			result.m_Size[1] = m_Max[1] - m_Min[1] + 1;
		}
		else
		{
			result.m_Position = PointI(0, 0);
			result.m_Size = PointI(0, 0);
		}

		return result;
	}

	inline int IsEmpty_get() const
	{
		return m_Min.x == BBOX_UNSET;
	}

	Vec2I m_Min;
	Vec2I m_Max;
};

// --------------------

class PlaneF
{
public:
	inline PlaneF()
	{
		m_Distance = 0.0f;
	}
	
	inline void Set(Vec2F normal, float distance)
	{
		m_Normal = normal;
		m_Distance = distance;
	}
	
	inline static PlaneF FromPoints(const Vec2F& p1, const Vec2F& p2)
	{
		PlaneF result;

		result.m_Normal = p1.UnitDelta(p2);
		result.m_Distance = result.m_Normal.Dot(p1);

		return result;
	}

	inline float Distance(const Vec2F& point) const
	{
		return m_Normal.Dot(point) - m_Distance;
	}

	inline Vec2F ProjectPoint(const Vec2F& point) const
	{
		float distance = Distance(point);

		return point - m_Normal * distance;
	}

	inline PlaneF operator-() const
	{
		PlaneF result;
		result.m_Distance = -m_Distance;
		result.m_Normal = -m_Normal;
		return result;
	}

	Vec2F m_Normal;
	float m_Distance;
};
