#include "Calc.h"
#include "Debug.h"
#include "Engine.h"
#include "EventManager.h"
#include "Game.h"
#include "PacketDispatcher.h"
#include "Player.h"
#include "Renderer.h"
#include "ResMgr.h"
#include "Stats.h"

// fixme : Create pluggable entity factory, let game class register with it.
#include "EntityBrick.h"
#include "EntityBrickSpawn.h"
#include "EntityFloor.h"
#include "WeaponDefault.h"

class InputListenerLL : public InputHandler
{
public:
	InputListenerLL() : InputHandler(INPUT_PRIO_LOWLEVEL)
	{
	}

	virtual bool OnEvent(Event& event)
	{
		if (event.type == EVT_KEY && (event.key.key == IK_q || event.key.key == IK_ESCAPE) && event.key.state == BUTTON_DOWN)
		{
			EventManager::I().AddEvent(Event(EVT_QUIT));

			return true;
		}

		return false;
	}
};

static InputListenerLL s_inputLL;

Engine::Engine(Game* game)
{
	m_game = game;

	m_serverTimerUpdateLogic.Initialize(&g_TimerRT);
	m_clientTimerUpdateAnimation.Initialize(&g_TimerRT);
	m_serverTimerUpdateNetReplication.Initialize(&g_TimerRT);
	m_clientTimerFps.Initialize(&g_TimerRT);
}

Engine::~Engine()
{
}

bool Engine::Initialize(ROLE role, bool localConnect)
{
	DB_TRACE("");

	Calc::Initialize();

	if (m_game)
		m_game->HandleInitialize(this);

	m_role = role;

	m_channelMgr = new ChannelManager();
	m_repMgr = new Replication::Manager();
	m_inputMgr = new InputManager();
	m_serverScene = new Scene(this, m_repMgr);
	m_clientClient = 0;

	DB_TRACE("registering protocols");

	m_packetDispatcher.RegisterProtocol(PROTOCOL_CHANNEL, m_channelMgr);
	m_packetDispatcher.RegisterProtocol(PROTOCOL_REPLICATION, m_repMgr);
	m_packetDispatcher.RegisterProtocol(PROTOCOL_INPUT, m_inputMgr);

	DB_TRACE("initializing channel manager");

	int port = role & ROLE_SERVER ? 6000 : 0;

	m_channelMgr->Initialize(&m_packetDispatcher, this, port, (role & ROLE_SERVER) != 0);

	DB_TRACE("initializing event manager");

	EventManager::I().AddEventHandler(m_inputMgr);

	if (role & ROLE_CLIENT)
	{
		DB_TRACE("creating client");

		m_repMgr->CL_RegisterHandler(this);

		m_clientClient = new Client(this);

		DB_TRACE("registering scene protocol");

		m_packetDispatcher.RegisterProtocol(PROTOCOL_SCENE, m_clientClient->m_clientScene);

		DB_TRACE("creating client channel");

		m_clientClient->m_channel = m_channelMgr->CreateChannel(ChannelPool_Client);
		if (localConnect)
			m_clientClient->m_channel->Connect(NetAddress(127, 0, 0, 1, 6000));
		else
			m_clientClient->m_channel->Connect(NetAddress(127, 0, 0, 1, 6000));

		m_clientClient->m_repID = m_repMgr->CL_CreateClient(m_clientClient, 0);

		m_inputMgr->CL_AddClient(m_clientClient);
	}

	if (role & ROLE_CLIENT)
	{
		m_inputMgr->CL_AddInputHandler(&s_inputLL);
	}

	if (m_game)
	{
		DB_TRACE("loading config (game)");

		const GameConfig& cfg = m_game->GetConfig();

		// TODO: Just logic Hz?
		m_serverTimerUpdateLogic.SetFrequency((float)cfg.m_logicHz);
		m_clientTimerUpdateAnimation.SetFrequency((float)cfg.m_logicHz);
	}
	else
	{
		DB_TRACE("loading config (default)");

		// TODO: Remove non-game values.
		m_serverTimerUpdateLogic.SetFrequency(100.0);
		m_clientTimerUpdateAnimation.SetFrequency(100.0);
	}

	m_serverTimerUpdateLogic.Restart();
	m_clientTimerUpdateAnimation.Restart();

	m_serverTimerUpdateNetReplication.SetFrequency((float)m_serverNetFps);
	m_serverTimerUpdateNetReplication.Restart();

	m_clientTimerFps.SetFrequency(1.0);
	m_clientTimerFps.Restart();

	m_clientFpsFrame = 0;
	m_clientFps = 0;

	// TODO: Move elsewhere?
	SceneLoad();

	return true;
}

