#ifndef ENTITY_H
#define ENTITY_H
#pragma once

#include <string>
#include <vector>
#include "AABBObject.h"
#include "CDGroupFLT.h"
#include "CDObject.h"
#include "Debugging.h"
#include "Mat4x4.h"
#include "NetSerializable.h"
#include "PhyObject.h"
#include "ResShader.h"
#include "Scene.h"

#undef GetClassName

template <typename T> // todo : remove this class
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


			if (IsInit() || (entity->m_caps & (CAP_SYNC_POS | CAP_SYNC_ROT)) != 0)
			{
				// todo : compression, and only send if changed

				if (IsInit() || ((entity->m_caps & CAP_SYNC_POS_X) != 0))
					Serialize(entity->m_phyObject.m_position[0]);
				if (IsInit() || ((entity->m_caps & CAP_SYNC_POS_Y) != 0))
					Serialize(entity->m_phyObject.m_position[1]);
				if (IsInit() || ((entity->m_caps & CAP_SYNC_POS_Z) != 0))
					Serialize(entity->m_phyObject.m_position[2]);

				if (IsInit() || ((entity->m_caps & CAP_SYNC_ROT_X) != 0))
					Serialize(entity->m_phyObject.m_velocity[0]);
				if (IsInit() || ((entity->m_caps & CAP_SYNC_ROT_Y) != 0))
					Serialize(entity->m_phyObject.m_velocity[1]);
				if (IsInit() || ((entity->m_caps & CAP_SYNC_ROT_Z) != 0))
					Serialize(entity->m_phyObject.m_velocity[2]);

			#if 0
				printf("x/y/z: %g, %g, %g\n",
					entity->m_phyObject.m_position[0],
					entity->m_phyObject.m_position[1],
					entity->m_phyObject.m_position[2]);
			#endif
			}
		}
	};

public:
	enum CAPS
	{
		CAP_STATIC_PHYSICS  = 1 << 0, // entity is added to the collision scene
		CAP_DYNAMIC_PHYSICS = 1 << 1, // entity is added to the collision scene, and responds dynamically to forces applied to it
		CAP_PHYSICS         = CAP_STATIC_PHYSICS | CAP_DYNAMIC_PHYSICS,

		// position components that should be synced across the network
		CAP_SYNC_POS_X = 1 << 2,
		CAP_SYNC_POS_Y = 1 << 3,
		CAP_SYNC_POS_Z = 1 << 4,
		CAP_SYNC_POS = CAP_SYNC_POS_X | CAP_SYNC_POS_Y | CAP_SYNC_POS_Z,

		// rotation components that should be synced across the network
		CAP_SYNC_ROT_X = 1 << 5,
		CAP_SYNC_ROT_Y = 1 << 6,
		CAP_SYNC_ROT_Z = 1 << 7,
		CAP_SYNC_ROT = CAP_SYNC_ROT_X | CAP_SYNC_ROT_Y | CAP_SYNC_ROT_Z
	};

	Entity();
	virtual ~Entity();

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
//private: // FIXME
	Client* m_client;
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

typedef Entity * (*CreateEntityCB)();

class EntityFactory
{
	struct EntityRegistration
	{
		CreateEntityCB create;
	};

	std::map<std::string, EntityRegistration> m_registrations;

public:
	void RegisterEntity(const char * className, CreateEntityCB create)
	{
		Assert(create);

		EntityRegistration registration;

		registration.create = create;

		m_registrations[className] = registration;
	}

	Entity * CreateEntity(const char * className)
	{
		std::map<std::string, EntityRegistration>::iterator i = m_registrations.find(className);

		Assert(i != m_registrations.end());
		if (i != m_registrations.end())
		{
			EntityRegistration & registration = i->second;

			return registration.create();
		}
		else
		{
			return 0;
		}
	}
};

extern EntityFactory g_entityFactory;

#define DEFINE_ENTITY(type, name)                           \
class Register ## name                                      \
	{                                                       \
	public:                                                 \
	Register ## name()                                      \
		{                                                   \
			g_entityFactory.RegisterEntity(# name, Create); \
		}                                                   \
	                                                        \
		static Entity * Create()                            \
		{                                                   \
			return new type();                              \
		}                                                   \
	};                                                      \
	volatile Register ## name g_register_ ## name

#endif
