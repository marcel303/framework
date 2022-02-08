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

#include "CollideGeometry.h"

//////////////////////////////////////////////////////////////////////////////
// CollideGeometry

Vector CollideGeometry::GetCenter()
{
	return Vector(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////
// CollidePoint

CollidePoint::CollidePoint(const Vector& p)
{
	mPoint = p;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollidePoint::GetSupportPoint(const Vector& n)
{
	return mPoint;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollidePoint::GetCenter()
{
	return mPoint;
}

//////////////////////////////////////////////////////////////////////////////
// CollideSegment

CollideSegment::CollideSegment(float32 r)
{
	mRadius = r;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideSegment::GetSupportPoint(const Vector& n)
{
	if (n.X() < 0) return Vector(-mRadius, 0, 0);
	return Vector(mRadius, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////
// CollideRectangle

CollideRectangle::CollideRectangle(float32 rx, float32 ry)
{
	mRadius = Vector(rx, ry, 0);
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideRectangle::GetSupportPoint(const Vector& n)
{
	Vector result = mRadius;
	if (n.X() < 0) result.X() = -result.X();
	if (n.Y() < 0) result.Y() = -result.Y();
	return result;
}

//////////////////////////////////////////////////////////////////////////////
// CollideSphere

CollideSphere::CollideSphere(float32 r)
{
	mRadius = r;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideSphere::GetSupportPoint(const Vector& n)
{
	Vector n2 = n;
	n2.Normalize3();
	return mRadius * n2;
}

//////////////////////////////////////////////////////////////////////////////
// CollideEllipse

CollideEllipse::CollideEllipse(float32 rx, float32 ry)
{
	mRadius = Vector(rx, ry, 0);
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideEllipse::GetSupportPoint(const Vector& n)
{
	Vector n2 = CompMul(mRadius, n);
	if (!n2.CanNormalize3()) return Vector(0, 0, 0);
	n2.Normalize3();
	return CompMul(n2, mRadius);
}

//////////////////////////////////////////////////////////////////////////////
// CollideEllipsoid

CollideEllipsoid::CollideEllipsoid(const Vector& r)
{
	mRadius = r;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideEllipsoid::GetSupportPoint(const Vector& n)
{
	Vector n2 = CompMul(n, mRadius);
	n2.Normalize3();
	return CompMul(n2, mRadius);
}

//////////////////////////////////////////////////////////////////////////////
// CollideDisc

CollideDisc::CollideDisc(float32 r)
{
	mRadius = r;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideDisc::GetSupportPoint(const Vector& n)
{
	Vector n2 = n;
	n2.Z() = 0;
	if (n2.IsZero3())
	{
		return Vector(0, 0, 0);
	}
	n2.Normalize3();
	return mRadius * n2;
}

//////////////////////////////////////////////////////////////////////////////
// CollideBox

CollideBox::CollideBox(const Vector& r)
{
	mRadius = r;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideBox::GetSupportPoint(const Vector& n)
{
	Vector result = mRadius;
	if (n.X() < 0) result.X() = -result.X();
	if (n.Y() < 0) result.Y() = -result.Y();
	if (n.Z() < 0) result.Z() = -result.Z();
	return result;
}

//////////////////////////////////////////////////////////////////////////////
// CollideFootball

CollideFootball::CollideFootball(float32 length, float32 radius)
{
	mRadius = radius;
	mLength = length;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideFootball::GetSupportPoint(const Vector& n)
{
	// Radius
	float32 r1 = mRadius;

	// Half-length
	float32 h = mLength;

	// Radius of curvature
	float32 r2 = 0.5f * (h*h/r1 + r1);

	Vector n3 = n;
	n3.Normalize3();

	if (n3.X() * r2 < -h) return Vector(-h, 0, 0);
	if (n3.X() * r2 > h) return Vector(h, 0, 0);

	Vector n2 (0, n.Y(), n.Z());
	n2.Normalize3();

	Vector p = -n2*(r2-r1) + n3*r2;
	return p;
}

//////////////////////////////////////////////////////////////////////////////
// CollideBullet

CollideBullet::CollideBullet(float32 lengthTip, float32 lengthTail, float32 radius)
{
	mRadius = radius;
	mLengthTip = lengthTip;
	mLengthTail = lengthTail;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideBullet::GetSupportPoint(const Vector& n)
{
	if (n.X() < 0)
	{
		// Radius
		float32 r1 = mRadius;
		// Half-length
		float32 h = mLengthTip;

		// Radius of curvature
		float32 r2 = 0.5f * (h*h/r1 + r1);

		Vector n3 = n;
		n3.Normalize3();

		if (n3.X() * r2 < -h) return Vector(-h, 0, 0);
		if (n3.X() * r2 > h) return Vector(h, 0, 0);

		Vector n2 (0, n.Y(), n.Z());
		n2.Normalize3();

		Vector p = -n2*(r2-r1) + n3*r2;
		return p;
	}
	else
	{
		Vector n2 = n;
		n2.X() = 0;
		if (n2.IsZero3())
		{
			return Vector(mLengthTail, 0, 0);
		}
		n2.Normalize3();
		return mRadius * n2 + Vector(mLengthTail, 0, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideBullet::GetCenter()
{
	return Vector( 0.5f * (mLengthTail - mLengthTip), 0, 0 );
}

//////////////////////////////////////////////////////////////////////////////
// CollideSaucer

CollideSaucer::CollideSaucer(float32 radius, float32 halfThickness)
{
	mHalfThickness = halfThickness;
	mRadius = radius;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideSaucer::GetSupportPoint(const Vector& n)
{
	// Half-thickness
	float32 t = mHalfThickness;

	// Half-length
	float32 h = mRadius;

	// Radius of curvature
	float32 r2 = 0.5f * (h*h/t + t);

	Vector n3 = n;
	n3.Normalize3();

	Vector n4(0, n.Y(), n.Z());
	if (n4.CanNormalize3())
	{
		n4.Normalize3();
		float32 threshold = n3.Y()*n3.Y() + n3.Z()*n3.Z();
		if (threshold * r2 * r2 > h * h) return h * n4;
	}

	Vector n2(1, 0, 0);
	if (n.X() < 0)
	{
		n2 = -n2;
	}

	Vector p = -n2*(r2-t) + n3*r2;
	return p;

}

//////////////////////////////////////////////////////////////////////////////
// CollidePolytope

CollidePolytope::CollidePolytope(int32 maxVerts)
{
	mVertMax = maxVerts;
	mVert = new Vector[maxVerts];
	mVertCount = 0;
}

//////////////////////////////////////////////////////////////////////////////

void CollidePolytope::AddVert(const Vector& p)
{
	if (mVertCount >= mVertMax) return;
	mVert[mVertCount++] = p;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollidePolytope::GetSupportPoint(const Vector& n)
{
	int32 i = mVertCount-1;
	Vector r = mVert[i--];
	while (i>=0)
	{
		if ( (mVert[i] - r) * n > 0 )
		{
			r = mVert[i];
		}
		i--;
	}
	return r;
}

//////////////////////////////////////////////////////////////////////////////
// CollideSum

CollideSum::CollideSum(CollideGeometry* g1, const Quat& q1, const Vector& t1, CollideGeometry* g2, const Quat& q2, const Vector& t2)
{
	mGeometry1 = g1;
	mGeometry2 = g2;
	this->q1 = q1;
	this->t1 = t1;
	this->q2 = q2;
	this->t2 = t2;
}

//////////////////////////////////////////////////////////////////////////////

CollideSum::CollideSum(CollideGeometry* g1, const Vector& t1, CollideGeometry* g2, const Vector& t2)
{
	mGeometry1 = g1;
	mGeometry2 = g2;
	this->q1 = Quat(0, 0, 0, 1);
	this->t1 = t1;
	this->q2 = Quat(0, 0, 0, 1);
	this->t2 = t2;
}

//////////////////////////////////////////////////////////////////////////////

CollideSum::CollideSum(CollideGeometry* g1, CollideGeometry* g2)
{
	mGeometry1 = g1;
	mGeometry2 = g2;
	this->q1 = Quat(0, 0, 0, 1);
	this->t1 = Vector(0, 0, 0);
	this->q2 = Quat(0, 0, 0, 1);
	this->t2 = Vector(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideSum::GetSupportPoint(const Vector& n)
{
	return q1.Rotate(mGeometry1->GetSupportPoint( (~q1).Rotate(n))) + t1 + q2.Rotate(mGeometry2->GetSupportPoint((~q2).Rotate(n))) + t2;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideSum::GetCenter()
{
	return q1.Rotate(mGeometry1->GetCenter()) + t1 + q2.Rotate(mGeometry2->GetCenter()) + t2;
}

//////////////////////////////////////////////////////////////////////////////
// CollideDiff

CollideDiff::CollideDiff(CollideGeometry* g1, const Quat& q1, const Vector& t1, CollideGeometry* g2, const Quat& q2, const Vector& t2)
{
	mGeometry1 = g1;
	mGeometry2 = g2;
	this->q1 = q1;
	this->t1 = t1;
	this->q2 = q2;
	this->t2 = t2;
}

//////////////////////////////////////////////////////////////////////////////

CollideDiff::CollideDiff(CollideGeometry* g1, const Vector& t1, CollideGeometry* g2, const Vector& t2)
{
	mGeometry1 = g1;
	mGeometry2 = g2;
	this->q1 = Quat(0, 0, 0, 1);
	this->t1 = t1;
	this->q2 = Quat(0, 0, 0, 1);
	this->t2 = t2;
}

//////////////////////////////////////////////////////////////////////////////

CollideDiff::CollideDiff(CollideGeometry* g1, CollideGeometry* g2)
{
	mGeometry1 = g1;
	mGeometry2 = g2;
	this->q1 = Quat(0, 0, 0, 1);
	this->t1 = Vector(0, 0, 0);
	this->q2 = Quat(0, 0, 0, 1);
	this->t2 = Vector(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideDiff::GetSupportPoint(const Vector& n)
{
	return q1.Rotate(mGeometry1->GetSupportPoint( (~q1).Rotate(n))) + t1 - q2.Rotate(mGeometry2->GetSupportPoint((~q2).Rotate(-n))) - t2;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideDiff::GetCenter()
{
	return q1.Rotate(mGeometry1->GetCenter()) + t1 - q2.Rotate(mGeometry2->GetCenter()) - t2;
}

//////////////////////////////////////////////////////////////////////////////
// CollideNeg

CollideNeg::CollideNeg(CollideGeometry* g1, const Quat& q1, const Vector& t1)
{
	mGeometry1 = g1;
	this->q1 = q1;
	this->t1 = t1;
}

//////////////////////////////////////////////////////////////////////////////

CollideNeg::CollideNeg(CollideGeometry* g1, const Vector& t1)
{
	mGeometry1 = g1;
	this->q1 = Quat(0, 0, 0, 1);
	this->t1 = t1;
}

//////////////////////////////////////////////////////////////////////////////

CollideNeg::CollideNeg(CollideGeometry* g1)
{
	mGeometry1 = g1;
	this->q1 = Quat(0, 0, 0, 1);
	this->t1 = Vector(0, 0, 0);;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideNeg::GetSupportPoint(const Vector& n)
{
	return -(q1.Rotate(mGeometry1->GetSupportPoint( (~q1).Rotate(-n))) + t1);
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideNeg::GetCenter()
{
	return -(q1.Rotate(mGeometry1->GetCenter()) + t1);
}

//////////////////////////////////////////////////////////////////////////////
// CollideMax

CollideMax::CollideMax(CollideGeometry* g1, const Quat& q1, const Vector& t1, CollideGeometry* g2, const Quat& q2, const Vector& t2)
{
	mGeometry1 = g1;
	mGeometry2 = g2;
	this->q1 = q1;
	this->t1 = t1;
	this->q2 = q2;
	this->t2 = t2;
}

//////////////////////////////////////////////////////////////////////////////

CollideMax::CollideMax(CollideGeometry* g1, const Vector& t1, CollideGeometry* g2, const Vector& t2)
{
	mGeometry1 = g1;
	mGeometry2 = g2;
	this->q1 = Quat(0, 0, 0, 1);
	this->t1 = t1;
	this->q2 = Quat(0, 0, 0, 1);
	this->t2 = t2;
}

//////////////////////////////////////////////////////////////////////////////

CollideMax::CollideMax(CollideGeometry* g1, CollideGeometry* g2)
{
	mGeometry1 = g1;
	mGeometry2 = g2;
	this->q1 = Quat(0, 0, 0, 1);
	this->t1 = Vector(0, 0, 0);
	this->q2 = Quat(0, 0, 0, 1);
	this->t2 = Vector(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideMax::GetSupportPoint(const Vector& n)
{
	Vector v1 = q1.Rotate(mGeometry1->GetSupportPoint((~q1).Rotate(n))) + t1;
	Vector v2 = q2.Rotate(mGeometry2->GetSupportPoint((~q2).Rotate(n))) + t2;

	if ( (v2-v1) * n > 0 )
	{
		return v2;
	}

	return v1;
}

//////////////////////////////////////////////////////////////////////////////

Vector CollideMax::GetCenter()
{
	// Return the average of the two centers
	return 0.5f * (q1.Rotate(mGeometry1->GetCenter()) + t1 + q2.Rotate(mGeometry2->GetCenter()) + t2);
}

//////////////////////////////////////////////////////////////////////////////