bool Engine::Shutdown()
{
	DB_TRACE("shutting down game");

	if (m_game)
		m_game->HandleShutdown();

	DB_TRACE("shutting down replication manager");

	m_repMgr->CL_Shutdown();

	//m_serverScene->Clear();

	if (m_role & ROLE_SERVER)
	{
		DB_TRACE("removing server side clients");

		while (!m_serverClients.empty())
		{
			Channel* channel = m_serverClients.front()->m_channel;
			m_channelMgr->DestroyChannel(channel);
		}
		m_serverClients.clear();
	}

	if (m_role & ROLE_CLIENT)
	{
		DB_TRACE("removing client side client");

		m_channelMgr->DestroyChannel(m_clientClient->m_channel);
		SAFE_FREE(m_clientClient);
	}

	DB_TRACE("shutting down event manager");

	EventManager::I().RemoveEventHandler(m_inputMgr);

	DB_TRACE("shutting down channel manager");

	m_channelMgr->Shutdown(true);

	SAFE_FREE(m_serverScene);
	SAFE_FREE(m_inputMgr);
	SAFE_FREE(m_repMgr);
	SAFE_FREE(m_channelMgr);

	return true;
}

void Engine::ThreadSome()
{
	for (int i = 0; i < 1; ++i)
		Sleep(0);
}

void Engine::SceneLoad()
{
	DB_TRACE("loading scene");

	if (m_game)
		m_game->HandleSceneLoadBegin();

	// TODO: Load scene.

	if (m_game)
		m_game->HandleSceneLoadEnd();
}

void Engine::SceneUnLoad()
{
	DB_TRACE("unloading scene");

	// TODO: Unload scene.

	if (m_game)
		m_game->HandleSceneUnLoadEnd();
}

void Engine::GameStart()
{
	DB_TRACE("starting game");

	if (m_game)
		m_game->HandleGameStart();
}

void Engine::GameEnd()
{
	DB_TRACE("ending game");

	if (m_game)
		m_game->HandleGameEnd();
}

void Engine::Update()
{
	// server side

	ResMgr::I().Update();
	EventManager::I().Purge();
	ThreadSome();

	m_channelMgr->Update(g_TimerRT.TimeMS_get());
	ThreadSome();

	if (m_role & ROLE_SERVER)
		UpdateServer();
	ThreadSome();

	// client side

	ResMgr::I().Update();
	EventManager::I().Purge();
	ThreadSome();

	m_channelMgr->Update(g_TimerRT.TimeMS_get());
	ThreadSome();

	if (m_role & ROLE_CLIENT)
		UpdateClient();
	ThreadSome();

	//

	Stats::I().NextNet();
	Stats::I().NextRep(); // FIXME: @ rep.
}

void Engine::UpdateServer()
{
	m_inputMgr->SV_Update();

	while (m_serverTimerUpdateLogic.TickCount_get() > 5)
		m_serverTimerUpdateLogic.ReadTick();
	while (m_serverTimerUpdateLogic.ReadTick())
	{
		const float dt = m_serverTimerUpdateLogic.Interval_get();

		m_serverScene->UpdateServer(dt);

		if (m_game)
			m_game->HandleUpdateServer(dt);
	}

	if (m_serverTimerUpdateNetReplication.ReadTick())
	{
		m_serverTimerUpdateNetReplication.ClearTick();

		m_repMgr->SV_Update();
	}
}

void Engine::UpdateClient()
{
	while (m_clientTimerUpdateAnimation.TickCount_get() > 5)
		m_clientTimerUpdateAnimation.ReadTick();
	while (m_clientTimerUpdateAnimation.ReadTick())
	{
		Stats::I().NextScene();

		if (m_clientClient)
		{
			const float dt = m_clientTimerUpdateAnimation.Interval_get();

			m_clientClient->m_clientScene->UpdateClient(dt);

			if (m_game)
				m_game->HandleUpdateClient(dt);
		}
	}
}

void Engine::Render()
{
	// Scene.
	if (m_role & ROLE_CLIENT)
	{
		m_clientClient->m_clientScene->Render();

		Renderer::I().GetGraphicsDevice()->SetVS(0);
		Renderer::I().GetGraphicsDevice()->SetPS(0);

		if (m_game)
			m_game->HandleRender();
	}
#if 0
	else
		m_serverScene->Render();
#endif

	++m_clientFpsFrame;

	if (m_clientTimerFps.ReadTick())
	{
		m_clientTimerFps.ClearTick();

		m_clientFps = m_clientFpsFrame;
		m_clientFpsFrame = 0;
		Stats::I().m_gfx.NextFps(m_clientFps);

		#if defined(DEBUG) && 0
		printf("Gfx: Frames per second: %d.\n", m_clientFps);

		if (m_clientClient)
			printf("Scene: Object count: %d.\n", m_clientClient->m_clientScene->m_entities.size());

		printf("Net: Channel count: %d.\n", m_channelMgr->m_channels.size());

		for (ChannelManager::ChannelMapItr i = m_channelMgr->m_channels.begin(); i != m_channelMgr->m_channels.end(); ++i)
			printf("Net: Channel %d -> %d: RTT: %d. RPQ: %d.\n", i->second->m_id, i->second->m_destinationId, i->second->m_rtt, i->second->m_rtQueue.size());

		printf("Net: [AVERAGE/second] Packets sent: %.3f.\n",     Stats::I().m_net.m_packetsSent.CalcAverageFS());
		printf("Net: [AVERAGE/second] Packets received: %.3f.\n", Stats::I().m_net.m_packetsReceived.CalcAverageFS());
		printf("Net: [AVERAGE/second] Bytes sent: %.3f.\n",       Stats::I().m_net.m_bytesSent.CalcAverageFS());
		printf("Net: [AVERAGE/second] Bytes received: %.3f.\n",   Stats::I().m_net.m_bytesReceived.CalcAverageFS());
		#endif
	}
}

