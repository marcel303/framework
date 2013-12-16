#pragma once

#include "types2.h"

enum EntityType
{
	EntityType_Bullet,
	EntityType_Enemy,
	EntityType_Powerup,
	EntityType_Player,
};

class Entity
{
public:
	virtual EntityType EntityType_get() = 0;
	virtual void Kill() = 0;
	virtual void Damage(float amount) = 0;
	virtual bool CollidesWith(const Vec2F& position) = 0;
};
