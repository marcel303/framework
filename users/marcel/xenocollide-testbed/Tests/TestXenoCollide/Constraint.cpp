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

#include "Constraint.h"

namespace XenoCollide
{
	extern bool gFriction;

	extern float32 gTimeStep;
	extern float32 gTimeRatio;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Implementation of ContactConstraint
	///////////////////////////////////////////////////////////////////////////////////////////////////

	ContactConstraint::ContactConstraint(Body* b1, Body* b2, const Vector& r1, const Vector& r2, const Vector& normal)
	{
		m_body1 = b1;
		m_body2 = b2;
		m_r1 = r1;
		m_r2 = r2;
		m_velocityConstraintDirection = normal;
		m_beta = 1.f;
		m_cachedMomentum = 0;
		m_cachedTangentMomentum = Vector(0, 0, 0);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////

	void ContactConstraint::PrepareForIteration()
	{
		Vector r1 = m_body1->q.Rotate(m_r1);
		Vector r2 = m_body2->q.Rotate(m_r2);

		Vector x1 = m_body1->x + r1;
		Vector x2 = m_body2->x + r2;

		// Compute the positional constraint error (scaled by the Baumgarte coefficient 'm_beta')
	//	m_beta = 0.98f / gTimeStep;
		m_beta = 0.50f / gTimeStep;
		m_positionError = m_beta * (x2 - x1) * m_velocityConstraintDirection;

		// Add a boundary layer to the position error -- this will ensure the objects remain in contact
	//	m_positionError -= m_beta * 0.01f;
		m_positionError -= 1.0f;

		// Add a boundary layer to the position error and clamp the result
	//	m_positionError = Max(0.0f, m_positionError - 1.0f);

		// The velocity constraint direction is aligned with the contact normal
		// (This represents how much angular velocity we get for every unit of momentum transferred along the constraint direction)
		m_invMoment1 = m_body1->inv_I * (r1 % m_velocityConstraintDirection);
		m_invMoment2 = m_body2->inv_I * (r2 % m_velocityConstraintDirection);

		// Compute effective mass of the constraint system -- this is a measure of how easy it
		// is to accelerate the contact points apart along the constraint direction -- it's analogous
		// to effective resistance in an electric circuit [i.e., 1 / (1/R1 + 1/R2)]
		m_effectiveMass =
			1.0f /
			(
				m_body1->inv_m +
				m_body2->inv_m +
				m_velocityConstraintDirection *
				(
					m_invMoment1 % r1 +
					m_invMoment2 % r2
					)
				);

		// Convert last frame's momentum to momentum for the new time step
		float32 timeRatio = gTimeRatio;
		m_cachedMomentum *= gTimeRatio;
		m_cachedTangentMomentum *= gTimeRatio;

		// Apply last frame's momentum
		m_body1->v -= m_cachedMomentum * m_velocityConstraintDirection * m_body1->inv_m;
		m_body2->v += m_cachedMomentum * m_velocityConstraintDirection * m_body2->inv_m;
		m_body1->omega -= m_cachedMomentum * m_invMoment1;
		m_body2->omega += m_cachedMomentum * m_invMoment2;

		m_cachedTangentMomentum = Vector(0, 0, 0);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////

	void ContactConstraint::Iterate()
	{
		// Compute the relative velocity between the bodies
		Vector relativeVelocity = (m_body2->v + m_body2->omega % m_body2->q.Rotate(m_r2)) - (m_body1->v + m_body1->omega % m_body1->q.Rotate(m_r1));

		// Project the relative velocity onto the constraint direction
		float32 velocityError = relativeVelocity * m_velocityConstraintDirection;

		// Compute the velocity delta needed to satisfy the constraint
		float32 deltaVelocity = -velocityError - m_positionError;

		// Compute the momentum to be exchanged to correct velocities
		float32 momentumPacket = deltaVelocity * m_effectiveMass;

		// Clamp the momentum packet to reflect the fact that the contact can only push the objects apart
		momentumPacket = Min(momentumPacket, -m_cachedMomentum);

		Vector momentumPacketWithDir = momentumPacket * m_velocityConstraintDirection;

		// Exchange the correctional momentum between the bodies
		m_body1->v -= momentumPacketWithDir * m_body1->inv_m;
		m_body2->v += momentumPacketWithDir * m_body2->inv_m;
		m_body1->omega -= momentumPacket * m_invMoment1;
		m_body2->omega += momentumPacket * m_invMoment2;

		// Test code
		Vector newRelativeVelocity = (m_body2->v + m_body2->omega % m_body2->q.Rotate(m_r2)) - (m_body1->v + m_body1->omega % m_body1->q.Rotate(m_r1));
		float32 newVelocityError = newRelativeVelocity * m_velocityConstraintDirection;

		// Accumulate the momentum for next frame
		m_cachedMomentum += momentumPacket;

		///
		// FRICTION

		if (gFriction)
		{
			// Undo previous iteration
			/*
			m_body1->v += m_cachedTangentMomentum * m_body1->inv_m;
			m_body2->v -= m_cachedTangentMomentum * m_body2->inv_m;
			m_body1->omega += m_cachedTangentMomentum.Len3() * m_invMoment1;
			m_body2->omega -= m_cachedTangentMomentum.Len3() * m_invMoment2;
			*/

			relativeVelocity = (m_body2->v + m_body2->omega % m_body2->q.Rotate(m_r2)) - (m_body1->v + m_body1->omega % m_body1->q.Rotate(m_r1));

			Vector tangentVelocityDirection = relativeVelocity - relativeVelocity * m_velocityConstraintDirection * m_velocityConstraintDirection;;
			if (tangentVelocityDirection.IsZero3()) return;
			tangentVelocityDirection.Normalize3();

			float32 tangentVelocityError = relativeVelocity * tangentVelocityDirection;

			Vector r1 = m_body1->q.Rotate(m_r1);
			Vector r2 = m_body2->q.Rotate(m_r2);

			Vector invMoment1 = m_body1->inv_I * (r1 % tangentVelocityDirection);
			Vector invMoment2 = m_body2->inv_I * (r2 % tangentVelocityDirection);

			// Compute effective mass of the constraint system -- this is a measure of how easy it
			// is to accelerate the contact points apart along the constraint direction -- it's analogous
			// to effective resistance in an electric circuit [i.e., 1 / (1/R1 + 1/R2)]
			float32 effectiveMass =
				1.0f /
				(
					m_body1->inv_m +
					m_body2->inv_m +
					tangentVelocityDirection *
					(
						invMoment1 % r1 +
						invMoment2 % r2
						)
					);

			float32 tangentDeltaVelocity = -tangentVelocityError;

			float32 tangentMomentumPacket = tangentDeltaVelocity * effectiveMass;

			tangentMomentumPacket = Max(tangentMomentumPacket, m_cachedMomentum * 0.5f);

			Vector tangentMomentumPacketWithDir = tangentMomentumPacket * tangentVelocityDirection;

			// Exchange the correctional momentum between the bodies
			m_body1->v -= tangentMomentumPacketWithDir * m_body1->inv_m;
			m_body2->v += tangentMomentumPacketWithDir * m_body2->inv_m;
			m_body1->omega -= tangentMomentumPacket * invMoment1;
			m_body2->omega += tangentMomentumPacket * invMoment2;

			m_cachedTangentMomentum = tangentMomentumPacketWithDir;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////

	void ContactConstraint::Update(const Vector& p1, const Vector& p2, const Vector& normal)
	{
		m_r1 = p1;
		m_r2 = p2;
		m_velocityConstraintDirection = normal;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////

	void ContactConstraint::Draw()
	{
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////

	bool ContactConstraint::StillValid(float32 tolerance)
	{
		return false;
	}
}
