#include <algorithm>
#include <functional>
#include "arena.h"
#include "BitStream.h"
#include "bullet.h"
#include "Calc.h"
#include "ChannelManager.h"
#include "client.h"
#include "discover.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "host.h"
#include "main.h"
#include "netobject.h"
#include "NetProtocols.h"
#include "netsprite.h"
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

OPTION_DECLARE(std::string, s_mapList, "arena.txt");
OPTION_DEFINE(std::string, s_mapList, "App/Map List");
OPTION_ALIAS(s_mapList, "maps");
OPTION_FLAGS(s_mapList, OPTION_FLAG_HIDDEN);
std::vector<std::string> g_mapList;

OPTION_DECLARE(std::string, g_map, "");
OPTION_DEFINE(std::string, g_map, "App/Startup Map");
OPTION_ALIAS(g_map, "map");

OPTION_DECLARE(bool, g_hosting, true);
OPTION_DEFINE(bool, g_hosting, "App/Enable Hosting");
OPTION_ALIAS(g_hosting, "hosting");

OPTION_DECLARE(bool, g_connectLocal, false);
OPTION_DEFINE(bool, g_connectLocal, "App/Connect Locally");
OPTION_ALIAS(g_connectLocal, "localconnect");

OPTION_DECLARE(std::string, g_connect, "");
OPTION_DEFINE(std::string, g_connect, "App/Direct Connect");
OPTION_ALIAS(g_connect, "connect");

OPTION_DECLARE(bool, g_noSound, false);
OPTION_DEFINE(bool, g_noSound, "Sound/Disable Sound Effects");
OPTION_ALIAS(g_noSound, "nosound");

OPTION_EXTERN(int, g_playerCharacterIndex);

//

TIMER_DEFINE(g_appTickTime, PerFrame, "App/Tick");
TIMER_DEFINE(g_appDrawTime, PerFrame, "App/Draw");

//

App * g_app = 0;

int g_updateTicks = 0;

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

	if (action == "char_select")
	{
		const int clientChannelId = args.getInt("client_channel", -1);
		const int playerNetId = args.getInt("player_net_id", -1);
		const int characterIndex = args.getInt("char", -1);

		g_app->netSetPlayerCharacterIndex(clientChannelId, playerNetId, characterIndex);
	}
}

//

enum RpcMethod
{
	s_rpcSyncGameSim,
#if ENABLE_CLIENT_SIMULATION
	s_rpcAddPlayer,
	s_rpcAddPlayerBroadcast,
	s_rpcRemovePlayer,
	s_rpcRemovePlayerBroadcast,
#endif
	s_rpcPlaySound,
#if !ENABLE_CLIENT_SIMULATION
	s_rpcScreenShake,
#endif
	s_rpcSetPlayerInputs,
	s_rpcBroadcastPlayerInputs,
	s_rpcSetPlayerCharacterIndex,
#if ENABLE_CLIENT_SIMULATION
	s_rpcSetPlayerCharacterIndexBroadcast,
#endif
	s_rpcSpawnBullet,
	s_rpcKillBullet,
	s_rpcUpdateBullet,
	s_rpcAddSprite,
	s_rpcSyncSprite,
	s_rpcRemoveSprite,
#if !ENABLE_CLIENT_SIMULATION
	s_rpcSpawnParticles,
#endif
	s_rpcUpdateBlock
};

void App::handleRpc(Channel * channel, uint32_t method, BitStream & bitStream)
{
	if (method == s_rpcSyncGameSim)
	{
		LOG_DBG("handleRpc: s_rpcSyncGameSim");

		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client)
		{
			NetSerializationContext context;
			context.Set(true, false, bitStream);
			client->m_gameSim->serialize(context);
		}
	}
