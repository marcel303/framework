#pragma once

// Interface declaration and implementation.

#define GG_INTERFACE_BEGIN(name) class name { public: virtual ~name() { }
#define GG_INTERFACE_METHOD(return, name, args) virtual return name args = 0
#define GG_INTERFACE_END() };
#define GG_INTERFACE_IMP(name) public name

// Primitive types.

typedef short Int16;
typedef int Int32;

typedef unsigned char UInt8;
typedef unsigned long int UInt32;

// Primitive classes.

class PointI
{
public:
	PointI()
	{
		x = 0;
		y = 0;
	}
	
	PointI(int x, int y)
	{
		this->x = x;
		this->y = y;
	}
	
	int x;
	int y;
};

#define BB_UNSET -1337

class BoundingBoxI
{
public:
	BoundingBoxI()
	{
		Reset();
	}
	
	void Reset()
	{
		min = PointI(BB_UNSET, BB_UNSET);
		max = PointI(BB_UNSET, BB_UNSET);
	}
	
	inline void Merge(const PointI& p)
	{
		if (IsSet_get())
		{
			if (p.x < min.x)
				min.x = p.x;
			if (p.y < min.y)
				min.y = p.y;
			
			if (p.x > max.x)
				max.x = p.x;
			if (p.y > max.y)
				max.y = p.y;
		}
		else
		{
			min = p;
			max = p;
		}
	}
	
	inline bool IsSet_get() const
	{
		return min.x != BB_UNSET;
	}
	
	PointI min;
	PointI max;
};

// Fixed point stuff.

#if 0

#define FIX_IS_INT

typedef Int16 Fix; // 8.8 fixed point real.

#define INT_TO_FIX(x) ((x) << 8)
#define FIX_TO_INT(x) ((x) >> 8)
//#define FIX_TO_INT5(p) ((p) >> 19)
#define REAL_TO_FIX(x) Fix((x) * 256.0f)
#define FIX_TO_REAL(x) ((x) / 256.0f)
#define FIX_INV(x) (FIX_ONE - (x))
#define FIX_MUL(x, y) REAL_TO_FIX(FIX_TO_REAL(x) * FIX_TO_REAL(y))
//#define FIX_MUL(x, y) REAL_TO_FIX(FIX_TO_REAL(x) * FIX_TO_REAL(y))
#define FIX_MIX(p1, p2, t1, t2) (FIX_MUL(p1, t1) + FIX_MUL(p2, t2))
#define FIX_DIV(p1, p2) REAL_TO_FIX(FIX_TO_REAL(p1) / FIX_TO_REAL(p2))
//#define FIX_DIV(p1, p2) (((p1) << 5) / ((p2) >> 11))
//#define FIX_DIV(p1, p2) (((p1) << 5) / ((p2) >> 11) + 1)
#define FIX_ONE 256
#define FIX_SMALLEST_NON_ZERO 1

#elif 0

#define FIX_IS_INT

typedef Int32 Fix; // 16.16 fixed point real.

#define INT_TO_FIX(x) ((x) << 16)
#define FIX_TO_INT(x) ((x) >> 16)
//#define FIX_TO_INT5(p) ((p) >> 19)
#define REAL_TO_FIX(x) Fix((x) * 65536.0f)
#define FIX_TO_REAL(x) ((x) / 65536.0f)
#define FIX_INV(x) (FIX_ONE - (x))
#define FIX_MUL(x, y) (((x) >> 5) * ((y) >> 5) >> 6)
//#define FIX_MUL(x, y) REAL_TO_FIX(FIX_TO_REAL(x) * FIX_TO_REAL(y))
#define FIX_MIX(p1, p2, t1, t2) (FIX_MUL(p1, t1) + FIX_MUL(p2, t2))
#define FIX_DIV(p1, p2) REAL_TO_FIX(FIX_TO_REAL(p1) / FIX_TO_REAL(p2))
//#define FIX_DIV(p1, p2) (((p1) << 5) / ((p2) >> 11))
//#define FIX_DIV(p1, p2) (((p1) << 5) / ((p2) >> 11) + 1)
#define FIX_ONE 65536
#define FIX_SMALLEST_NON_ZERO 1

#else

#define FIX_IS_FLOAT

typedef float Fix; // real real :)

#define INT_TO_FIX(x) ((float)(x))
#define FIX_TO_INT(x) ((int)(x))
//#define FIX_TO_INT5(p) ((p) >> 19)
#define REAL_TO_FIX(x) (x)
#define FIX_TO_REAL(x) ((float)(x))
#define FIX_INV(x) (FIX_ONE - (x))
#define FIX_MUL(x, y) ((x) * (y))
#define FIX_MIX(p1, p2, t1, t2) (FIX_MUL(p1, t1) + FIX_MUL(p2, t2))
#define FIX_DIV(p1, p2) ((p1) / (p2))
#define FIX_ONE 1.0f
#define FIX_SMALLEST_NON_ZERO 0.0001f

#endif
