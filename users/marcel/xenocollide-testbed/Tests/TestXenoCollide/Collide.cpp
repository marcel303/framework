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

#include "Collide.h"
#include "CollideGeometry.h"
#include "DebugWidget.h"

//////////////////////////////////////////////////////////////////////////////

inline Vector TransformSupportVert( CollideGeometry& p, const Quat& q, const Vector& t, const Vector& n )
{
	Vector localNormal = (~q).Rotate(n);
	Vector localSupport = p.GetSupportPoint(localNormal);
	Vector worldSupport = q.Rotate(localSupport) + t;
	return worldSupport;
}

//////////////////////////////////////////////////////////////////////////////
// Collide

// Global Test Data
float32 gAvgSupportCount = 0.0f;
int32 gMaxPhase1 = 0;
int32 gMaxPhase2 = 0;
int32 gMinPhase1 = 1000;
int32 gMinPhase2 = 1000;
float32 gAvgPhase1 = 0;
float32 gAvgPhase2 = 0;
float32 gCountPhase1 = 0;
float32 gCountPhase2 = 0;

//////////////////////////////////////////////////////////////////////////////

inline void Swap(Vector& a, Vector& b)
{
	Vector tmp = a;
	a = b;
	b = tmp;
}

#define USE_INSTRUMENTED_XENOCOLLIDE

#ifdef USE_INSTRUMENTED_XENOCOLLIDE

//////////////////////////////////////////////////////////////////////////////