#if ENABLE_CLIENT_SIMULATION
	else if (method == s_rpcAddPlayer)
	{
		LOG_DBG("handleRpc: s_rpcAddPlayer");

		uint8_t characterIndex;

		bitStream.Read(characterIndex);

		PlayerToAddOrRemove playerToAdd;
		playerToAdd.add = true;
		playerToAdd.channel = channel;
		playerToAdd.characterIndex = characterIndex;
		g_app->m_playersToAddOrRemove.push_back(playerToAdd);
	}
	else if (method == s_rpcAddPlayerBroadcast)
	{
		LOG_DBG("handleRpc: s_rpcAddPlayerBroadcast");

		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client)
		{
			uint16_t channelId;
			uint32_t netId;
			uint8_t index;
			uint8_t characterIndex;

			bitStream.Read(channelId);
			bitStream.Read(netId);
			bitStream.Read(index);
			bitStream.Read(characterIndex);

			Player * player = &client->m_gameSim->m_state.m_players[index];
			*player = Player();

			PlayerNetObject * netObject = new PlayerNetObject(netId, channelId, player, client->m_gameSim);
			netObject->setPlayerId(index);
			player->m_netObject = netObject;
			netObject->setCharacterIndex(characterIndex);

			client->addPlayer(netObject);
		}
	}
	else if (method == s_rpcRemovePlayer)
	{
		LOG_DBG("handleRpc: s_rpcRemovePlayer");

		g_app->m_rpcMgr->Call(s_rpcRemovePlayerBroadcast, bitStream, ChannelPool_Server, 0, true, false);
	}
	else if (method == s_rpcRemovePlayerBroadcast)
	{
		LOG_DBG("handleRpc: s_rpcRemovePlayerBroadcast");

		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client)
		{
			uint8_t index;

			bitStream.Read(index);

			const uint32_t crc1 = client->m_gameSim->calcCRC();

			PlayerNetObject * player = client->m_gameSim->m_players[index];
			Assert(player);
			if (player)
			{
				client->removePlayer(player);
				delete player;
			}

			const uint32_t crc2 = client->m_gameSim->calcCRC();

			LOG_DBG("remove CRCs: %08x, %08x", crc1, crc2);
		}
	}
#endif
	else if (method == s_rpcPlaySound)
	{
		const std::string filename = bitStream.ReadString();
		uint8_t volume;
		bitStream.Read(volume);

		if (g_app->isSelectedClient(channel))
		{
			if (!g_noSound)
			{
				Sound(filename.c_str()).play(volume);
			}
		}
	}
#if !ENABLE_CLIENT_SIMULATION
	else if (method == s_rpcScreenShake)
	{
		LOG_DBG("handleRpc: s_rpcScreenShake");

		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client)
		{
			float dx;
			float dy;
			float stiffness;
			float life;

			bitStream.Read(dx);
			bitStream.Read(dy);
			bitStream.Read(stiffness);
			bitStream.Read(life);

			client->m_gameSim->addScreenShake(
				Vec2(dx, dy),
				stiffness,
				life);
		}
	}
#endif
	else if (method == s_rpcSetPlayerInputs)
	{
		//LOG_DBG("handleRpc: s_rpcSetPlayerInputs");

		uint32_t netId;
		PlayerInput input;

		bitStream.Read(netId);
		bitStream.Read(input.buttons);
		bitStream.Read(input.analogX);
		bitStream.Read(input.analogY);

		PlayerNetObject * player = g_host->findPlayerByNetId(netId);

		Assert(player);
		if (player)
		{
			player->m_input.m_currState = input;
		}
	}
	else if (method == s_rpcBroadcastPlayerInputs)
	{
		//LOG_DBG("handleRpc: s_rpcBroadcastPlayerInputs");

		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client)
		{
			uint32_t crc;

			bitStream.Read(crc);

			if (g_devMode)
			{
				const uint32_t clientCRC = client->m_gameSim->calcCRC();

				Assert(crc == 0 || crc == clientCRC);

				if (ENABLE_GAMESTATE_CRC_LOGGING && crc != 0 && crc != clientCRC)
				{
					LOG_ERR("crc mismatch! host=%08x, client=%08x", crc, clientCRC);

					if (g_host)
					{
						g_host->m_gameSim.clearPlayerPtrs();
						client->m_gameSim->clearPlayerPtrs();

						const uint8_t * hostBytes = (uint8_t*)&g_host->m_gameSim.m_state;
						const uint8_t * clientBytes = (uint8_t*)&client->m_gameSim->m_state;
						const int numBytes = sizeof(GameSim::GameState);

						uint8_t * temp = (uint8_t*)alloca(numBytes);
						static GameSim::GameState * state = (GameSim::GameState*)temp;

						for (int i = 0; i < numBytes; ++i)
						{
							if (hostBytes[i] != clientBytes[i])
							{
								LOG_ERR("byte mismatch @ %d", i);
								//break;
							}
							temp[i] = hostBytes[i] ^ clientBytes[i];
						}

						g_host->m_gameSim.setPlayerPtrs();
						client->m_gameSim->setPlayerPtrs();
					}
				}
			}

			//

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				PlayerInput input;

				const bool hasButtons = bitStream.ReadBit();
				if (hasButtons)
					bitStream.Read(input.buttons);

				const bool hasAnalogX = bitStream.ReadBit();
				if (hasAnalogX)
					bitStream.Read(input.analogX);

				const bool hasAnalogY = bitStream.ReadBit();
				if (hasAnalogY)
					bitStream.Read(input.analogY);

				PlayerNetObject * player = client->m_gameSim->m_players[i];

				if (player)
					player->m_input.m_currState = input;
			}

		#if ENABLE_CLIENT_SIMULATION
			client->m_gameSim->tick();
		#endif
		}
	}
	else if (method == s_rpcSetPlayerCharacterIndex)
	{
		LOG_DBG("handleRpc: s_rpcSetPlayerCharacterIndex");

		uint32_t netId;
		uint8_t characterIndex;

		bitStream.Read(netId);
		bitStream.Read(characterIndex);

		PlayerNetObject * player = g_host->findPlayerByNetId(netId);
		Assert(player);
		if (player)
		{
			player->setCharacterIndex(characterIndex);
		}
	#if ENABLE_CLIENT_SIMULATION
		g_app->netBroadcastCharacterIndex(netId, characterIndex);
	#endif
	}
