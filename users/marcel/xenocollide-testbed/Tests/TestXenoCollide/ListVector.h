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
// ListVector is a helper class.
//
// Vector objects must be aligned to 16 bytes.  Unfortunately, the STL
// containers don't guarantee 16-byte alignment.  ListVector is a simple
// data structure that can hold the same data used by Vector, but doesn't
// require 16-byte alignment.  ListVectors are used whenever Vectors need
// to be stored in an STL container.

class ListVector
{

public:

	ListVector(const Vector& v) { *this = v; }
	ListVector operator= (const Vector& v) { x = v.X(); y = v.Y(); z = v.Z(); return *this; }
	operator Vector () { return Vector(x, y, z); }
	float x;
	float y;
	float z;
};

inline bool operator < (const ListVector& a, const ListVector& b)
{
	if (a.x < b.x) return true;
	if (a.x > b.x) return false;
	if (a.y < b.y) return true;
	if (a.y > b.y) return false;
	if (a.z < b.z) return true;
	if (a.z > b.z) return false;
	return false;
}
