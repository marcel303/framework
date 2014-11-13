#ifndef SCENE_H
#define SCENE_H
#pragma once

#include <map>
#include <vector>
#include "EntityPtr.h"
#include "NetHandlePool.h"
#include "PacketListener.h"
#include "RenderList.h"

class Client;
class Engine;
class Entity;
class ReplicationManager;
class SceneRenderer;

namespace Phy
{
	class Scene;
}

class Scene : public PacketListener
{
public:
	Scene(Engine* engine, ReplicationManager* repMgr);
	~Scene();

	void AddEntity(ShEntity entity, int id = 0);
	void AddEntityQueued(ShEntity entity, int id = 0);
	void RemoveEntity(ShEntity entity);
	void RemoveEntity(int entityID);
	void RemoveEntityQueued(ShEntity entity);
	void RemoveEntityQueued(int entityID);
	void Clear();
	ShEntity FindEntity(int id);
	std::vector<ShEntity> FindEntitiesByClassName(const std::string& className);

	float GetTime();
	Engine* GetEngine();
	Phy::Scene* GetPhyScene();
	SceneRenderer* GetRenderer();

	void UpdateServer(float dt);
	void UpdateClient(float dt);

	void Render();

	void Activate(Client* client, int entityID);
	void SetController(Client* client, int controllerID);

	ShEntity CastRay(Entity* source, const Vec3& position, const Vec3& direction, float maxDistance, float* out_distance);

	void MessageEntity(Client* client, int entityID, int message, int value);

	virtual void OnReceive(Packet& packet, Channel* channel);

//private: // FIXME
	typedef std::map<ShEntity, int> EntityColl;
	typedef EntityColl::iterator EntityCollItr;
	typedef std::map<int, ShEntity> EntityCache;
	typedef EntityCache::iterator EntityCacheItr;

	void HandleActivate(Packet& packet, Channel* channel);
	void HandleSetController(Packet& packet, Channel* channel);
	void HandleEntityMessage(Packet& packet, Channel* channel);

	EntityColl m_entities;
	EntityCache m_entityCache;
	HandlePool<uint16_t> m_entityIDs;
	std::vector<ShEntity> m_destroyedEntities;
	std::vector<ShEntity> m_addedEntities;
	int m_activeEntityID;
	RefEntity m_activeEntity;
	int m_activeControllerID;
	float m_time;
	RenderList m_renderList;

	Engine* m_engine;
	ReplicationManager* m_repMgr;
	Phy::Scene* m_phyScene;

	SceneRenderer* m_renderer;

	friend class SceneRenderer;
};

#endif