#if ENABLE_CLIENT_SIMULATION
	else if (method == s_rpcSetPlayerCharacterIndexBroadcast)
	{
		LOG_DBG("handleRpc: s_rpcSetPlayerCharacterIndexBroadcast");

		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client)
		{
			uint32_t netId;
			uint8_t characterIndex;

			bitStream.Read(netId);
			bitStream.Read(characterIndex);

			PlayerNetObject * player = client->findPlayerByNetId(netId);
			Assert(player);
			if (player)
			{
				player->setCharacterIndex(characterIndex);
			}
		}
	}
#endif
#if 0
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
			uint8_t _angle;

			bitStream.Read(id);
			bitStream.Read(type);
			bitStream.Read(x);
			bitStream.Read(y);
			bitStream.Read(_angle);

			
		}
	}
#endif
#if !ENABLE_CLIENT_SIMULATION
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
			//Assert(b.isAlive);
			
			bitStream.ReadAlignedBytes(&b, sizeof(b));
		}
	}
#endif
	else if (method == s_rpcAddSprite || method == s_rpcSyncSprite)
	{
		NetSpriteManager * spriteManager = 0;

		if (channel)
		{
			Client * client = g_app->findClientByChannel(channel);
			Assert(client);
			if (client)
				spriteManager = client->m_spriteManager;
		}
		else
		{
			spriteManager = g_hostSpriteManager;
		}

		Assert(spriteManager);
		if (spriteManager)
		{
			uint16_t id;
			std::string filename;
			int16_t x;
			int16_t y;

			bitStream.Read(id);
			filename = bitStream.ReadString();
			bitStream.Read(x);
			bitStream.Read(y);

			NetSprite & sprite = spriteManager->m_sprites[id];

			sprite.set(filename.c_str(), x, y);
		}
	}
	else if (method == s_rpcRemoveSprite)
	{
		NetSpriteManager * spriteManager = 0;

		if (channel)
		{
			Client * client = g_app->findClientByChannel(channel);
			Assert(client);
			if (client)
				spriteManager = client->m_spriteManager;
		}
		else
		{
			spriteManager = g_hostSpriteManager;
		}

		Assert(spriteManager);
		if (spriteManager)
		{
			uint16_t id;

			bitStream.Read(id);

			NetSprite & sprite = spriteManager->m_sprites[id];
			Assert(sprite.enabled);
			sprite.enabled = false;
		}
	}
#if !ENABLE_CLIENT_SIMULATION
	else if (method == s_rpcSpawnParticles)
	{
		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client)
		{
			NetSerializationContext context;
			context.Set(true, false, bitStream);

			ParticleSpawnInfo spawnInfo;
			spawnInfo.serialize(context);

			client->spawnParticles(spawnInfo);
		}
	}
#endif
	else if (method == s_rpcUpdateBlock)
	{
		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client && client->m_arena)
		{
			uint8_t x;
			uint8_t y;
			
			bitStream.Read(x);
			bitStream.Read(y);

			Block & block = client->m_arena->m_arena->getBlock(x, y);

			bitStream.ReadAlignedBytes(&block, sizeof(block));
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

void App::processPlayerChanges()
{
	for (size_t i = 0; i < m_playersToAddOrRemove.size(); ++i)
	{
		// todo : remove from playerToAdd on channel disconnect

		const PlayerToAddOrRemove & playerToAddOrRemove = m_playersToAddOrRemove[i];

		if (playerToAddOrRemove.add)
		{
			// todo : add a separate client sync action

			Channel * channel = playerToAddOrRemove.channel;

		#if ENABLE_CLIENT_SIMULATION
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				PlayerNetObject * netObject = m_host->m_gameSim.m_players[i];

				if (netObject)
					netAddPlayerBroadcast(channel, netObject->getOwningChannelId(), netObject->getNetId(), i, netObject->getCharacterIndex());
			}
		#endif

			m_host->syncNewClient(channel);

			//

			PlayerNetObject * player = m_host->allocPlayer(playerToAddOrRemove.channel->m_destinationId);

			if (player)
			{
				ClientInfo & clientInfo = m_hostClients[playerToAddOrRemove.channel];

				clientInfo.player = player;

				player->setCharacterIndex(playerToAddOrRemove.characterIndex);

				m_host->addPlayer(player);

			#if ENABLE_CLIENT_SIMULATION
				netAddPlayerBroadcast(0, playerToAddOrRemove.channel->m_destinationId, player->getNetId(), player->getPlayerId(), player->getCharacterIndex());
			#else
				m_replicationMgr->SV_AddObject(player);
			#endif
			}
		}
		else
		{
		#if ENABLE_CLIENT_SIMULATION
			netRemovePlayerBroadcast(playerToAddOrRemove.playerId);
		#endif

			PlayerNetObject * player = m_host->m_gameSim.m_players[playerToAddOrRemove.playerId];

			m_host->removePlayer(player);
			delete player;
			player = 0;
		}
	}
	m_playersToAddOrRemove.clear();
}

