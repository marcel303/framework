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
	IMPLEMENT_ENTITY(Floor);

	class Floor_NS : public NetSerializable
	{
		EntityFloor * m_owner;

	public:
		Floor_NS(EntityFloor * owner)
			: NetSerializable(owner)
			, m_owner(owner)
		{
		}

		virtual void SerializeStruct()
		{
			if (IsInit())
			{
				Serialize(m_owner->m_size[0]);
				Serialize(m_owner->m_size[1]);
				Serialize(m_owner->m_size[2]);
				Serialize(m_owner->m_orientation);
			}
		}
	};

public:
	EntityFloor();
	virtual ~EntityFloor();

	void Initialize(Vec3 position, Vec3 size, int orientation);

	virtual Mat4x4 GetTransform() const;

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void UpdateRender();
	virtual void Render();

	virtual void OnSceneAdd(Scene* scene);
	virtual void OnSceneRemove(Scene* scene);

private:
	Floor_NS * m_floor_NS;
	Vec3 m_size;
	int8_t m_orientation;
	CD::ShObject m_cdCube; // Derived.

	Mesh m_mesh;
	ShTex m_tex;
};

#endif
