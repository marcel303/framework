#ifndef ENTITYMISSILE_H
#define ENTITYMISSILE_H
#pragma once

#include "Entity.h"
#include "Player.h"
#include "Renderer.h"
#include "ShapeBuilder.h"

class EntityMissile : public Entity
{
public:
	EntityMissile(Vec3 position, Vec3 speed, Entity* target);
	virtual ~EntityMissile();
	virtual void PostCreate();

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void Render();

private:
	//Phy::Object m_phyObject;
	Vec3 m_position;
	Vec3 m_speed;
	Entity* m_target;
	int32_t m_targetID;
	float m_life;
	Mesh m_mesh;
};

#endif