void App::broadcastPlayerInputs()
{
#if ENABLE_CLIENT_SIMULATION
	if (g_devMode)
		for (int i = 0; i < 10; ++i)
			m_channelMgr->Update(g_TimerRT.TimeUS_get());

	netSetPlayerInputsBroadcast();

	if (g_devMode)
		for (int i = 0; i < 10; ++i)
			m_channelMgr->Update(g_TimerRT.TimeUS_get());
#endif
}

//

void App::SV_OnChannelConnect(Channel * channel)
{
	ClientInfo clientInfo;

	clientInfo.replicationId = m_replicationMgr->SV_CreateClient(channel, 0);

	m_hostClients[channel] = clientInfo;

	//

	PlayerToAddOrRemove playerToAdd;
	playerToAdd.add = true;
	playerToAdd.channel = channel;
	playerToAdd.characterIndex = 0;
	m_playersToAddOrRemove.push_back(playerToAdd);
}

void App::SV_OnChannelDisconnect(Channel * channel)
{
	// todo : remove from m_playersToAdd

	std::map<Channel*, ClientInfo>::iterator i = m_hostClients.find(channel);

	if (i != m_hostClients.end())
	{
		ClientInfo & clientInfo = i->second;

		// remove player created for this channel

		if (clientInfo.player)
		{
		#if !ENABLE_CLIENT_SIMULATION
			m_replicationMgr->SV_RemoveObject(clientInfo.player->GetObjectID());
		#endif

			PlayerToAddOrRemove playerToRemove;
			playerToRemove.add = false;
			playerToRemove.playerId = clientInfo.player->getPlayerId();
			m_playersToAddOrRemove.push_back(playerToRemove);

			clientInfo.player = 0;
		}

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
	Client * gameClient = static_cast<Client*>(client->m_up);

	uint8_t type;

	bitStream.Read(type);

	NetObject * netObject = 0;

	switch (type)
	{
	case kNetObjectType_Arena:
		{
			ArenaNetObject * arenaNetObject = new ArenaNetObject(true);
			netObject = arenaNetObject;
		}
		break;

	#if !ENABLE_CLIENT_SIMULATION
	case kNetObjectType_Player:
		netObject = new PlayerNetObject();
		break;
	#endif
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
		gameClient->m_arena = static_cast<ArenaNetObject*>(object);
		break;

	#if !ENABLE_CLIENT_SIMULATION
	case kNetObjectType_Player:
		{
			PlayerNetObject * playerNetObject = static_cast<PlayerNetObject*>(object);
			gameClient->addPlayer(playerNetObject);
			break;
		}
	#endif

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
	case kNetObjectType_Arena:
		delete gameClient->m_arena;
		gameClient->m_arena = 0;
		break;

	#if !ENABLE_CLIENT_SIMULATION
	case kNetObjectType_Player:
		gameClient->removePlayer(static_cast<PlayerNetObject*>(netObject));
		break;
	#endif

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
	, m_selectedClient(-1)
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

	std::string mapList = s_mapList;

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
			g_mapList.push_back(file);

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
					if (g_host)
						g_host->newRound((char*)param);
				}, fileCopy
			);
		}
	} while (!mapList.empty());

	if (g_devMode)
	{
		framework.minification = 2;
		framework.fullscreen = false;
		//framework.reloadCachesOnActivate = true;
	}
	else
	{
		framework.fullscreen = true;
	}

	framework.actionHandler = HandleAction;

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		if (!g_devMode)
		{
			framework.fillCachesWithPath(".");
		}

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

		m_rpcMgr->SetDefaultHandler(handleRpc);

		//

		if (m_isHost)
		{
			m_host = new Host();

			m_host->init();
		}

		//

		Assert(m_freeControllerList.empty());
		m_freeControllerList.clear();
		for (int i = 0; i < MAX_GAMEPAD + 1; ++i)
			m_freeControllerList.push_back(i);

		//

		m_optionMenu = new OptionMenu();
		m_optionMenuIsOpen = false;

		m_statTimerMenu = new StatTimerMenu();
		m_statTimerMenuIsOpen = false;

		//

		if (g_host)
		{
			g_host->newGame();
		}

		if (g_connectLocal)
		{
			g_app->connect("127.0.0.1");
		}

		std::string connect = g_connect;

		if (!connect.empty())
		{
			g_app->connect(connect.c_str());
		}

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

	m_freeControllerList.clear();

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

	Client * client = new Client;

	client->initialize(channel);
	client->m_replicationId = m_replicationMgr->CL_CreateClient(client->m_channel, client);

	m_clients.push_back(client);

	//

	m_selectedClient = (int)m_clients.size() - 1;
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

		//

		if (m_selectedClient >= (int)m_clients.size())
		{
			m_selectedClient = (int)m_clients.size() - 1;
		}
	}
}

