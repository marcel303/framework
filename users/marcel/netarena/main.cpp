#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include "arena.h"
#include "BitStream.h"
#include "bullet.h"
#include "Calc.h"
#include "ChannelManager.h"
#include "client.h"
#include "discover.h"
#include "FileStream.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "host.h"
#include "main.h"
#include "mainmenu.h"
#include "NetProtocols.h"
#include "OptionMenu.h"
#include "PacketDispatcher.h"
#include "Path.h"
#include "player.h"
#include "RpcManager.h"
#include "StatTimerMenu.h"
#include "StatTimers.h"
#include "StreamReader.h"
#include "StringBuilder.h"
#include "textfield.h"
#include "Timer.h"

#include "spriter.h"

//

OPTION_DECLARE(bool, g_devMode, false);
OPTION_DEFINE(bool, g_devMode, "App/Developer Mode");
OPTION_ALIAS(g_devMode, "devmode");

OPTION_DECLARE(bool, g_monkeyMode, false);
OPTION_DEFINE(bool, g_monkeyMode, "App/Monkey Mode");
OPTION_ALIAS(g_monkeyMode, "monkeymode");

OPTION_DECLARE(bool, g_logCRCs, false);
OPTION_DEFINE(bool, g_logCRCs, "App/Enable CRC Logging");
OPTION_ALIAS(g_logCRCs, "logcrc");

OPTION_DECLARE(bool, g_windowed, false);
OPTION_DEFINE(bool, g_windowed, "App/Windowed Mode");
OPTION_ALIAS(g_windowed, "windowed");

OPTION_DECLARE(std::string, s_mapList, "testArena");
OPTION_DEFINE(std::string, s_mapList, "App/Map List");
OPTION_ALIAS(s_mapList, "maps");
OPTION_FLAGS(s_mapList, OPTION_FLAG_HIDDEN);
std::vector<std::string> g_mapList;

OPTION_DECLARE(std::string, g_map, "");
OPTION_DEFINE(std::string, g_map, "App/Startup Map");
OPTION_ALIAS(g_map, "map");

OPTION_DECLARE(bool, g_connectLocal, false);
OPTION_DEFINE(bool, g_connectLocal, "App/Connect Locally");
OPTION_ALIAS(g_connectLocal, "localconnect");

OPTION_DECLARE(std::string, g_connect, "");
OPTION_DEFINE(std::string, g_connect, "App/Direct Connect");
OPTION_ALIAS(g_connect, "connect");

OPTION_DECLARE(bool, g_pauseOnOptionMenuOption, true);
OPTION_DEFINE(bool, g_pauseOnOptionMenuOption, "App/Pause On Option Menu");

OPTION_DECLARE(bool, g_fakeIncompatibleServerVersion, false);
OPTION_DEFINE(bool, g_fakeIncompatibleServerVersion, "Net/Fake Incompatible Server Version");

COMMAND_OPTION(s_dropCoins, "Player/Drop Coins", []{ g_app->netDebugAction("dropCoins", ""); });

COMMAND_OPTION(s_addAnnoucement, "App/Add Annoucement", []{ g_app->netDebugAction("addAnnouncement", "Test Annoucenment"); });

OPTION_EXTERN(int, g_playerCharacterIndex);

//

TIMER_DEFINE(g_appTickTime, PerFrame, "App/Tick");
TIMER_DEFINE(g_appDrawTime, PerFrame, "App/Draw");

//

#define NET_TIME g_TimerRT.TimeUS_get() // fixme : use TimeMS
//#define NET_TIME g_TimerRT.TimeMS_get()

//

uint32_t g_buildId = 0;

App * g_app = 0;

int g_updateTicks = 0;

int g_keyboardLock = 0;

Surface * g_colorMap = 0;
Surface * g_lightMap = 0;
Surface * g_finalMap = 0;

//

static void animationTestInit();
static bool animationTestIsActive();
static void animationTestToggleIsActive();
static void animationTestChangeAnim(int direction, int x, int y);
static void animationTestTick(float dt);
static void animationTestDraw();

//