void Engine::BindClientToEntity(Client* client, ShEntity entity)
{
	// TODO: Revise~

	client->m_entities.push_back(entity);

	m_inputMgr->SV_AddClient(client);

	m_serverScene->AddEntity(entity);

	LOG_DBG("Engine::BindClientToEntity. clientSideChannelId=%d, entityId=%d", client->m_channel->m_destinationId, entity->GetID());

	m_serverScene->Activate(client, entity->m_id);
}

Entity* Engine::CreateEntity(Client* client, std::string className)
{
	//LOG_DBG("creating entity");

	Entity* entity = 0;

	// Generic.
	if (className == "Entity")
		entity = new Entity();
	else if (className == "Weapon")
		entity = new Weapon(!client->m_clientSide ? (Player*)client->m_entities[0].lock().get() : 0); // FIXME: Ugly as fuck.
	// BrickBuster.
	else if (className == "Brick")
		entity = new EntityBrick(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 0.0f), false);
	else if (className == "BrickSpawn")
		entity = new EntityBrickSpawn();
	else if (className == "Floor")
		entity = new EntityFloor(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 0.0f), +1);
	else if (className == "Player")
		entity = new Player(client, m_inputMgr);
	else if (className == "WeaponDefault")
		entity = new WeaponDefault(0);

	//

	AssertMsg(entity, "cannot create entity. unknown type. className=%s", className.c_str());
	if (entity)
		entity->PostCreate();

	//LOG_DBG("done creating entity");

	return entity;
}

void Engine::SV_OnChannelConnect(Channel* channel)
{
	DB_TRACE("");

	if (m_role & ROLE_SERVER)
	{
		Client* client = new Client(this);
		client->Initialize(channel, false);
		client->m_repID = m_repMgr->SV_CreateClient(client, 0);

		m_serverClients.push_back(client);

		if (m_game)
			m_game->HandlePlayerConnect(client);

		DB_TRACE("Created new player.\n");
	}
}

void Engine::SV_OnChannelDisconnect(Channel* channel)
{
	DB_TRACE("");

	if (m_role & ROLE_SERVER)
	{
		std::vector<Client*>::iterator i = m_serverClients.end();

		for (std::vector<Client*>::iterator j = m_serverClients.begin(); j != m_serverClients.end(); ++j)
		{
			if ((*j)->m_channel == channel)
				i = j;
		}

		if (i != m_serverClients.end())
		{
			Client* client = *i;

			m_serverClients.erase(i);

			for (size_t i = 0; i < client->m_entities.size(); ++i)
				if (!client->m_entities[i].expired())
					m_serverScene->RemoveEntity(client->m_entities[i].lock());
			client->m_entities.clear();

			m_inputMgr->SV_RemoveClient(client);
			m_repMgr->SV_DestroyClient(client->m_repID);

			delete client;
		}
		else
			DB_TRACE("Received disconnect from unknown channel.\n");
	}
}

bool Engine::OnReplicationObjectCreate1(Replication::Client* client, const std::string& className, NetSerializableObject** out_serializableObject, void** out_up)
{
	// TODO: build list.
	Entity* entity = CreateEntity(client->GetClient(), className);

	if (entity)
	{
		*out_serializableObject = entity;
		*out_up = entity;
		return true;
	}
	else
	{
		return false;
	}
}

void Engine::OnReplicationObjectCreate2(Replication::Client* client, void* up)
{
	Entity* entity1 = (Entity*)up;
	ShEntity entity2 = ShEntity(entity1);

	LOG_DBG("OnReplicationObjectCreate2: adding entity of type '%s' to scene", entity1->GetClassName().c_str());

	m_clientClient->m_clientScene->AddEntity(entity2, entity2->m_id);
}

void Engine::OnReplicationObjectDestroy(Replication::Client* client, void* up)
{
	DB_TRACE("");

	Entity* entity = (Entity*)up;

	m_clientClient->m_clientScene->RemoveEntity(entity->m_id);
}