void App::selectClient(int index)
{
	m_selectedClient = index;
}

Client * App::getSelectedClient()
{
	if (m_selectedClient >= 0 && m_selectedClient < (int)m_clients.size())
		return m_clients[m_selectedClient];
	else
		return 0;
}

bool App::isSelectedClient(Client * client)
{
	return client == getSelectedClient();
}

bool App::isSelectedClient(Channel * channel)
{
	Client * client = findClientByChannel(channel);
	if (client)
		return isSelectedClient(client);
	else
		return false;
}

bool App::isSelectedClient(uint16_t channelId)
{
	Channel * channel = m_channelMgr->FindChannel(channelId);
	if (channel)
		return isSelectedClient(channel);
	else
		return false;
}

bool App::tick()
{
	TIMER_SCOPE(g_appTickTime);

	// calculate time step

	const uint64_t time = g_TimerRT.TimeMS_get();
	static uint64_t lastTime = time;
	if (time == lastTime)
	{
		SDL_Delay(1);
		return true;
	}

	float dt = (time - lastTime) / 1000.f;
	if (dt > 1.f / 60.f)
		dt = 1.f / 60.f;
	lastTime = time;

	// determine number of update ticks

	static float dtAccum = 0.f;
	dtAccum += dt;
	g_updateTicks = (int)(dtAccum * 60.f);
	dtAccum -= g_updateTicks / 60.f;

#if 1
	g_updateTicks = 1;
#endif

	// shared update

	framework.process();

	m_discoveryService->update(m_isHost);
	
	m_discoveryUi->process();

	// update host

	if (m_isHost)
	{
		m_channelMgr->Update(g_TimerRT.TimeUS_get());

		if (g_updateTicks)
		{
			m_replicationMgr->SV_Update();
		}

		m_channelMgr->Update(g_TimerRT.TimeUS_get());

		if (!m_optionMenuIsOpen)
		{
			if (g_updateTicks)
			{
				const uint32_t crc1 = m_host->m_gameSim.calcCRC();

				processPlayerChanges();

				const uint32_t crc2 = m_host->m_gameSim.calcCRC();

				broadcastPlayerInputs();

				m_host->m_gameSim.tick();

				const uint32_t crc3 = m_host->m_gameSim.calcCRC();

			#if ENABLE_CLIENT_SIMULATION
				if (g_devMode)
					LOG_DBG("tick CRCs: %08x, %08x, %08x", crc1, crc2, crc3);
			#endif
			}

			m_host->tick(dt);

			if (g_updateTicks)
			{
				m_channelMgr->Update(g_TimerRT.TimeUS_get());
			}
		}
	}

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
			setDrawRect(0, 0, ARENA_SX_PIXELS, ARENA_SY_PIXELS);

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

		#if ENABLE_CLIENT_SIMULATION
			{
				int y = 100;
				setFont("calibri.ttf");
				drawText(0, y += 30, 24, +1, +1, "random seed=%08x, next pickup tick=%u, crc=%08x, px=%g, py=%g",
					g_host->m_gameSim.m_state.m_randomSeed,
					(uint32_t)g_host->m_gameSim.m_state.m_nextPickupSpawnTick,
					g_host->m_gameSim.calcCRC(),
					g_host->m_gameSim.m_players[0] ? g_host->m_gameSim.m_players[0]->m_player->m_pos[0] : 0.f,
					g_host->m_gameSim.m_players[0] ? g_host->m_gameSim.m_players[0]->m_player->m_pos[1] : 0.f);
				for (size_t i = 0; i < m_clients.size(); ++i)
				{
					drawText(0, y += 30, 24, +1, +1, "random seed=%08x, next pickup tick=%u, crc=%08x, px=%g, py=%g",
						m_clients[i]->m_gameSim->m_state.m_randomSeed,
						(uint32_t)m_clients[i]->m_gameSim->m_state.m_nextPickupSpawnTick,
						m_clients[i]->m_gameSim->calcCRC(),
						m_clients[i]->m_gameSim->m_players[0] ? m_clients[i]->m_gameSim->m_players[0]->m_player->m_pos[0] : 0.f,
						m_clients[i]->m_gameSim->m_players[0] ? m_clients[i]->m_gameSim->m_players[0]->m_player->m_pos[1] : 0.f);
				}
			}
		#endif
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

	#if 1
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

		Client * client = getSelectedClient();

		if (client)
		{
			int index = 0;

			for (size_t i = 0; i < client->m_players.size(); ++i)
			{
				PlayerNetObject * player = client->m_players[i];

				if (player->getOwningChannelId() != client->m_channel->m_id)
					continue;

				if (!player->hasValidCharacterIndex())
				{
					for (int c = 0; c < 2; ++c)
					{
						char name[32];
						sprintf(name, "select_%d_%d", i, c);
						Dictionary & button = (*m_discoveryUi)[name];
						char props[1024];
						sprintf(props, "type:button name:%s x:%d y:%d scale:0.65 action:char_select client_channel:%d player_net_id:%d char:%d image:button.png image_over:button-over.png image_down:button-down.png text:char text_color:000000 font:calibri.ttf font_size:24",
							name,
							index * 150,
							GFX_SY - 150 + c * 50,
							client->m_channel->m_id,
							player->getNetId(),
							c);
						button.parse(props);
					}

					++index;
				}
			}
		}

		m_discoveryUi->draw();
	#endif
	}
	TIMER_STOP(g_appDrawTime);
	framework.endDraw();
	TIMER_START(g_appDrawTime);
}

