#ifndef ENTITYBRICK_H
#define ENTITYBRICK_H
#pragma once

#include "CDCube.h"
#include "Entity.h"
#include "Mesh.h"
#include "ResPS.h"
#include "ResVS.h"
#include "Vec3.h"

class EntityBrick : public Entity
{
public:
	EntityBrick(Vec3 position, Vec3 size, int indestructible);
	virtual ~EntityBrick();
	virtual void PostCreate();

	void Initialize();

	virtual Mat4x4 GetTransform() const;

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void UpdateRender();
	virtual void Render();

	virtual void OnSceneAdd(Scene* scene);
	virtual void OnSceneRemove(Scene* scene);

	virtual void ScriptDamageAntiBrick(int amount);

	bool IsIndestructible();
	bool IsFortified();
	void Damage(int amount);
	void Fort(int amount);

//private: //FIXME
	const static int MAX_HP = 100;
	const static int MAX_LIFE = 5;
	Vec3 m_position;
	Vec3 m_size;
	CD::ShObject m_cdCube; // Derived.

	int8_t m_fort;
	int8_t m_hp;
	float m_life;
	int8_t m_indestructible;

	uint8_t m_repHp;
	uint8_t m_repLife;

	Mesh m_mesh;
	ShTex m_tex;
	ShTex m_texFort;
	ShTex m_nmap;
	ShTex m_hmap;
};

#endif
