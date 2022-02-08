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
#include "DrawUtil.h"
#include "HullMaker.h"
#include "ListVector.h"
#include "ShrinkWrap.h"

#include <list>
#include <set>

using namespace std;

struct ClipVert
{
	Vector		point;
	int32		clipTimeStamp;
	int32		refCount;
};

struct ClipFace;

struct ClipEdge
{
	ClipVert*	vert1;
	ClipVert*	vert2;
	ClipFace*	face1;
	ClipFace*	face2;
};

struct ClipFace
{
	list<ClipEdge*>	edge;
};

struct ClipTri
{
	Vector n1;
	Vector n2;
	Vector n3;
	int32 generation;
};

void ShrinkWrap(HullMaker* hullMaker, CollideGeometry& g, int32 generationThreshold)
{
	hullMaker->Clear();

	float32 distanceThreshold = 0.0f;

	if (generationThreshold < 0) generationThreshold = 3;

	list <ClipTri*> activeTriList;

	Vector v[6] =
	{
		Vector( -1,  0,  0 ),
		Vector(  1,  0,  0 ),

		Vector(  0, -1,  0 ),
		Vector(  0,  1,  0 ),

		Vector(  0,  0, -1 ),
		Vector(  0,  0,  1 ),
	};

	int32 kTriangleVerts[8][3] =
	{
		{ 5, 1, 3 },
		{ 4, 3, 1 },
		{ 3, 4, 0 },
		{ 0, 5, 3 },

		{ 5, 2, 1 },
		{ 4, 1, 2 },
		{ 2, 0, 4 },
		{ 0, 2, 5 }
	};

	for (int32 i=0; i < 8; i++)
	{
		ClipTri* tri = new ClipTri;
		tri->n1 = v[ kTriangleVerts[i][0] ];
		tri->n2 = v[ kTriangleVerts[i][1] ];
		tri->n3 = v[ kTriangleVerts[i][2] ];
		tri->generation = 0;

		activeTriList.push_back(tri);
	}

	set<ListVector> pointSet;

	while (activeTriList.size() > 0)
	{
		ClipTri* tri = activeTriList.front();
		activeTriList.pop_front();

		Vector p1 = g.GetSupportPoint( tri->n1 );
		Vector p2 = g.GetSupportPoint( tri->n2 );
		Vector p3 = g.GetSupportPoint( tri->n3 );

		if (pointSet.find(p1) == pointSet.end())
		{
			hullMaker->AddPoint(p1);
			pointSet.insert(p1);
		}
		if (pointSet.find(p2) == pointSet.end())
		{
			hullMaker->AddPoint(p2);
			pointSet.insert(p2);
		}
		if (pointSet.find(p3) == pointSet.end())
		{
			hullMaker->AddPoint(p3);
			pointSet.insert(p3);
		}

		float32 d1 = (p2 - p1).Len3();
		float32 d2 = (p3 - p2).Len3();
		float32 d3 = (p1 - p3).Len3();

		if ( Max( Max(d1, d2), d3 ) > distanceThreshold && tri->generation < generationThreshold )
		{
			ClipTri* tri1 = new ClipTri;
			ClipTri* tri2 = new ClipTri;
			ClipTri* tri3 = new ClipTri;
			ClipTri* tri4 = new ClipTri;

			tri1->generation = tri->generation+1;
			tri2->generation = tri->generation+1;
			tri3->generation = tri->generation+1;
			tri4->generation = tri->generation+1;

			tri1->n1 = tri->n1;
			tri2->n2 = tri->n2;
			tri3->n3 = tri->n3;

			Vector n = 0.5f * (tri->n1 + tri->n2);
			n.Normalize3();

			tri1->n2 = n;
			tri2->n1 = n;
			tri4->n3 = n;

			n = 0.5f * (tri->n2 + tri->n3);
			n.Normalize3();

			tri2->n3 = n;
			tri3->n2 = n;
			tri4->n1 = n;

			n = 0.5f * (tri->n3 + tri->n1);
			n.Normalize3();

			tri1->n3 = n;
			tri3->n1 = n;
			tri4->n2 = n;

			activeTriList.push_back(tri1);
			activeTriList.push_back(tri2);
			activeTriList.push_back(tri3);
			activeTriList.push_back(tri4);
		}

		delete tri;
	}

	if ( !hullMaker->CreateHull() )
	{
		// CreateHull will fail if the shape is one or two dimensional...  so sweep it with a small sphere and try again
		MapPtr<CollideSphere> s = new CollideSphere(0.1f);
		MapPtr<CollideSum> sum = new CollideSum(&g, s);
		ShrinkWrap(hullMaker, *(sum.Pointer()), generationThreshold);
	}
}
