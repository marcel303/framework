#pragma once

#include "HashSpace.h"
#include <vector>

class CollisionSphere;

class CollisionScene
{
public:
	void Initialize();

	void Add(CollisionSphere * sphere);

	void MoveBegin(CollisionSphere * sphere);
	void MoveEnd(CollisionSphere * sphere);

	std::vector<CollisionSphere*> m_spheres;

	HashSpace<CollisionSphere*> m_hashSpace;
};
