#pragma once

#include <algorithm>
#include <math.h>
#include <limits>

/**
 * Intersects a bounding box given by (min, max) with a ray specified by its origin and direction. If there is an intersection, the function returns true, and stores the distance of the point of intersection in 't'. If there is no intersection, the function returns false and leaves 't' unmodified. Note that the ray direction is expected to be the inverse of the actual ray direction, for efficiency reasons.
 * @param min: Minimum values of bounding box extents.
 * @param max: Maximum values of bounding box extents.
 * @param px: Origin X of ray.
 * @param py: Origin Y of ray.
 * @param pz: Origin Z of ray.
 * @param dxInv: Inverse of direction X of ray.
 * @param dyInv: Inverse of direction Y of ray.
 * @param dzInv: Inverse of direction Z of ray.
 * @param t: Stores the distance to the intersection point if there is an intersection.
 * @return: True if there is an intersection. False otherwise.
 */
inline bool intersectBoundingBox3d(
	const float * min,
	const float * max,
	const float px,
	const float py,
	const float pz,
	const float dxInv,
	const float dyInv,
	const float dzInv,
	float & t)
{
	float tmin = std::numeric_limits<float>().min();
	float tmax = std::numeric_limits<float>().max();

	const float p[3] = { px, py, pz };
	const float rd[3] = { dxInv, dyInv, dzInv };

	for (int i = 0; i < 3; ++i)
	{
		const float t1 = (min[i] - p[i]) * rd[i];
		const float t2 = (max[i] - p[i]) * rd[i];

		tmin = std::max(tmin, std::min(t1, t2));
		tmax = std::min(tmax, std::max(t1, t2));
	}

	if (tmax >= tmin)
	{
		t = tmin;

		return true;
	}
	else
	{
		return false;
	}
}

//

#if 0 // todo : doesn't compile on ARM

#include "SimdVec.h"

/**
 * Intersects a bounding box given by (min, max) with a ray specified by its origin and direction. If there is an intersection, the function returns true, and stores the distance of the point of intersection in 't'. If there is no intersection, the function returns false and leaves 't' unmodified. Note that the ray direction is expected to be the inverse of the actual ray direction, for efficiency reasons.
 * @param boxMin: Minimum values of bounding box extents.
 * @param boxMax: Maximum values of bounding box extents.
 * @param rayOrigin: Origin of ray.
 * @param rayDirectionInv: Inverse of direction of ray.
 * @param ioDistance: Stores the distance to the intersection point if there is an intersection.
 * @return: True if there is an intersection. False otherwise.
 */
inline bool intersectBoundingBox3d_simd(
	SimdVecArg boxMin,
	SimdVecArg boxMax,
	SimdVecArg rayOrigin,
	SimdVecArg rayDirectionInv,
	/*SimdVec & ioDistance*/ float & ioDistance)
{
	const static SimdVec infNeg(-std::numeric_limits<float>::infinity());
	const static SimdVec infPos(+std::numeric_limits<float>::infinity());

	// distances
	const SimdVec minVecTemp = boxMin.Sub(rayOrigin).Mul(rayDirectionInv);
	const SimdVec maxVecTemp = boxMax.Sub(rayOrigin).Mul(rayDirectionInv);

	const SimdVec minVec = minVecTemp.Min(maxVecTemp);
	const SimdVec maxVec = minVecTemp.Max(maxVecTemp);

	// calculcate distance
	const SimdVec max =
		     maxVec.ReplicateX()
		.Min(maxVec.ReplicateY()
		.Min(maxVec.ReplicateZ()));
	const SimdVec min =
		SimdVec(VZERO)
		.Max(minVec.ReplicateX()
		.Max(minVec.ReplicateY()
		.Max(minVec.ReplicateZ())));

	// no intersection or greater distance?
	if (min.ANY_GE3(max))
		return false;

	ioDistance = min.X();

	return true;
}

#endif

//

inline bool intersectCircle(
	const float px, const float py,
	const float dx, const float dy,
	const float cx, const float cy, const float cr,
	float & t)
{
	const float a = dx * dx + dy * dy;
	const float b = 2.f * (dx * (px - cx) + dy * (py - cy));
	
	float c = cx * cx + cy * cy;
	c += px * px + py * py;
	c -= 2.f * (cx * px + cy * py);
	c -= cr * cr;
	
	float det = b * b - 4.f * a * c;

  	if (det < 0.f)
		return false;
	else
	{
		const float t_temp = (- b - sqrtf(det)) / (2.f * a);
		
		if (t_temp < 0.f)
			return false;
		
		t = t_temp;
		
		return true;
	}
}

inline bool intersectCircle2(
	const float px, const float py,
	const float dx, const float dy,
	const float cx, const float cy, const float cr,
	float & t1,
	float & t2)
{
	const float a = dx * dx + dy * dy;
	const float b = 2.f * (dx * (px - cx) + dy * (py - cy));
	
	float c = cx * cx + cy * cy;
	c += px * px + py * py;
	c -= 2.f * (cx * px + cy * py);
	c -= cr * cr;
	
	float det = b * b - 4.f * a * c;

  	if (det < 0.f)
		return false;
	else
	{
		t1 = (- b - sqrtf(det)) / (2.f * a);
		t2 = (- b + sqrtf(det)) / (2.f * a);
		
		return true;
	}
}

//

static bool intersectSphere(
	const float px, const float py, const float pz,
	const float dx, const float dy, const float dz,
	const float sx, const float sy, const float sz, const float sr,
	float & t1,
	float & t2)
{
	const float a = dx * dx + dy * dy + dz * dz;
	const float b = 2.f * (dx * (px - sx) + dy * (py - sy) + dz * (pz - sz));
	
	/*
	float c = sx * sx + sy * sy + sz * sz;
	c += px * px + py * py + pz * pz;
	c -= 2.f * (sx * px + sy * py + dz * pz);
	c -= sr * sr;
	*/
	float c =
		(sx - px) * (sx - px) +
		(sy - py) * (sy - py) +
		(sz - pz) * (sz - pz) -
		sr * sr;
	
	float det = b * b - 4.f * a * c;

  	if (det < 0.f)
		return false;
	else
	{
		t1 = (- b - sqrtf(det)) / (2.f * a);
		t2 = (- b + sqrtf(det)) / (2.f * a);
		
		return true;
	}
}
