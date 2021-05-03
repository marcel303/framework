#pragma once

#include "Vec3.h"

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

inline bool intersectCircle3d_2(
	const float px, const float py, const float pz,
	const float dx, const float dy, const float dz,
	const float cx, const float cy, const float cz, const float cr,
	float & t1,
	float & t2)
{
	const float a = dx * dx + dy * dy + dz * dz;
	const float b = 2.f * (dx * (px - cx) + dy * (py - cy) + dz * (pz - cz));
	
	float c = cx * cx + cy * cy + cz * cz;
	c += px * px + py * py + pz * pz;
	c -= 2.f * (cx * px + cy * py + cz * pz);
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

static float intersectCone_squaref(const float x)
{
	return x * x;
}

static bool intersectCone_surface(
	Vec3Arg rayOrigin,
	Vec3Arg rayDirection,
	Vec3Arg coneC, // tip position
	Vec3Arg coneAxis, // axis
	const float coneRadius,
	const float coneHeight,
	float & out_t)
{
	const float coneGamma = coneRadius / coneHeight;            // sin(coneAngle / 2)
	const float coneAlpha = sqrtf(1.f - coneGamma * coneGamma); // cos(coneAngle / 2)
	
	// source: https://www.shadertoy.com/view/MtcXWr
	
    const Vec3 co = rayOrigin - coneC;
    const float coneAlphaSq = coneAlpha * coneAlpha;

    const float a = intersectCone_squaref(rayDirection * coneAxis) - coneAlphaSq;
    const float b = 2.f * ((rayDirection * coneAxis) * (co * coneAxis) - (rayDirection * co) * coneAlphaSq);
    const float c = intersectCone_squaref(co * coneAxis) - (co * co) * coneAlphaSq;

    float det = b * b - 4.f * a * c;
    
    if (det < 0.f)
		return false;

    det = sqrtf(det);
    
    const float t1 = (-b - det) / (2.f * a);
    const float t2 = (-b + det) / (2.f * a);

    float t = t1;
    if (t < 0.f || (t2 > 0.f && t2 < t))
		t = t2;
    if (t < 0.f)
		return false;

	// calculate the distance from the plane running from the tip & perpendicular to the axis, and the point on the cone
	
#if 0
	const Vec3 conePoint = rayOrigin + rayDirection * t;
    
    const float height = conePoint * coneAxis - coneC * coneAxis;
#else
    const Vec3 conePoint = rayOrigin + rayDirection * t - coneC;
    
    const float height = conePoint * coneAxis;
#endif
    
    if (height < 0.f || height > coneHeight)
		return false;

	out_t = t;
	
	return true;
}

static bool intersectCone_base(
	Vec3Arg rayOrigin,
	Vec3Arg rayDirection,
	Vec3Arg coneC,    // tip position
	Vec3Arg coneAxis, // axis
	const float coneRadius,
	const float coneHeight,
	float & out_t)
{
	const Vec3 coneBase = coneC + coneAxis * coneHeight;

	float t1, t2;
	if (intersectCircle3d_2(
		rayOrigin[0],
		rayOrigin[1],
		rayOrigin[2],
		rayDirection[0],
		rayDirection[1],
		rayDirection[2],
		coneBase[0],
		coneBase[1],
		coneBase[2],
		coneRadius,
		t1,
		t2) == false)
	{
		return false;
	}
	else
	{
		const float t = fmaxf(0.f, fminf(t1, t2));
		
		out_t = t;
		
		return true;
	}
}

static bool intersectCone(
	Vec3Arg rayOrigin,
	Vec3Arg rayDirection,
	Vec3Arg coneC,    // tip position
	Vec3Arg coneAxis, // axis
	const float coneRadius,
	const float coneHeight,
	float & out_t,
	bool & out_hitsBase)
{
	float surface_t;
	if (intersectCone_surface(
		rayOrigin,
		rayDirection,
		coneC,
		coneAxis,
		coneRadius,
		coneHeight,
		surface_t) == false)
	{
		surface_t = -1.f;
	}
	
	float base_t;
	if (intersectCone_base(
		rayOrigin,
		rayDirection,
		coneC,
		coneAxis,
		coneRadius,
		coneHeight,
		base_t) == false)
	{
		base_t = -1.f;
	}
	
	if (base_t != -1.f && (base_t < surface_t || surface_t == -1.f))
	{
		out_t = base_t;
		out_hitsBase = true;
		return true;
	}
	
	if (surface_t != -1.f && (surface_t < base_t || base_t == -1.f))
	{
		out_t = surface_t;
		out_hitsBase = false;
		return true;
	}
	
	return false;
}
