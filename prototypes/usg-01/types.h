#pragma once

#include <assert.h>
#include <math.h>
#include <vector>

#define dt (1.0f / 60.0f) // fixme, remove

typedef int BOOL;
#define TRUE -1
#define FALSE 0

#define RAD2ALLEG(x) ((x) / M_PI * 128.0f)

#define RANDOM(min, max) (min + (max - min) * ((rand() & 4093) / 4093.0f))

typedef char Int8;

#if 0
class Vec2
{
public:
	Vec2()
	{
	}

	Vec2(const Vec2& other)
	{
		v[0] = other[0];
		v[1] = other[1];
	}

	Vec2(float x, float y)
	{
		v[0] = x;
		v[1] = y;
	}

	inline Vec2& operator=(const Vec2& other)
	{
		v[0] = other[0];
		v[1] = other[1];

		return *this;
	}

	inline float& operator[](int index)
	{
		return v[index];
	}

	inline float operator[](int index) const
	{
		return v[index];
	}

	inline Vec2 operator-(const Vec2& other) const
	{
		return Vec2(
			v[0] - other[0],
			v[1] - other[1]);
	}

	inline Vec2 operator+(const Vec2& other) const
	{
		return Vec2(
			v[0] + other[0],
			v[1] + other[1]);
	}

	inline void operator-=(const Vec2& other)
	{
		v[0] -= other[0];
		v[1] -= other[1];
	}

	inline void operator+=(const Vec2& other)
	{
		v[0] += other[0];
		v[1] += other[1];
	}

	inline void operator/=(float s)
	{
		v[0] /= s;
		v[1] /= s;
	}

	inline Vec2 operator*(float s) const
	{
		return Vec2(
			v[0] * s,
			v[1] * s);
	}

	inline void operator*=(float s)
	{
		v[0] *= s;
		v[1] *= s;
	}

	inline Vec2 operator/(float s) const
	{
		return Vec2(
			v[0] / s,
			v[1] / s);
	}

	inline float operator*(const Vec2& other) const
	{
		return
			v[0] * other[0] +
			v[1] * other[1];
	}

	inline Vec2 operator-() const
	{
		return Vec2(
			-v[0],
			-v[1]);
	}

	inline float LengthSq_get() const
	{
		return v[0] * v[0] + v[1] * v[1];
	}

	inline float Length_get() const
	{
		float lengthSq = LengthSq_get();

		return sqrt(lengthSq);
	}

	inline void Normalize()
	{
		float length = Length_get();

		if (length == 0.0f)
		{
			v[0] = 1.0f;
			v[1] = 0.0f;
			return;
		}

		v[0] /= length;
		v[1] /= length;
	}

	float v[2];
};

#endif

#if 0
class Vec3
{
public:
	Vec3()
	{
	}

	Vec3(const Vec3& other)
	{
		v[0] = other[0];
		v[1] = other[1];
		v[2] = other[2];
	}

	Vec3(float x, float y, float z)
	{
		v[0] = x;
		v[1] = y;
		v[2] = z;
	}

	inline Vec3& operator=(const Vec3& other)
	{
		v[0] = other[0];
		v[1] = other[1];
		v[2] = other[2];

		return *this;
	}

	inline float& operator[](int index)
	{
		return v[index];
	}

	inline float operator[](int index) const
	{
		return v[index];
	}

	inline Vec3 operator-(const Vec3& other) const
	{
		return Vec3(
			v[0] - other[0],
			v[1] - other[1],
			v[2] - other[2]);
	}

	inline Vec3 operator+(const Vec3& other) const
	{
		return Vec3(
			v[0] + other[0],
			v[1] + other[1],
			v[2] + other[2]);
	}

	inline void operator-=(const Vec3& other)
	{
		v[0] -= other[0];
		v[1] -= other[1];
		v[2] -= other[2];
	}

	inline void operator+=(const Vec3& other)
	{
		v[0] += other[0];
		v[1] += other[1];
		v[2] += other[2];
	}

	inline void operator/=(float s)
	{
		v[0] /= s;
		v[1] /= s;
		v[2] /= s;
	}

	inline Vec3 operator*(float s) const
	{
		return Vec3(
			v[0] * s,
			v[1] * s,
			v[2] * s);
	}

