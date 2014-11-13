#include "Calc.h"
#include "CDScene.h"
#include "Debug.h"
#include "Engine.h"
#include "Entity.h"
#include "EntityPtr.h"
#include "PhyScene.h"
#include "ReplicationManager.h"
#include "Scene.h"
#include "SceneRenderer.h"

Scene::Scene(Engine* engine, ReplicationManager* repMgr)
{
	Assert(engine);

	m_engine = engine;
	m_repMgr = repMgr;

	m_activeEntityID = 0; // TODO: Use entity reference/link class. TODO: How to replicate?
	m_activeControllerID = 0;
	m_phyScene = new Phy::Scene();
	m_time = 0;

	m_renderer = new SceneRenderer(this);
}

Scene::~Scene()
{
	Clear();

	m_activeEntity = ShEntity();

	delete m_renderer;

	delete m_phyScene;
}

void Scene::AddEntity(ShEntity entity, int id)
{
	Assert(entity.get());

	if (id == 0)
		entity->m_id = m_entityIDs.Allocate();
	else
		entity->m_id = id;

	m_entities[entity] = 1;
	m_entityCache[entity->m_id] = entity;

	entity->SetScene(this);

	if (m_repMgr) // FIXME: m_serverSide?
	{
		entity->m_repObjectID = m_repMgr->SV_AddObject(entity->m_className, entity.get());
	}

	if (!m_repMgr)
	{
		m_renderList.Add(entity.get());
	}
}

void Scene::AddEntityQueued(ShEntity entity, int id)
{
	Assert(entity.get());

	if (id == 0)
		entity->m_id = m_entityIDs.Allocate();
	else
		entity->m_id = id;

	m_addedEntities.push_back(entity);
}

void Scene::RemoveEntity(ShEntity entity)
{
	Assert(entity.get());

	if (!m_repMgr)
	{
		m_renderList.Remove(entity.get());
	}

	EntityCollItr i = m_entities.find(entity);

	if (i != m_entities.end())
	{
		if (m_repMgr) // FIXME: m_serverSide?
		{
			if (entity->m_repObjectID != 0)
			{
				m_repMgr->SV_RemoveObject(entity->m_repObjectID);
				entity->m_repObjectID = 0;
			}
		}

		entity->SetScene(0);

		m_entityCache.erase(entity->m_id);
		entity->m_id = 0;
		m_entities.erase(i);
	}
}

void Scene::RemoveEntity(int entityID)
{
	RemoveEntity(m_entityCache[entityID]);
}

void Scene::RemoveEntityQueued(ShEntity entity)
{
	Assert(entity.get());

	// todo : check if not queued already

	m_destroyedEntities.push_back(entity);
}

void Scene::RemoveEntityQueued(int entityID)
{
	RemoveEntityQueued(m_entityCache[entityID]);
}

void Scene::Clear()
{
	while (m_entities.size() > 0)
		RemoveEntity(m_entities.begin()->first);

	m_entities.clear();
}

ShEntity Scene::FindEntity(int id)
{
	ShEntity result;

	if (m_entityCache.find(id) != m_entityCache.end())
		result = m_entityCache[id];

	return result;
}

std::vector<ShEntity> Scene::FindEntitiesByClassName(const std::string& className)
{
	std::vector<ShEntity> r;

	for (EntityCollItr i = m_entities.begin(); i != m_entities.end(); ++i)
	{
		// FIXME: IsA.
		if (i->first.get()->m_className == className)
		{
			r.push_back(i->first);
		}
	}

	return r;
}

float Scene::GetTime()
{
	return m_time;
}

Engine* Scene::GetEngine()
{
	return m_engine;
}

Phy::Scene* Scene::GetPhyScene()
{
	return m_phyScene;
}

SceneRenderer* Scene::GetRenderer()
{
	return m_renderer;
}

void Scene::UpdateServer(float dt)
{
	m_time += dt;

	for (EntityCollItr i = m_entities.begin(); i != m_entities.end(); ++i)
	{
		i->first.get()->SetActive(true);

		i->first.get()->UpdateLogic(dt);
		i->first.get()->UpdateAnimation(dt);
	}

	m_phyScene->Gravity(Vec3(0.0f, -100.0f, 0.0f));
	m_phyScene->Update(dt);

	for (size_t i = 0; i < m_destroyedEntities.size(); ++i)
		RemoveEntity(m_destroyedEntities[i]);
	m_destroyedEntities.clear();

	for (size_t i = 0; i < m_addedEntities.size(); ++i)
		AddEntity(m_addedEntities[i]);
	m_addedEntities.clear();
}