static void HandleAction(const std::string & action, const Dictionary & args)
{
	if (action == "connect")
	{
		const std::string address = args.getString("address", "");

		if (!address.empty())
		{
			g_connect = address;
			g_connectLocal = false;

			g_app->findGame();
		}
	}

	if (action == "disconnect")
	{
		const int index = args.getInt("client", -1);

		if (index >= 0 && index < (int)g_app->m_clients.size())
		{
			g_app->leaveGame(g_app->m_clients[index]);
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

void applyLightMap(Surface & colormap, Surface & lightmap, Surface & dest)
{
	// apply lightmap

	setBlend(BLEND_OPAQUE);
	pushSurface(&dest);
	{
		Shader lightShader("lightmap");
		setShader(lightShader);

		lightShader.setTexture("colormap", 0, colormap.getTexture());
		lightShader.setTexture("lightmap", 1, lightmap.getTexture());

		drawRect(0, 0, colormap.getWidth(), colormap.getHeight());

		lightShader.setTexture("colormap", 0, 0);
		lightShader.setTexture("lightmap", 1, 0);

		clearShader();

		glActiveTexture(GL_TEXTURE0 + 1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0 + 0);
		glDisable(GL_TEXTURE_2D);
	}
	popSurface();
	setBlend(BLEND_ALPHA);
}

//

enum RpcMethod
{
	s_rpcAction,
	s_rpcActionBroadcast,
	s_rpcSyncGameSim,
	s_rpcAddPlayer,
	s_rpcAddPlayerBroadcast,
	s_rpcRemovePlayer,
	s_rpcRemovePlayerBroadcast,
	s_rpcSetPlayerInputs,
	s_rpcBroadcastPlayerInputs,
	s_rpcSetPlayerCharacterIndex,
	s_rpcSetPlayerCharacterIndexBroadcast,
	s_rpcDebugAction,
	s_rpcCOUNT
};

static GameSim * findGameSimForChannel(Channel * channel)
{
	if (!channel)
	{
		Assert(g_host);
		if (g_host)
		{
			return &g_host->m_gameSim;
		}
	}
	else
	{
		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client)
		{
			return client->m_gameSim;
		}
	}

	return 0;
}

static bool canHandleOptionChange(Channel * channel)
{
	if (g_app->m_isHost)
		return channel == 0;
	else if (g_app->m_clients.empty())
		return true;
	else
		return channel == g_app->m_clients.front()->m_channel;
}

void App::handleRpc(Channel * channel, uint32_t method, BitStream & bitStream)
{
	if (method == s_rpcAction)
	{
		Assert(g_host);
		if (g_host)
		{
			uint8_t action;
			uint8_t param1;
			uint8_t param2;
			std::string param3;

			bitStream.Read(action);
			bitStream.Read(param1);
			bitStream.Read(param2);
			param3 = bitStream.ReadString();

			BitStream bs;
			bs.Write(action);
			bs.Write(param1);
			bs.Write(param2);
			bs.WriteString(param3);

			g_app->m_rpcMgr->Call(s_rpcActionBroadcast, bs, ChannelPool_Server, 0, true, true);
		}
	}
	else if (method == s_rpcActionBroadcast)
	{
		Client * client = g_app->findClientByChannel(channel);
		GameSim * gameSim = findGameSimForChannel(channel);

		if (gameSim)
		{
			uint8_t action;
			uint8_t param1;
			uint8_t param2;
			std::string param3;

			bitStream.Read(action);
			bitStream.Read(param1);
			bitStream.Read(param2);
			param3 = bitStream.ReadString();

			switch ((NetAction)action)
			{
			case kPlayerInputAction_CycleGameMode:
				{
					int delta = (int8_t)param1;
					Assert(delta == -1 || delta == +1);
					if (delta == -1 || delta == +1)
					{
						gameSim->m_gameMode = (GameMode)((gameSim->m_gameMode + kGameMode_COUNT + delta) % kGameMode_COUNT);
					}
				}
				break;

			case kNetAction_PlayerInputAction:
				Assert(param1 >= 0 && param1 < MAX_PLAYERS);
				if (param1 >= 0 && param1 < MAX_PLAYERS)
				{
					PlayerInstanceData * playerInstanceData = gameSim->m_playerInstanceDatas[param1];
					Assert(playerInstanceData);
					if (playerInstanceData)
					{
						playerInstanceData->m_player->m_input.m_actions |= (1 << param2);
					}
				}
				break;

			case kNetAction_TextChat:
				Assert(param1 >= 0 && param1 < MAX_PLAYERS);
				if (param1 >= 0 && param1 < MAX_PLAYERS)
				{
					PlayerInstanceData * playerInstanceData = gameSim->m_playerInstanceDatas[param1];
					Assert(playerInstanceData);
					if (playerInstanceData)
					{
						playerInstanceData->addTextChat(param3);

						if (client)
						{
							client->addTextChat(param1, param3);
						}
					}
				}
				break;

			default:
				Assert(false);
				break;
			}
		}
	}
	else if (method == s_rpcSyncGameSim)
	{
		Client * client = g_app->findClientByChannel(channel);
		Assert(client);
		if (client)
		{
			uint8_t isFirst;
			uint8_t isLast;
			uint16_t numBits;

			bitStream.Read(isFirst);
			bitStream.Read(isLast);
			bitStream.Read(numBits);

			if (!isFirst && !isLast)
			{
				LOG_DBG("handleRpc: s_rpcSyncGameSim (continuation)");
			}

			if (isFirst)
			{
				LOG_DBG("handleRpc: s_rpcSyncGameSim: first");

				delete client->m_syncStream;
				client->m_syncStream = new BitStream();
			}

			const int numBytes = Net::BitsToBytes(numBits);
			uint8_t * temp = (uint8_t*)alloca(numBytes);
			bitStream.ReadAlignedBytes(temp, numBytes);

			client->m_syncStream->WriteAlignedBytes(temp, numBytes);

			if (isLast)
			{
				LOG_DBG("handleRpc: s_rpcSyncGameSim: last");

				BitStream bs2(client->m_syncStream->GetData(), client->m_syncStream->GetDataSize());

				NetSerializationContext context;
				context.Set(true, false, bs2);
				client->m_gameSim->serialize(context);

				for (int i = 0; i < MAX_PLAYERS; ++i)
				{
					if (client->m_gameSim->m_players[i].m_isUsed)
					{
						PlayerInstanceData * instanceData = new PlayerInstanceData(&client->m_gameSim->m_players[i], client->m_gameSim);
						client->addPlayer(instanceData, -1);
						instanceData->handleCharacterIndexChange();
					}
				}

				client->m_isSynced = true;

			#if ENABLE_GAMESTATE_CRC_LOGGING
				LOG_DBG("netSyncGameSim: client CRC: %08x", client->m_gameSim->calcCRC());
			#endif

			#if GG_ENABLE_OPTIONS
				for (OptionBase * option = g_optionManager.m_head; option != 0; option = option->GetNext())
				{
					std::string path;
					std::string value;

					context.Serialize(path);
					context.Serialize(value);

					Assert(option && path == option->GetPath());
					if (option)
						option->FromString(value.c_str());
				}
			#endif
			}
		}
		else
		{
			LOG_ERR("handleRpc: s_rpcSyncGameSim: couldn't find client");
		}
	}
	else if (method == s_rpcAddPlayer)
	{
		LOG_DBG("handleRpc: s_rpcAddPlayer");

		uint8_t characterIndex;
		int8_t controllerIndex;
		std::string displayName;

		bitStream.Read(characterIndex);
		bitStream.Read(controllerIndex);
		displayName = bitStream.ReadString();

		//

		g_app->netAddPlayerBroadcast(
			channel->m_destinationId,
			-1,
			characterIndex,
			displayName,
			controllerIndex);
	}
	else if (method == s_rpcAddPlayerBroadcast)
	{
		LOG_DBG("handleRpc: s_rpcAddPlayerBroadcast");

		GameSim * gameSim = findGameSimForChannel(channel);
		Assert(gameSim);
		if (gameSim)
		{
			uint16_t channelId;
			uint8_t index;
			uint8_t characterIndex;
			int8_t controllerIndex;
			std::string displayName;

			bitStream.Read(channelId);
			bitStream.Read(index);
			bitStream.Read(characterIndex);
			bitStream.Read(controllerIndex);
			displayName = bitStream.ReadString();

			//

			PlayerInstanceData * playerInstanceData = gameSim->allocPlayer(channelId);

			if (playerInstanceData)
			{
				Player & player = *playerInstanceData->m_player;

				playerInstanceData->setCharacterIndex(characterIndex);
				player.setDisplayName(displayName);

				if (channel)
				{
					Client * client = g_app->findClientByChannel(channel);
					Assert(client);
					if (client)
					{
						client->addPlayer(playerInstanceData, controllerIndex);
					}
				}
				else
				{
					ClientInfo & clientInfo = g_app->m_hostClients[channelId];

					clientInfo.players.push_back(playerInstanceData);
				}
			}
		}
	}
	else if (method == s_rpcRemovePlayer)
	{
		LOG_DBG("handleRpc: s_rpcRemovePlayer");

		uint8_t index;

		bitStream.Read(index);

		Assert(index >= 0 && index < MAX_PLAYERS);
		if (index >= 0 && index < MAX_PLAYERS)
		{
			BitStream bs2;

			bs2.Write(index);

			g_app->m_rpcMgr->Call(s_rpcRemovePlayerBroadcast, bs2, ChannelPool_Server, 0, true, true);
		}
	}
	else if (method == s_rpcRemovePlayerBroadcast)
	{
		LOG_DBG("handleRpc: s_rpcRemovePlayerBroadcast");

		GameSim * gameSim = findGameSimForChannel(channel);
		Assert(gameSim);
		if (gameSim)
		{
			uint8_t index;

			bitStream.Read(index);

			Assert(index >= 0 && index < MAX_PLAYERS);
			if (index >= 0 && index < MAX_PLAYERS)
			{
			#if ENABLE_GAMESTATE_CRC_LOGGING
				const uint32_t crc1 = gameSim->calcCRC();
			#endif

				PlayerInstanceData * playerInstanceData = gameSim->m_playerInstanceDatas[index];
				Assert(playerInstanceData);
				if (playerInstanceData)
				{
					if (channel)
					{
						Client * client = g_app->findClientByChannel(channel);
						Assert(client);
						if (client)
						{
							client->removePlayer(playerInstanceData);
						}
					}

					gameSim->freePlayer(playerInstanceData);

					delete playerInstanceData;
				}

			#if ENABLE_GAMESTATE_CRC_LOGGING
				const uint32_t crc2 = gameSim->calcCRC();

				if (g_logCRCs)
					LOG_DBG("remove CRCs: %08x, %08x", crc1, crc2);
			#endif
			}
		}
	}
	else if (method == s_rpcSetPlayerInputs)
	{
		//LOG_DBG("handleRpc: s_rpcSetPlayerInputs");

		uint8_t playerId;
		PlayerInput input;

		bitStream.Read(playerId);
		bitStream.Read(input.buttons);
		bitStream.Read(input.analogX);
		bitStream.Read(input.analogY);

		PlayerInstanceData * playerInstanceData = g_host->findPlayerByPlayerId(playerId);

		Assert(playerInstanceData);
		if (playerInstanceData)
		{
			playerInstanceData->m_input.m_lastRecv = input;
		}
	}
	else if (method == s_rpcBroadcastPlayerInputs)
	{
		//LOG_DBG("handleRpc: s_rpcBroadcastPlayerInputs");

	#if ENABLE_GAMESTATE_DESYNC_DETECTION
		uint32_t crc;

		bitStream.Read(crc);

		if (channel)
		{
			Client * client = g_app->findClientByChannel(channel);
			Assert(client);
			if (client)
			{
				const uint32_t clientCRC = client->m_gameSim->calcCRC();
				const bool isInSync = crc == 0 || crc == clientCRC;

				Assert(isInSync);

				if (!isInSync)
				{
					if (!client->m_isDesync)
						client->m_gameSim->playSound("desync.ogg");
					client->m_isDesync = true;
				}

			#if ENABLE_GAMESTATE_CRC_LOGGING
				if (!isInSync && g_devMode)
				{
					LOG_ERR("crc mismatch! host=%08x, client=%08x", crc, clientCRC);

					if (g_host)
					{
						g_host->m_gameSim.clearPlayerPtrs();
						client->m_gameSim->clearPlayerPtrs();

						const uint8_t * hostBytes = (uint8_t*)&static_cast<GameStateData&>(g_host->m_gameSim);
						const uint8_t * clientBytes = (uint8_t*)&static_cast<GameStateData&>(*client->m_gameSim);
						const int numBytes = sizeof(GameStateData);

						uint8_t * temp = (uint8_t*)alloca(numBytes);
						static GameStateData * state;
						state = (GameStateData*)temp;

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

						Assert(g_host->m_gameSim.m_bulletPool->calcCRC() == client->m_gameSim->m_bulletPool->calcCRC());
					}
				}
			#endif
			}
		}
	#endif

		//

		GameSim * gameSim = findGameSimForChannel(channel);
		Assert(gameSim);

		if (gameSim)
		{
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

				PlayerInstanceData * playerInstanceData = gameSim->m_playerInstanceDatas[i];

				if (playerInstanceData)
				{
					playerInstanceData->m_player->m_input.m_currState = input;
				}
			}

			gameSim->tick();
		}
	}
	else if (method == s_rpcSetPlayerCharacterIndex)
	{
		LOG_DBG("handleRpc: s_rpcSetPlayerCharacterIndex");

		Assert(g_host);

		if (g_host && g_host->m_gameSim.m_gameState == kGameState_OnlineMenus)
		{
			uint8_t playerId;
			uint8_t characterIndex;

			bitStream.Read(playerId);
			bitStream.Read(characterIndex);

			g_app->netBroadcastCharacterIndex(playerId, characterIndex);
		}
	}
	else if (method == s_rpcSetPlayerCharacterIndexBroadcast)
	{
		LOG_DBG("handleRpc: s_rpcSetPlayerCharacterIndexBroadcast");

		GameSim * gameSim = findGameSimForChannel(channel);
		Assert(gameSim);
		if (gameSim)
		{
			uint8_t playerId;
			uint8_t characterIndex;

			bitStream.Read(playerId);
			bitStream.Read(characterIndex);

			PlayerInstanceData * playerInstanceData = gameSim->m_playerInstanceDatas[playerId];
			Assert(playerInstanceData);
			if (playerInstanceData)
			{
				playerInstanceData->setCharacterIndex(characterIndex);
			}
		}
	}
	else if (method == s_rpcDebugAction)
	{
		const std::string action = bitStream.ReadString();
		const std::string param = bitStream.ReadString();

		if (action == "newGame")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
				gameSim->newGame();
		}
		else if (action == "newRound")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
				gameSim->newRound(0);
		}
		else if (action == "endRound")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
				gameSim->endRound();
		}
		else if (action == "loadMap")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
				gameSim->newRound(param.c_str());
		}
		else if (action == "killPlayers")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
			{
				for (int i = 0; i < MAX_PLAYERS; ++i)
					if (gameSim->m_players[i].m_isUsed && gameSim->m_players[i].m_isAlive)
						gameSim->m_players[i].handleDamage(1.f, Vec2(gameSim->RandomFloat(-100.f, +100.f), -50.f), 0);
			}
		}
		else if (action == "dropCoins")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
			{
				Assert(!g_gameSim);
				g_gameSim = gameSim;

				for (int p = 0; p < MAX_PLAYERS; ++p)
				{
					Player & player = gameSim->m_players[p];

					if (player.m_isAlive)
					{
						player.dropCoins(player.m_score / 2);
					}
				}

				g_gameSim = 0;
			}
		}
		else if (action == "addAnnouncement")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
			{
				gameSim->addAnnouncement(param.c_str());
			}
		}
		else if (action == "optionSelect")
		{
			if (canHandleOptionChange(channel))
			{
				OptionBase * option = g_optionManager.FindOptionByPath(param.c_str());
				Assert(option);
				if (option)
					option->Select();
			}
		}
		else if (action == "optionIncrement")
		{
			if (canHandleOptionChange(channel))
			{
				OptionBase * option = g_optionManager.FindOptionByPath(param.c_str());
				Assert(option);
				if (option)
					option->Increment();
			}
		}
		else if (action == "optionDecrement")
		{
			if (canHandleOptionChange(channel))
			{
				OptionBase * option = g_optionManager.FindOptionByPath(param.c_str());
				Assert(option);
				if (option)
					option->Decrement();
			}
		}
		else if (action == "loadOptions")
		{
			if (canHandleOptionChange(channel))
			{
				g_optionManager.Load(param.c_str());
			}
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

void App::broadcastPlayerInputs()
{
	netSetPlayerInputsBroadcast();
}

//

void App::SV_OnChannelConnect(Channel * channel)
{
	ClientInfo clientInfo;

	m_hostClients[channel->m_id] = clientInfo;

	netSyncGameSim(channel);
}

void App::SV_OnChannelDisconnect(Channel * channel)
{
	// todo : remove from m_playersToAdd

	auto i = m_hostClients.find(channel->m_destinationId);

	if (i != m_hostClients.end())
	{
		ClientInfo & clientInfo = i->second;

		// remove player created for this channel

		auto players = clientInfo.players;

		for (size_t p = 0; p < players.size(); ++p)
		{
			PlayerInstanceData * playerInstanceData = players[p];
			const int playerIndex = playerInstanceData->m_player->m_index;
			Assert(playerIndex >= 0 && playerIndex < MAX_PLAYERS);

			g_app->netRemovePlayerBroadcast(playerIndex);
		}

		clientInfo.players.clear();

		m_hostClients.erase(i);
	}
}

void App::CL_OnChannelDisconnect(Channel * channel)
{
	Client * client = 0;

	for (size_t i = 0; i < m_clients.size(); ++i)
	{
		if (m_clients[i]->m_channel == channel)
		{
			client = m_clients[i];
		}
	}

	if (client)
	{
		LOG_DBG("CL_OnChannelDisconnect: found client %p for channel %p", client, channel);

		leaveGame(client);
	}
	else
	{
		LOG_ERR("CL_OnChannelDisconnect: couldn't find client for channel %p. numClients=%d", channel, (int)m_clients.size());
	}
}

//

App::App()
	: m_isHost(false)
	, m_host(0)
	, m_packetDispatcher(0)
	, m_channelMgr(0)
	, m_rpcMgr(0)
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

	Assert(m_discoveryService == 0);
	Assert(m_discoveryUi == 0);

	Assert(m_host == 0);

	Assert(m_clients.empty());
}

