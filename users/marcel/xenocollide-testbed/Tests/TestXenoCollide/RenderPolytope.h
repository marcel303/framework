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

class CollideGeometry;
class HullMaker;

//////////////////////////////////////////////////////////////////////////////
// RenderPolytope represents a drawable polytope.
//
// You can pass a CollideGeometry to RenderPolytope::Init() to have it create
// a renderable approximation of the shape.

class RenderPolytope
{
public:
	struct RenderFace
	{
		Vector mNormal;
		int32* mVertList;
		int32 mVertCount;
	};

	RenderFace*	mFaces;
	Vector* mVerts;

	int32 mFaceCount;
	int32 mVertCount;

	Vector	mColor;

	bool mListValid;
	uint32 mDrawList;

public:

	RenderPolytope();
	~RenderPolytope();

	void Init(HullMaker& hullMaker);
	void Init(CollideGeometry& geom, int32 level = -1);
	void Draw(const Quat& q, const Vector& x);
	void SetColor(const Vector& color);
};

