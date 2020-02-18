#include "CollisionScene.h"
#include "CollisionSphere.h"

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

	//printf("HashCount: %d.\n", sphere->m_hashes.size());

	m_hashSpace.Add(
		sphere->m_hashes,
		sphere);
}