bool App::init()
{
	Calc::Initialize();

	g_optionManager.Load("options.txt");
	g_optionManager.Load("gameoptions.txt");

	if (DEMOMODE)
	{
		LIBNET_CHANNEL_ENABLE_TIMEOUTS = false;

		NUM_LOCAL_PLAYERS_TO_ADD = 4;
		PLAYER_INACTIVITY_KICK = true;
		VOLCANO_LOOP = true;

		PLAYER_INACTIVITY_TIME = 3;
	}

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
						g_app->netDebugAction("loadMap", (char*)param);
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
	else if (g_windowed)
	{
		framework.minification = 2;
		framework.fullscreen = false;
	}
	else
	{
		framework.fullscreen = true;
	}

	framework.useClosestDisplayMode = true;
	framework.windowTitle = "NetArena";
	framework.actionHandler = HandleAction;

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		if (!g_devMode)
		{
			for (int i = 0; i < 10; ++i)
			{
				framework.beginDraw(0, 0, 0, 0);
				Sprite("loading-back.png").draw();
				framework.endDraw();
			}

			framework.fillCachesWithPath(".", true);
		}

		// input the user's display name

		if (g_devMode)
		{
			m_displayName = "Developer";
		}
		else
		{
			TextField nameInput(GFX_SX/2-200, GFX_SY/3, 400, 60);
			nameInput.open(MAX_PLAYER_DISPLAY_NAME, false); // todo : add max name length define
			while (!nameInput.tick(1.f/60.f))
			{
				framework.process();
				framework.beginDraw(0, 0, 0, 0);
				setColor(colorWhite);
				Sprite("loading-back.png").draw();
				setFont("calibri.ttf");
				drawText(GFX_SX/2, GFX_SY/3 - 30.f, 24, 0.f, -1.f, "Your name?");
				nameInput.draw();
				framework.endDraw();
			}
			m_displayName = nameInput.getText();
		}

		//

		m_packetDispatcher = new PacketDispatcher();
		m_channelMgr = new ChannelManager();
		m_rpcMgr = new RpcManager(m_channelMgr);

		m_discoveryService = new NetSessionDiscovery::Service();
		m_discoveryService->init(2, 10);

		m_discoveryUi = new Ui();

		//

		m_packetDispatcher->RegisterProtocol(PROTOCOL_CHANNEL, m_channelMgr);
		m_packetDispatcher->RegisterProtocol(PROTOCOL_RPC, m_rpcMgr);

		//

		m_rpcMgr->SetDefaultHandler(handleRpc);

		//

		Assert(m_freeControllerList.empty());
		m_freeControllerList.clear();
		for (int i = 0; i < MAX_GAMEPAD + 1; ++i)
			m_freeControllerList.push_back(i);

		//

		m_mainMenu = new MainMenu();

		//

		animationTestInit();

		//

		m_optionMenu = new OptionMenu();
		m_optionMenuIsOpen = false;

		m_statTimerMenu = new StatTimerMenu();
		m_statTimerMenuIsOpen = false;

		//

		g_colorMap = new Surface(ARENA_SX_PIXELS, ARENA_SY_PIXELS);
		g_lightMap = new Surface(ARENA_SX_PIXELS, ARENA_SY_PIXELS);
		g_finalMap = new Surface(ARENA_SX_PIXELS, ARENA_SY_PIXELS);

		//

		initArenaData();

		initCharacterData();

		return true;
	}

	return false;
}

