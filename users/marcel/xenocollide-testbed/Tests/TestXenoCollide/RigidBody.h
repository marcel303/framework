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

#include "CollideGeometry.h"
#include "Constraint.h"
#include "MapPtr.h"
#include "RenderPolytope.h"

//////////////////////////////////////////////////////////////////////////////
// RigidBody
//
// These are the objects you see in the physics simulation within the demo.
// Each RigidBody object contains a physics model (Body), a collision model
// (CollideGeometry), and render model (RenderPolytope) and a unique color.
//
// The physics and render models are created directly from the collision
// model, which itself is a support mapping that represents the shape.

class RigidBody
{

public:

	RigidBody(Body* b, CollideGeometry* cg, RenderPolytope* rp, float32 radius);

	Vector						color;
	MapPtr<Body>				body;
	MapPtr<CollideGeometry>		collideModel;
	MapPtr<RenderPolytope>		renderModel;
	float32						maxRadius;

};

