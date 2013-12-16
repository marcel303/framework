#pragma once

#include <assert.h>
#include "Intersection.h"

inline SimdVec ISSphere::MinMax(SimdVecArg n) const
{
	assert(mCenter(3) == 1.0f);
	SimdVec d = n.Dot4(mCenter);
	SimdVec min = d.Sub(SimdVec(mRadius));
	SimdVec max = d.Add(SimdVec(mRadius));
	return SimdVec(min, max);
}

inline SimdVec ISAABB::MinMax(SimdVecArg n) const
{
	assert(mCenter(3) == 1.0f);
	SimdVec dc = n.Dot4(mCenter);
	SimdVec de = n.Abs().Dot3(mExt.Abs());
	return SimdVec(dc.Sub(de), dc.Add(de));
}

inline SimdVec ISOBB::MinMax(SimdVecArg n) const
{
	assert(mCenter(3) == 1.0f);
	SimdVec dc = n.Dot4(mCenter);
	SimdVec dex = n.Dot3(mExtX.Abs());
	SimdVec dey = n.Dot3(mExtY.Abs());
	SimdVec dez = n.Dot3(mExtZ.Abs());
	SimdVec de = dex.Add(dey).Add(dez);
	return SimdVec(dc.Sub(de), dc.Add(de));
}

inline bool Intersects(const ISAABB & v1, const ISPlane & v2)
{
	SimdVec minMax = v1.MinMax(v2);

	return
		minMax.ReplicateX().ALL_LE4(v2.ReplicateW()) &&
		minMax.ReplicateY().ALL_GE4(v2.ReplicateW());
}

inline bool Intersects(const ISAABB & v1, const ISAABB & v2)
{
	SimdVec d = v1.mCenter.Sub(v2.mCenter);
	SimdVec e = v1.mExt.Add(v2.mExt);
	return d.Abs().ALL_LE4(e);
}

static SimdVec ClosestPointToPointOnBox(const ISAABB & box, SimdVecArg point)
{
	SimdVec d = point.Sub(box.mCenter);
	SimdVec p = d.Max(box.mExt.Neg()).Min(box.mExt);
	return box.mCenter.Add(p);
}

inline bool Intersects(const ISAABB & v1, const ISSphere & v2)
{
	SimdVec closest = ClosestPointToPointOnBox(v1, v2.mCenter);
	SimdVec d = v2.mCenter.Sub(closest);
	return d.Dot3(d).ALL_LE4(SimdVec(v2.mRadiusSq));
}

inline bool CSIntersect(const ISAABB & v1, const ISSphere & v2, SimdVec & rNormal)
{
	SimdVec closest = ClosestPointToPointOnBox(v1, v2.mCenter);
	SimdVec d = v2.mCenter.Sub(closest);
	SimdVec d2 = d.Dot3(d);
	if (d2.ANY_GT4(SimdVec(v2.mRadiusSq)))
		return false;
	//SimdVec d3 = v2.mCenter.Sub(v1.mCenter);
	SimdVec d3 = v2.mCenter.Sub(closest);
	rNormal = d3.UnitVec3();
	return true;
}

inline bool Intersects(const ISPlane & v1, const ISSphere & v2)
{
	SimdVec d = v1.Distance4(v2.mCenter);
	return d.ALL_GE4(SimdVec(-v2.mRadius)) && d.ALL_LE4(SimdVec(+v2.mRadius));
}

inline bool Intersects(const ISPlane & v1, const ISAABB & v2)
{
	SimdVec minMax = v2.MinMax(v1).Sub(v1.ReplicateW());

	return
		minMax.ReplicateX().ALL_LE4(SimdVec(VZERO)) &&
		minMax.ReplicateY().ALL_GE4(SimdVec(VZERO));
}

inline bool Intersects(const ISSphere & v1, const ISPlane & v2)
{
	return Intersects(v2, v1);
}

inline bool Intersects(const ISSphere & v1, const ISAABB & v2)
{
	return Intersects(v2, v1);
}

inline bool Intersects(const ISSphere & v1, const ISOBB & v2)
{
	assert(false);
	return false;
}

inline bool Intersects(const ISConvex & v1, const ISAABB & v2)
{
	if (!Intersects(v1.mBox, v2))
		return false;

	for (uint32_t i = 0; i < v1.mNumPlanes; ++i)
	{
		SimdVec d = v2.MinMax(v1.mPlanes[i]);

		if (d.ALL_GT4(SimdVec(VZERO)))
			return false;
	}

	return true;
}

inline ISC TestIntersection(const ISConvex & v1, const ISAABB & v2)
{
	for (uint32_t i = 0; i < v1.mNumPlanes; ++i)
	{
		ISC isc = TestIntersection(v1.mPlanes[i], v2);

		if (isc == kISCOutside)
			return isc;
	}

	return kISCIntersecting;
}
