#pragma once

#include <vector>
#include "HashSpace.h"

struct IntersectionInfo
{
	float m_normal[3];
	float m_distance;
};

class CollisionSphere
{
public:
	inline CollisionSphere();
	inline CollisionSphere(float x, float y, float z, float radius);
	inline ~CollisionSphere();
	inline void Zero();

	inline void SetMass(float mass);
	inline void Set(float x, float y, float z, float radius);

	inline void AddForce(float x, float y, float z);

	inline bool Intersect(const CollisionSphere & other, IntersectionInfo & out_intersection);

	// Primary.
	float m_position[3];
	float m_radius;
	float m_mass;
	float m_invMass;

	// Secondary.
	float m_speed[3];

	// Modifiers.
	float m_force[3];

	// Speedups.
	HashList m_hashes;
	bool m_inactive;
};

#include "CollisionSphere.inl"
