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
#include "ReplicationObject.h"
#include "ResShader.h"
#include "Scene.h"

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

class Entity : public NetSerializableObject, public ReplicationObject
{
	const static int kNetPosRange = 256;
	const static int kNetPosBits = 16;
	const static int kNetVelRange = 256;
	const static int kNetVelBits = 16;

	class Entity_NS : public NetSerializable
	{
	public:
		Entity_NS(Entity * owner)
			: NetSerializable(owner, (1 << REPLICATION_CHANNEL_RELIABLE) | (1 << REPLICATION_CHANNEL_UNRELIABLE), REPLICATION_CHANNEL_UNRELIABLE)
		{
			//SetChannel(REPLICATION_CHANNEL_RELIABLE);
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

				const int posMask[3] = { CAP_SYNC_POS_X, CAP_SYNC_POS_Y, CAP_SYNC_POS_Z };
				for (int i = 0; i < 3; ++i)
				{
					if (IsInit() || ((entity->m_caps & posMask[i]) != 0))
						SerializeFloatRange(entity->m_phyObject.m_position[i], -kNetPosRange, +kNetPosRange, kNetPosBits);
						//Serialize(entity->m_phyObject.m_position[i]);
				}

				const int rotMask[3] = { CAP_SYNC_ROT_X, CAP_SYNC_ROT_Y, CAP_SYNC_ROT_Z };
				for (int i = 0; i < 3; ++i)
				{
					if (IsInit() || ((entity->m_caps & rotMask[i]) != 0))
					{
						bool isZero = (entity->m_phyObject.m_velocity[i] == 0.0f);
						Serialize(isZero);
						if (!isZero)
							SerializeFloatRange(entity->m_phyObject.m_velocity[i], -kNetVelRange, +kNetVelRange, kNetVelBits);
							//Serialize(entity->m_phyObject.m_velocity[i]);
						else
							entity->m_phyObject.m_velocity[i] = 0.0f;
					}
				}

			#if 0
				printf("x/y/z: %g, %g, %g\n",
					entity->m_phyObject.m_position[0],
					entity->m_phyObject.m_position[1],
					entity->m_phyObject.m_position[2]);
			#endif
			}
		}
	};

	virtual bool RequiresUpdating() const { return (m_caps & CAP_NET_UPDATABLE) != 0; }
	virtual bool RequiresUpdate() const { return NetSerializableObject::IsDirty(); }

	virtual bool Serialize(BitStream & bitStream, bool init, bool send, int channel)
	{
		return NetSerializableObject::Serialize(init, send, channel, bitStream);
	}

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
		CAP_SYNC_ROT = CAP_SYNC_ROT_X | CAP_SYNC_ROT_Y | CAP_SYNC_ROT_Z,

		CAP_NET_UPDATABLE = 1 << 8
	};

	Entity();
	virtual ~Entity();

	ShEntity Self();

	void EnableCaps(int caps);
	void DisableCaps(int caps);

	virtual uint8_t GetClassID() const = 0;
	const char * ClassName() const { return m_className.c_str(); }
	Scene* GetScene() const;
	int GetID() const;
	virtual Mat4x4 GetTransform() const;
	Vec3 GetPosition() const;
	Vec3 GetOrientation() const;

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
	uint16_t m_id;
	bool m_active;
	uint32_t m_caps;

	std::string m_className;

	std::vector<AABBObject*> m_aabbObjects;

	uint16_t m_repObjectID;
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

#define IMPLEMENT_ENTITY(name)         \
	virtual uint8_t GetClassID() const \
	{                                  \
		return kClassID_ ## name;      \
	}

enum ClassID
{
	kClassID_Brick,
	kClassID_BrickSpawn,
	kClassID_EntityPlayer,
	kClassID_Floor,
	kClassID_Player,
	kClassID_Weapon,
	kClassID_WeaponDefault
};

#endif