void App::netSyncGameSim(Channel * channel)
{
	LOG_DBG("netSyncGameSim");

	BitStream bs;

	NetSerializationContext context;
	context.Set(true, true, bs);
	m_host->m_gameSim.serialize(context);

	m_rpcMgr->Call(s_rpcSyncGameSim, bs, ChannelPool_Client, &channel->m_id, false, false);
}

#if ENABLE_CLIENT_SIMULATION
void App::netAddPlayer(Channel * channel, uint8_t characterIndex)
{
	LOG_DBG("netAddPlayer");

	BitStream bs;

	bs.Write(characterIndex);

	m_rpcMgr->Call(s_rpcAddPlayer, bs, ChannelPool_Client, &channel->m_id, false, false);
}

void App::netAddPlayerBroadcast(Channel * channel, uint16_t owningChannelId, uint32_t netId, uint8_t index, uint8_t characterIndex)
{
	LOG_DBG("netAddPlayerBroadcast");

	BitStream bs;

	bs.Write(owningChannelId);
	bs.Write(netId);
	bs.Write(index);
	bs.Write(characterIndex);

	if (channel)
		m_rpcMgr->Call(s_rpcAddPlayerBroadcast, bs, ChannelPool_Server, &channel->m_id, false, false);
	else
		m_rpcMgr->Call(s_rpcAddPlayerBroadcast, bs, ChannelPool_Server, 0, true, false);
}

void App::netRemovePlayer(uint8_t index)
{
	LOG_DBG("netRemovePlayer");
}

void App::netRemovePlayerBroadcast(uint8_t index)
{
	LOG_DBG("netRemovePlayerBroadcast");

	BitStream bs;

	bs.Write(index);

	m_rpcMgr->Call(s_rpcRemovePlayerBroadcast, bs, ChannelPool_Server, 0, true, false);
}
#endif

void App::netPlaySound(const char * filename, uint8_t volume)
{
	BitStream bs;

	bs.WriteString(filename);
	bs.Write(volume);

	m_rpcMgr->Call(s_rpcPlaySound, bs, ChannelPool_Server, 0, true, false);
}

void App::netScreenShake(GameSim & gameSim, float dx, float dy, float stiffness, float life)
{
#if !ENABLE_CLIENT_SIMULATION
	BitStream bs;

	bs.Write(dx);
	bs.Write(dy);
	bs.Write(stiffness);
	bs.Write(life);

	m_rpcMgr->Call(s_rpcScreenShake, bs, ChannelPool_Server, 0, true, false);
#else
	gameSim.addScreenShake(Vec2(dx, dy), stiffness, life);
#endif
}

