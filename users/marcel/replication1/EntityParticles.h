#ifndef ENTITYPARTICLES_H
#define ENTITYPARTICLES_H
#pragma once

#include "Entity.h"
#include "ParticleSys.h"

class EntityParticles : public Entity
{
public:
	EntityParticles(Vec3 position, int particleCount, float size);
	virtual ~EntityParticles();

	virtual void Render();
	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void UpdateRender();

	virtual void OnSceneAdd(Scene* scene);

	Vec3 m_position;
	int32_t m_particleCount;
	float m_size;

	ParticleSys m_psys;
};

#endif
