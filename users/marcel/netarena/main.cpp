#include "arena.h"
#include "BitStream.h"
#include "Calc.h"
#include "ChannelManager.h"
#include "client.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "netobject.h"
#include "NetProtocols.h"
#include "PacketDispatcher.h"
#include "player.h"
#include "ReplicationClient.h"
#include "ReplicationManager.h"
#include "RpcManager.h"
#include "Timer.h"

//

App * g_app = 0;

//

static uint32_t s_rpcPlaySound = 0;
static uint32_t s_rpcSetPlayerInputs = 0;

static void HandleRpc(uint32_t method, BitStream & bitStream)
{
	if (method == s_rpcPlaySound)
	{
		const std::string filename = bitStream.ReadString();

		Sound(filename.c_str()).play();
	}
	else if (method == s_rpcSetPlayerInputs)
	{
		uint16_t channelId;
		uint16_t buttons;

		bitStream.Read(channelId);
		bitStream.Read(buttons);

		Player * player = g_host->findPlayerByChannelId(channelId);

		Assert(player);
		if (player)
		{
			player->m_input.m_currButtons = buttons;
		}
	}
	else
	{
		AssertMsg(false, "unknown RPC call: %u", method);
	}
}

//

void App::SV_OnChannelConnect(Channel * channel)
{
	ClientInfo clientInfo;

	clientInfo.replicationId = m_replicationMgr->SV_CreateClient(channel, 0);

	clientInfo.player = new Player(channel->m_destinationId);

	m_replicationMgr->SV_AddObject(clientInfo.player);

	m_hostClients[channel] = clientInfo;

	//

	m_host->addPlayer(clientInfo.player);
}

void App::SV_OnChannelDisconnect(Channel * channel)
{
	std::map<Channel*, ClientInfo>::iterator i = m_hostClients.find(channel);

	if (i != m_hostClients.end())
	{
		ClientInfo & clientInfo = i->second;

		// remove player created for this channel

		m_replicationMgr->SV_RemoveObject(clientInfo.player->GetObjectID());

		delete clientInfo.player;
		clientInfo.player = 0;

		m_replicationMgr->SV_DestroyClient(clientInfo.replicationId);

		m_hostClients.erase(i);
	}
}

//

bool App::OnReplicationObjectSerializeType(ReplicationClient * client, ReplicationObject * object, BitStream & bitStream)
{
	const NetObject * netObject = static_cast<NetObject*>(object);

	const uint8_t type = netObject->getType();

	bitStream.Write(type);

	return true;
}

bool App::OnReplicationObjectCreateType(ReplicationClient * client, BitStream & bitStream, ReplicationObject ** out_object)
{
	uint8_t type;

	bitStream.Read(type);

	NetObject * netObject = 0;

	switch (type)
	{
	case kNetObjectType_Arena:
		netObject = new Arena();
		break;

	case kNetObjectType_Player:
		netObject = new Player();
		break;
	}

	Assert(netObject);

	*out_object = netObject;

	return (netObject != 0);
}

void App::OnReplicationObjectCreated(ReplicationClient * client, ReplicationObject * object)
{
	Client * gameClient = static_cast<Client*>(client->m_up);
	NetObject * netObject = static_cast<NetObject*>(object);

	switch (netObject->getType())
	{
	case kNetObjectType_Arena:
		Assert(gameClient->m_arena == 0);
		gameClient->m_arena = static_cast<Arena*>(object);
		break;

	case kNetObjectType_Player:
		gameClient->m_players.push_back(static_cast<Player*>(object));
		break;
	}
}

void App::OnReplicationObjectDestroyed(ReplicationClient * client, ReplicationObject * object)
{
	Client * gameClient = static_cast<Client*>(client->m_up);
	NetObject * netObject = static_cast<NetObject*>(object);

	// todo : remove object from game client

	switch (netObject->getType())
	{
	default:
		Assert(false);
		break;
	}
}

//

App::App()
	: m_isHost(false)
	, m_host(0)
	, m_packetDispatcher(0)
	, m_channelMgr(0)
	, m_rpcMgr(0)
	, m_replicationMgr(0)
{
}

App::~App()
{
	Assert(m_isHost == false);

	Assert(m_packetDispatcher == 0);
	Assert(m_channelMgr == 0);
	Assert(m_rpcMgr == 0);
	Assert(m_replicationMgr == 0);

	Assert(m_host == 0);

	Assert(m_clients.empty());
}

