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

#include "Math/Math.h"

//////////////////////////////////////////////////////////////////////////////
// This file (and its associated *.cpp file) contain the implementation of
// the XenoCollide algorithm.

class CollideGeometry;

//////////////////////////////////////////////////////////////////////////////
// Intersect() is the simplest XenoCollide routine.  It returns true if two
// CollideGeometry objects overlap, or false if they do not.

bool Intersect(CollideGeometry& p1, const Quat& q1, const Vector& t1, CollideGeometry& p2, const Quat& q2, const Vector& t2, float32 boundaryTolerance);

//////////////////////////////////////////////////////////////////////////////
// CollideAndFindPoint() also determines whether or not two CollideGeometry
// objects overlap.  If they do, it finds contact information.

bool CollideAndFindPoint(CollideGeometry& p1, const Quat& q1, const Vector& t1, CollideGeometry& p2, const Quat& q2, const Vector& t2, Vector* returnNormal, Vector* point1, Vector* point2);

//////////////////////////////////////////////////////////////////////////////
// TransformSupportVert() finds the support point for a rotated and/or
// translated CollideGeometry.

Vector TransformSupportVert( CollideGeometry& p, const Quat& q, const Vector& t, const Vector& n );
