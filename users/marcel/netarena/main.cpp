#include "arena.h"
#include "BitStream.h"
#include "bullet.h"
#include "Calc.h"
#include "ChannelManager.h"
#include "client.h"
#include "discover.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "netobject.h"
#include "NetProtocols.h"
#include "OptionMenu.h"
#include "PacketDispatcher.h"
#include "player.h"
#include "ReplicationClient.h"
#include "ReplicationManager.h"
#include "RpcManager.h"
#include "StatTimerMenu.h"
#include "StatTimers.h"
#include "StringBuilder.h"
#include "Timer.h"

//

OPTION_DECLARE(bool, g_devMode, false);
OPTION_DEFINE(bool, g_devMode, "App/Developer Mode");
OPTION_ALIAS(g_devMode, "devmode");

OPTION_DECLARE(std::string, g_mapList, "arena.txt");
OPTION_DEFINE(std::string, g_mapList, "App/Map List");
OPTION_ALIAS(g_mapList, "maps");

//

TIMER_DEFINE(g_appTickTime, PerFrame, "App/Tick");
TIMER_DEFINE(g_appDrawTime, PerFrame, "App/Draw");

//

App * g_app = 0;

//

static void HandleAction(const std::string & action, const Dictionary & args)
{
	if (action == "connect")
	{
		const std::string address = args.getString("address", "");

		if (!address.empty())
		{
			g_app->connect(address.c_str());
		}
	}

	if (action == "disconnect")
	{
		const int index = args.getInt("client", -1);

		if (index != -1)
		{
			g_app->disconnectClient(index);
		}
	}

	if (action == "select")
	{
		const int index = args.getInt("client", -1);

		if (index != -1)
		{
			g_app->selectClient(index);
		}
	}
}

//

enum RpcMethod
{
	s_rpcPlaySound,
	s_rpcSetPlayerInputs,
	s_rpcSpawnBullet,
	s_rpcKillBullet,
	s_rpcUpdateBullet
};

void App::handleRpc(Channel * channel, uint32_t method, BitStream & bitStream)
{
	if (method == s_rpcPlaySound)
	{
		const std::string filename = bitStream.ReadString();
		uint8_t volume;
		bitStream.Read(volume);

		Sound(filename.c_str()).play(volume);
	}
	else if (method == s_rpcSetPlayerInputs)
	{
		uint32_t netId;
		uint16_t buttons;

		bitStream.Read(netId);
		bitStream.Read(buttons);

		Player * player = g_host->findPlayerByNetId(netId);

		Assert(player);
		if (player)
		{
			player->m_input.m_currButtons = buttons;
		}
	}
	else if (method == s_rpcSpawnBullet)
	{
		BulletPool * bulletPool = 0;

		if (channel)
		{
			Client * client = g_app->findClientByChannel(channel);
			Assert(client);
			if (client)
				bulletPool = client->m_bulletPool;
		}
		else
		{
			bulletPool = g_hostBulletPool;
		}

		Assert(bulletPool);
		if (bulletPool)
		{
			uint16_t id;
			uint8_t type;
			int16_t x;
			int16_t y;
			uint8_t angle;

			bitStream.Read(id);
			bitStream.Read(type);
			bitStream.Read(x);
			bitStream.Read(y);
			bitStream.Read(angle);

			Bullet & b = bulletPool->m_bullets[id];
			Assert(!b.isAlive);

			memset(&b, 0, sizeof(b));
			b.isAlive = true;
			b.type = static_cast<BulletType>(type);
			b.x = x;
			b.y = y;
			b.angle = angle / 128.f * float(M_PI);
			
			switch (type)
			{
			case kBulletType_A:
				b.velocity = BULLET_TYPE0_SPEED;
				b.maxWrapCount = BULLET_TYPE0_MAX_WRAP_COUNT;
				b.maxReflectCount = BULLET_TYPE0_MAX_REFLECT_COUNT;
				b.maxDistanceTravelled = BULLET_TYPE0_MAX_DISTANCE_TRAVELLED;
				break;
			default:
				Assert(false);
				break;
			}
		}
	}
	else if (method == s_rpcKillBullet)
	{
		Client * client = g_app->findClientByChannel(channel);

		Assert(client);
		if (client)
		{
			uint16_t id;

			bitStream.Read(id);

			Bullet & b = client->m_bulletPool->m_bullets[id];
			Assert(b.isAlive);
			
			b.isAlive = false;
		}
	}
	else if (method == s_rpcUpdateBullet)
	{
		Client * client = g_app->findClientByChannel(channel);

		Assert(client);
		if (client)
		{
			uint16_t id;

			bitStream.Read(id);

			Bullet & b = client->m_bulletPool->m_bullets[id];
			Assert(b.isAlive);
			
			bitStream.ReadAlignedBytes(&b, sizeof(b));
		}
	}
	else
	{
		AssertMsg(false, "unknown RPC call: %u", method);
	}
}