void App::shutdown()
{
	stopHosting();

	//

	shutArenaData();

	shutCharacterData();

	//

	delete g_colorMap;
	g_colorMap = 0;

	delete g_lightMap;
	g_lightMap = 0;

	delete g_finalMap;
	g_finalMap = 0;

	//

	delete m_statTimerMenu;
	m_statTimerMenu = 0;

	delete m_optionMenu;
	m_optionMenu = 0;

	//

	delete m_mainMenu;
	m_mainMenu = 0;

	//

	m_freeControllerList.clear();

	//

	delete m_discoveryUi;
	m_discoveryUi = 0;

	delete m_discoveryService;
	m_discoveryService = 0;

	delete m_rpcMgr;
	m_rpcMgr = 0;

	delete m_channelMgr;
	m_channelMgr = 0;

	delete m_packetDispatcher;
	m_packetDispatcher = 0;

	framework.shutdown();
}

void App::quit()
{
	exit(0);
}

bool App::startHosting()
{
	Assert(m_host == 0);

	if (!m_channelMgr->Initialize(m_packetDispatcher, this, NET_PORT, true, g_buildId))
		return false;

	//

	LOG_DBG("creating host");

	m_host = new Host();

	m_host->init();

	LOG_DBG("creating host [done]");

	//

	m_isHost = true;

	return true;
}

