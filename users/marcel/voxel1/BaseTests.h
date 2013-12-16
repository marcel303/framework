#pragma once

#include <limits>
#include "SIMD.h"

#if 0
static BOOL BT_IntersectSphere(const VecF& pos, const VecF& dir, const PlaneF& rayPlane, const SphereF& sphere)
{
	const float d = rayPlane * sphere.m_Pos;

	if (d <= -sphere.m_Radius)
		return FALSE;

	VecF delta = sphere.m_Pos - dir * d - pos;

	const float lengthSq = delta.LengthSq_get();

	if (lengthSq >= sphere.m_RadiusSq)
		return FALSE;

	return TRUE;
}

static BOOL BT_IntersectBox(float* minB, float* maxB, int solid, float* ray_origin, float* ray_dir, int fast/*, float* coord*/, float* out_t, float* out_normal)
{
	// From Graphics Gems I.. Fixed a bug in the clipping bit, hehe. :)
	// Also added normal generation.

	const int RIGHT = 0;
	const int LEFT = 1;
	const int MIDDLE = 2;

	BOOL inside = TRUE;
	int quadrant[3];
	float candidatePlane[3];

	for (int i = 0; i < 3; ++i)
	{
		if (ray_origin[i] < minB[i])
		{
#if 0 // todo: remove
			if (ray_dir[i] < 0.0f)
				return 0;
#endif
			quadrant[i] = LEFT;
			candidatePlane[i] = minB[i];
			inside = 0;
		}
		else if (ray_origin[i] > maxB[i])
		{
#if 0 // todo: remove
			if (ray_dir[i] > 0.0f)
				return 0;
#endif
			quadrant[i] = RIGHT;
			candidatePlane[i] = maxB[i];
			inside = 0;
		}
		else
		{
			quadrant[i] = MIDDLE;
		}
	}

	if (inside)
	{
		// TODO: Normal?

		*out_t = 0.0f;

		return TRUE;
	}

	float maxT[3];
	int whichPlane = 3;

	for (int i = 0; i < 3; ++i)
	{
		if (quadrant[i] != MIDDLE && ray_dir[i] != 0.0f)
		{
			maxT[i] = (candidatePlane[i] - ray_origin[i]) / ray_dir[i];

			if (maxT[i] > maxT[whichPlane])
				whichPlane = i;
		}
		else
			maxT[i] = -1.0;
	}

	if (whichPlane == 3)
		return FALSE;

	const float t = maxT[whichPlane];

	for (int i = 0; i < 3; ++i)
	{
		if (i != whichPlane)
		{
			const float coord =
				ray_origin[i] +
				maxT[whichPlane] * ray_dir[i];

			if (coord < minB[i] || coord > maxB[i])
				return FALSE;
		}
	}

	//if (t < 0.0f)
	//	return 0;

	*out_t = t;

	// Calculate normal.
	out_normal[0] = 0.0f;
	out_normal[1] = 0.0f;
	out_normal[2] = 0.0f;

	if (ray_origin[whichPlane] <= minB[whichPlane])
		out_normal[whichPlane] = -1.0f;
	else
		out_normal[whichPlane] = +1.0f;

	return TRUE;
}
#endif

static __forceinline BOOL BT_IntersectBox_FastSIMD(SimdVecArg rayOrigin, SimdVecArg rayDirectionInv, SimdVecArg boxMin, SimdVecArg boxMax, /*SimdVec & ioDistance*/float & ioDistance)
{
	const static SimdVec infNeg(-std::numeric_limits<float>::infinity());
	const static SimdVec infPos(+std::numeric_limits<float>::infinity());

	// distances
	SimdVec minVecTemp = boxMin.Sub(rayOrigin).Mul(rayDirectionInv);
	SimdVec maxVecTemp = boxMax.Sub(rayOrigin).Mul(rayDirectionInv);

	SimdVec minVec = minVecTemp.Min(maxVecTemp);
	SimdVec maxVec = minVecTemp.Max(maxVecTemp);

	// calculcate distance
	const SimdVec max =                    maxVec.ReplicateX().Min(maxVec.ReplicateY().Min(maxVec.ReplicateZ()));
	const SimdVec min = SimdVec(VZERO).Max(minVec.ReplicateX().Max(minVec.ReplicateY().Max(minVec.ReplicateZ())));

	// no intersection or greater distance?
	if (min.ANY_GE3(max))
		return false;

	ioDistance = min.X();

	return true;
}
