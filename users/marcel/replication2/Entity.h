#ifndef ENTITY_H
#define ENTITY_H
#pragma once

#include <string>
#include <vector>
#include "AABBObject.h"
#include "CDGroupFLT.h"
#include "CDObject.h"
#include "Mat4x4.h"
#include "NetSerializable.h"
#include "PhyObject.h"
#include "ResShader.h"
#include "Scene.h"

#undef GetClassName

template <typename T>
class NetValue
{
	T m_value;
	NetSerializable * m_owner;

public:
	NetValue(NetSerializable * owner, const T & value = T())
		: m_owner(owner)
		, m_value(value)
	{
	}

	void operator=(const T & value)
	{
		if (value != m_value)
		{
			m_value = value;
			m_owner->SetDirty();
		}
	}

	const T & get() const
	{
		return m_value;
	}

	void Serialize(NetSerializationContext * context)
	{
		context->Serialize(m_value);
	}
};

class Entity : public NetSerializableObject
{
	class Entity_NS : public NetSerializable
	{
	public:
		Entity_NS(Entity * owner)
			: NetSerializable(owner)
		{
		}

		virtual void SerializeStruct()
		{
			Entity * entity = static_cast<Entity*>(GetOwner());

			if (IsInit())
			{
				Serialize(entity->m_id);
			}

			// todo : add special net sync flags for pos/vel + rot/vel

			if (IsInit() || (entity->m_caps & CAP_DYNAMIC_PHYSICS))
			{
				// todo : compression, and only send if changed

				Serialize(entity->m_phyObject.m_position[0]);
				Serialize(entity->m_phyObject.m_position[1]);
				Serialize(entity->m_phyObject.m_position[2]);
				Serialize(entity->m_phyObject.m_velocity[0]);
				Serialize(entity->m_phyObject.m_velocity[1]);
				Serialize(entity->m_phyObject.m_velocity[2]);

				/*printf("x/y/z: %g, %g, %g\n",
					entity->m_phyObject.m_position[0],
					entity->m_phyObject.m_position[1],
					entity->m_phyObject.m_position[2]);*/
			}
		}
	};

public:
	enum CAPS
	{
		CAP_STATIC_PHYSICS  = 1 << 0,
		CAP_DYNAMIC_PHYSICS = 1 << 1,
		CAP_PHYSICS         = CAP_STATIC_PHYSICS | CAP_DYNAMIC_PHYSICS
	};

	Entity();
	virtual ~Entity();
	virtual void PostCreate();

	ShEntity Self();

	void EnableCaps(int caps);
	void DisableCaps(int caps);

	Scene* GetScene() const;
	int GetID() const;
	virtual Mat4x4 GetTransform() const;
	Vec3 GetPosition() const;
	Vec3 GetOrientation() const;
	const std::string& GetClassName() const;

	void SetClassName(const std::string& className);
	void SetScene(Scene* scene);
	void SetActive(bool active);
	virtual void SetController(int id);
	void SetTransform(const Mat4x4& transform);

	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void UpdateRender();
	virtual void Render();

	void ReplicatedMessage(int message, int value);

	virtual void OnSceneAdd(Scene* scene);
	virtual void OnSceneRemove(Scene* scene);

	virtual void OnActivate();
	virtual void OnDeActivate();

	virtual void OnMessage(int message, int value);

	virtual void OnTransformChange();

protected:
	void UpdateCaps(int oldCaps, int newCaps);
	void AddAABBObject(AABBObject* object);

public:
//private: FIXME
	Scene* m_scene;
	int m_id;
	bool m_active;
	int m_caps;

	std::string m_className;

	std::vector<AABBObject*> m_aabbObjects;

	int m_repObjectID;
	Entity_NS * m_entity_NS;

	// CD & Physics.
	typedef std::vector<Phy::Object*> PhyObjectColl;
	typedef PhyObjectColl::iterator PhyObjectCollItr;

	CD::Group m_cdGroup;
	CD::GroupFltEX m_cdFlt;
	Phy::Object m_phyObject;

	Mat4x4 m_transform;
	ShShader m_shader;

	AABB m_aabb; // Derived from AABB objects.
	bool m_hasAABB;
};

#include "EntityLink.h"
#include "EntityPtr.h"

#endif
