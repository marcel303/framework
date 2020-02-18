#pragma once

#include <math.h>

inline CollisionSphere::CollisionSphere()
{
	Zero();
}

inline CollisionSphere::CollisionSphere(float x, float y, float z, float radius)
{
	Zero();

	Set(x, y, z, radius);
}

inline CollisionSphere::~CollisionSphere()
{
}

inline void CollisionSphere::Zero()
{
	SetMass(1.0f);
	Set(0.0f, 0.0f, 0.0f, 1.0f);

	m_speed[0] = 0.0f;
	m_speed[1] = 0.0f;
	m_speed[2] = 0.0f;

	m_force[0] = 0.0f;
	m_force[1] = 0.0f;
	m_force[2] = 0.0f;

	m_inactive = false;
}

inline void CollisionSphere::SetMass(float mass)
{
	m_mass = mass;
	m_invMass = 1.0f / m_mass;
}

inline void CollisionSphere::Set(float x, float y, float z, float radius)
{
	m_position[0] = x;
	m_position[1] = y;
	m_position[2] = z;

	m_radius = radius;
}

inline void CollisionSphere::AddForce(float x, float y, float z)
{
	m_force[0] += x;
	m_force[1] += y;
	m_force[2] += z;
}

inline bool CollisionSphere::Intersect(const CollisionSphere & other, IntersectionInfo & out_intersection)
{
	const float delta[3] =
	{
		other.m_position[0] - m_position[0],
		other.m_position[1] - m_position[1],
		other.m_position[2] - m_position[2]
	};

	const float distanceSquared = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];

	const float maxDistance = m_radius + other.m_radius;
	const float maxDistanceSquared = maxDistance * maxDistance;

	if (distanceSquared > maxDistanceSquared)
		return false;

	// TODO: Compute normal, depth.
	out_intersection.m_normal[0] = delta[0];
	out_intersection.m_normal[1] = delta[1];
	out_intersection.m_normal[2] = delta[2];

	const float normalLengthSquared = distanceSquared;
	const float normalLength = sqrtf(normalLengthSquared);
	const float invNormalLength = 1.0f / normalLength;

	out_intersection.m_normal[0] *= invNormalLength;
	out_intersection.m_normal[1] *= invNormalLength;
	out_intersection.m_normal[2] *= invNormalLength;

	out_intersection.m_distance = normalLength - maxDistance;

	return true;
}