void App::stopHosting()
{
	Assert(m_isHost);

	while (!m_clients.empty())
	{
		destroyClient(0);
	}

	m_channelMgr->Shutdown(true);

	if (m_host)
	{
		LOG_DBG("destroying host");

		m_host->shutdown();

		delete m_host;
		m_host = 0;

		LOG_DBG("destroying host [done]");
	}

	m_isHost = false;
}

bool App::findGame()
{
	const int port = 0;

	if (!m_channelMgr->Initialize(m_packetDispatcher, this, port, false, g_buildId))
		return false;

	//

	if (g_connectLocal)
	{
		connect("127.0.0.1");
	}
	else
	{
		std::string address = g_connect;

		if (!address.empty())
		{
			connect(address.c_str());
		}
	}

	return true;
}

void App::leaveGame(Client * client)
{
	for (size_t i = 0; i < m_clients.size(); ++i)
	{
		if (m_clients[i] == client)
		{
			destroyClient(i);
			break;
		}
	}
}

Client * App::connect(const char * address)
{
	LOG_DBG("connect: %s", address);

	Channel * channel = m_channelMgr->CreateChannel(ChannelPool_Client);

	const auto serverVersion = m_channelMgr->m_serverVersion;
	if (g_fakeIncompatibleServerVersion)
		m_channelMgr->m_serverVersion = -1;
	channel->Connect(NetAddress(address, NET_PORT));
	m_channelMgr->m_serverVersion = serverVersion;

	LOG_DBG("connect: %s [done]", address);

	LOG_DBG("connect: creating client");

	Client * client = new Client;

	client->initialize(channel);

	m_clients.push_back(client);

	LOG_DBG("connect: creating client [done] client=%p", client);

	//

	m_selectedClient = (int)m_clients.size() - 1;

	LOG_DBG("connect: updated selected client to %d", m_selectedClient);

	return client;
}

