#include "CollisionScene.h"
#include "CollisionSphere.h"
#include <string.h>

#define HASH_SPACE_SIZE (100 * 100 * 100)
//#define HASH_SPACE_SIZE (128 * 128 * 128)

void CollisionScene::Initialize()
{
	m_hashSpace.Initialize(10.0f, HASH_SPACE_SIZE);
}

void CollisionScene::Add(CollisionSphere * sphere)
{
	m_spheres.push_back(sphere);

	MoveEnd(sphere);
}

void CollisionScene::MoveBegin(CollisionSphere * sphere)
{
	m_hashSpace.Remove(
		sphere->m_hashes,
		sphere);
}

void CollisionScene::MoveEnd(CollisionSphere * sphere)
{
	m_hashSpace.UpdateHashVolume(
		sphere->m_position[0] - sphere->m_radius,
		sphere->m_position[1] - sphere->m_radius,
		sphere->m_position[2] - sphere->m_radius,
		sphere->m_position[0] + sphere->m_radius,
		sphere->m_position[1] + sphere->m_radius,
		sphere->m_position[2] + sphere->m_radius,
		sphere->m_hashes);

	//LOG_DBG("hashCount: %d", sphere->m_hashes.size());

	m_hashSpace.Add(
		sphere->m_hashes,
		sphere);
}

void CollisionScene::MoveUpdate(CollisionSphere * sphere)
{
	HashValue * oldHashes = (HashValue*)alloca(sphere->m_hashes.size() * sizeof(HashValue));
	memcpy(oldHashes, sphere->m_hashes.data(), sphere->m_hashes.size() * sizeof(HashValue));
	const size_t numOldHashes = sphere->m_hashes.size();
	
	if (m_hashSpace.UpdateHashVolume(
		sphere->m_position[0] - sphere->m_radius,
		sphere->m_position[1] - sphere->m_radius,
		sphere->m_position[2] - sphere->m_radius,
		sphere->m_position[0] + sphere->m_radius,
		sphere->m_position[1] + sphere->m_radius,
		sphere->m_position[2] + sphere->m_radius,
		sphere->m_hashes))
	{
		m_hashSpace.Remove(oldHashes, numOldHashes, sphere);
		
		m_hashSpace.Add(
			sphere->m_hashes,
			sphere);
	}
}
