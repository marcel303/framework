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

#include <stdio.h>
#include <map>

#include "DrawUtil.h"
#include "HullMaker.h"
#include "ListVector.h"
#include "MathUtil.h"
#include "RenderPolytope.h"
#include "ShrinkWrap.h"

#include "framework.h"

using namespace std;

extern bool gCullFrontFace;

RenderPolytope::RenderPolytope() : mFaces(NULL), mVerts(NULL), mFaceCount(0), mVertCount(0), mListValid(false)
{
}

RenderPolytope::~RenderPolytope()
{
	int32 i;

	for (i=0; i < mFaceCount; i++)
	{
		delete[] mFaces[i].mVertList;
	}
	delete[] mFaces;
	mFaces = NULL;

	delete[] mVerts;
	mVerts = NULL;

	if (mListValid)
	{
	// todo : free cached mesh
		mListValid = false;
	}
}

void RenderPolytope::Init(CollideGeometry& geom, int32 level)
{
	HullMaker hullMaker;
	ShrinkWrap(&hullMaker, geom, level);
	Init(hullMaker);
}

void RenderPolytope::Init(HullMaker& hullMaker)
{
	mVertCount = (int32) hullMaker.m_vertCount;
	mFaceCount = (int32) hullMaker.m_faceCount;

	mVerts = new Vector[ mVertCount ];
	mFaces = new RenderFace[ mFaceCount ];

	map< ListVector, int32 > indexOfVert;

	int32 i = 0;

	while (!hullMaker.m_faceEdges.empty())
	{
		HullMaker::EdgeList faceEdges;
		HullMaker::Edge* e = hullMaker.m_faceEdges.front();
		Vector n (e->n1.X(), e->n1.Y(), e->n1.Z());
		int32 id = e->id1;
		hullMaker.m_faceEdges.pop_front();
		faceEdges.push_back(e);

		while (!hullMaker.m_faceEdges.empty() && hullMaker.m_faceEdges.front()->id1 == id)
		{
			e = hullMaker.m_faceEdges.front();
			hullMaker.m_faceEdges.pop_front();
			faceEdges.push_back(e);
		}

		mFaces[i].mNormal = n;
		mFaces[i].mVertList = new int32[faceEdges.size()];
		mFaces[i].mVertCount = (int32) faceEdges.size();

		int32 j = 0;

		while (!faceEdges.empty())
		{
			HullMaker::Edge* e = faceEdges.front();
			faceEdges.pop_front();

			Vector p(e->p1.X(), e->p1.Y(), e->p1.Z());
			delete e;

			if (indexOfVert.find(p) == indexOfVert.end())
			{
				mVerts[indexOfVert.size()] = p;
				indexOfVert[p] = (int32) indexOfVert.size();
			}
			mFaces[i].mVertList[j++] = indexOfVert[p];
		}

		i++;
	}

	mColor = Vector( RandFloat(0.5f, 1.0f), RandFloat(0.5f, 1.0f), RandFloat(0.5f, 1.0f) );
//	mColor = Vector( 0.5f, 0.5f, 0.5f );
}

void RenderPolytope::Draw(const Quat& q, const Vector& x)
{
	gxPushMatrix();
	SetTransform(x, q);
	gxColor4f( mColor.X(), mColor.Y(), mColor.Z(), 1.0f );

	if (mListValid == false)
	{
		// todo : cache mesh
	}
	
	if (gCullFrontFace)
	{
		pushCullMode(CULL_FRONT, CULL_CCW);
	}

	if (mListValid)
	{
		// todo : draw cached mesh
	}
	else
	{
		for (int32 i=0; i < mFaceCount; i++)
		{
			gxBegin(GX_TRIANGLE_FAN);
			{
				gxNormal3f(mFaces[i].mNormal.X(), mFaces[i].mNormal.Y(), mFaces[i].mNormal.Z());

				for (int32 j=0; j < mFaces[i].mVertCount; j++)
				{
					const Vector& v = mVerts[mFaces[i].mVertList[j]];
					gxVertex3f(v.X(), v.Y(), v.Z());
				}
			}
			gxEnd();
		}
	}

	if (gCullFrontFace)
	{
		pushBlend(BLEND_ALPHA);
		popCullMode();
		gxColor4f( mColor.X(), mColor.Y(), mColor.Z(), 0.5f );
	}

	if (mListValid)
	{
		// todo : draw cached mesh
	}
	else
	{
		for (int32 i=0; i < mFaceCount; i++)
		{
			gxBegin(GX_TRIANGLE_FAN);
			{
				gxNormal3f(mFaces[i].mNormal.X(), mFaces[i].mNormal.Y(), mFaces[i].mNormal.Z());

				for (int32 j=0; j < mFaces[i].mVertCount; j++)
				{
					Vector v = mVerts[mFaces[i].mVertList[j]];
					gxVertex3f(v.X(), v.Y(), v.Z());
				}
			}
			gxEnd();
		}
	}

	if (gCullFrontFace)
	{
		popBlend();
	}

	gxPopMatrix();
}

void RenderPolytope::SetColor(const Vector& color)
{
	mColor = color;
	if (mListValid)
	{
		// todo : free cached mesh
		// todo : use a shader to change color
		mListValid = false;
	}
}