void App::destroyClient(int index)
{
	if (index >= 0 && index < (int)m_clients.size())
	{
		Client * client = m_clients[index];

		LOG_DBG("destroyClient: destroying client %p", client);

		if (client->m_channel->m_state != ChannelState_Disconnected)
			client->m_channel->Disconnect();

		while (!client->m_players.empty())
		{
			PlayerInstanceData * player = client->m_players.front();
			client->removePlayer(player);
		}

		delete client;

		m_clients.erase(m_clients.begin() + index);

		//

		if (m_selectedClient >= (int)m_clients.size())
		{
			m_selectedClient = (int)m_clients.size() - 1;

			LOG_DBG("destroyClient: updated selected client to %d", m_selectedClient);
		}
	}
	else
	{
		LOG_ERR("destroyClient: invalid index %d", (int)index);
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

#if 0 // fixme : revert
	g_updateTicks = 1;
#endif

	// shared update

	framework.process();

	m_discoveryService->update(m_isHost);
	
	m_discoveryUi->process();

	// update main menu

	if (m_clients.empty())
	{
		m_mainMenu->tick(dt);
	}

	// update host

	if (m_isHost && m_clients.empty())
	{
		// todo : only do this if client is the host

		stopHosting();
	}

	if (m_isHost)
	{
		m_channelMgr->Update(NET_TIME);

		if (!m_optionMenuIsOpen || !g_pauseOnOptionMenuOption)
		{
			if (g_updateTicks)
			{
			#if ENABLE_GAMESTATE_CRC_LOGGING
				if (g_logCRCs)
				{
					for (int i = 0; i < 10; ++i)
						m_channelMgr->Update(NET_TIME);
				}

				const uint32_t crc1 = g_logCRCs ? m_host->m_gameSim.calcCRC() : 0;
			#endif

				broadcastPlayerInputs();

			#if ENABLE_GAMESTATE_CRC_LOGGING
				const uint32_t crc2 = g_logCRCs ? m_host->m_gameSim.calcCRC() : 0;

				if (g_logCRCs)
				{
					for (int i = 0; i < 10; ++i)
						m_channelMgr->Update(NET_TIME);

					LOG_DBG("tick CRCs: %08x, %08x", crc1, crc2);
				}
			#endif
			}

			m_host->tick(dt);

			if (g_updateTicks)
			{
				m_channelMgr->Update(NET_TIME);
			}
		}
	}

	// update client

	m_channelMgr->Update(NET_TIME);

	for (size_t i = 0; i < m_clients.size(); ++i)
	{
		Client * client = m_clients[i];

		client->tick(dt);
	}

	m_channelMgr->Update(NET_TIME);

	if (DEMOMODE && g_host && !m_clients.empty())
	{
		Client * client = m_clients.front();

		for (int i = 0; i < MAX_GAMEPAD; ++i)
		{
			if (gamepad[i].wentDown(GAMEPAD_START))
			{
				bool isUsed = false;
				for (auto p : client->m_players)
				{
					if (p->m_input.m_controllerIndex == i)
					{
						g_app->netRemovePlayer(client->m_channel, p->m_player->m_index);
						isUsed = true;
					}
				}

				if (!isUsed)
				{
					char name[32];
					sprintf_s(name, sizeof(name), "Player %d", i);
					g_app->netAddPlayer(client->m_channel, i, name, i);
				}

				for (int j = 0; j < 10; ++j)
					m_channelMgr->Update(NET_TIME);
			}
		}
	}

	// debug

#if 1
	static bool testAnimationMode = false;
	if (keyboard.wentDown(SDLK_1) && !(getSelectedClient() && getSelectedClient()->m_textChat->isActive()))
		animationTestToggleIsActive();

	if (keyboard.wentDown(SDLK_F1))
	{
		std::vector<Client*> clients = m_clients;

		for (size_t i = 0; i < clients.size(); ++i)
			leaveGame(clients[i]);
	}

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

	if (keyboard.wentDown(SDLK_F12))
	{
		netDebugAction("newRound", "");
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

	if (g_devMode && m_isHost && g_keyboardLock == 0 && keyboard.wentDown(SDLK_t))
	{
		netDebugAction("loadOptions", "options.txt");
	}

	animationTestTick(dt);
#endif

#if GG_ENABLE_TIMERS
	TIMER_STOP(g_appTickTime);
	g_statTimerManager.Update();
	TIMER_START(g_appTickTime);
#endif

	if ((g_keyboardLock == 0 && keyboard.wentDown(SDLK_ESCAPE)) || framework.quitRequested)
	{
		quit();
	}

	return true;
}

void App::draw()
{
	TIMER_SCOPE(g_appDrawTime);

	framework.beginDraw(10, 15, 10, 0);
	{
		// draw main menu

		if (m_clients.empty())
		{
			m_mainMenu->draw();
		}

		if (m_selectedClient >= 0 && m_selectedClient < (int)m_clients.size())
		{
			Client * client = m_clients[m_selectedClient];

			client->draw();

			if (g_devMode)
			{
				client->debugDraw();

				float timeDilation;
				bool playerAttackTimeDilation;
				client->m_gameSim->getCurrentTimeDilation(timeDilation, playerAttackTimeDilation);

				setColor(255, 255, 255);
				Font font("calibri.ttf");
				setFont(font);
				drawText(5, GFX_SY - 25, 20, +1.f, -1.f, "viewing client %d. time dilation %01.2f. state %s", m_selectedClient, timeDilation, g_gameStateNames[client->m_gameSim->m_gameState]);
			}
		}

		if (g_devMode)
		{
			setColor(255, 255, 255);
			Font font("calibri.ttf");
			setFont(font);
			drawText(GFX_SX - 5, GFX_SY - 25, 20, -1.f, -1.f, "build %08x", g_buildId);
		}

		if (g_devMode && g_host)
		{
			g_host->debugDraw();

			{
				int y = 100;
				setFont("calibri.ttf");
				drawText(0, y += 30, 24, +1, +1, "random seed=%08x, next pickup tick=%02.1f, crc=%08x, px=%g, py=%g",
					g_host->m_gameSim.m_randomSeed,
					g_host->m_gameSim.m_nextPickupSpawnTimeRemaining,
				#if ENABLE_GAMESTATE_DESYNC_DETECTION
					g_host->m_gameSim.calcCRC(),
				#else
					-1,
				#endif
					g_host->m_gameSim.m_playerInstanceDatas[0] ? g_host->m_gameSim.m_playerInstanceDatas[0]->m_player->m_pos[0] : 0.f,
					g_host->m_gameSim.m_playerInstanceDatas[0] ? g_host->m_gameSim.m_playerInstanceDatas[0]->m_player->m_pos[1] : 0.f);
				for (size_t i = 0; i < m_clients.size(); ++i)
				{
					drawText(0, y += 30, 24, +1, +1, "random seed=%08x, next pickup tick=%02.1f, crc=%08x, px=%g, py=%g",
						m_clients[i]->m_gameSim->m_randomSeed,
						m_clients[i]->m_gameSim->m_nextPickupSpawnTimeRemaining,
					#if ENABLE_GAMESTATE_DESYNC_DETECTION
						m_clients[i]->m_gameSim->calcCRC(),
					#else
						-1,
					#endif
						m_clients[i]->m_gameSim->m_playerInstanceDatas[0] ? m_clients[i]->m_gameSim->m_playerInstanceDatas[0]->m_player->m_pos[0] : 0.f,
						m_clients[i]->m_gameSim->m_playerInstanceDatas[0] ? m_clients[i]->m_gameSim->m_playerInstanceDatas[0]->m_player->m_pos[1] : 0.f);
				}
			}
		}

		// draw desync notifier

		bool isDesync = false;

		for (size_t i = 0; i < m_clients.size(); ++i)
			isDesync |= m_clients[i]->m_isDesync;

		if (isDesync)
		{
			setColor(colorRed);
			drawRect(0, 0, GFX_SX, 40);
			setColor(colorWhite);
			setFont("calibri.ttf");
			drawText(GFX_SX/2, 12, 30, 0.f, 0.f, "DESYNC");
		}

		//

		animationTestDraw();

		//

		if (m_optionMenuIsOpen)
		{
			const int sx = 500;
			const int sy = GFX_SY / 2;
			const int x = (GFX_SX - sx) / 2;
			const int y = (GFX_SY - sy) / 2;

			m_optionMenu->Draw(x, y, sx, sy);
		}

		if (m_statTimerMenuIsOpen)
		{
			const int sx = 500;
			const int sy = GFX_SY / 2;
			const int x = (GFX_SX - sx) / 2;
			const int y = (GFX_SY - sy) / 2;

			m_statTimerMenu->Draw(x, y, sx, sy);
		}

	#if 1
		if (!DEMOMODE)
		{
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
					sprintf(props, "type:button name:%s x:%d y:00 scale:0.65 action:disconnect client:%d image:button.png image_over:button-over.png image_down:button-down.png text:disconnect text_color:000000 font:calibri.ttf font_size:24",
						name, GFX_SX + (i - m_clients.size()) * 150,
						i);
					button.parse(props);
				}

				{
					char name[32];
					sprintf(name, "view_%d", i);
					Dictionary & button = (*m_discoveryUi)[name];
					char props[1024];
					sprintf(props, "type:button name:%s x:%d y:50 scale:0.65 action:select client:%d image:button.png image_over:button-over.png image_down:button-down.png text:view text_color:000000 font:calibri.ttf font_size:24",
						name, GFX_SX + (i - m_clients.size()) * 150,
						i);
					button.parse(props);
				}
			}

			m_discoveryUi->draw();
		}
	#endif

	#if 0 // todo : remove test collision/SAT code
		gxPushMatrix();
		gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);
		gxScalef(4.f, 4.f, 1.f);
		{
			static int shape1Id = 0;
			static int shape2Id = 0;

			if (keyboard.wentDown(SDLK_1))
				shape1Id = (shape1Id + 1) % kBlockShape_COUNT;
			if (keyboard.wentDown(SDLK_2))
				shape2Id = (shape2Id + 1) % kBlockShape_COUNT;

			CollisionShape shape1 = Arena::getBlockCollision((BlockShape)shape1Id);
			CollisionShape shape2 = Arena::getBlockCollision((BlockShape)shape2Id);

			static float dx = BLOCK_SX;
			static float dy = 0.f;

			if (keyboard.isDown(SDLK_LEFT))
				dx -= 1.f;
			if (keyboard.isDown(SDLK_RIGHT))
				dx += 1.f;
			if (keyboard.isDown(SDLK_UP))
				dy -= 1.f;
			if (keyboard.isDown(SDLK_DOWN))
				dy += 1.f;

			CollisionShape shape3 = shape1;

			shape3.translate(dx, dy);

			float contactDistance;
			Vec2 contactNormal;
			
			const bool collision =
				shape3.checkCollision(shape2, Vec2(1.f, 0.f), contactDistance, contactNormal) ||
				shape3.checkCollision(shape2, Vec2(0.f, 1.f), contactDistance, contactNormal);
			//const bool collision = false;

			if (collision && keyboard.isDown(SDLK_c))
			{
				dx += contactNormal[0] * contactDistance;
				dy += contactNormal[1] * contactDistance;

				shape3 = shape1;
				shape3.translate(dx, dy);
			}

			if (collision)
				setColor(colorGreen);
			else
				setColor(colorRed);

			shape2.debugDraw();
			shape3.debugDraw();

			setColor(colorWhite);
		}
		gxPopMatrix();
	#endif
	}
	TIMER_STOP(g_appDrawTime);
	framework.endDraw();
	TIMER_START(g_appDrawTime);
}

