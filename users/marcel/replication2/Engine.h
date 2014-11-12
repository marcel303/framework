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

class Engine : public ChannelHandler, public Replication::Handler
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
	virtual bool OnReplicationObjectCreate1(Replication::Client* client, const std::string& className, Replication::IObject** out_object);
	virtual void OnReplicationObjectCreate2(Replication::Client* client, Replication::IObject* object);
	virtual void OnReplicationObjectDestroy(Replication::Client* client, Replication::IObject* object);

//private: // FIXME
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
	std::vector<Client*> m_serverClients;
	ChannelManager* m_channelMgr;
	Replication::Manager* m_repMgr;
	InputManager* m_inputMgr;
	Scene* m_serverScene;
	Client* m_clientClient;
	// TODO: Keep sep list of client channels?
};

#endif