void App::netSetPlayerInputs(uint16_t channelId, uint32_t netId, const PlayerInput & input)
{
	if (g_devMode)
		LOG_DBG("netSetPlayerInputs");

	BitStream bs;

	bs.Write(netId);
	bs.Write(input.buttons);
	bs.Write(input.analogX);
	bs.Write(input.analogY);

	m_rpcMgr->Call(s_rpcSetPlayerInputs, bs, ChannelPool_Client, &channelId, false, false);
}

#if ENABLE_CLIENT_SIMULATION
void App::netSetPlayerInputsBroadcast()
{
	if (g_devMode)
		LOG_DBG("netSetPlayerInputsBroadcast");

	BitStream bs;

	// todo : if debug, serialize game state CRC

	const uint32_t crc = (g_devMode ? m_host->m_gameSim.calcCRC() : 0);
	bs.Write(crc);

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		const PlayerNetObject * player = m_host->m_gameSim.m_players[i];
		const uint16_t buttons = (player ? player->m_input.m_currState.buttons : 0);
		const int8_t analogX = (player ? player->m_input.m_currState.analogX : 0);
		const int8_t analogY = (player ? player->m_input.m_currState.analogY : 0);

		const bool hasButtons = (buttons != 0);
		bs.WriteBit(hasButtons);
		if (hasButtons)
			bs.Write(buttons);

		const bool hasAnalogX = (analogX != 0);
		bs.WriteBit(hasAnalogX);
		if (hasAnalogX)
			bs.Write(analogX);

		const bool hasAnalogY = (analogY != 0);
		bs.WriteBit(hasAnalogY);
		if (hasAnalogY)
			bs.Write(analogY);
	}

	m_rpcMgr->Call(s_rpcBroadcastPlayerInputs, bs, ChannelPool_Server, 0, true, false);
}
#endif

void App::netSetPlayerCharacterIndex(uint16_t channelId, uint32_t netId, uint8_t characterIndex)
{
	LOG_DBG("netSetPlayerCharacterIndex");

	// client -> host

	BitStream bs;

	bs.Write(netId);
	bs.Write(characterIndex);

	m_rpcMgr->Call(s_rpcSetPlayerCharacterIndex, bs, ChannelPool_Client, &channelId, false, false);
}

#if ENABLE_CLIENT_SIMULATION
void App::netBroadcastCharacterIndex(uint32_t netId, uint8_t characterIndex)
{
	LOG_DBG("netBroadcastCharacterIndex");

	// host -> clients

	BitStream bs;

	bs.Write(netId);
	bs.Write(characterIndex);

	m_rpcMgr->Call(s_rpcSetPlayerCharacterIndexBroadcast, bs, ChannelPool_Server, 0, true, false);
}
#endif

uint16_t App::netSpawnBullet(GameSim & gameSim, int16_t x, int16_t y, uint8_t _angle, uint8_t type, uint32_t ownerNetId)
{
	LOG_DBG("netSpawnBullet");

	const uint16_t id = gameSim.m_bulletPool->alloc();

	if (id != INVALID_BULLET_ID)
	{
		Bullet & b = gameSim.m_bulletPool->m_bullets[id];

		Assert(!b.isAlive);
		memset(&b, 0, sizeof(b));
		b.isAlive = true;
		b.type = static_cast<BulletType>(type);
		b.pos[0] = x;
		b.pos[1] = y;
		b.color = 0xffffffff;

		float angle = _angle / 128.f * float(M_PI);
		float velocity = 0.f;

		switch (type)
		{
		case kBulletType_A:
			velocity = BULLET_TYPE0_SPEED;
			b.maxWrapCount = BULLET_TYPE0_MAX_WRAP_COUNT;
			b.maxReflectCount = BULLET_TYPE0_MAX_REFLECT_COUNT;
			b.maxDistanceTravelled = BULLET_TYPE0_MAX_DISTANCE_TRAVELLED;
			b.maxDestroyedBlocks = 1;
			break;
		case kBulletType_B:
			velocity = BULLET_TYPE0_SPEED;
			b.maxWrapCount = BULLET_TYPE0_MAX_WRAP_COUNT;
			b.maxReflectCount = BULLET_TYPE0_MAX_REFLECT_COUNT;
			b.maxDistanceTravelled = BULLET_TYPE0_MAX_DISTANCE_TRAVELLED;
			b.maxDestroyedBlocks = 1;
			break;
		case kBulletType_Grenade:
			velocity = BULLET_GRENADE_NADE_SPEED;
			b.maxWrapCount = 100;
			b.doGravity = true;
			b.doBounce = true;
			b.bounceAmount = BULLET_GRENADE_NADE_BOUNCE_AMOUNT;
			b.noDamageMap = true;
			b.life = BULLET_GRENADE_NADE_LIFE;
			break;
		case kBulletType_GrenadeA:
			velocity = gameSim.RandomFloat(BULLET_GRENADE_FRAG_SPEED_MIN, BULLET_GRENADE_FRAG_SPEED_MAX);
			b.maxWrapCount = 1;
			b.maxReflectCount = 0;
			b.maxDistanceTravelled = gameSim.RandomFloat(BULLET_GRENADE_FRAG_RADIUS_MIN, BULLET_GRENADE_FRAG_RADIUS_MAX);
			b.maxDestroyedBlocks = 1;
			break;
		default:
			Assert(false);
			break;
		}

		b.setVel(angle, velocity);

		b.ownerNetId = ownerNetId;

		netUpdateBullet(gameSim, id);
	}

	return id;
}