void App::netAction(Channel * channel, NetAction action, uint8_t param1, uint8_t param2, const std::string & param3)
{
	LOG_DBG("netAction");

	BitStream bs;

	uint8_t actionInt = action;

	bs.Write(actionInt);
	bs.Write(param1);
	bs.Write(param2);
	bs.WriteString(param3);

	m_rpcMgr->Call(s_rpcAction, bs, ChannelPool_Client, &channel->m_id, false, false);
}

void App::netSyncGameSim(Channel * channel)
{
	LOG_DBG("netSyncGameSim");
	Assert(m_isHost);

	// todo : when serializing the state, make sure all packets that are destined for the host are flushed
	//        first, to ensure the game sim is up to date and properly synced to the client
	for (int i = 0; i < 10; ++i)
		m_channelMgr->Update(0); // fixme

#if ENABLE_GAMESTATE_CRC_LOGGING
	LOG_DBG("netSyncGameSim: host CRC: %08x", m_host->m_gameSim.calcCRC());
#endif

	BitStream bs;

	NetSerializationContext context;
	context.Set(true, true, bs);
	m_host->m_gameSim.serialize(context);

#if GG_ENABLE_OPTIONS
	for (OptionBase * option = g_optionManager.m_head; option != 0; option = option->GetNext())
	{
		const int bufferSize = 2048;
		char buffer[bufferSize];

		option->ToString(buffer, bufferSize);
		std::string path = option->GetPath();
		std::string value = buffer;

		context.Serialize(path);
		context.Serialize(value);
	}
#endif

	const int kChunkSize = 1024 * 8;

	for (int i = 0, remaining = bs.GetDataSize(); remaining != 0; i += kChunkSize)
	{
		const uint8_t * bytes = (uint8_t*)bs.GetData() + (i >> 3);
		const uint16_t numBits = remaining > kChunkSize ? kChunkSize : remaining;
		remaining -= numBits;

		const uint8_t isFirst = (i == 0);
		const uint8_t isLast = (remaining == 0);

		BitStream bs2;
		
		bs2.Write(isFirst);
		bs2.Write(isLast);
		bs2.Write(numBits);
		bs2.WriteAlignedBytes(bytes, Net::BitsToBytes(numBits));

		m_rpcMgr->Call(s_rpcSyncGameSim, bs2, ChannelPool_Client, &channel->m_id, false, false);
	}
}

void App::netAddPlayer(Channel * channel, uint8_t characterIndex, const std::string & displayName, int8_t controllerIndex)
{
	LOG_DBG("netAddPlayer");

	BitStream bs;

	bs.Write(characterIndex);
	bs.Write(controllerIndex);
	bs.WriteString(displayName);

	m_rpcMgr->Call(s_rpcAddPlayer, bs, ChannelPool_Client, &channel->m_id, false, false);
}

void App::netAddPlayerBroadcast(uint16_t owningChannelId, uint8_t index, uint8_t characterIndex, const std::string & displayName, int8_t controllerIndex)
{
	LOG_DBG("netAddPlayerBroadcast");

	BitStream bs;

	bs.Write(owningChannelId);
	bs.Write(index);
	bs.Write(characterIndex);
	bs.Write(controllerIndex);
	bs.WriteString(displayName);

	m_rpcMgr->Call(s_rpcAddPlayerBroadcast, bs, ChannelPool_Server, 0, true, true);
}

void App::netRemovePlayer(Channel * channel, uint8_t index)
{
	LOG_DBG("netRemovePlayer");

	Assert(index >= 0 && index < MAX_PLAYERS);
	if (index >= 0 && index < MAX_PLAYERS)
	{
		BitStream bs;

		bs.Write(index);

		if (channel)
		{
			m_rpcMgr->Call(s_rpcRemovePlayer, bs, ChannelPool_Client, &channel->m_id, false, false);
		}
		else
		{
			Assert(g_host);

			m_rpcMgr->Call(s_rpcRemovePlayer, bs, ChannelPool_Server, 0, false, true);
		}
	}
}

void App::netRemovePlayerBroadcast(uint8_t index)
{
	LOG_DBG("netRemovePlayerBroadcast");

	Assert(index >= 0 && index < MAX_PLAYERS);
	if (index >= 0 && index < MAX_PLAYERS)
	{
		BitStream bs;

		bs.Write(index);

		m_rpcMgr->Call(s_rpcRemovePlayerBroadcast, bs, ChannelPool_Server, 0, true, true);
	}
}

void App::netSetPlayerInputs(uint16_t channelId, uint8_t playerId, const PlayerInput & input)
{
	//if (g_devMode)
	//	LOG_DBG("netSetPlayerInputs");

	BitStream bs;

	bs.Write(playerId);
	bs.Write(input.buttons);
	bs.Write(input.analogX);
	bs.Write(input.analogY);

	m_rpcMgr->Call(s_rpcSetPlayerInputs, bs, ChannelPool_Client, &channelId, false, false);
}

void App::netSetPlayerInputsBroadcast()
{
	//if (g_devMode)
	//	LOG_DBG("netSetPlayerInputsBroadcast");

	BitStream bs;

	// todo : if debug, serialize game state CRC

#if ENABLE_GAMESTATE_DESYNC_DETECTION
	const uint32_t crc = m_host->m_gameSim.calcCRC();
	bs.Write(crc);
#endif

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		const PlayerInstanceData * playerInstanceData = m_host->m_gameSim.m_playerInstanceDatas[i];
		const uint16_t buttons = (playerInstanceData ? playerInstanceData->m_input.m_lastRecv.buttons : 0);
		const int8_t analogX = (playerInstanceData ? playerInstanceData->m_input.m_lastRecv.analogX : 0);
		const int8_t analogY = (playerInstanceData ? playerInstanceData->m_input.m_lastRecv.analogY : 0);

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

	if (g_logCRCs)
	{
		m_rpcMgr->Call(s_rpcBroadcastPlayerInputs, bs, ChannelPool_Server, 0, true, false);
		for (int i = 0; i < 10; ++i)
			m_channelMgr->Update(NET_TIME);
		m_rpcMgr->Call(s_rpcBroadcastPlayerInputs, bs, ChannelPool_Server, 0, false, true);
	}
	else
	{
		m_rpcMgr->Call(s_rpcBroadcastPlayerInputs, bs, ChannelPool_Server, 0, true, true);
	}
}

void App::netSetPlayerCharacterIndex(uint16_t channelId, uint8_t playerId, uint8_t characterIndex)
{
	LOG_DBG("netSetPlayerCharacterIndex");

	// client -> host

	BitStream bs;

	bs.Write(playerId);
	bs.Write(characterIndex);

	m_rpcMgr->Call(s_rpcSetPlayerCharacterIndex, bs, ChannelPool_Client, &channelId, false, false);
}

void App::netBroadcastCharacterIndex(uint8_t playerId, uint8_t characterIndex)
{
	LOG_DBG("netBroadcastCharacterIndex");

	// host -> clients

	BitStream bs;

	bs.Write(playerId);
	bs.Write(characterIndex);

	m_rpcMgr->Call(s_rpcSetPlayerCharacterIndexBroadcast, bs, ChannelPool_Server, 0, true, true);
}

