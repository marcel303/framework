#pragma once

#include <assert.h>
#include <stdint.h>
#include "SIMD.h"

class ISPlane;
class ISSphere;
class ISAABB;
class ISOBB;
class ISConvex;

typedef const ISPlane & ISPlaneArg;

enum ISC
{
	kISCOutside = 0,
	kISCIntersecting = 1,
	kISCInside = 2
};

class ISPlane : public SimdVec
{
public:
	inline ISPlane()
	{
	}

	inline ISPlane(SimdVecArg equation)
		: SimdVec(equation.Vec128())
	{
	}

	inline ISPlane(SimdVecArg n, SimdVecArg d)
	{
		Vec128() = n.Select(d, false, false, false, true).Vec128();
	}

	inline ISPlane(SimdVecArg p1, SimdVecArg p2, SimdVecArg p3)
	{
		SimdVec d1 = p2.Sub(p1);
		SimdVec d2 = p3.Sub(p2);

		SimdVec n = d1.Cross3(d2);
		SimdVec d = n.Dot3(p1);

		Vec128() = SimdVec(n.ReplicateX(), n.ReplicateY(), n.ReplicateZ(), d.Neg()).Vec128();
	}

	inline SimdVec Distance3(SimdVecArg v) const
	{
		return Dot3(v).Add(ReplicateW());
	}

	inline SimdVec Distance4(SimdVecArg v) const
	{
		assert(v(3) == 1.0f);

		return Dot4(v);
	}
};

class ISSphere
{
public:
	inline ISSphere()
	{
	}

	inline ISSphere(SimdVecArg center, float radius)
		: mCenter(center)
		, mRadius(radius)
		, mRadiusSq(radius * radius)
	{
	}

	void Set(SimdVecArg center, float radius)
	{
		mCenter = center;
		mRadius = radius;
		mRadiusSq = radius * radius;
	}

	SimdVec mCenter;
	float mRadius;
	float mRadiusSq;

	SimdVec MinMax(SimdVecArg n) const;
};

class ISAABB
{
public:
	inline ISAABB()
	{
	}

	inline ISAABB(SimdVecArg center, SimdVecArg ext)
		: mCenter(center.Select(SimdVec(VONE), false, false, false, true))
		, mExt(ext)
	{
	}

	void Set(SimdVecArg center, SimdVecArg ext)
	{
		mCenter = center.Select(SimdVec(VONE), false, false, false, true);
		mExt = ext;
	}

	void SetFromMinMax(SimdVecArg min, SimdVecArg max)
	{
		SimdVec center = min.Add(max).Mul(SimdVec(0.5f));
		SimdVec ext = max.Sub(min).Mul(SimdVec(0.5f));
		Set(center, ext);
	}

	SimdVec mCenter;
	SimdVec mExt;

	SimdVec MinMax(SimdVecArg n) const;
};

class ISOBB
{
public:
	inline ISOBB()
	{
	}

	inline ISOBB(SimdVecArg center, SimdVecArg extX, SimdVecArg extY, SimdVecArg extZ)
		: mCenter(center)
		, mExtX(extX)
		, mExtY(extY)
		, mExtZ(extZ)
	{
	}

	void Set(SimdVecArg center, SimdVecArg extX, SimdVecArg extY, SimdVecArg extZ)
	{
		mCenter = center;
		mExtX = extX;
		mExtY = extY;
		mExtZ = extZ;
	}

	SimdVec mCenter;
	SimdVec mExtX;
	SimdVec mExtY;
	SimdVec mExtZ;

	SimdVec MinMax(SimdVecArg n) const;
};

class ISConvex
{
public:
	inline ISConvex()
		: mNumPlanes(0)
		, mNumPoints(0)
	{
	}

	inline void SetFromFrustumPoints(const SimdVec * points, uint32_t numPoints)
	{
		assert(numPoints == 8);

		for (uint32_t i = 0; i < 4; ++i)
		{
			uint32_t i1 = i;
			uint32_t i2 = (i + 1) & 3;
			uint32_t i3 = i + 4;

			SimdVec p1 = points[i1];
			SimdVec p2 = points[i2];
			SimdVec p3 = points[i3];

			AddPoint(p1);
			AddPoint(p3);

			ISPlane plane(p1, p2, p3);

			AddPlane(plane);
		}
	}

	inline void AddPlane(ISPlaneArg plane)
	{
		mPlanes[mNumPlanes++] = plane;
	}

	inline void AddPoint(SimdVecArg point)
	{
		mPoints[mNumPoints++] = point;
	}

	inline void Finalize()
	{
		assert(mNumPoints > 0);
		assert(mNumPlanes > 0);

		SimdVec min;
		SimdVec max;

		min = max = mPoints[0];

		for (uint32_t i = 1; i < mNumPoints; ++i)
		{
			min = min.Min(mPoints[i]);
			max = max.Max(mPoints[i]);
		}

		mBox.SetFromMinMax(min, max);
	}

	//

	const static uint32_t kMaxPlanes = 16;
	const static uint32_t kMaxPoints = 32;

	ISPlane mPlanes[kMaxPlanes];
	SimdVec mPoints[kMaxPoints];
	uint32_t mNumPlanes;
	uint32_t mNumPoints;
	ISAABB mBox;
};

inline bool Intersects(const ISAABB & v1, const ISPlane & v2);
inline bool Intersects(const ISAABB & v1, const ISAABB & v2);
inline bool Intersects(const ISAABB & v1, const ISSphere & v2);
inline bool Intersects(const ISAABB & v1, const ISConvex & v2); // todo
inline bool CSIntersect(const ISAABB & v1, const ISSphere & v2, SimdVec & rNormal);

inline bool Intersects(const ISPlane & v1, const ISSphere & v2);
inline bool Intersects(const ISPlane & v1, const ISAABB & v2);

inline bool Intersects(const ISSphere & v1, const ISPlane & v2);
inline bool Intersects(const ISSphere & v1, const ISAABB & v2);
inline bool Intersects(const ISSphere & v1, const ISOBB & v2);
inline bool Intersects(const ISSphere & v1, const ISConvex & v2); // todo

inline bool Intersects(const ISOBB & v1, const ISSphere & v2); // todo

inline bool Intersects(const ISConvex & v1, const ISAABB & v2);
inline bool Intersects(const ISConvex & v1, const ISOBB & v2); // todo

inline ISC TestIntersection(const ISPlane & v1, const ISAABB & v2); // todo
inline ISC TestIntersection(const ISConvex & v1, const ISAABB & v2); // todo
inline ISC TestIntersection(const ISConvex & v1, const ISSphere & v2); // todo
inline ISC TestIntersection(const ISConvex & v1, const ISOBB & v2); // todo

#include "IntersectionInline.h"
