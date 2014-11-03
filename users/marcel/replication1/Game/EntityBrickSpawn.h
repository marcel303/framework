#ifndef ENTITYBRICKSPAWN_H
#define ENTITYBRICKSPAWN_H
#pragma once

// TODO: Implement, move from scene.

#include "Entity.h"
#include "PolledTimer.h"

class EntityBrickSpawn : public Entity
{
public:
	EntityBrickSpawn();
	virtual ~EntityBrickSpawn();

	virtual void UpdateLogic(float dt);

private:
	PolledTimer m_timer;
};

#endif