	inline void operator*=(float s)
	{
		v[0] *= s;
		v[1] *= s;
		v[2] *= s;
	}

	inline Vec3 operator/(float s) const
	{
		return Vec3(
			v[0] / s,
			v[1] / s,
			v[2] / s);
	}

	inline float operator*(const Vec3& other) const
	{
		return
			v[0] * other[0] +
			v[1] * other[1] +
			v[2] * other[2];
	}

	inline float LengthSq_get() const
	{
		return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	}

	inline float Length_get() const
	{
		float lengthSq = LengthSq_get();

		return sqrt(lengthSq);
	}

	inline void Normalize()
	{
		float length = Length_get();

		if (length == 0.0f)
		{
			v[0] = 1.0f;
			v[1] = 0.0f;
			v[2] = 0.0f;
			return;
		}

		v[0] /= length;
		v[1] /= length;
		v[2] /= length;
	}

	float v[3];
};
#endif

template<typename T, int size>
class BaseVec
{
public:
	inline BaseVec()
	{
		m_V[0] = 0.0f;
		m_V[1] = 0.0f;
		m_V[2] = 0.0f;
	}

	inline BaseVec(T x, T y)
	{
		m_V[0] = x;
		m_V[1] = y;
		m_V[2] = (T)0;
	}

	inline BaseVec(T x, T y, T z)
	{
		m_V[0] = x;
		m_V[1] = y;
		m_V[2] = z;
	}