void App::netDebugAction(const char * name, const char * param)
{
	BitStream bs;

	bs.WriteString(name);
	bs.WriteString(param ? param : "");

	m_rpcMgr->Call(s_rpcDebugAction, bs, ChannelPool_Server, 0, true, true);
}

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

// spriter animation tests

static bool s_animationTestIsActive = false;
static SpriterState s_animationTestState;
static Spriter * s_animationTestSprite = 0;

static void animationTestInit()
{
	auto files = listFiles("testAnimations", true);

	for (auto file : files)
	{
		if (file.find(".scml") == std::string::npos || file.find("autosave") != std::string::npos)
			continue;

		std::string * path = new std::string(file);
		std::string * optionPath = new std::string("Artist Tools/Animation Test/Load '" + Path::GetBaseName(file) + "'");

		g_optionManager.AddCommandOption(optionPath->c_str(),
			[](void * param)
			{
				std::string * path = (std::string*)param;
				s_animationTestState = SpriterState();
				delete s_animationTestSprite;
				s_animationTestSprite = new Spriter(path->c_str());

				if (!animationTestIsActive())
					animationTestToggleIsActive();
			},
			path);
	}
}

static bool animationTestIsActive()
{
	return s_animationTestIsActive;
}

static void animationTestToggleIsActive()
{
	s_animationTestIsActive = !s_animationTestIsActive;
}

static void animationTestChangeAnim(int direction, int x, int y)
{
	if (s_animationTestIsActive && s_animationTestSprite)
	{
		const int animCount = s_animationTestSprite->getAnimCount();

		if (animCount != 0)
		{
			int index = 0;
			if (s_animationTestState.animIndex >= 0)
				index = (s_animationTestState.animIndex + direction + animCount) % animCount;
			s_animationTestState.startAnim(*s_animationTestSprite, index);
			s_animationTestState.x = x;
			s_animationTestState.y = y;
		}
		else
		{
			s_animationTestState.stopAnim(*s_animationTestSprite);
		}
	}
}

static void animationTestTick(float dt)
{
	if (s_animationTestIsActive)
	{
		if (mouse.wentDown(BUTTON_LEFT))
			animationTestChangeAnim(0, mouse.x, mouse.y);
		if (mouse.wentDown(BUTTON_RIGHT))
			animationTestChangeAnim(keyboard.isDown(SDLK_LSHIFT) ? -1 : +1, mouse.x, mouse.y);

		if (s_animationTestSprite)
			s_animationTestState.updateAnim(*s_animationTestSprite, dt);
	}
}

static void animationTestDraw()
{
	if (s_animationTestIsActive)
	{
		setColor(colorGreen);
		drawRect(0, 0, GFX_SX, 40);

		setColor(colorBlack);
		setFont("calibri.ttf");
		drawText(GFX_SX/2, 20, 24, 0.f, 0.f, "Animation Test (Toggle Using '1')");

		setColor(colorWhite);
		if (s_animationTestSprite)
			s_animationTestSprite->draw(s_animationTestState);
	}
}

//

static bool calculateFileCRC(const char * filename, uint32_t & crc)
{
	try
	{
		FileStream stream(filename, OpenMode_Read);
		StreamReader reader(&stream, false);

		const size_t length = stream.Length_get();
		const uint8_t * bytes = reader.ReadAllBytes();

		crc = 0;
		for (size_t i = 0; i < length; ++i)
			crc = crc * 13 + bytes[i];
		delete[] bytes;

		return true;
	}
	catch (std::exception&)
	{
		return false;
	}
}

//

int main(int argc, char * argv[])
{
#if defined(__WIN32__)
	const int kMaxModuleNameSize = 1024;
	char moduleName[kMaxModuleNameSize];
	if (GetModuleFileName(NULL, moduleName, kMaxModuleNameSize) == kMaxModuleNameSize || !calculateFileCRC(moduleName, g_buildId))
	{
		logError("failed to calculate CRC for %s", argv[0]);
		return -1;
	}
	log("build ID: %0x8", g_buildId);
#else
	#error calculate g_buildId
#endif

	changeDirectory("data");

	g_optionManager.LoadFromCommandLine(argc, argv);

	LIBNET_CHANNEL_ENABLE_TIMEOUTS = false;

	if (!g_devMode && !g_connectLocal && (std::string)g_connect == "")
	{
		std::cout << "host IP address: ";
		std::string ip;
		std::getline(std::cin, ip);
		g_connect = ip;
	}

	g_app = new App();

	if (!g_app->init())
	{
		//
	}
	else
	{
	#if 0
		//Spriter spriter("../../ArtistCave/JoyceTestcharacter/char0(Sword)/sprite/Sprite.scml");
		Spriter spriter("char2/sprite/Sprite.scml");

		const int animIndex = 0;
		const float animLength = spriter.getAnimLength(animIndex);
		SpriterState state;
		state.x = GFX_SX/2;
		state.y = GFX_SY/2;
		state.scale = 2.f;
		state.animSpeed = 1.f;//.5f;

		for (int a = 0; a < 300; ++a)
		{
			state.startAnim(spriter, "Idle");

			while (state.animTime < animLength / 1000.f)
			{
				state.updateAnim(spriter, 1.f/60.f);

				framework.process();

				framework.beginDraw(0, 7, 15, 0);
				{
					setColor(0, 0, 255, 63);
					drawLine(0, GFX_SY/2, GFX_SX, GFX_SY/2);
					drawLine(GFX_SX/2, 0, GFX_SX/2, GFX_SY);

					setColor(colorWhite);
					spriter.draw(state);

					Vec2 points[4];

					if (spriter.getHitboxAtTime(
						state.animIndex,
						"box_001",
						state.animTime,
						points))
					{
						CollisionShape shape;
						shape.set(
							points[0],
							points[1],
							points[2],
							points[3]);

						for (int p = 0; p < shape.numPoints; ++p)
						{
							const int p1 = (p + 0) % shape.numPoints;
							const int p2 = (p + 1) % shape.numPoints;

							drawLine(
								state.x + shape.points[p1][0],
								state.y + shape.points[p1][1],
								state.x + shape.points[p2][0],
								state.y + shape.points[p2][1]);
						}
					}

					setColor(colorWhite);
					setFont("calibri.ttf");
					drawText(0.f, 0.f, 24, +1.f, +1.f, "time: %.2f, speed: %.2fx", state.animTime, state.animSpeed);
				}
				framework.endDraw();
			}
		}
	#endif

		//

		if (g_devMode && (g_connectLocal || (std::string)g_connect != ""))
		{
			g_app->startHosting();

			g_app->m_host->m_gameSim.setGameState(kGameState_OnlineMenus);

			g_app->findGame();
		}

		//

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
