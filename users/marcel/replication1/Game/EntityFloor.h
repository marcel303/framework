#ifndef ENTITYFLOOR_H
#define ENTITYFLOOR_H
#pragma once

#include "CDCube.h"
#include "Entity.h"
#include "Mesh.h"
#include "ResPS.h"
#include "ResTex.h"
#include "ResVS.h"
#include "Vec3.h"

class EntityFloor : public Entity
{
public:
	EntityFloor(Vec3 position, Vec3 size, int orientation);
	virtual ~EntityFloor();
	virtual void PostCreate();

	void Initialize(Vec3 position, Vec3 size, int orientation);

	virtual Mat4x4 GetTransform() const;

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void UpdateRender();
	virtual void Render();

	virtual void OnSceneAdd(Scene* scene);
	virtual void OnSceneRemove(Scene* scene);

//private: //FIXME
	Vec3 m_position;
	Vec3 m_size;
	int8_t m_orientation;
	CD::ShObject m_cdCube; // Derived.
	//Phy::Object m_phyObject; // Derived.

	Mesh m_mesh;
	ShTex m_tex;
};

#endif