	void SetZero()
	{
		for (int i = 0; i < size; ++i)
			m_V[i] = (T)0;
		//m_V[1] = (T)0;
		//m_V[2] = (T)0;
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

	inline T operator*(const BaseVec<T, size>& v) const
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

	inline BaseVec<T, size> operator+(const BaseVec<T, size>& v) const
	{
		return BaseVec<T, size>(
			m_V[0] + v.m_V[0],
			m_V[1] + v.m_V[1],
			m_V[2] + v.m_V[2]);
	}

	inline BaseVec<T, size> operator-(const BaseVec<T, size>& v) const
	{
		return BaseVec<T, size>(
			m_V[0] - v.m_V[0],
			m_V[1] - v.m_V[1],
			m_V[2] - v.m_V[2]);
	}

	inline BaseVec<T, size> operator*(T v) const
	{
		return BaseVec<T, size>(
			m_V[0] * v,
			m_V[1] * v,
			m_V[2] * v);
	}

	inline BaseVec<T, size> operator/(T v) const
	{
		return BaseVec<T, size>(
			m_V[0] / v,
			m_V[1] / v,
			m_V[2] / v);
	}

	inline void operator-=(const BaseVec<T, size>& other)
	{
		m_V[0] -= other[0];
		m_V[1] -= other[1];
		m_V[2] -= other[2];
	}

	inline void operator+=(const BaseVec<T, size>& other)
	{
		m_V[0] += other[0];
		m_V[1] += other[1];
		m_V[2] += other[2];
	}

	inline void operator*=(T v)
	{
		m_V[0] *= v;
		m_V[1] *= v;
		m_V[2] *= v;
	}

	inline BaseVec<float, size> ToF() const
	{
		return BaseVec<float, size>(
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

typedef BaseVec<int, 2> Vec2I;
typedef BaseVec<float, 2> Vec2F;

typedef BaseVec<int, 3> Vec3I;
typedef BaseVec<float, 3> Vec3F;

template<typename T, int size>
class BasePlane
{
public:
	inline BasePlane()
	{
		m_Normal.SetZero();
		m_Distance = (T)0;
	}

	inline BasePlane(const BaseVec<T, size>& normal, T distance)
	{
		Initialize(normal, distance);
	}

	inline void Initialize(const BaseVec<T, size>& normal, T distance)
	{
		m_Normal = normal;
		m_Distance = distance;
	}

	inline T operator*(const BaseVec<T, size>& v) const
	{
		return v * m_Normal - m_Distance;
	}

	BaseVec<T, size> m_Normal;
	T m_Distance;
};

typedef BasePlane<int, 2> Plane2I;
typedef BasePlane<float, 2> Plane2F;

typedef BasePlane<int, 3> Plane3I;
typedef BasePlane<float, 3> Plane3F;

template <typename T>
class BaseSphere
{
public:
	void Initialize(
		BaseVec<T, 3> pos,
		T radius)
	{
		m_Pos = pos;
		m_Radius = radius;
		m_RadiusSq = radius * radius;
	}

	BaseVec<T, 3> m_Pos;
	T m_Radius;
	T m_RadiusSq;
};

typedef BaseSphere<float> SphereF;
typedef BaseSphere<int> SphereI;

//

#if 0
class CT
{
public:
	// Returns true if a horizontal line fired in the +X direction intersects the line segment (p1, p2).
	static BOOL HitTest_HLineO(Vec2 p1, Vec2 p2)
	{
		Vec2 d = p2 - p1;

		if (d[1] == 0.0f)
			return false;

		float t = -p1[1] / d[1];

		float x = p1[0] + d[0] * t;

		return x >= 0.0f;
	}

	static BOOL HitTest_Outline(const std::vector<Vec2T>& outline, Vec2 p)
	{
		int count = 0;

		for (size_t i = 0; i < outline.size(); ++i)
		{
			int index1 = (i + 0) % outline.size();
			int index2 = (i + 1) % outline.size();

			Vec2 v1 = outline[index1].vT - p;
			Vec2 v2 = outline[index2].vT - p;

			if (v1[1] > v2[1])
			{
				std::swap(v1, v2);
			}

			if (v1[1] <= 0.0f && v2[1] >= 0.0f)
			{
				if (v1[0] >= 0.0f && v2[0] >= 0.0f)
				{
					count++;
				}
				else if (v1[0] >= 0.0f || v2[0] >= 0.0f)
				{
					if (HitTest_HLineO(v1, v2))
					{
						count++;
					}
				}
			}
		}

		return (count & 1) != 0;
	}
};
#endif

enum Compare
{
	Compare_Front,
	Compare_Back,
	Compare_Span
};

#define PLANE_EPS 0.01f

class Plane
{
public:
	Vec2F Normal;
	float Distance;

	float operator*(const Vec2F& v) const
	{
		return Normal * v - Distance;
	}
};

class Poly
{
public:
	Compare Compare(const Plane& plane) const
	{
		BOOL f = TRUE;
		BOOL b = TRUE;

		for (size_t i = 0; i < Points.size(); ++i)
		{
			float distance = plane * Points[i];

			if (distance < -PLANE_EPS)
				f = FALSE;
			if (distance > +PLANE_EPS)
				b = FALSE;
		}

		if (f && b)
			return Compare_Span;
		if (f && !b)
			return Compare_Front;
		if (!f && b)
			return Compare_Back;
	}

	void Split(const Plane& plane, Poly* o_Front, Poly* o_Back) const
	{
		for (size_t i = 0; i < Points.size(); ++i)
		{
			float distance = plane * Points[i];

			if (distance > +PLANE_EPS)
			{
				o_Front->Points.push_back(Points[i]);
			}
			else if (distance < -PLANE_EPS)
			{
				o_Back->Points.push_back(Points[i]);
			}
			else
			{
				o_Front->Points.push_back(Points[i]);
				o_Back->Points.push_back(Points[i]);
			}
		}
	}

	std::vector<Vec2F> Points;
};

static inline int MidI(int v, int min, int max)
{
	if (v < min)
		return min;
	if (v > max)
		return max;

	return v;
}

static inline float MidF(float v, float min, float max)
{
	if (v < min)
		return min;
	if (v > max)
		return max;

	return v;
}

template <typename T>
class BaseRect
{
public:
	BaseRect()
	{
	}

	BaseRect(BaseVec<T, 2> position, BaseVec<T, 2> size)
	{
		Setup(position, size);
	}

	void Setup(BaseVec<T, 2> position, BaseVec<T, 2> size)
	{
		m_Position = position;
		m_Size = size;
	}

	BOOL IsInside(BaseVec<T, 2> p) const
	{
		for (int i = 0; i < 2; ++i)
		{
			if (p[i] < m_Position[i])
				return FALSE;
			if (p[i] > m_Position[i] + m_Size[i])
				return FALSE;
		}

		return TRUE;
	}

	inline T Width_get() const
	{
		return m_Size[0];
	}

	inline T Height_get() const
	{
		return m_Size[1];
	}

	inline BaseVec<T, 2> Min_get() const
	{
		return m_Position;
	}

	inline BaseVec<T, 2> Max_get() const
	{
		return m_Position + m_Size;
	}

	BaseVec<T, 2> m_Position;
	BaseVec<T, 2> m_Size;
};

typedef BaseRect<int> RectI;
typedef BaseRect<float> RectF;

class BoundingBox
{
public:
	BoundingBox()
	{
		IsEmpty = TRUE;
	}

	BoundingBox(const Vec2F& min, const Vec2F& max)
	{
		IsEmpty = FALSE;

		Min = min;
		Max = max;
	}

	void Merge(const Vec2F& p)
	{
		if (IsEmpty)
		{
			IsEmpty = FALSE;

			Min = p;
			Max = p;
		}
		else
		{
			for (int i = 0; i < 2; ++i)
			{
				if (p[i] < Min[i])
					Min[i] = p[i];
				if (p[i] > Max[i])
					Max[i] = p[i];
			}
		}
	}

	void Merge(const BoundingBox& bb)
	{
		Merge(bb.Min);
		Merge(bb.Max);
	}

	BOOL Inside(const Vec2F& p) const
	{
		for (int i = 0; i < 2; ++i)
		{
			if (p[i] < Min[i])
				return FALSE;
			if (p[i] > Max[i])
				return FALSE;
		}

		return TRUE;
	}

	BOOL IsEmpty;
	Vec2F Min;
	Vec2F Max;
};

class BoundingSphere
{
public:
	BoundingSphere()
	{
		Radius = 0.0f;
	}

	BoundingSphere(Vec2F pos, float radius)
	{
		Pos = pos;
		Radius = radius;
	}

	void FromBB(const BoundingBox& bb)
	{
		Pos = (bb.Min + bb.Max) / 2.0f;

		Merge(bb.Min);
		Merge(bb.Max);
	}

	BoundingBox ToBB() const
	{
		Vec2F min = Pos - Vec2F(Radius, Radius);
		Vec2F max = Pos + Vec2F(Radius, Radius);

		return BoundingBox(min, max);
	}

	void FromBSs(const std::vector<BoundingSphere>& spheres)
	{
		const BoundingSphere* sphere1 = 0;
		const BoundingSphere* sphere2 = 0;
		float maxDistance = 0.0f;

		for (size_t i = 0; i < spheres.size(); ++i)
		{
			for (size_t j =0; j < spheres.size(); ++j)
			{
				Vec2F p1 = spheres[i].Pos;
				Vec2F p2 = spheres[j].Pos;

				Vec2F delta = p2 - p1;

				float length = delta.Length_get();

				float distance = length + spheres[i].Radius + spheres[j].Radius;

				if (distance > maxDistance)
				{
					maxDistance = distance;
					sphere1 = &spheres[i];
					sphere2 = &spheres[j];
				}
			}
		}

		assert(sphere1);
		assert(sphere2);

		Vec2F mid = (sphere1->Pos + sphere2->Pos) / 2.0f;

		Pos = Vec2F(mid[0], mid[1]);
		Radius = maxDistance;

		//if (mid[0] == 0.0f)
		//	assert(false);

#if 0
		printf("mid = %f, %f, %f\n",
			mid[0],
			mid[1],
			mid[2]);

		rest(10);
#endif
	}

	void Merge(const Vec2F& p)
	{
		Vec2F delta = p - Pos;

		float length = delta.Length_get();

		if (length > Radius)
			Radius = length;
	}

	BOOL HitTest(const Vec2F& pos)
	{
		Vec2F delta = pos - Pos;

		float lengthSq = delta.LengthSq_get();

		if (lengthSq > Radius * Radius)
			return FALSE;

		return TRUE;
	}

	Vec2F Pos;
	float Radius;
};

#if 1
class Vec2T
{
public:
	Vec2T()
	{
	}

	Vec2T(float x, float y)
	{
		v = vT = Vec2F(x, y);
	}

	Vec2F v;
	Vec2F vT;
};
#endif