bool App::init(bool isHost)
{
	Calc::Initialize();

	framework.minification = 2;

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		m_isHost = isHost;

		//

		m_packetDispatcher = new PacketDispatcher();
		m_channelMgr = new ChannelManager();
		m_rpcMgr = new RpcManager(m_channelMgr);
		m_replicationMgr = new ReplicationManager();

		//

		m_packetDispatcher->RegisterProtocol(PROTOCOL_CHANNEL, m_channelMgr);
		m_packetDispatcher->RegisterProtocol(PROTOCOL_RPC, m_rpcMgr);
		m_packetDispatcher->RegisterProtocol(PROTOCOL_REPLICATION, m_replicationMgr);

		//

		const int port = m_isHost ? 6000 : 0;

		m_channelMgr->Initialize(m_packetDispatcher, this, port, m_isHost);

		//

		m_replicationMgr->CL_RegisterHandler(this);

		//

		s_rpcPlaySound = m_rpcMgr->Register("PlaySound", HandleRpc);
		s_rpcSetPlayerInputs = m_rpcMgr->Register("SetPlayerInputs", HandleRpc);

		//

		if (m_isHost)
		{
			m_host = new Host();

			m_host->init();
		}

		// add clients

		for (int i = 0; i < 1; ++i)
		{
			Channel * channel = m_channelMgr->CreateChannel(ChannelPool_Client);

			channel->Connect(NetAddress(127, 0, 0, 1, 6000));

			Client * client = new Client(i);

			client->initialize(channel);
			client->m_replicationId = m_replicationMgr->CL_CreateClient(client->m_channel, client);

			m_clients.push_back(client);
		}

		return true;
	}

	return false;
}

void App::shutdown()
{
	for (size_t i = 0; i < m_clients.size(); ++i)
	{
		delete m_clients[i];
		m_clients[i] = 0;
	}

	m_clients.clear();

	if (m_isHost)
	{
		m_host->shutdown();

		delete m_host;
		m_host = 0;
	}

	//

	m_replicationMgr->SV_Shutdown();

	m_replicationMgr->CL_Shutdown();

	m_rpcMgr->Unregister(s_rpcPlaySound, HandleRpc);
	m_rpcMgr->Unregister(s_rpcSetPlayerInputs, HandleRpc);

	m_channelMgr->Shutdown(true);

	delete m_replicationMgr;
	m_replicationMgr = 0;

	delete m_rpcMgr;
	m_rpcMgr = 0;

	delete m_channelMgr;
	m_channelMgr = 0;

	delete m_packetDispatcher;
	m_packetDispatcher = 0;

	m_isHost = false;

	framework.shutdown();
}

bool App::tick()
{
	const float dt = 1.f / 60.f; // todo : calculate time step

	// update host

	m_channelMgr->Update(g_TimerRT.TimeUS_get());

	m_replicationMgr->SV_Update();

	framework.process();

	m_host->tick(dt);

	m_channelMgr->Update(g_TimerRT.TimeUS_get());

	// update client

	m_channelMgr->Update(g_TimerRT.TimeUS_get());

	m_replicationMgr->CL_Update();

	for (size_t i = 0; i < m_clients.size(); ++i)
	{
		Client * client = m_clients[i];

		client->tick(dt);
	}

	m_channelMgr->Update(g_TimerRT.TimeUS_get());

	return true;
}

void App::draw()
{
	framework.beginDraw(155, 205, 255, 0);
	{
		for (size_t i = 0; i < m_clients.size(); ++i)
		{
			Client * client = m_clients[i];

			client->draw();
		}
	}
	framework.endDraw();
}

void App::netPlaySound(const char * filename)
{
	BitStream bs;

	bs.WriteString(filename);

	m_rpcMgr->Call(s_rpcPlaySound, bs, ChannelPool_Server, 0, true, true);
}

void App::netSetPlayerInputs(uint16_t channelId, uint16_t buttons)
{
	BitStream bs;

	bs.Write(channelId);
	bs.Write(buttons);

	m_rpcMgr->Call(s_rpcSetPlayerInputs, bs, ChannelPool_Client, &channelId, false, false);
}

//

int main(int argc, char * argv[])
{
	changeDirectory("data");

	g_app = new App();

	if (!g_app->init(true))
	{
		//
	}
	else
	{
		while (g_app->tick())
		{
			g_app->draw();
		}

		g_app->shutdown();
	}

	delete g_app;
	g_app = 0;

	return 0;
}
