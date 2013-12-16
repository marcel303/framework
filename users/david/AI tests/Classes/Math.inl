/*
 *  Math.inl
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */


inline Vec2::Vec2()
{
}

inline Vec2::Vec2(float x, float y)
{
	this->x = x; this->y = y;
}

inline Vec2::Vec2(const Vec2& v)
{
	x = v.x;
	y = v.y;
}

inline void Vec2::operator =(float v)
{
	x = v;
	y = v;
}

inline void Vec2::operator =(const Vec2& v)
{
	x = v.x;
	y = v.y;
}


inline void Vec2::operator +=(float v)
{
	x += v;
	y += v;
}

inline void Vec2::operator +=(const Vec2& v)
{
	x += v.x;
	y += v.y;
}

inline void Vec2::operator -=(float v)
{
	x -= v;
	y -= v;
}

inline void Vec2::operator -=(const Vec2& v)
{
	x -= v.x;
	y -= v.y;
}

inline void Vec2::operator *=(float v)
{
	x *= v;
	y *= v;
}

//inline void Vec2::operator *=(const Vec2& v)
//{
//	x *= v.x;
//	y *= v.y;
//}

inline void Vec2::operator /=(float v)
{
	x /= v;
	y /= v;
}

inline void Vec2::operator /=(const Vec2& v)
{
	x /= v.x;
	y /= v.y;
}


inline Vec2 Vec2::operator +(float v) const
{
	return Vec2(x + v, y + v);
}

inline Vec2 Vec2::operator +(const Vec2& v) const
{
	return Vec2(x + v.x, y + v.y);
}

inline Vec2 Vec2::operator -(float v) const
{
	return Vec2(x - v, y - v);
}

inline Vec2 Vec2::operator -(const Vec2& v) const
{
	return Vec2(x - v.x, y - v.y);
}

inline Vec2 Vec2::operator *(float v) const
{
	return Vec2(x * v, y * v);
}

//inline Vec2 Vec2::operator *(const Vec2& v) const
//{
//	return Vec2(x * v.x, y * v.y);
//}

inline Vec2 Vec2::operator /(float v) const
{
	return Vec2(x / v, y / v);
}

inline Vec2 Vec2::operator /(const Vec2& v) const
{
	return Vec2(x / v.x, y / v.y);
}


inline Vec2 Vec2::operator -() const
{
	return Vec2(-x, -y);
}


inline bool Vec2::operator ==(const Vec2& v) const
{
	return (x == v.x && y == v.y);
}

inline bool Vec2::operator !=(const Vec2& v) const
{
	return (x != v.x && y != v.y);
}

inline bool Vec2::operator <=(const Vec2& v) const
{
	return (x <= v.x && y <= v.y);
}

inline bool Vec2::operator >=(const Vec2& v) const
{
	return (x >= v.x && y >= v.y);
}

bool Vec2::operator <(const Vec2& v) const
{
	return (x < v.x && y < v.y);
}

inline bool Vec2::operator >(const Vec2& v) const
{
	return (x > v.x && y > v.y);
}

inline Vec2 Vec2::Normalize()
{
#if 0
	float l = Len();
	if(l != 0.0f)
		*this /= l;
#else
	const float s2 = qLen();
	const float si = FastInvSqrt(s2);
	*this *= si;
#endif
	
	return *this;
}

inline Vec2 Vec2::NormalizeB()
{
	if(fabsf(x) > fabsf(y))
		*this /= fabsf(x);
	else
		*this /= fabsf(y);
	
	return *this;
}

inline float Vec2::Dot(const Vec2& v)
{
	return (x * v.x) + (y * v.y);
}


inline float Vec2::qLen(const Vec2& v) const
{
	return (x-v.x)*(x-v.x) + (y-v.y)*(y-v.y);
}

inline float Vec2::qLen() const
{
	return (x)*(x) + (y)*(y);
}

inline float Vec2::Len(const Vec2& v) const
{
	const float dx = x - v.x;
	const float dy = y - v.y;
	
	return sqrtf(dx * dx + dy * dy);
}

inline float Vec2::Len() const
{
	return sqrtf((x*x) + (y*y));
}

