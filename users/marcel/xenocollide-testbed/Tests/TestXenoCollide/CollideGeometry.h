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

#include "MapPtr.h"
#include "Math/Math.h"

//////////////////////////////////////////////////////////////////////////////
// This is the base class for XenoCollide shapes.  To create a new primitive,
// derive from CollideGeometry and implement the GetSupportPoint()
// method.  By default, GetCenter() will return (0, 0, 0).  If this isn't
// a deep interior point for your shape, override this method and return a
// different point.

class CollideGeometry
{
//	friend class MapPtr<CollideGeometry>;
public:
	virtual ~CollideGeometry() {}
	virtual Vector GetSupportPoint(const Vector& n) = 0;
	virtual Vector GetCenter();
};

//////////////////////////////////////////////////////////////////////////////

class CollidePoint : public CollideGeometry
{
	Vector mPoint;

public:

	CollidePoint(const Vector& p);

	virtual Vector GetSupportPoint(const Vector& n);
	virtual Vector GetCenter();
};

//////////////////////////////////////////////////////////////////////////////

class CollideSegment : public CollideGeometry
{
	float32 mRadius;

public:

	CollideSegment(float32 r);

	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollideRectangle : public CollideGeometry
{
	Vector mRadius;

public:

	CollideRectangle(float32 rx, float32 ry);

	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollideBox : public CollideGeometry
{
	Vector mRadius;

public:

	CollideBox(const Vector& r);

	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollideDisc : public CollideGeometry
{
	float32 mRadius;

public:

	CollideDisc(float32 r);

	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollideSphere : public CollideGeometry
{
	float32 mRadius;

public:

	CollideSphere(float32 r);

	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollideEllipse : public CollideGeometry
{
	Vector mRadius;

public:

	CollideEllipse(float32 rx, float32 ry);

	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollideEllipsoid : public CollideGeometry
{
	Vector mRadius;

public:

	CollideEllipsoid(const Vector& r);

	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollideFootball : public CollideGeometry
{

	float32 mLength;
	float32 mRadius;

public:

	CollideFootball(float32 length, float32 radius);

	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollideBullet : public CollideGeometry
{

	float32 mLengthTip;
	float32 mLengthTail;
	float32 mRadius;

public:

	CollideBullet(float32 lengthTip, float32 lengthTail, float32 radius);

	virtual Vector GetSupportPoint(const Vector& n);
	virtual Vector GetCenter();
};

//////////////////////////////////////////////////////////////////////////////

class CollideSaucer : public CollideGeometry
{

	float32 mHalfThickness;
	float32 mRadius;

public:

	CollideSaucer(float32 radius, float32 halfThickness);

	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollidePolytope : public CollideGeometry
{

	Vector* mVert;
	int32 mVertMax;
	int32 mVertCount;

public:

	CollidePolytope(int32 n);

	void AddVert(const Vector& p);
	virtual Vector GetSupportPoint(const Vector& n);
};

//////////////////////////////////////////////////////////////////////////////

class CollideSum : public CollideGeometry
{

public:

	Quat	q1;
	Quat	q2;
	Vector	t1;
	Vector	t2;

	MapPtr<CollideGeometry>	mGeometry1;
	MapPtr<CollideGeometry>	mGeometry2;

public:

	CollideSum(CollideGeometry* g1, const Quat& q1, const Vector& t1, CollideGeometry* g2, const Quat& q2, const Vector& t2);
	CollideSum(CollideGeometry* g1, const Vector& t1, CollideGeometry* g2, const Vector& t2);
	CollideSum(CollideGeometry* g1, CollideGeometry* g2);

	virtual Vector GetSupportPoint(const Vector& n);
	virtual Vector GetCenter();
};

//////////////////////////////////////////////////////////////////////////////

class CollideDiff : public CollideGeometry
{

	Quat	q1;
	Quat	q2;
	Vector	t1;
	Vector	t2;

	MapPtr<CollideGeometry>	mGeometry1;
	MapPtr<CollideGeometry>	mGeometry2;

public:

	CollideDiff(CollideGeometry* g1, const Quat& q1, const Vector& t1, CollideGeometry* g2, const Quat& q2, const Vector& t2);
	CollideDiff(CollideGeometry* g1, const Vector& t1, CollideGeometry* g2, const Vector& t2);
	CollideDiff(CollideGeometry* g1, CollideGeometry* g2);

	virtual Vector GetSupportPoint(const Vector& n);
	virtual Vector GetCenter();
};

//////////////////////////////////////////////////////////////////////////////

class CollideNeg : public CollideGeometry
{

	Quat	q1;
	Vector	t1;

	MapPtr<CollideGeometry>	mGeometry1;

public:

	CollideNeg(CollideGeometry* g1, const Quat& q1, const Vector& t1);
	CollideNeg(CollideGeometry* g1, const Vector& t1);
	CollideNeg(CollideGeometry* g1);

	virtual Vector GetSupportPoint(const Vector& n);
	virtual Vector GetCenter();
};

//////////////////////////////////////////////////////////////////////////////

class CollideMax : public CollideGeometry
{
	Quat	q1;
	Quat	q2;
	Vector	t1;
	Vector	t2;

	MapPtr<CollideGeometry>	mGeometry1;
	MapPtr<CollideGeometry>	mGeometry2;

public:

	CollideMax(CollideGeometry* g1, const Quat& q1, const Vector& t1, CollideGeometry* g2, const Quat& q2, const Vector& t2);
	CollideMax(CollideGeometry* g1, const Vector& t1, CollideGeometry* g2, const Vector& t2);
	CollideMax(CollideGeometry* g1, CollideGeometry* g2);

	virtual Vector GetSupportPoint(const Vector& n);
	virtual Vector GetCenter();

};

//////////////////////////////////////////////////////////////////////////////