Client * App::findClientByChannel(Channel * channel)
{
	for (size_t i = 0; i < m_clients.size(); ++i)
		if (m_clients[i]->m_channel == channel)
			return m_clients[i];
	return 0;
}

//

void App::SV_OnChannelConnect(Channel * channel)
{
	ClientInfo clientInfo;

	clientInfo.replicationId = m_replicationMgr->SV_CreateClient(channel, 0);

	clientInfo.player = new Player(m_host->allocNetId());

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

		m_host->removePlayer(clientInfo.player);

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

	default:
		Assert(false);
		break;
	}
}

void App::OnReplicationObjectDestroyed(ReplicationClient * client, ReplicationObject * object)
{
	Client * gameClient = static_cast<Client*>(client->m_up);
	NetObject * netObject = static_cast<NetObject*>(object);

	switch (netObject->getType())
	{
	case kNetObjectType_Player:
		{
			auto i = std::find(gameClient->m_players.begin(), gameClient->m_players.end(), static_cast<Player*>(netObject));
			Assert(i != gameClient->m_players.end());
			if (i != gameClient->m_players.end())
				gameClient->m_players.erase(i);
		}
		break;

	case kNetObjectType_Arena:
		{
			delete gameClient->m_arena;
			gameClient->m_arena = 0;
		}
		break;

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
	, m_discoveryService(0)
	, m_discoveryUi(0)
	, m_selectedClient(0)
	, m_optionMenu(0)
	, m_optionMenuIsOpen(false)
	, m_statTimerMenu(0)
	, m_statTimerMenuIsOpen(false)
{
}

App::~App()
{
	Assert(m_isHost == false);

	Assert(m_packetDispatcher == 0);
	Assert(m_channelMgr == 0);
	Assert(m_rpcMgr == 0);
	Assert(m_replicationMgr == 0);

	Assert(m_discoveryService == 0);
	Assert(m_discoveryUi == 0);

	Assert(m_host == 0);

	Assert(m_clients.empty());
}

bool App::init(bool isHost)
{
	Calc::Initialize();

	g_optionManager.Load("options.txt");
	g_optionManager.Load("gameoptions.txt");

	std::string mapList = g_mapList;

	do
	{
		const size_t pos = mapList.find(',');

		std::string file;

		if (pos == mapList.npos)
		{
			file = mapList;
			mapList.clear();
		}
		else
		{
			file = mapList.substr(0, pos);
			mapList = mapList.substr(pos + 1);
		}

		if (!file.empty())
		{
			StringBuilder<64> sb;
			sb.AppendFormat("Arena/Load %s", file.c_str());
			std::string name = sb.ToString();
			char * nameCopy = new char[name.size() + 1];
			char * fileCopy = new char[file.size() + 1];
			strcpy_s(nameCopy, name.size() + 1, name.c_str());
			strcpy_s(fileCopy, file.size() + 1, file.c_str());
			g_optionManager.AddCommandOption(nameCopy,
				[](void * param)
				{
					if (g_hostArena)
						g_hostArena->load((char*)param);
				}, fileCopy
			);
		}
	} while (!mapList.empty());

	if (g_devMode)
	{
		framework.minification = 2;
		framework.fullscreen = false;
		framework.reloadCachesOnActivate = true;
	}
	else
	{
		framework.fullscreen = true;
	}

	framework.actionHandler = HandleAction;

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		m_isHost = isHost;

		//

		m_packetDispatcher = new PacketDispatcher();
		m_channelMgr = new ChannelManager();
		m_rpcMgr = new RpcManager(m_channelMgr);
		m_replicationMgr = new ReplicationManager();

		m_discoveryService = new NetSessionDiscovery::Service();
		m_discoveryService->init(2, 10);

		m_discoveryUi = new Ui();

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

		m_rpcMgr->RegisterWithID(s_rpcPlaySound, handleRpc);
		m_rpcMgr->RegisterWithID(s_rpcSetPlayerInputs, handleRpc);
		m_rpcMgr->RegisterWithID(s_rpcSpawnBullet, handleRpc);
		m_rpcMgr->RegisterWithID(s_rpcKillBullet, handleRpc);
		m_rpcMgr->RegisterWithID(s_rpcUpdateBullet, handleRpc);

		//

		if (m_isHost)
		{
			m_host = new Host();

			m_host->init();
		}

		//

		m_optionMenu = new OptionMenu();
		m_optionMenuIsOpen = false;

		m_statTimerMenu = new StatTimerMenu();
		m_statTimerMenuIsOpen = false;

		return true;
	}

	return false;
}

void App::shutdown()
{
	delete m_statTimerMenu;
	m_statTimerMenu = 0;

	delete m_optionMenu;
	m_optionMenu = 0;

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

	delete m_discoveryUi;
	m_discoveryUi = 0;

	delete m_discoveryService;
	m_discoveryService = 0;

	m_replicationMgr->SV_Shutdown();

	m_replicationMgr->CL_Shutdown();

	m_rpcMgr->Unregister(s_rpcPlaySound, handleRpc);
	m_rpcMgr->Unregister(s_rpcSetPlayerInputs, handleRpc);
	m_rpcMgr->Unregister(s_rpcSpawnBullet, handleRpc);
	m_rpcMgr->Unregister(s_rpcKillBullet, handleRpc);
	m_rpcMgr->Unregister(s_rpcUpdateBullet, handleRpc);

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

void App::connect(const char * address)
{
	Channel * channel = m_channelMgr->CreateChannel(ChannelPool_Client);

	channel->Connect(NetAddress(address, 6000));

	Client * client = new Client(m_clients.size());

	client->initialize(channel);
	client->m_replicationId = m_replicationMgr->CL_CreateClient(client->m_channel, client);

	m_clients.push_back(client);
}

void App::disconnectClient(int index)
{
	if (index >= 0 && index < (int)m_clients.size())
	{
		Client * client = m_clients[index];

		m_replicationMgr->CL_DestroyClient(client->m_replicationId);

		m_clients[index]->m_channel->Disconnect();

		delete client;

		m_clients.erase(m_clients.begin() + index);
	}
}

void App::selectClient(int index)
{
	m_selectedClient = index;
}

bool App::tick()
{
	TIMER_SCOPE(g_appTickTime);

	const float dt = 1.f / 60.f; // todo : calculate time step

	// shared update

	framework.process();

	m_discoveryService->update(m_isHost);
	
	m_discoveryUi->process();

	// update host

	m_channelMgr->Update(g_TimerRT.TimeUS_get());

	m_replicationMgr->SV_Update();

	if (!m_optionMenuIsOpen)
	{
		m_host->tick(dt);
	}

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

	// debug

	if (keyboard.wentDown(SDLK_F5))
	{
		m_optionMenuIsOpen = !m_optionMenuIsOpen;
		m_statTimerMenuIsOpen = false;
	}

	if (keyboard.wentDown(SDLK_F6))
	{
		m_optionMenuIsOpen = false;
		m_statTimerMenuIsOpen = !m_statTimerMenuIsOpen;
	}

	if (m_optionMenuIsOpen || m_statTimerMenuIsOpen)
	{
		MultiLevelMenuBase * menu = 0;

		if (m_optionMenuIsOpen)
			menu = m_optionMenu;
		else if (m_statTimerMenuIsOpen)
			menu = m_statTimerMenu;

		menu->Update();

		if (keyboard.isDown(SDLK_UP) || gamepad[0].isDown(DPAD_UP))
			menu->HandleAction(OptionMenu::Action_NavigateUp, dt);
		if (keyboard.isDown(SDLK_DOWN) || gamepad[0].isDown(DPAD_DOWN))
			menu->HandleAction(OptionMenu::Action_NavigateDown, dt);
		if (keyboard.wentDown(SDLK_RETURN) || gamepad[0].wentDown(GAMEPAD_A))
			menu->HandleAction(OptionMenu::Action_NavigateSelect);
		if (keyboard.wentDown(SDLK_BACKSPACE) || gamepad[0].wentDown(GAMEPAD_B))
		{
			if (menu->HasNavParent())
				menu->HandleAction(OptionMenu::Action_NavigateBack);
			else
			{
				m_optionMenuIsOpen = false;
				m_statTimerMenuIsOpen = false;
			}
		}
		if (keyboard.isDown(SDLK_LEFT) || gamepad[0].isDown(DPAD_LEFT))
			menu->HandleAction(OptionMenu::Action_ValueDecrement, dt);
		if (keyboard.isDown(SDLK_RIGHT) || gamepad[0].isDown(DPAD_RIGHT))
			menu->HandleAction(OptionMenu::Action_ValueIncrement, dt);
	}

	if (keyboard.wentDown(SDLK_t))
	{
		g_optionManager.Load("options.txt");
	}

#if GG_ENABLE_TIMERS
	TIMER_STOP(g_appTickTime);
	g_statTimerManager.Update();
	TIMER_START(g_appTickTime);
#endif

	if (keyboard.wentDown(SDLK_ESCAPE))
	{
		exit(0);
	}

	return true;
}

void App::draw()
{
	TIMER_SCOPE(g_appDrawTime);

	framework.beginDraw(10, 15, 10, 0);
	{
		if (m_selectedClient >= 0 && m_selectedClient < (int)m_clients.size())
		{
			setDrawRect(0, 0, ARENA_SX * BLOCK_SX, ARENA_SY * BLOCK_SY);

			setBlend(BLEND_OPAQUE);
			Sprite("back.png").draw();
			setBlend(BLEND_ALPHA);

			Client * client = m_clients[m_selectedClient];

			client->draw();

			clearDrawRect();

			setColor(255, 255, 255);
			Font font("calibri.ttf");
			setFont(font);
			drawText(5, GFX_SY - 25, 20, +1.f, -1.f, "viewing client %d", m_selectedClient);
		}

		if (g_devMode && g_host)
		{
			g_host->debugDraw();
		}

		if (m_optionMenuIsOpen)
		{
			const int sx = 500;
			const int sy = GFX_SY / 3;
			const int x = (GFX_SX - sx) / 2;
			const int y = (GFX_SY - sy) / 2;

			m_optionMenu->Draw(x, y, sx, sy);
		}

		if (m_statTimerMenuIsOpen)
		{
			const int sx = 500;
			const int sy = GFX_SY / 3;
			const int x = (GFX_SX - sx) / 2;
			const int y = (GFX_SY - sy) / 2;

			m_statTimerMenu->Draw(x, y, sx, sy);
		}

		m_discoveryUi->clear();

		const auto serverList = m_discoveryService->getServerList();

		for (size_t i = 0; i < serverList.size(); ++i)
		{
			const auto & serverInfo = serverList[i];

			char name[32];
			sprintf(name, "connect_%d", i);
			Dictionary & button = (*m_discoveryUi)[name];
			char props[1024];
			sprintf(props, "type:button name:%s x:%d y:0 action:connect address:%s image:button.png image_over:button-over.png image_down:button-down.png text_color:000000 font:calibri.ttf font_size:24",
				name,
				i * 300,
				serverInfo.m_address.ToString(false).c_str(),
				name);
			button.parse(props);
			button.setString("text", serverInfo.m_address.ToString(false).c_str());
		}

		for (size_t i = 0; i < m_clients.size(); ++i)
		{
			Client * client = m_clients[i];

			{
				char name[32];
				sprintf(name, "disconnect_%d", i);
				Dictionary & button = (*m_discoveryUi)[name];
				char props[1024];
				sprintf(props, "type:button name:%s x:%d y:70 scale:0.65 action:disconnect client:%d image:button.png image_over:button-over.png image_down:button-down.png text:disconnect text_color:000000 font:calibri.ttf font_size:24",
					name, i * 150,
					i);
				button.parse(props);
			}

			{
				char name[32];
				sprintf(name, "view_%d", i);
				Dictionary & button = (*m_discoveryUi)[name];
				char props[1024];
				sprintf(props, "type:button name:%s x:%d y:120 scale:0.65 action:select client:%d image:button.png image_over:button-over.png image_down:button-down.png text:view text_color:000000 font:calibri.ttf font_size:24",
					name, i * 150,
					i);
				button.parse(props);
			}
		}

		m_discoveryUi->draw();
	}
	TIMER_STOP(g_appDrawTime);
	framework.endDraw();
	TIMER_START(g_appDrawTime);
}

void App::netPlaySound(const char * filename, uint8_t volume)
{
	BitStream bs;

	bs.WriteString(filename);
	bs.Write(volume);

	m_rpcMgr->Call(s_rpcPlaySound, bs, ChannelPool_Server, 0, true, false);
}

void App::netSetPlayerInputs(uint16_t channelId, uint32_t netId, uint16_t buttons)
{
	BitStream bs;

	bs.Write(netId);
	bs.Write(buttons);

	m_rpcMgr->Call(s_rpcSetPlayerInputs, bs, ChannelPool_Client, &channelId, false, false);
}

uint16_t App::netSpawnBullet(int16_t x, int16_t y, uint8_t angle, uint8_t type, uint32_t ownerNetId)
{
	const uint16_t id = g_hostBulletPool->alloc();

	if (id != INVALID_BULLET_ID)
	{
		BitStream bs;

		bs.Write(id);
		bs.Write(type);
		bs.Write(x);
		bs.Write(y);
		bs.Write(angle);

		m_rpcMgr->Call(s_rpcSpawnBullet, bs, ChannelPool_Server, 0, true, true);

		// todo : do extra initialization here, after the basic setup has been completed

		Bullet & b = g_hostBulletPool->m_bullets[id];

		b.ownerNetId = ownerNetId;
	}

	return id;
}

void App::netKillBullet(uint16_t id)
{
	BitStream bs;

	bs.Write(id);

	m_rpcMgr->Call(s_rpcKillBullet, bs, ChannelPool_Server, 0, true, false);
}

void App::netUpdateBullet(uint16_t id)
{
	Bullet & b = g_hostBulletPool->m_bullets[id];

	BitStream bs;

	bs.Write(id);
	bs.WriteAlignedBytes(&b, sizeof(b));

	m_rpcMgr->Call(s_rpcUpdateBullet, bs, ChannelPool_Server, 0, true, false);
}

//

int main(int argc, char * argv[])
{
	changeDirectory("data");

	g_optionManager.LoadFromCommandLine(argc, argv);

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