bool CollideAndFindPoint(CollideGeometry& p1, const Quat& q1, const Vector& t1, CollideGeometry& p2, const Quat& q2, const Vector& t2, Vector* returnNormal, Vector* point1, Vector* point2)
{
	int32 supportCount = 0;

	static float32 kCollideEpsilon = 1e-3f;
	static int32 callCount = 0;

	callCount++;

	MapPtr<CollideNeg> neg = new CollideNeg(&p1, q1, t1);
	MapPtr<CollideSum> diff = new CollideSum(&p2, q2, t2, neg, Quat(0, 0, 0, 1), Vector(0, 0, 0));
	DebugPushPolytope(*diff, Quat(0, 0, 0, 1), Vector(0, 0, 0));

	DebugPushPoint(Vector(0, 0, 0));

	// v0 = center of Minkowski sum
	Vector v01 = q1.Rotate(p1.GetCenter()) + t1;
	Vector v02 = q2.Rotate(p2.GetCenter()) + t2; 
	Vector v0 = v02 - v01;
	DebugPushPoint(v0);
	DebugPushVector(v0, -v0);

	// Avoid case where centers overlap -- any direction is fine in this case
	if (v0.IsZero3()) v0 = Vector(0.00001f, 0, 0);

	// v1 = support in direction of origin
	Vector n = -v0;
	supportCount++;
	Vector v11 = TransformSupportVert(p1, q1, t1, -n);
	Vector v12 = TransformSupportVert(p2, q2, t2, n);
	Vector v1 = v12 - v11;
	DebugPushVector(v0, v0+n, 4);
	DebugPushPoint(v1, 4);
	if (v1 * n <= 0)
	{
		if (returnNormal) *returnNormal = n;
		return false;
	}

	// v2 - support perpendicular to v1,v0
	n = v1 % v0;
	if (n.IsZero3())
	{
		n = v1 - v0;
		n.Normalize3();
		if (returnNormal) *returnNormal = n;
		if (point1) *point1 = v11;
		if (point2) *point2 = v12;
		return true;
	}
	DebugPushVector(v0, v0+n, 4);
	supportCount++;
	Vector v21 = TransformSupportVert(p1, q1, t1, -n);
	Vector v22 = TransformSupportVert(p2, q2, t2, n);
	Vector v2 = v22 - v21;
	DebugPushPoint(v2, 4);
	if (v2 * n <= 0)
	{
		if (returnNormal) *returnNormal = n;
		return false;
	}

	// Determine whether origin is on + or - side of plane (v1,v0,v2)
	n = (v1 - v0) % (v2 - v0);
	float32 dist = n * v0;

	ASSERT( !n.IsZero3() );

	// If the origin is on the - side of the plane, reverse the direction of the plane
	if (dist > 0)
	{
		Swap(v1, v2);
		Swap(v11, v21);
		Swap(v12, v22);
		n = -n;
	}
	DebugPushVector(v0, v0+n, 4);

	///
	// Phase One: Identify a portal

	while (1)
	{
		// Obtain the support point in a direction perpendicular to the existing plane
		// Note: This point is guaranteed to lie off the plane
		supportCount++;
		Vector v31 = TransformSupportVert(p1, q1, t1, -n);
		Vector v32 = TransformSupportVert(p2, q2, t2, n); 
		Vector v3 = v32 - v31;
		DebugPushTri(v1, v2, v3, Vector(1, 1, 0.5), 2);
		if (v3 * n <= 0)
		{
			if (returnNormal) *returnNormal = n;
			return false;
		}
		
		// If origin is outside (v1,v0,v3), then eliminate v2 and loop
		if (v1 % v3 * v0 < 0)
		{
			v2 = v3;
			v21 = v31;
			v22 = v32;
			n = (v1 - v0) % (v3 - v0);
			continue;
		}

		// If origin is outside (v3,v0,v2), then eliminate v1 and loop
		if (v3 % v2 * v0 < 0)
		{
			v1 = v3;
			v11 = v31;
			v12 = v32;
			n = (v3 - v0) % (v2 - v0);
			continue;
		}

		bool hit = false;

		///
		// Phase Two: Refine the portal

		int32 phase2 = 0;

		// We are now inside of a wedge...
		while (1)
		{
			phase2++;
			if (phase2 > 1)
			{
				static bool doneIt = false;
				if (!gTrackingOn && !doneIt)
				{
					doneIt = true;
					g_debugQueue.clear();
					gTrackingOn = true;
					CollideAndFindPoint(p1, q1, t1, p2, q2, t2, returnNormal, point1, point2);
					gTrackingOn = false;
					return false;
				}
			}

			// Compute normal of the wedge face
			n = (v2 - v1) % (v3 - v1);
			DebugPushTri(v1, v2, v3, Vector(0.5, 1, 0.5f), 1);
			DebugPushTri(v1, v2, v3, Vector(1, 0.5f, 0.5f), 4);
			DebugPushVector((v1+v2+v3)/3, (v1+v2+v3)/3+n, 4);

			// Can this happen???  Can it be handled more cleanly?
			if (n.IsZero3())
			{
				ASSERT(0);
				return true;
			}

			n.Normalize3();

			// Compute distance from origin to wedge face
			float32 d = n * v1;

			// If the origin is inside the wedge, we have a hit
			if (d >= 0 && !hit)
			{

				if (returnNormal) *returnNormal = n;

				// Compute the barycentric coordinates of the origin

				float32 b0 = v1 % v2 * v3;
				float32 b1 = v3 % v2 * v0;
				float32 b2 = v0 % v1 * v3;
				float32 b3 = v2 % v1 * v0;

				float32 sum = b0 + b1 + b2 + b3;

				if (sum <= 0)
				{


					b0 = 0;
					b1 = v2 % v3 * n;
					b2 = v3 % v1 * n;
					b3 = v1 % v2 * n;

					sum = b1 + b2 + b3;
				}

				float32 inv = 1.0f / sum;

				if (point1)
				{
					Vector p1 = (b0 * v01 + b1 * v11 + b2 * v21 + b3 * v31) * inv;
					*point1 = p1;
				}

				if (point2)
				{
					Vector p2 = (b0 * v02 + b1 * v12 + b2 * v22 + b3 * v32) * inv;
					*point2 = p2;
				}

				// HIT!!!
				hit = true;
			}

			// Find the support point in the direction of the wedge face
			supportCount++;
			Vector v41 = TransformSupportVert(p1, q1, t1, -n);
			Vector v42 = TransformSupportVert(p2, q2, t2, n); 
			Vector v4 = v42 - v41;
			DebugPushPoint(v4, 4);

			float32 delta = (v4 - v3) * n;
			float32 separation = -(v4 * n);

			// If the boundary is thin enough or the origin is outside the support plane for the newly discovered vertex, then we can terminate
			if ( delta <= kCollideEpsilon || separation >= 0 /*|| phase2 > 300*/ )
			{
				if (returnNormal) *returnNormal = n;

				// MISS!!! (We didn't move closer)
				static int32 maxPhase2 = 0;
				static int32 maxCallCount = 0;
				static float32 avg = 0.0f;
				static int32 hitCount = 0;

				static float32 avgSupportCount = 0.0f;

				if (hit)
				{
					hitCount++;

					avg = (avg * (hitCount-1) + phase2) / hitCount;
					avgSupportCount = (avgSupportCount * (hitCount-1) + supportCount) / hitCount;
					gAvgSupportCount = avgSupportCount;

					if (phase2 > maxPhase2)
					{
						maxPhase2 = phase2;
						maxCallCount = callCount;
					}
				}

				return hit;
			}

			// Compute the tetrahedron dividing face (v4,v0,v1)
			float32 d1 = v4 % v1 * v0;

			// Compute the tetrahedron dividing face (v4,v0,v2)
			float32 d2 = v4 % v2 * v0;

			// Compute the tetrahedron dividing face (v4,v0,v3)
			float32 d3 = v4 % v3 * v0;

			if (d1 < 0)
			{
				if (d2 < 0)
				{
					// Inside d1 & inside d2 ==> eliminate v1
					v1 = v4;
					v11 = v41;
					v12 = v42;
				}
				else
				{
					// Inside d1 & outside d2 ==> eliminate v3
					v3 = v4;
					v31 = v41;
					v32 = v42;
				}
			}
			else
			{
				if (d3 < 0)
				{
					// Outside d1 & inside d3 ==> eliminate v2
					v2 = v4;
					v21 = v41;
					v22 = v42;
				}
				else
				{
					// Outside d1 & outside d3 ==> eliminate v1
					v1 = v4;
					v11 = v41;
					v12 = v42;
				}
			}
		}
	}
}