void App::netKillBullet(uint16_t id)
{
#if !ENABLE_CLIENT_SIMULATION
	BitStream bs;

	bs.Write(id);

	m_rpcMgr->Call(s_rpcKillBullet, bs, ChannelPool_Server, 0, true, false);
#endif
}

void App::netUpdateBullet(GameSim & gameSim, uint16_t id)
{
#if !ENABLE_CLIENT_SIMULATION
	Bullet & b = gameSim.m_bulletPool->m_bullets[id];

	BitStream bs;

	bs.Write(id);
	bs.WriteAlignedBytes(&b, sizeof(b));

	m_rpcMgr->Call(s_rpcUpdateBullet, bs, ChannelPool_Server, 0, true, false);
#endif
}

uint16_t App::netAddSprite(const char * filename, int16_t x, int16_t y)
{
	uint16_t id = g_hostSpriteManager->alloc();

	if (id != SPRITE_ID_INVALID)
	{
		BitStream bs;

		bs.Write(id);
		bs.WriteString(filename);
		bs.Write(x);
		bs.Write(y);

		m_rpcMgr->Call(s_rpcAddSprite, bs, ChannelPool_Server, 0, true, true);
	}

	return id;
}

void App::netSyncSprite(uint16_t id, Channel * channel)
{
	NetSprite & sprite = g_hostSpriteManager->m_sprites[id];
	Assert(sprite.enabled);
	if (sprite.enabled)
	{
		BitStream bs;

		int16_t x = sprite.sprite->x;
		int16_t y = sprite.sprite->y;

		bs.Write(id);
		bs.WriteString(sprite.filename.c_str());
		bs.Write(x);
		bs.Write(y);

		m_rpcMgr->Call(s_rpcSyncSprite, bs, ChannelPool_Server, &channel->m_id, false, false);
	}
}

void App::netRemoveSprite(uint16_t id)
{
	Assert(id != SPRITE_ID_INVALID);
	if (id != SPRITE_ID_INVALID)
	{
		BitStream bs;

		bs.Write(id);

		m_rpcMgr->Call(s_rpcRemoveSprite, bs, ChannelPool_Server, 0, true, true);

		g_hostSpriteManager->free(id);
	}
}

#if !ENABLE_CLIENT_SIMULATION
void App::netSpawnParticles(const ParticleSpawnInfo & spawnInfo)
{
	BitStream bs;

	NetSerializationContext context;
	context.Set(true, true, bs);

	const_cast<ParticleSpawnInfo&>(spawnInfo).serialize(context);

	m_rpcMgr->Call(s_rpcSpawnParticles, bs, ChannelPool_Server, 0, true, false);
}

void App::netUpdateBlock(uint8_t x, uint8_t y, const Block & block)
{
	BitStream bs;

	bs.Write(x);
	bs.Write(y);
	bs.WriteAlignedBytes(&block, sizeof(block));

	m_rpcMgr->Call(s_rpcUpdateBlock, bs, ChannelPool_Server, 0, true, false);
}
#endif

int App::allocControllerIndex()
{
	if (m_freeControllerList.empty())
		return -1;
	std::sort(m_freeControllerList.begin(), m_freeControllerList.end(), std::greater<int>());
	const int index = m_freeControllerList.back();
	m_freeControllerList.pop_back();
	return index;
}

void App::freeControllerIndex(int index)
{
	Assert(index != -1);
	if (index != -1)
		m_freeControllerList.push_back(index);
}

int App::getControllerAllocationCount() const
{
	return (MAX_GAMEPAD + 1) - m_freeControllerList.size();
}

//

int main(int argc, char * argv[])
{
	changeDirectory("data");

	g_optionManager.LoadFromCommandLine(argc, argv);

	g_app = new App();

	bool isHost = g_hosting;

	if (!g_app->init(isHost))
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
