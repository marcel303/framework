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

//////////////////////////////////////////////////////////////////////////////
// Body
//
// Each Body represents a three-dimensional rigid body.
//////////////////////////////////////////////////////////////////////////////

#include "MapPtr.h"
#include "Math/Math.h"

namespace XenoCollide
{
	class Body
	{

	public:

		Body() : com(0, 0, 0), x(0, 0, 0), v(0, 0, 0), m(1), inv_m(1), q(0, 0, 0, 1), omega(0, 0, 0) { I.BuildIdentity(); inv_I.BuildIdentity(); }

		Vector com;		// center of mass
		Vector	x;		// position
		Vector	v;		// velocity
		float32	m;		// mass
		float32 inv_m;	// inverse mass (1 / mass)

		Quat	q;		// rotation
		Vector	omega;	// angular velocity
		Matrix	I;		// inertia tensor
		Matrix	inv_I;	// inverse intertia tensor

	};

	//////////////////////////////////////////////////////////////////////////////
	// Constraint
	//
	// A Constraint represents one or more constraints on the physical motion of
	// one or more Body objects.
	//
	// Common examples of Constraints include joints and physical contact.
	//////////////////////////////////////////////////////////////////////////////

	class Constraint
	{

	public:

		virtual void PrepareForIteration() = 0;
		virtual void Iterate() = 0;
		virtual void Draw() {}

	};

	//////////////////////////////////////////////////////////////////////////////
	// ContactConstraint
	//
	// This Constraint represents contact between two Body objects.  The two
	// bodies are allowed to move away from each other along their contact
	// normal, but they cannot move toward each other.
	//
	// Movement perpendicular to the normal results in frictional resistance.
	//////////////////////////////////////////////////////////////////////////////

	class ContactConstraint : public Constraint
	{

	public:

		ContactConstraint(Body* b1, Body* b2, const Vector& r1, const Vector& r2, const Vector& normal);

		virtual void PrepareForIteration();
		virtual void Iterate();
		virtual void Draw();

		void	Update(const Vector& p1, const Vector& p2, const Vector& normal);
		bool	StillValid(float32 tolerance);


	private:

		MapPtr<Body>	m_body1;
		MapPtr<Body>	m_body2;

		Vector	m_r1;
		Vector	m_r2;

		float32	m_positionError;
		Vector m_velocityConstraintDirection;

		Vector	m_invMoment1;
		Vector m_invMoment2;

		float32	m_effectiveMass;

		float32	m_beta;

		float32	m_cachedMomentum;

		Vector	m_cachedTangentMomentum;
	};
}