#else

bool CollideAndFindPoint(CollideGeometry& p1, const Quat& q1, const Vector& t1, CollideGeometry& p2, const Quat& q2, const Vector& t2, Vector* returnNormal, Vector* point1, Vector* point2)
{
	static float32 kCollideEpsilon = 1e-3f;

	// v0 = center of Minkowski sum
	Vector v01 = q1.Rotate(p1.GetCenter()) + t1;
	Vector v02 = q2.Rotate(p2.GetCenter()) + t2; 
	Vector v0 = v02 - v01;

	// Avoid case where centers overlap -- any direction is fine in this case
	if (v0.IsZero3()) v0 = Vector(0.00001f, 0, 0);

	// v1 = support in direction of origin
	Vector n = -v0;
	Vector v11 = TransformSupportVert(p1, q1, t1, -n);
	Vector v12 = TransformSupportVert(p2, q2, t2, n);
	Vector v1 = v12 - v11;
	if (v1 * n <= 0)
	{
		if (returnNormal) *returnNormal = n;
		return false;
	}

	// v2 - support perpendicular to v1,v0
	n = v1 % v0;
	if (n.IsZero3())
	{
		n = v1 - v0;
		n.Normalize3();
		if (returnNormal) *returnNormal = n;
		if (point1) *point1 = v11;
		if (point2) *point2 = v12;
		return true;
	}
	Vector v21 = TransformSupportVert(p1, q1, t1, -n);
	Vector v22 = TransformSupportVert(p2, q2, t2, n);
	Vector v2 = v22 - v21;
	if (v2 * n <= 0)
	{
		if (returnNormal) *returnNormal = n;
		return false;
	}

	// Determine whether origin is on + or - side of plane (v1,v0,v2)
	n = (v1 - v0) % (v2 - v0);
	float32 dist = n * v0;

	ASSERT( !n.IsZero3() );

	// If the origin is on the - side of the plane, reverse the direction of the plane
	if (dist > 0)
	{
		Swap(v1, v2);
		Swap(v11, v21);
		Swap(v12, v22);
		n = -n;
	}

	///
	// Phase One: Identify a portal

	while (1)
	{
		// Obtain the support point in a direction perpendicular to the existing plane
		// Note: This point is guaranteed to lie off the plane
		Vector v31 = TransformSupportVert(p1, q1, t1, -n);
		Vector v32 = TransformSupportVert(p2, q2, t2, n); 
		Vector v3 = v32 - v31;
		if (v3 * n <= 0)
		{
			if (returnNormal) *returnNormal = n;
			return false;
		}
		
		// If origin is outside (v1,v0,v3), then eliminate v2 and loop
		if (v1 % v3 * v0 < 0)
		{
			v2 = v3;
			v21 = v31;
			v22 = v32;
			n = (v1 - v0) % (v3 - v0);
			continue;
		}

		// If origin is outside (v3,v0,v2), then eliminate v1 and loop
		if (v3 % v2 * v0 < 0)
		{
			v1 = v3;
			v11 = v31;
			v12 = v32;
			n = (v3 - v0) % (v2 - v0);
			continue;
		}

		bool hit = false;

		///
		// Phase Two: Refine the portal

		int32 phase2 = 0;

		// We are now inside of a wedge...
		while (1)
		{
			phase2++;
			if (phase2 > 1)
			{
				static bool doneIt = false;
				if (!gTrackingOn && !doneIt)
				{
					doneIt = true;
					g_debugQueue.clear();
					gTrackingOn = true;
					CollideAndFindPoint(p1, q1, t1, p2, q2, t2, returnNormal, point1, point2);
					gTrackingOn = false;
					return false;
				}
			}

			// Compute normal of the wedge face
			n = (v2 - v1) % (v3 - v1);

			// Can this happen???  Can it be handled more cleanly?
			if (n.IsZero3())
			{
				ASSERT(0);
				return true;
			}

			n.Normalize3();

			// Compute distance from origin to wedge face
			float32 d = n * v1;

			// If the origin is inside the wedge, we have a hit
			if (d >= 0 && !hit)
			{

				if (returnNormal) *returnNormal = n;

				// Compute the barycentric coordinates of the origin

				float32 b0 = v1 % v2 * v3;
				float32 b1 = v3 % v2 * v0;
				float32 b2 = v0 % v1 * v3;
				float32 b3 = v2 % v1 * v0;

				float32 sum = b0 + b1 + b2 + b3;

				if (sum <= 0)
				{


					b0 = 0;
					b1 = v2 % v3 * n;
					b2 = v3 % v1 * n;
					b3 = v1 % v2 * n;

					sum = b1 + b2 + b3;
				}

				float32 inv = 1.0f / sum;

				if (point1)
				{
					Vector p1 = (b0 * v01 + b1 * v11 + b2 * v21 + b3 * v31) * inv;
					*point1 = p1;
				}

				if (point2)
				{
					Vector p2 = (b0 * v02 + b1 * v12 + b2 * v22 + b3 * v32) * inv;
					*point2 = p2;
				}

				// HIT!!!
				hit = true;
			}

			// Find the support point in the direction of the wedge face
			Vector v41 = TransformSupportVert(p1, q1, t1, -n);
			Vector v42 = TransformSupportVert(p2, q2, t2, n); 
			Vector v4 = v42 - v41;

			float32 delta = (v4 - v3) * n;
			float32 separation = -(v4 * n);

			// If the boundary is thin enough or the origin is outside the support plane for the newly discovered vertex, then we can terminate
			if ( delta <= kCollideEpsilon || separation >= 0 /*|| phase2 > 300*/ )
			{
				if (returnNormal) *returnNormal = n;
				return hit;
			}

			// Compute the tetrahedron dividing face (v4,v0,v1)
			float32 d1 = v4 % v1 * v0;

			// Compute the tetrahedron dividing face (v4,v0,v2)
			float32 d2 = v4 % v2 * v0;

			// Compute the tetrahedron dividing face (v4,v0,v3)
			float32 d3 = v4 % v3 * v0;

			if (d1 < 0)
			{
				if (d2 < 0)
				{
					// Inside d1 & inside d2 ==> eliminate v1
					v1 = v4;
					v11 = v41;
					v12 = v42;
				}
				else
				{
					// Inside d1 & outside d2 ==> eliminate v3
					v3 = v4;
					v31 = v41;
					v32 = v42;
				}
			}
			else
			{
				if (d3 < 0)
				{
					// Outside d1 & inside d3 ==> eliminate v2
					v2 = v4;
					v21 = v41;
					v22 = v42;
				}
				else
				{
					// Outside d1 & outside d3 ==> eliminate v1
					v1 = v4;
					v11 = v41;
					v12 = v42;
				}
			}
		}
	}
}

