#ifndef ENTITYLIGHT_H
#define ENTITYLIGHT_H
#pragma once

#include "Entity.h"

enum LIGHT_TYPE
{
	LT_SPOT        = 0,
	LT_DIRECTIONAL = 1,
	LT_OMNI        = 2
};

class EntityLight : public Entity
{
public:
	EntityLight(LIGHT_TYPE type, Vec3 position, Vec3 direction);

	virtual void UpdateAnimation(float dt);

	int32_t m_type;
	Vec3 m_position;
	Vec3 m_direction;
	float m_time; // fixme, remove.
};

#endif