void Scene::UpdateClient(float dt)
{
	m_time += dt;

	m_activeEntity = FindEntity(m_activeEntityID);

	if (!m_activeEntity.expired())
	{
		m_activeEntity.lock()->SetActive(true);
		m_activeEntity.lock()->SetController(m_activeControllerID);
	}

	for (EntityCollItr i = m_entities.begin(); i != m_entities.end(); ++i)
		i->first.get()->UpdateAnimation(dt);

	m_phyScene->Gravity(Vec3(0.0f, -100.0f, 0.0f));
	m_phyScene->Update(dt);
}

void Scene::Render()
{
	m_renderer->Render();
}

void Scene::Activate(Client* client, int entityID)
{
	Assert(client);

	PacketBuilder<4> packetBuilder;

	uint8_t protocolID = PROTOCOL_SCENE;
	uint8_t messageID = SCENEMSG_ACTIVATE;
	uint16_t id = (uint16_t)entityID;

	packetBuilder.Write8(&protocolID);
	packetBuilder.Write8(&messageID);
	packetBuilder.Write16(&id);

	Packet packet = packetBuilder.ToPacket();
	client->m_channel->Send(packet, 0);
}

void Scene::SetController(Client* client, int controllerID)
{
	Assert(client);

	PacketBuilder<4> packetBuilder;

	uint8_t protocolID = PROTOCOL_SCENE;
	uint8_t messageID = SCENEMSG_SETCONTROLLER;
	uint16_t id = (uint16_t)controllerID;

	packetBuilder.Write8(&protocolID);
	packetBuilder.Write8(&messageID);
	packetBuilder.Write16(&id);

	Packet packet = packetBuilder.ToPacket();
	client->m_channel->Send(packet, 0);
}

ShEntity Scene::CastRay(Entity* source, const Vec3& position, const Vec3& direction, float maxDistance, float* out_distance)
{
	CD::Object* object = m_phyScene->GetCDScene()->CastRay(source ? &source->m_cdFlt : 0, position, direction, maxDistance, out_distance);

	if (!object)
		return ShEntity((Entity*)0);

	Entity* entityPtr = (Entity*)object->GetUP();

	Assert(entityPtr);

	ShEntity entity = entityPtr->Self();

	Assert(entity.get());

	return entity;
}

void Scene::MessageEntity(Client* client, int in_entityID, int in_message, int in_value)
{
	Assert(client);

	PacketBuilder<10> packetBuilder;

	uint8_t protocolID = PROTOCOL_SCENE;
	uint8_t messageID = SCENEMSG_ENTITY_MESSAGE;
	uint16_t entityID = in_entityID;
	uint16_t message = in_message;
	uint32_t value = in_value;

	packetBuilder.Write8(&protocolID);
	packetBuilder.Write8(&messageID);
	packetBuilder.Write16(&entityID);
	packetBuilder.Write16(&message);
	packetBuilder.Write32(&value);

	Packet packet = packetBuilder.ToPacket();
	client->m_channel->Send(packet, 0);
}

void Scene::OnReceive(Packet& packet, Channel* channel)
{
	uint8_t messageID;

	if (!packet.Read8(&messageID))
		return;

	switch (messageID)
	{
	case SCENEMSG_ACTIVATE:
		HandleActivate(packet, channel);
		break;
	case SCENEMSG_SETCONTROLLER:
		HandleSetController(packet, channel);
		break;
	case SCENEMSG_ENTITY_MESSAGE:
		HandleEntityMessage(packet, channel);
		break;
	default:
		DB_ERR("received unknown message");
		break;
	}
}

void Scene::HandleActivate(Packet& packet, Channel* channel)
{
	uint16_t entityID;

	if (!packet.Read16(&entityID))
		return;

	LOG_DBG("Scene::HandleActivate: entityID=%d", entityID);

	m_activeEntityID = entityID;
}

void Scene::HandleSetController(Packet& packet, Channel* channel)
{
	uint16_t controllerID;

	if (!packet.Read16(&controllerID))
		return;

	LOG_DBG("Scene::HandleSetController: controllerId=%d", controllerID);

	m_activeControllerID = controllerID;
}

void Scene::HandleEntityMessage(Packet& packet, Channel* channel)
{
	uint16_t entityID;
	uint16_t message;
	uint32_t value;

	if (!packet.Read16(&entityID))
		return;
	if (!packet.Read16(&message))
		return;
	if (!packet.Read32(&value))
		return;

	ShEntity entity = FindEntity(entityID);

	AssertMsg(entity.get(), "received message for non-existing entity. entityId=%d", (int)entityID);
	if (entity.get())
	{
		LOG_DBG("Scene::HandleEntityMessage: entityId=%d", entityID);

		entity->OnMessage(message, value);
	}
}
