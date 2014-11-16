#ifndef ENGINE_H
#define ENGINE_H
#pragma once

#include <vector>
#include "ChannelHandler.h"
#include "ChannelManager.h"
#include "InputManager.h"
#include "PacketDispatcher.h"
#include "PolledTimer.h"
#include "ReplicationHandler.h"
#include "ReplicationManager.h"
#include "Scene.h"
#include "Timer.h"

class Game;

class Engine : public ChannelHandler, public ReplicationHandler
{
public:
	enum ROLE
	{
		ROLE_SERVER = 0x1,
		ROLE_CLIENT = 0x2,
		ROLE_SERVER_AND_CLIENT = ROLE_SERVER | ROLE_CLIENT
	};

	Engine(Game* game);
	virtual ~Engine();

	bool Initialize(ROLE role, bool localConnect);
	bool Shutdown();

	void ThreadSome();

	void SceneLoad();
	void SceneUnLoad();
	void GameStart();
	void GameEnd();

	void Update();
	void UpdateServer();
	void UpdateClient();

	void Render();

	void BindClientToEntity(Client* client, ShEntity entity);
	Entity* CreateEntity(Client* client, std::string className);

	virtual void SV_OnChannelConnect(Channel* channel);
	virtual void SV_OnChannelDisconnect(Channel* channel);
	virtual void CL_OnChannelConnect(Channel* channel) { }
	virtual void CL_OnChannelDisconnect(Channel* channel) { }
	virtual bool OnReplicationObjectSerializeType(ReplicationClient * client, ReplicationObject * object, BitStream & bitStream);
	virtual bool OnReplicationObjectCreateType(ReplicationClient* client, BitStream& bitStream, ReplicationObject** out_object);
	virtual void OnReplicationObjectCreated(ReplicationClient* client, ReplicationObject* object);
	virtual void OnReplicationObjectDestroyed(ReplicationClient* client, ReplicationObject* object);

//private: // FIXME
	class ScenePacketListener : public PacketListener
	{
		Engine * m_engine;

	public:
		ScenePacketListener(Engine * engine)
			: m_engine(engine)
		{
		}

		virtual void OnReceive(Packet & packet, Channel * channel)
		{
			for (auto i = m_engine->m_clientClients.begin(); i != m_engine->m_clientClients.end(); ++i)
			{
				Client * client = *i;

				if (client->m_channel == channel)
					client->m_clientScene->OnReceive(packet, channel);
			}
		}
	};

	Game* m_game;
	ROLE m_role;
	PolledTimer m_serverTimerUpdateLogic;
	PolledTimer m_clientTimerUpdateAnimation;
	PolledTimer m_serverTimerUpdateNetReplication;
	PolledTimer m_clientTimerFps;
	int m_clientFpsFrame;
	int m_clientFps;
	const static int m_serverNetFps = 60; // fixme
	//const static int m_serverNetFps = 10;
	//const static int m_serverNetFps = 5;
	PacketDispatcher m_packetDispatcher;
	ScenePacketListener m_scenePacketListener;
	std::vector<Client*> m_serverClients;
	ChannelManager* m_channelMgr;
	ReplicationManager* m_repMgr;
	InputManager* m_inputMgr;
	Scene* m_serverScene;
	std::vector<Client*> m_clientClients;
	// TODO: Keep sep list of client channels?
};

#endif