#endif

extern int32 gCurrentDebugTick;

void DebugDraw()
{
	int32 currentTick = (int32)g_debugQueue.size();
	DebugWidgetList::iterator it;
	for (it = g_debugQueue.end(); it != g_debugQueue.begin();)
	{
		it--;
		currentTick--;
		if (gCurrentDebugTick >= currentTick)
		{
			int32 lifetime = (*it)->GetLifetime();
			if (lifetime < 0 || gCurrentDebugTick - currentTick < lifetime) (*it)->Draw();
		}
	}
}

///
// bool Intersect(...)
//
// Returns true if two CollideGeometry objects intersect.
//
bool Intersect(CollideGeometry& p1, const Quat& q1, const Vector& t1, CollideGeometry& p2, const Quat& q2, const Vector& t2, float32 boundaryTolerance)
{
	// v0 = center of Minkowski difference
	Vector v0 = (q2.Rotate(p2.GetCenter()) + t2) - (q1.Rotate(p1.GetCenter()) + t1);
	if (v0.IsZero3()) return true;	// v0 and origin overlap ==> hit

	// v1 = support in direction of origin
	Vector n = -v0;
	Vector v1 = TransformSupportVert(p2, q2, t2, n) - TransformSupportVert(p1, q1, t1, -n);
	if (v1 * n <= 0) return false;	// origin outside v1 support plane ==> miss

	// v2 = support perpendicular to plane containing origin, v0 and v1
	n = v1 % v0;
	if (n.IsZero3()) return true;	// v0, v1 and origin colinear (and origin inside v1 support plane) == > hit
	Vector v2 = TransformSupportVert(p2, q2, t2, n) - TransformSupportVert(p1, q1, t1, -n);
	if (v2 * n <= 0) return false;	// origin outside v2 support plane ==> miss

	// v3 = support perpendicular to plane containing v0, v1 and v2
	n = (v1 - v0) % (v2 - v0);

	// If the origin is on the - side of the plane, reverse the direction of the plane
	if (n * v0 > 0)
	{
		Swap(v1, v2);
		n = -n;
	}

	///
	// Phase One: Find a valid portal

	while (1)
	{
		// Obtain the next support point
		Vector v3 = TransformSupportVert(p2, q2, t2, n) - TransformSupportVert(p1, q1, t1, -n);
		if (v3 * n <= 0) return false;	// origin outside v3 support plane ==> miss
		
		// If origin is outside (v1,v0,v3), then portal is invalid -- eliminate v2 and find new support outside face
		if (v1 % v3 * v0 < 0)
		{
			v2 = v3;
			n = (v1 - v0) % (v3 - v0);
			continue;
		}

		// If origin is outside (v3,v0,v2), then portal is invalid -- eliminate v1 and find new support outside face
		if (v3 % v2 * v0 < 0)
		{
			v1 = v3;
			n = (v3 - v0) % (v2 - v0);
			continue;
		}

		///
		// Phase Two: Refine the portal

		while (1)
		{
			// Compute outward facing normal of the portal
			n = (v2 - v1) % (v3 - v1);

			// If the origin is inside the portal, we have a hit
			if (n * v1 >= 0) return true;

			// Find the support point in the direction of the portal's normal
			Vector v4 = TransformSupportVert(p2, q2, t2, n) - TransformSupportVert(p1, q1, t1, -n);

			// If the origin is outside the support plane or the boundary is thin enough, we have a miss
			n.Normalize3();
			if ( -(v4 * n) >= 0 || (v4 - v3) * n <= boundaryTolerance ) return false;

#if 1
			// Test origin against the three planes that separate the new portal candidates: (v1,v4,v0) (v2,v4,v0) (v3,v4,v0)
			// Note:  We're taking advantage of the triple product identities here as an optimization
			//        (v1 % v4) * v0 == v1 * (v4 % v0)    > 0 if origin inside (v1, v4, v0)
			//        (v2 % v4) * v0 == v2 * (v4 % v0)    > 0 if origin inside (v2, v4, v0)
			//        (v3 % v4) * v0 == v3 * (v4 % v0)    > 0 if origin inside (v3, v4, v0)
			Vector cross = v4 % v0;
			if (v1 * cross > 0)
			{
				if (v2 * cross > 0) v1 = v4;	// Inside v1 & inside v2 ==> eliminate v1
				else v3 = v4;					// Inside v1 & outside v2 ==> eliminate v3
			}
			else
			{
				if (v3 * cross > 0) v2 = v4;	// Outside v1 & inside v3 ==> eliminate v2
				else v1 = v4;					// Outside v1 & outside v3 ==> eliminate v1
			}
#else
			// Test origin against the three planes that separate the new portal candidates: (v1,v4,v0) (v2,v4,v0) (v3,v4,v0)
			// Note: non-optimized version
			if (v1 % v4 * v0 > 0)
			{
				if (v2 % v4 * v0 > 0) v1 = v4;	// Inside v1 & inside v2 ==> eliminate v1
				else v3 = v4;					// Inside v1 & outside v2 ==> eliminate v3
			}
			else
			{
				if (v3 % v4 * v0 > 0) v2 = v4;	// Outside v1 & inside v3 ==> eliminate v2
				else v1 = v4;					// Outside v1 & outside v3 ==> eliminate v1
			}
#endif
		}
	}
}
