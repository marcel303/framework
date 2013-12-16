/*
 *  Math.h
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "Defines.h"
#include <math.h>

inline float FastInvSqrt(float v)
{
	float half = 0.5f * v;
	
	int i = *(int*)&v;         // store floating-point bits in integer
	i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
	
	v = *(float*)&i;               // convert new bits into float
	v = v * (1.5f - half * v * v);  // One round of Newton's method
	
	return v;
}


static float toRad(int angle)
{
	return (angle%360)*PI/180.0f;
}

static float toDeg(float rad)
{
	return (int(rad*180.0f/PI));
}

class Vec2
{
public:
	
	float x, y;
	
	inline Vec2();
	inline Vec2(float x, float y);
	inline Vec2(const Vec2& v);
	
	inline void operator =(float v);
	inline void operator =(const Vec2& v);
	
	inline void operator +=(float v);
	inline void operator +=(const Vec2& v);
	inline void operator -=(float v);
	inline void operator -=(const Vec2& v);
	inline void operator *=(float v);
	inline void operator *=(const Vec2& v);
	inline void operator /=(float v);
	inline void operator /=(const Vec2& v);
	
	inline Vec2 operator +(float v) const;
	inline Vec2 operator +(const Vec2& v) const;
	inline Vec2 operator -(float v) const;
	inline Vec2 operator -(const Vec2& v) const;
	inline Vec2 operator *(float v) const;
	//inline Vec2 operator *(const Vec2& v) const;
	inline Vec2 operator /(float v) const;
	inline Vec2 operator /(const Vec2& v) const;
	
	inline Vec2 operator -() const;
	
	inline bool operator ==(const Vec2& v) const;
	inline bool operator !=(const Vec2& v) const;
	inline bool operator <=(const Vec2& v) const;
	inline bool operator >=(const Vec2& v) const;
	inline bool operator <(const Vec2& v) const;
	inline bool operator >(const Vec2& v) const;
	
	inline Vec2 Normalize();
	inline Vec2 NormalizeB();
	inline float Dot(const Vec2& v);
	
	inline float qLen(const Vec2& v) const;
	inline float qLen() const;
	inline float Len(const Vec2& v) const;
	inline float Len() const;
	
};

class Rad
{
public:
	
	Vec2 pos;
	float rad;
	
	Rad();
	~Rad();
	
	bool Hit(Rad& r) const;
	bool Hit(Vec2 v) const;
	
	void Set(Vec2 pos, float r);
};

#include "Math.inl"
