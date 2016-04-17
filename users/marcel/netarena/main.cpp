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
#include "dialog.h"
#include "discover.h"
#include "FileStream.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "host.h"
#include "main.h"
#include "mainmenu.h"
#include "NetProtocols.h"
#include "online.h"
#include "OptionMenu.h"
#include "PacketDispatcher.h"
#include "Path.h"
#include "player.h"
#include "RpcManager.h"
#include "settings.h"
#include "StatTimerMenu.h"
#include "StatTimers.h"
#if ENABLE_STEAM
	#include "steam_api.h"
	#include "steam_gameserver.h"
#endif
#include "StreamReader.h"
#include "StringBuilder.h"
#include "textfield.h"
#include "Timer.h"
#include "title.h"
#include "tools.h"
#include "uicommon.h"
#include "MemoryStream.h"
#include "spriter.h"
#include "StreamWriter.h"

//

#if (DEPLOY_BUILD || PUBLIC_DEMO_BUILD) && !defined(DEBUG)
	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

//

OPTION_DECLARE(bool, g_devMode, false);
OPTION_DEFINE(bool, g_devMode, "App/Developer Mode");
OPTION_ALIAS(g_devMode, "devmode");

OPTION_DECLARE(bool, g_monkeyMode, false);
OPTION_DEFINE(bool, g_monkeyMode, "App/Monkey Mode");
OPTION_ALIAS(g_monkeyMode, "monkeymode");

#if ENABLE_STEAM
	OPTION_DECLARE(bool, g_testSteamMatchmaking, false);
	OPTION_DEFINE(bool, g_testSteamMatchmaking, "App/Test Steam Matchmaking");
	OPTION_ALIAS(g_testSteamMatchmaking, "debugSteam");
#endif

OPTION_DECLARE(bool, g_precacheResources, true);
OPTION_DEFINE(bool, g_precacheResources, "App/Precache Resources");
OPTION_ALIAS(g_precacheResources, "precache");

OPTION_DECLARE(bool, g_logCRCs, false);
OPTION_DEFINE(bool, g_logCRCs, "App/Enable CRC Logging");
OPTION_ALIAS(g_logCRCs, "logcrc");

OPTION_DECLARE(bool, g_windowed, false);
OPTION_DEFINE(bool, g_windowed, "App/Windowed Mode");
OPTION_ALIAS(g_windowed, "windowed");

OPTION_DECLARE(int, g_scale, 0);
OPTION_DEFINE(int, g_scale, "App/Window Scale");
OPTION_ALIAS(g_scale, "scale");

OPTION_DECLARE(std::string, s_mapList, "testArena");
OPTION_DEFINE(std::string, s_mapList, "App/Map List");
OPTION_ALIAS(s_mapList, "maps");
OPTION_FLAGS(s_mapList, OPTION_FLAG_HIDDEN);
std::vector<std::string> g_mapList;

OPTION_DECLARE(std::string, s_mapRotationList, "testArena");
OPTION_DEFINE(std::string, s_mapRotationList, "App/Map Rotation List");
OPTION_ALIAS(s_mapRotationList, "maprotation");
OPTION_FLAGS(s_mapRotationList, OPTION_FLAG_HIDDEN);
std::vector<std::string> g_mapRotationList;

OPTION_DECLARE(std::string, g_map, "");
OPTION_DEFINE(std::string, g_map, "App/Startup Map");
OPTION_ALIAS(g_map, "map");

OPTION_DECLARE(bool, g_connectLocal, false);
OPTION_DEFINE(bool, g_connectLocal, "App/Connect Locally");
OPTION_ALIAS(g_connectLocal, "localconnect");

OPTION_DECLARE(std::string, g_connect, "");
OPTION_DEFINE(std::string, g_connect, "App/Direct Connect");
OPTION_ALIAS(g_connect, "connect");

#if ENABLE_STEAM
static CSteamID g_connectLobby;
#endif

OPTION_DECLARE(bool, g_pauseOnOptionMenuOption, true);
OPTION_DEFINE(bool, g_pauseOnOptionMenuOption, "App/Pause On Option Menu");

OPTION_DECLARE(bool, g_fakeIncompatibleServerVersion, false);
OPTION_DEFINE(bool, g_fakeIncompatibleServerVersion, "Network/Fake Incompatible Server Version");

COMMAND_OPTION(s_dropCoins, "Player/Drop Coins", []{ g_app->netDebugAction("dropCoins", ""); });

static void HandleDialogResult(void * arg, int dialogId, DialogResult result)
{
	LOG_DBG("dialog result: arg=%p, id=%d, result=%d", arg, dialogId, (int)result);
}
static int s_lastDialogId = -1;
COMMAND_OPTION(s_addAnnoucement, "Debug/Add Annoucement", []{ g_app->netDebugAction("addAnnouncement", "Test Annoucenment"); });
COMMAND_OPTION(s_addLightEffect_Darken, "Debug/Add Light Effect: Darken", [] { g_app->netDebugAction("addLightEffect", "darken"); });
COMMAND_OPTION(s_addLightEffect_Lighten, "Debug/Add Light Effect: Lighten", [] { g_app->netDebugAction("addLightEffect", "lighten"); });
COMMAND_OPTION(s_addDialog, "Debug/Add Dialog", [] { s_lastDialogId = g_app->m_dialogMgr->push(DialogType_YesNo, "Hello", "World", HandleDialogResult, 0); });
COMMAND_OPTION(s_dismissDialog, "Debug/Dismiss Last Dialog", [] { g_app->m_dialogMgr->dismiss(s_lastDialogId); });
COMMAND_OPTION(s_triggerLevelEvent_QuakeEvent, "Debug/Trigger Quake Level Event", []{ g_app->netDebugAction("triggerLevelEvent", "quake"); });
COMMAND_OPTION(s_triggerLevelEvent_GravityWell, "Debug/Trigger Gravity Well Level Event", []{ g_app->netDebugAction("triggerLevelEvent", "gravity"); });
COMMAND_OPTION(s_triggerLevelEvent_SpikeWalls, "Debug/Trigger Spike Walls Level Event", []{ g_app->netDebugAction("triggerLevelEvent", "spikewalls"); });
COMMAND_OPTION(s_triggerLevelEvent_TimeDilation, "Debug/Trigger Time Dilation Level Event", []{ g_app->netDebugAction("triggerLevelEvent", "timedilation"); });

OPTION_DECLARE(bool, s_noBgm, false);
OPTION_DEFINE(bool, s_noBgm, "Sound/No BGM");
OPTION_ALIAS(s_noBgm, "nobgm");

OPTION_EXTERN(int, g_playerCharacterIndex);
OPTION_EXTERN(bool, g_noSound);

#if ENABLE_STEAM
OPTION_DECLARE(int, STEAM_AUTHENTICATION_PORT, 22768);
OPTION_DECLARE(int, STEAM_SERVER_PORT, 22769);
OPTION_DECLARE(int, STEAM_MASTER_SERVER_UPDATER_PORT, 22770);
OPTION_DECLARE(std::string, STEAM_SERVER_VERSION, "0.100");
OPTION_DEFINE(int, STEAM_AUTHENTICATION_PORT, "Steam/Authentication Port");
OPTION_DEFINE(int, STEAM_SERVER_PORT, "Steam/Server Port");
OPTION_DEFINE(int, STEAM_MASTER_SERVER_UPDATER_PORT, "Steam/Master Service Updater Port");
OPTION_DEFINE(std::string, STEAM_SERVER_VERSION, "Steam/Server Version");
#endif

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
int g_keyboardLockRelease = 0;

PlayerInputState * g_uiInput = 0;

Surface * g_colorMap = 0;
Surface * g_decalMap = 0;
Surface * g_lightMap = 0;
Surface * g_finalMap = 0;

//

static std::vector<std::string> parseMapList(const std::string & list);

//

static void HandleAction(const std::string & action, const Dictionary & args)
{
	if (action == "connect")
	{
		const std::string address = args.getString("address", "");
		const uint64_t userData = args.getInt64("userdata", 0);

		if (!address.empty())
		{
		#if ENABLE_STEAM
			if (USE_STEAMAPI)
			{
				NetAddress netAddress;
				netAddress.SetFromString(address.c_str(), NET_PORT);
				netAddress.m_userData = userData;
				g_app->connect(netAddress);
			}
			else
		#endif
			{
				g_connect = address;
				g_connectLocal = false;

				Verify(g_app->findGame());
			}
		}
	}

	if (action == "disconnect")
	{
		const int index = args.getInt("client", -1);

		if (index >= 0 && index < (int)g_app->m_clients.size())
		{
			g_app->m_clients[index]->m_channel->Disconnect();
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

static void HandleFillCachesCallback(float filePercentage)
{
	framework.process();

	framework.beginDraw(0, 0, 0, 0);
	{
		setMainFont();
		setColor(colorWhite);
		drawText(GFX_SX/2, GFX_SY/2, 24, 0.f, 0.f, "Loading %0.2f%%", filePercentage * 100.f);
	}
	framework.endDraw();
}

static void HandleFillCachesUnknownResourceCallback(const char * filename)
{
	if (strstr(filename, ".pfx"))
	{
		// todo : load particle effect

		logDebug("particle effect: %s", filename);
	}
}

static void HandleInitError(INIT_ERROR error)
{
	switch (error)
	{
	case INIT_ERROR_SDL:
		showErrorMessage("Startup Error", "Failed to initialize SDL. Error: %08x", (int)error);
		break;
	case INIT_ERROR_VIDEO_MODE:
		showErrorMessage("Startup Error", "Failed to find a suitable video mode. Error: %08x", (int)error);
		break;
	case INIT_ERROR_WINDOW:
		showErrorMessage("Startup Error", "Failed to create the window. Error: %08x", (int)error);
		break;
	case INIT_ERROR_OPENGL:
		showErrorMessage("Startup Error", "Failed to initialize OpenGL. Error: %08x", (int)error);
		break;
	case INIT_ERROR_OPENGL_EXTENSIONS:
		showErrorMessage("Startup Error", "Failed to initialize OpenGL extensions. Error: %08x", (int)error);
		break;
	case INIT_ERROR_SOUND:
		showErrorMessage("Startup Error", "Failed to initialize sound. Error: %08x", (int)error);
		break;
	case INIT_ERROR_FREETYPE:
		showErrorMessage("Startup Error", "Failed to initialize FreeType font library. Error: %08x", (int)error);
		break;

	default:
		showErrorMessage("Startup Error", "Failed to initialize the system. Error: %08x", (int)error);
		break;
	}
}

//

void inputLockAcquire()
{
	g_keyboardLock++;
}

void inputLockRelease()
{
	g_keyboardLockRelease++;
	Assert(g_keyboardLockRelease <= g_keyboardLock);
}

//

void applyLightMap(Surface & colormap, Surface & lightmap, Surface & dest)
{
	gpuTimingBlock(applyLightMap);
	cpuTimingBlock(applyLightMap);

	// apply lightmap

	setBlend(BLEND_OPAQUE);
	pushSurface(&dest);
	{
		Shader lightShader("lightmap");
		setShader(lightShader);

		lightShader.setTexture("colormap", 0, colormap.getTexture(), false, true);
		lightShader.setTexture("lightmap", 1, lightmap.getTexture(), false, true);

		drawRect(0, 0, colormap.getWidth(), colormap.getHeight());

		clearShader();
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
#if ENABLE_NETWORKING_DEBUGS
	s_rpcDebugAction,
#endif
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
	cpuTimingBlock(appHandleRpc);

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
						do
						{
							gameSim->m_desiredGameMode = (GameMode)((gameSim->m_desiredGameMode + kGameMode_COUNT + delta) % kGameMode_COUNT);
						} while (gameSim->m_desiredGameMode == kGameMode_Lobby);
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

			LOG_DBG("handleRpc: s_rpcAddPlayerBroadcast: channelId=%d, index=%d, characterIndex=%d, controllerIndex=%d", channelId, index, characterIndex, controllerIndex);

			//

			PlayerInstanceData * playerInstanceData = gameSim->allocPlayer(channelId);
			Assert(playerInstanceData);

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
					//if (!client->m_isDesync)
					//	client->m_gameSim->playSound("desync.ogg");
					client->m_isDesync = true;
				}
				else
				{
					client->m_isDesync = false;
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

			LOG_DBG("handleRpc: s_rpcSetPlayerCharacterIndexBroadcast: playerId=%d, characterIndex=%d", playerId, characterIndex);

			PlayerInstanceData * playerInstanceData = gameSim->m_playerInstanceDatas[playerId];
			Assert(playerInstanceData);
			if (playerInstanceData)
			{
				playerInstanceData->setCharacterIndex(characterIndex);
			}
		}
	}
#if ENABLE_NETWORKING_DEBUGS
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

			if (gameSim && gameSim->m_gameState >= kGameState_RoundBegin)
				gameSim->newRound(0);
		}
		else if (action == "endRound")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim && gameSim->m_gameState >= kGameState_RoundBegin)
				gameSim->endRound();
		}
		else if (action == "goToLobby")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim && gameSim->m_gameState >= kGameState_RoundBegin)
				gameSim->setGameState(kGameState_OnlineMenus);
		}
		else if (action == "loadMap")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim && gameSim->m_gameState >= kGameState_RoundBegin)
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
				gameSim->addAnnouncement(colorBlack, param.c_str());
			}
		}
		else if (action == "addLightEffect")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
			{
				if (param == "darken")
					gameSim->addLightEffect(LightEffect::kType_Darken, gameSim->RandomFloat(1.f, 4.f), gameSim->RandomFloat(0.f, .5f));
				if (param == "lighten")
					gameSim->addLightEffect(LightEffect::kType_Lighten, gameSim->RandomFloat(1.f, 4.f), gameSim->RandomFloat(.8f, 1.f));
			}
		}
		else if (action == "addBlastEffect")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
			{
				Dictionary args;
				args.parse(param);
				const float speed = args.getFloat("speed", 0.f);
				const Curve curve(speed, 0.f);

				gameSim->doBlastEffect(
					Vec2(args.getFloat("x", GFX_SX/2.f), args.getFloat("y", GFX_SY/2.f)),
					args.getFloat("radius", 100.f),
					curve);
			}
		}
		else if (action == "triggerLevelEvent")
		{
			GameSim * gameSim = findGameSimForChannel(channel);
			Assert(gameSim);

			if (gameSim)
			{
				LevelEvent e = kLevelEvent_COUNT;

				if (param == "quake")
					e = kLevelEvent_EarthQuake;
				else if (param == "gravity")
					e = kLevelEvent_GravityWell;
				else if (param == "spikewalls")
					e = kLevelEvent_SpikeWalls;
				else if (param == "timedilation")
					e = kLevelEvent_TimeDilation;

				Assert(e != kLevelEvent_COUNT);
				if (e != kLevelEvent_COUNT)
				{
					gameSim->triggerLevelEvent(e);
				}
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
#endif
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

	Assert(m_hostClients.find(channel->m_id) == m_hostClients.end());
	m_hostClients[channel->m_id] = clientInfo;

	netSyncGameSim(channel);
}

void App::SV_OnChannelDisconnect(Channel * channel)
{
	Assert(g_host);
	if (!g_host)
	{
		Assert(m_hostClients.empty());
	}
	else
	{
		// todo : remove from m_playersToAdd

		auto i = m_hostClients.find(channel->m_id);

		Assert(i != m_hostClients.end());
		if (i != m_hostClients.end())
		{
			ClientInfo & clientInfo = i->second;

			// remove player created for this channel

			bool removedAny = false;
			for (int playerIndex = 0; playerIndex < MAX_PLAYERS; ++playerIndex)
			{
				const Player & player = g_host->m_gameSim.m_players[playerIndex];
				if (player.m_owningChannelId == channel->m_destinationId)
				{
					g_app->netRemovePlayerBroadcast(playerIndex);
					removedAny = true;
				}
			}
			Assert(removedAny);

			m_hostClients.erase(i);
		}
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

	Assert(client);
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

void App::OnOnlineLobbyCreateResult(OnlineRequestId requestId, bool success)
{
	LOG_DBG("OnOnlineLobbyCreateResult: requestId=%d, result=%d", requestId, success);

	m_isMatchmaking = false;
	m_matchmakingResult = success;

	g_online->lobbyCreateEnd(requestId);
}

void App::OnOnlineLobbyJoinResult(OnlineRequestId requestId, bool success)
{
	LOG_DBG("OnOnlineLobbyJoinResult: requestId=%d, result=%d", requestId, success);

	m_isMatchmaking = false;
	m_matchmakingResult = success;

	g_online->lobbyFindEnd(requestId);
}

void App::OnOnlineLobbyLeaveResult(OnlineRequestId requestId, bool success)
{
	LOG_DBG("OnOnlineLobbyLeaveResult: requestId=%d, result=%d", requestId, success);

	m_isMatchmaking = false;
	m_matchmakingResult = success;

	g_online->lobbyLeaveEnd(requestId);
}

void App::OnOnlineLobbyMemberJoined(OnlineLobbyMemberId memberId)
{
	LOG_DBG("OnOnlineLobbyMemberJoined: memberId=%llx", memberId);
}

void App::OnOnlineLobbyMemberLeft(OnlineLobbyMemberId memberId)
{
	LOG_DBG("OnOnlineLobbyMemberLeft: memberId=%llx", memberId);
}

//

App::App()
	: m_appState(AppState_Offline)
	, m_netState(NetState_Offline)
	, m_isHost(false)
	, m_isMatchmaking(false)
	, m_matchmakingResult(false)
	, m_host(0)
	, m_packetDispatcher(0)
	, m_channelMgr(0)
	, m_rpcMgr(0)
#if ENABLE_NETWORKING_DISCOVERY
	, m_discoveryService(0)
#endif
#if ENABLE_NETWORKING
	, m_discoveryUi(0)
#endif
	, m_selectedClient(-1)
	, m_dialogMgr(0)
	, m_menuMgr(0)
	, m_userSettings(0)
#if ENABLE_OPTIONS
	, m_optionMenu(0)
	, m_optionMenuIsOpen(false)
#endif
#if ENABLE_OPTIONS
	, m_statTimerMenu(0)
	, m_statTimerMenuIsOpen(false)
#endif
{
}

App::~App()
{
	Assert(m_isHost == false);

	Assert(m_packetDispatcher == 0);
	Assert(m_channelMgr == 0);
	Assert(m_rpcMgr == 0);

#if ENABLE_NETWORKING_DISCOVERY
	Assert(m_discoveryService == 0);
#endif
#if ENABLE_NETWORKING
	Assert(m_discoveryUi == 0);
#endif

	Assert(m_host == 0);

	Assert(m_clients.empty());
}

bool App::init()
{
	Calc::Initialize();

	// todo : add ability to compare loaded options vs default and error on difference

	g_optionManager.Load("options.txt");
	g_optionManager.Load("gameoptions.txt");

#if PUBLIC_DEMO_BUILD
	DEMOMODE = true;
#endif

#if ENABLE_DEVMODE
	if (g_devMode)
	{
		LIBNET_CHANNEL_ENABLE_TIMEOUTS = false;

		GAMESTATE_ROUNDBEGIN_TRANSITION_TIME = 1.f / 60.f;
		GAMESTATE_ROUNDBEGIN_SPAWN_DELAY = 1.f / 60.f;
		GAMESTATE_ROUNDBEGIN_MESSAGE_DELAY = 1.f / 60.f;
		GAMESTATE_ROUNDCOMPLETE_SHOWWINNER_TIME = 1.f / 60.f;
		GAMESTATE_ROUNDCOMPLETE_SHOWRESULTS_TIME = 1.f / 60.f;
		GAMESTATE_ROUNDCOMPLETE_TRANSITION_TIME = 1.f / 60.f;
	}
#endif

	if (DEMOMODE)
	{
		UI_DEBUG_VISIBLE = false;
		UI_LOBBY_LEAVEJOIN_ENABLE = true;
		UI_LOBBY_GAMEMODE_SELECT_ENABLE = false;
		UI_TEXTCHAT_ENABLE = false;
		NUM_LOCAL_PLAYERS_TO_ADD = 1;
		VOLCANO_LOOP = true;
		GAMESTATE_LOBBY_STARTZONE_ENABLED = false;

	#if !PUBLIC_DEMO_BUILD
		LIBNET_CHANNEL_ENABLE_TIMEOUTS = false;
		PLAYER_INACTIVITY_KICK = true;
		GAMESTATE_ROUNDBEGIN_SHOW_CONTROLS = true;
	#endif

		//

		LIGHTING_DEBUG_MODE = 3;
		GAME_SPEED_MULTIPLIER = 1.15f;
		MAX_CONSECUTIVE_ROUND_COUNT = 2;
	}

#if ENABLE_DEVMODE
	if (RECORDMODE)
	{
		UI_DEBUG_VISIBLE = false;
		LIBNET_CHANNEL_ENABLE_TIMEOUTS = false;
		VOLCANO_LOOP = true;
	}
#endif

	g_mapList = parseMapList(s_mapList);
	g_mapRotationList = parseMapList(s_mapRotationList);

	for (size_t i = 0; i < g_mapList.size(); ++i)
	{
		const std::string & file = g_mapList[i];

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

#if ENABLE_DEVMODE
	if (g_devMode)
	{
		framework.minification = 2;
		framework.fullscreen = false;
		//framework.reloadCachesOnActivate = true;
		framework.cacheResourceData = true;
	}
	else
#endif
	if (g_windowed)
	{
		framework.minification = 2;
		framework.fullscreen = false;
	}
	else
	{
		framework.fullscreen = true;
	}

	if (g_scale >= 1)
	{
		framework.minification = g_scale;
	}

	framework.exclusiveFullscreen = false;
	framework.useClosestDisplayMode = true;
#if PUBLIC_DEMO_BUILD
	framework.windowTitle = "Riposte (Public Demo Build)";
#else
	framework.windowTitle = "Riposte";
#endif
	framework.windowIcon = "icon.png";
	framework.actionHandler = HandleAction;
	framework.fillCachesCallback = HandleFillCachesCallback;
	framework.fillCachesUnknownResourceCallback = HandleFillCachesUnknownResourceCallback;
	framework.initErrorHandler = HandleInitError;

	if (!framework.init(0, 0, GFX_SX, GFX_SY))
	{
		return false;
	}
	else
	{
	#if ENABLE_STEAM
		if (USE_STEAMAPI)
		{
			if (!FileStream::Exists("steam_appid.txt"))
			{
				FileStream stream("steam_appid.txt", (OpenMode)(OpenMode_Write | OpenMode_Text));
				StreamWriter writer(&stream, false);
				writer.WriteText("420620");
			}

			if (!SteamAPI_Init())
			{
				showErrorMessage("Startup Error", "Failed to initialize the Steam API. Is Steam running?");
				return false;
			}

			m_steamPersonaStateChangeCallback.Register(this, &App::OnSteamPersonaStateChange);
			m_steamAvatarImageLoadedCallback.Register(this, &App::OnSteamAvatarImageLoaded);
			m_steamGameLobbyJoinRequestedCallback.Register(this, &App::OnSteamGameLobbyJoinRequested);
			m_steamP2PSessionRequestCallback.Register(this, &App::OnSteamP2PSessionRequest);

			g_online = new OnlineSteam(this);
		}
		else
	#endif
		{
			//g_online = new OnlineLAN(this);
			g_online = new OnlineLocal(this);
		}

		SDL_ShowCursor(0);

		if (!g_devMode && g_precacheResources)
		{
			for (int i = 0; i < 10; ++i)
				HandleFillCachesCallback(0.f);

			framework.fillCachesWithPath(".", true);
		}

		//Spriter("char0/sprite/sprite.scml").getSpriterScene()->saveBinary("test.sbin");
		//spriter::Scene scene;
		//scene.loadBinary("test.sbin");

		// input the user's display name

		if (ITCHIO_BUILD)
		{
			m_displayName = "Player 1";
		}
		else if (DEMOMODE)
		{
			m_displayName = "Riposte";
		}
	#if ENABLE_STEAM
		else if (USE_STEAMAPI)
		{
			m_displayName = SteamFriends()->GetPersonaName();
		}
	#endif
		else if (g_devMode)
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
				setMainFont();
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

	#if ENABLE_NETWORKING_DISCOVERY
		m_discoveryService = new NetSessionDiscovery::Service();
		m_discoveryService->init(2, 10);
	#endif
	#if ENABLE_NETWORKING
		m_discoveryUi = new Ui();
	#endif

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

		g_uiInput = new PlayerInputState();

		m_dialogMgr = new DialogMgr();

		m_menuMgr = new MenuMgr();

		m_userSettings = new UserSettings();

		//

	#if ENABLE_DEVMODE
		animationTestInit();
	#endif

		//

	#if ENABLE_OPTIONS
		m_optionMenu = new OptionMenu();
		m_optionMenuIsOpen = false;
	#endif
	#if ENABLE_OPTIONS
		m_statTimerMenu = new StatTimerMenu();
		m_statTimerMenuIsOpen = false;
	#endif

		//

		g_colorMap = new Surface(GFX_SX, GFX_SY);
		g_decalMap = new Surface(MAX_ARENA_SX_PIXELS, MAX_ARENA_SY_PIXELS);
		g_lightMap = new Surface(GFX_SX, GFX_SY);
		g_finalMap = new Surface(GFX_SX, GFX_SY);

		//

		g_tileTransition = new TileTransition();

		//

		initArenaData();

		initCharacterData();

		loadUserSettings();

		setLocal(m_userSettings->language.locale.c_str());

		m_menuMgr->push(new MainMenu());

	#if ITCHIO_BUILD
		if (!g_devMode)
		{
			Menu * splash1 = new SplashScreen("title/itchio1.png", 5.f, .5f, .3f);
			Menu * splash2 = new SplashScreen("title/itchio2.png", 5.f, .5f, .3f);
			splash1->m_menuId = kMenuId_IntroScreen;
			splash2->m_menuId = kMenuId_IntroScreen;
			m_menuMgr->push(splash2);
			m_menuMgr->push(splash1);
		}
	#else
		m_menuMgr->push(new Title());
	#endif

		return true;
	}
}

void App::shutdown()
{
	stopHosting();

	//

	shutArenaData();

	shutCharacterData();

	//

	delete g_tileTransition;
	g_tileTransition = 0;

	//

	delete g_colorMap;
	g_colorMap = 0;

	delete g_decalMap;
	g_decalMap = 0;

	delete g_lightMap;
	g_lightMap = 0;

	delete g_finalMap;
	g_finalMap = 0;

	//

#if ENABLE_OPTIONS
	delete m_statTimerMenu;
	m_statTimerMenu = 0;
#endif

#if ENABLE_OPTIONS
	delete m_optionMenu;
	m_optionMenu = 0;
#endif

	//

	delete m_userSettings;
	m_userSettings = 0;

	delete m_menuMgr;
	m_menuMgr = 0;

	delete m_dialogMgr;
	m_dialogMgr = 0;

	delete g_uiInput;
	g_uiInput = 0;

	//

	m_freeControllerList.clear();

	//

#if ENABLE_NETWORKING
	delete m_discoveryUi;
	m_discoveryUi = 0;
#endif

#if ENABLE_NETWORKING_DISCOVERY
	delete m_discoveryService;
	m_discoveryService = 0;
#endif

	delete m_rpcMgr;
	m_rpcMgr = 0;

	delete m_channelMgr;
	m_channelMgr = 0;

	delete m_packetDispatcher;
	m_packetDispatcher = 0;

	delete g_online;
	g_online = 0;

#if ENABLE_STEAM
	if (USE_STEAMAPI)
	{
		m_steamPersonaStateChangeCallback.Unregister();
		m_steamAvatarImageLoadedCallback.Unregister();
		m_steamGameLobbyJoinRequestedCallback.Unregister();
		m_steamP2PSessionRequestCallback.Unregister();

		SteamAPI_Shutdown();
	}
#endif

	framework.shutdown();
}

static const char * s_appStates[2] =
{
	"AppState_Offline",
	"AppState_Online",
};

void App::setAppState(AppState state)
{
	Assert(state < sizeof(s_appStates) / sizeof(s_appStates[0]));

	if (state == m_appState)
		return;

	LOG_DBG("setAppState: %s -> %s", s_appStates[m_appState], s_appStates[state]);

	switch (m_appState)
	{
	case AppState_Offline:
		break;
	case AppState_Online:
		break;
	}

	m_appState = state;

	switch (m_appState)
	{
	case AppState_Offline:
		Assert(m_clients.empty());
		m_menuMgr->reset(0);
		m_menuMgr->push(new MainMenu());
	#if !ITCHIO_BUILD
		m_menuMgr->push(new Title());
	#endif
		break;
	case AppState_Online:
		m_menuMgr->reset(0);
		break;
	}
}

static const char * s_netStates[7] =
{
	"NetState_Offline",
	"NetState_HostCreate",
	"NetState_HostDestroy",
	"NetState_LobbyCreate",
	"NetState_LobbyJoin",
	"NetState_LobbyLeave",
	"NetState_Online",
};

void App::setNetState(NetState state)
{
	Assert(state < sizeof(s_netStates) / sizeof(s_netStates[0]));

	if (state == m_netState)
		return;

	LOG_DBG("setNetState: %s -> %s", s_netStates[m_netState], s_netStates[state]);

	m_netState = state;
}

void App::quit()
{
	exit(0);
}

#include <Shlobj.h> // todo : remove once proper Steam integration?

std::string App::getUserSettingsDirectory()
{
#if 1
	// todo : change this once we have Steam integration

	//FOLDERID_SavedGames // Vista+
	//FOLDERID_LocalAppData

	char path[MAX_PATH];
	if (SHGetSpecialFolderPathA(NULL, path, CSIDL_LOCAL_APPDATA, TRUE))
	//if (SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, NULL, &path) == S_OK)
	{
		std::string result = std::string(path) + "\\Damajo Games\\Riposte";
		//CoTaskMemFree(path);
		return result;
	}
	else
	{
		logWarning("failed to get local appdata path");

		return "..";
	}
#else
	return "..";
#endif
}

std::string App::getUserSettingsFilename()
{
#if 1
	return getUserSettingsDirectory() + "\\settings.txt";
#else
	char machineName[256];
	DWORD machineNameSize = sizeof(machineName);
	if (!GetComputerName(machineName, &machineNameSize))
		strcpy(machineName, "noname");
	return getUserSettingsDirectory() + "\\settings-" + machineName + ".txt";
#endif
}

void App::saveUserSettings()
{
	try
	{
		auto directory = getUserSettingsDirectory();
		auto filename = getUserSettingsFilename();

		int result = SHCreateDirectoryEx(NULL, directory.c_str(), NULL);
		fassert(result == S_OK || result == ERROR_FILE_EXISTS || result == ERROR_ALREADY_EXISTS);

		FileStream stream;
		stream.Open(filename.c_str(), OpenMode_Write);
		StreamWriter writer(&stream, false);
		m_userSettings->save(writer);
	}
	catch (std::exception & e)
	{
		logWarning(e.what());
	}
}

void App::loadUserSettings()
{
	try
	{
		FileStream stream;
		stream.Open(getUserSettingsFilename().c_str(), OpenMode_Read);
		StreamReader reader(&stream, false);
		m_userSettings->load(reader);
	}
	catch (std::exception & e)
	{
		logWarning(e.what());
	}
}

void App::startHosting()
{
	Assert(m_netState == NetState_Offline);
	Assert(m_host == 0);

	setNetState(NetState_HostCreate);
}

void App::stopHosting()
{
	Assert(m_netState == NetState_Online);
	Assert(m_host != 0);

	g_online->lobbyLeaveBegin();
	m_isMatchmaking = true;
	setNetState(NetState_LobbyLeave);
}

bool App::pollMatchmaking(bool & isDone, bool & success)
{
	if (m_isMatchmaking)
	{
		isDone = false;
		success = false;
	}
	else
	{
		isDone = true;
		success = m_matchmakingResult;
	}

	return true;
}

bool App::findGame()
{
	if (g_online)
	{
		g_online->lobbyFindBegin();
		m_isMatchmaking = true;
		setNetState(NetState_LobbyJoin);
		return true;
	}
	else
	{
		NetSocketUDP * socketUDP = new NetSocketUDP();
		SharedNetSocket socket(socketUDP);

		if (!m_channelMgr->Initialize(m_packetDispatcher, this, socket, false, g_buildId))
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
}

bool App::joinGame(uint64_t gameId)
{
	if (g_online)
	{
		g_online->lobbyJoinBegin(gameId);
		m_isMatchmaking = true;
		setNetState(NetState_LobbyJoin);
		return true;
	}
	else
	{
		return false;
	}
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

	if (m_clients.empty())
	{
		setAppState(AppState_Offline);
	}
}

Client * App::connect(const char * address)
{
	const NetAddress netAddress(address, NET_PORT);

	return connect(netAddress);
}

Client * App::connect(const NetAddress & address)
{
	LOG_DBG("connect: %s (%llx)", address.ToString(true).c_str(), address.m_userData);

	Channel * channel = m_channelMgr->CreateChannel(ChannelPool_Client);

	const auto serverVersion = m_channelMgr->m_serverVersion;
	if (g_fakeIncompatibleServerVersion)
		m_channelMgr->m_serverVersion = -1;

	Verify(channel->Connect(address));

	m_channelMgr->m_serverVersion = serverVersion;

	LOG_DBG("connect: %s (%llx) [done]", address.ToString(true).c_str(), address.m_userData);

	LOG_DBG("connect: creating client");

	Client * client = new Client;

	client->initialize(channel);

	m_clients.push_back(client);

	LOG_DBG("connect: creating client [done] client=%p", client);

	//

	m_selectedClient = (int)m_clients.size() - 1;

	LOG_DBG("connect: updated selected client to %d", m_selectedClient);

	//

	setAppState(AppState_Online);

	return client;
}

void App::destroyClient(int index)
{
	if (index >= 0 && index < (int)m_clients.size())
	{
		Client * client = m_clients[index];

		LOG_DBG("destroyClient: index=%d, client=%p", index, client);

		Assert(client->m_channel->m_state == ChannelState_Disconnected);

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

void App::debugSyncGameSims()
{
	for (int i = 0; i < 10; ++i)
		m_channelMgr->Update(NET_TIME);
}

bool App::tick()
{
	TIMER_SCOPE(g_appTickTime);
	cpuTimingBlock(appTick);

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

#if ENABLE_STEAM
	// steam update

	if (USE_STEAMAPI)
	{
		SteamAPI_RunCallbacks();

		if (m_isHost)
			SteamGameServer_RunCallbacks();
	}
#endif

	if (g_online)
	{
		g_online->tick();

	#if ENABLE_NETWORKING_DEBUGS
		// fixme : remove !

		if (keyboard.wentDown(SDLK_i))
			g_online->showInviteFriendsUi();
		//if (keyboard.wentDown(SDLK_j))
		//	Verify(findGame());
	#if ENABLE_STEAM
		if (keyboard.wentDown(SDLK_p))
		{
			EXCEPTION_POINTERS ep;
			
			CONTEXT c;
			EXCEPTION_RECORD r;
			memset(&c, 0, sizeof(c));
			memset(&r, 0, sizeof(r));
			ep.ContextRecord = &c;
			ep.ExceptionRecord = &r;
			SteamAPI_WriteMiniDump(0, &ep, g_buildId);
		}
	#endif
	#endif
	}

	// mouse cursor

	static Mouse lastMouse;
	static float mouseInactivityTime = 0.f;
	if (!memcmp(&mouse, &lastMouse, sizeof(Mouse)))
		mouseInactivityTime += dt;
	else
	{
		mouseInactivityTime =  0.f;
		lastMouse = mouse;
	}

	{
		cpuTimingBlock(cursorUpdate);
		bool showCursor = true;
		if (mouseInactivityTime >= 3.f)
			showCursor = false;
		if (m_menuMgr->getActiveMenuId() == kMenuId_IntroScreen)
			showCursor = false;
		SDL_ShowCursor(showCursor);
	}
	
	// UI input

	{
		cpuTimingBlock(uiInput);
		g_uiInput->next(false, dt);
		g_uiInput->m_currState.gather(true, 0, false);
	}

	// debug UI

#if ENABLE_NETWORKING_DISCOVERY
	m_discoveryService->update(m_isHost);
#endif
#if ENABLE_NETWORKING	
	m_discoveryUi->process();
#endif

	// update dialogs

	m_dialogMgr->tick(dt);

	// update menus

	m_menuMgr->tick(dt);

	// update networking

	tickNet();

	// update BGM

	tickBgm();

	// update host

	if (m_netState == NetState_Online && m_isHost && m_clients.empty())
	{
		// todo : only do this if client is the host

		stopHosting();
	}

	if (m_netState == NetState_Online && m_isHost)
	{
		cpuTimingBlock(hostTick);

		m_channelMgr->Update(NET_TIME);

	#if ENABLE_OPTIONS
		if (!m_optionMenuIsOpen || !g_pauseOnOptionMenuOption)
	#endif
		{
			if (g_updateTicks)
			{
			#if ENABLE_GAMESTATE_CRC_LOGGING
				if (g_logCRCs)
					debugSyncGameSims();
				const uint32_t crc1 = g_logCRCs ? m_host->m_gameSim.calcCRC() : 0;
			#endif

				broadcastPlayerInputs();

			#if ENABLE_GAMESTATE_CRC_LOGGING
				const uint32_t crc2 = g_logCRCs ? m_host->m_gameSim.calcCRC() : 0;

				if (g_logCRCs)
				{
					debugSyncGameSims();
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

	if (UI_LOBBY_LEAVEJOIN_ENABLE && g_host && g_host->m_gameSim.m_gameState == kGameState_OnlineMenus && !m_clients.empty())
	{
		Client * client = m_clients.front();

		for (int i = 0; i < MAX_GAMEPAD + 1; ++i)
		{
			if ((i < MAX_GAMEPAD && gamepad[i].wentDown(GAMEPAD_START)) || (i == MAX_GAMEPAD && keyboard.wentDown(SDLK_d)))
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
					sprintf_s(name, sizeof(name), "Player %d", (i == MAX_GAMEPAD) ? i : (i + 1));
					g_app->netAddPlayer(client->m_channel, g_validCharacterIndices[0], name, i);
				}

				debugSyncGameSims();
			}
		}
	}

#if ITCHIO_BUILD
	// itch.io controls

	if (keyboard.wentDown(SDLK_F10))
		netDebugAction("newRound", "");
	if (keyboard.wentDown(SDLK_END))
		netDebugAction("goToLobby", "");
#endif

	// debug

#if ENABLE_DEVMODE // debug controls for animation and blast test, etc
	if (keyboard.wentDown(SDLK_F2))
	{
		std::vector<Client*> clients = m_clients;

		for (size_t i = 0; i < clients.size(); ++i)
			leaveGame(clients[i]);
	}

	if (keyboard.wentDown(SDLK_F3) && !(getSelectedClient() && getSelectedClient()->m_textChat->isActive()))
		animationTestToggleIsActive();
	if (keyboard.wentDown(SDLK_F4) && !(getSelectedClient() && getSelectedClient()->m_textChat->isActive()))
		blastEffectTestToggleIsActive();
	if (keyboard.wentDown(SDLK_F10) && !(getSelectedClient() && getSelectedClient()->m_textChat->isActive()))
		particleEditorToggleIsActive();
	if (keyboard.wentDown(SDLK_F11) && !(getSelectedClient() && getSelectedClient()->m_textChat->isActive()))
		gifCaptureToggleIsActive(true);

#if ENABLE_OPTIONS
	if (keyboard.wentDown(SDLK_F5))
	{
		m_optionMenuIsOpen = !m_optionMenuIsOpen;
		m_statTimerMenuIsOpen = false;
	}
#endif

#if ENABLE_OPTIONS
	if (keyboard.wentDown(SDLK_F6))
	{
		m_optionMenuIsOpen = false;
		m_statTimerMenuIsOpen = !m_statTimerMenuIsOpen;
	}
#endif

#if ENABLE_DEVMODE
	if (keyboard.wentDown(SDLK_END))
	{
		netDebugAction("newRound", "");
	}
#endif

#if ENABLE_OPTIONS
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
#endif

	if (g_devMode && m_netState == NetState_Online && m_isHost && g_keyboardLock == 0 && keyboard.wentDown(SDLK_t))
	{
		netDebugAction("loadOptions", "options.txt");
	}

	if (g_devMode && keyboard.wentDown(SDLK_y))
		clearCaches(CACHE_SPRITE | CACHE_SPRITER | CACHE_TEXTURE);
	if (g_devMode && keyboard.wentDown(SDLK_u))
		clearCaches(CACHE_SOUND);
	if (g_devMode && keyboard.wentDown(SDLK_i))
		clearCaches(CACHE_SHADER);

	animationTestTick(dt);

	blastEffectTestTick(dt);

	particleEditorTick(dt);

	gifCaptureTick(dt);
#endif

	if ((g_keyboardLock == 0 && (keyboard.wentDown(SDLK_ESCAPE) || keyboard.wentDown(SDLK_q))) || framework.quitRequested)
	{
		m_dialogMgr->push(DialogType_YesNo, "Do you really want to quit?", "", DialogQuit, this);
	}

	Assert(g_keyboardLockRelease <= g_keyboardLock);
	g_keyboardLock -= g_keyboardLockRelease;
	g_keyboardLockRelease = 0;

#if GG_ENABLE_TIMERS
	TIMER_STOP(g_appTickTime);
	g_statTimerManager.Update();
	TIMER_START(g_appTickTime);
#endif

	return true;
}

void App::tickNet()
{
	switch (m_netState)
	{
	case NetState_Offline:
		break;

	case NetState_HostCreate:
		{
			Assert(!m_isHost);
			Assert(!m_host);
			Assert(!m_channelMgr->IsInitialized());

			SharedNetSocket socket;

		#if ENABLE_STEAM
			if (USE_STEAMAPI)
			{
				NetSocketSteam * socketSteam = new NetSocketSteam();
				socket = SharedNetSocket(socketSteam);
			}
			else
		#endif
			if (true)
			{
				socket = new NetSocketLocal();
			}
			else
			{
				NetSocketUDP * socketUDP = new NetSocketUDP();
				socket = SharedNetSocket(socketUDP);
				if (!socketUDP->Bind(NET_PORT))
				{
					setNetState(NetState_Offline);
					break;
				}
			}

			if (!m_channelMgr->Initialize(m_packetDispatcher, this, socket, true, g_buildId))
			{
				setNetState(NetState_Offline);
				break;
			}

		#if ENABLE_STEAM
			if (USE_STEAMAPI)
			{
				if (!SteamGameServer_Init(INADDR_ANY, STEAM_AUTHENTICATION_PORT, STEAM_SERVER_PORT, STEAM_MASTER_SERVER_UPDATER_PORT, eServerModeAuthenticationAndSecure, ((std::string)STEAM_SERVER_VERSION).c_str()))
				{
					m_channelMgr->Shutdown(false);
					setNetState(NetState_Offline);
					break;
				}
			}
		#endif

			//

			LOG_DBG("creating host");

			m_host = new Host();

			m_host->init();

			LOG_DBG("creating host [done]");

			//

			m_isHost = true;

			//

		#if ENABLE_STEAM
			if (USE_STEAMAPI)
			{
				SteamGameServer()->SetModDir("riposte");
				SteamGameServer()->SetProduct("SteamworksExample");
				SteamGameServer()->SetGameDescription("Steamworks Example");
				SteamGameServer()->LogOnAnonymous();
				SteamGameServer()->EnableHeartbeats(true);

				//

				SteamGameServer()->SetMaxPlayerCount(MAX_PLAYERS);
				SteamGameServer()->SetPasswordProtected(false);
				SteamGameServer()->SetServerName("Vir's Server");
				SteamGameServer()->SetBotPlayerCount(0);
				SteamGameServer()->SetMapName("Highlands");
			}
		#endif

			//

			g_online->lobbyCreateBegin();
			m_isMatchmaking = true;
			setNetState(NetState_LobbyCreate);
			break;
		}
		break;

	case NetState_HostDestroy:
		if (m_isHost)
		{
		#if ENABLE_STEAM
			if (USE_STEAMAPI)
			{
				SteamGameServer()->EnableHeartbeats(false);
				SteamGameServer()->LogOff();
				SteamGameServer_Shutdown();
			}
		#endif

			//

			while (!m_clients.empty())
			{
				destroyClient(0);
			}

			if (m_channelMgr->IsInitialized())
			{
				m_channelMgr->Shutdown(true);
			}

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

		setNetState(NetState_Offline);
		break;

	case NetState_LobbyCreate:
		{
			bool isDone;
			bool success;
			if (pollMatchmaking(isDone, success))
			{
				if (isDone)
				{
					if (success)
					{
						m_host->m_gameSim.setGameState(kGameState_OnlineMenus);

						uint64_t lobbyOwnerAddress;
						if (g_online->getLobbyOwnerAddress(lobbyOwnerAddress))
						{
							NetAddress netAddress;
							netAddress.SetFromString("127.0.0.1", NET_PORT);
							netAddress.m_userData = lobbyOwnerAddress;
							connect(netAddress);

							setNetState(NetState_Online);

						#if ENABLE_STEAM && ENABLE_NETWORKING_DEBUGS
							if (g_testSteamMatchmaking)
							{
								Verify(findGame());
							}
						#endif
						}
						else
						{
							stopHosting();
							break;
						}
					}
					else
					{
						setNetState(NetState_HostDestroy);
						break;
					}
				}
			}
			else
			{
				Assert(false);
				setNetState(NetState_HostDestroy);
				break;
			}
		}
		break;

	case NetState_LobbyJoin:
		{
			bool isDone;
			bool success;
			if (pollMatchmaking(isDone, success))
			{
				if (isDone)
				{
					if (success)
					{
						if (!m_channelMgr->IsInitialized())
						{
						#if ENABLE_STEAM
							SharedNetSocket socket(new NetSocketSteam());
							if (!m_channelMgr->Initialize(m_packetDispatcher, this, socket, false, g_buildId))
							{
								setNetState(NetState_LobbyLeave);
								break;
							}
						#endif
						}

						uint64_t lobbyOwnerAddress;
						if (g_online->getLobbyOwnerAddress(lobbyOwnerAddress))
						{
							NetAddress netAddress;
							netAddress.SetFromString("127.0.0.1", NET_PORT);
							netAddress.m_userData = lobbyOwnerAddress;
							connect(netAddress);

							setNetState(NetState_Online);
							break;
						}
						else
						{
							setNetState(NetState_LobbyLeave);
							break;
						}
					}
					else
					{
						setNetState(NetState_Offline);
						break;
					}
				}
			}
			else
			{
				Assert(false);
				setNetState(NetState_Offline);
				break;
			}
		}
		break;

	case NetState_LobbyLeave:
		{
			bool isDone;
			bool success;
			if (pollMatchmaking(isDone, success))
			{
				if (isDone)
				{
					setNetState(NetState_HostDestroy);
					break;
				}
			}
			else
			{
				Assert(false);
				setNetState(NetState_HostDestroy);
				break;
			}
		}
		break;

	case NetState_Online:
		break;
	}
}

void App::tickBgm()
{
	cpuTimingBlock(bgmTick);

	// background music

	static char bgm[64] = { };
	static Music * bgmSound = 0;

	Client * selectedClient = getSelectedClient();

	char temp[64];
	temp[0] = 0;

	bool loop = true;

	if (s_noBgm || !m_userSettings->audio.musicEnabled)
	{
		if (bgmSound)
		{
			bgmSound->stop();

			delete bgmSound;
			bgmSound = 0;

			memset(bgm, 0, sizeof(bgm));
		}
	}
	else if (selectedClient)
	{
		switch (selectedClient->m_gameSim->m_gameState)
		{
		case kGameState_Initial:
			Assert(false);
		case kGameState_Connecting:
		case kGameState_OnlineMenus:
		case kGameState_NewGame:
			strcpy_s(temp, sizeof(temp), "bgm/bgm-menus.ogg");
			break;
		case kGameState_RoundBegin:
		case kGameState_Play:
			sprintf_s(temp, sizeof(temp), "bgm/bgm-play%02d.ogg", selectedClient->m_gameSim->m_nextRoundNumber % 4);
			break;
		case kGameState_RoundComplete:
			//strcpy_s(temp, sizeof(temp), "bgm/bgm-round-complete.ogg");
			loop = false;
			break;

		default:
			Assert(false);
			break;
		}
	}
	else if (m_menuMgr->getActiveMenuId() == kMenuId_IntroScreen)
	{
		// no bgm
	}
	else
	{
		sprintf_s(temp, sizeof(temp), "bgm/bgm-menus.ogg");
	}

	if (strcmp(temp, bgm) != 0)
	{
		strcpy_s(bgm, sizeof(bgm), temp);

		delete bgmSound;
		bgmSound = 0;

		if (strlen(bgm))
		{
			bgmSound = new Music(bgm);
			bgmSound->play(loop);
		}
	}

	if (bgmSound)
	{
		bgmSound->setVolume(g_app->m_userSettings->audio.musicVolume * 100.f);
	}
}

void App::draw()
{
	TIMER_START(g_appDrawTime);
	gpuTimingBegin(appDraw);
	cpuTimingBegin(appDraw);

	framework.beginDraw(10, 15, 10, 0);
	{
		// draw client

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
				setDebugFont();
				drawText(5, GFX_SY - 25, 20, +1.f, -1.f, "viewing client %d. time dilation %01.2f. state %s. mouse %d, %d", m_selectedClient, timeDilation, g_gameStateNames[client->m_gameSim->m_gameState], mouse.x, mouse.y);
			}
		}

		// draw menus

		m_menuMgr->draw();

		// draw dialogs

		m_dialogMgr->draw();

	#if ITCHIO_SHOW_BANNER
		if (m_menuMgr->getActiveMenuId() != kMenuId_IntroScreen)
		{
			// draw itch.io badge
			setColor(colorWhite);
			Sprite itchBadge("itch-badge.png");
			itchBadge.drawEx(1510, 1010);
		}
	#endif

		{
			gpuTimingBlock(tileTransitionDraw);
			cpuTimingBlock(tileTransitionDraw);

			g_tileTransition->tick(framework.timeStep); // todo : move to tick
			g_tileTransition->draw();
		}

		// draw debug stuff

		debugDraw();
	}
	cpuTimingEnd();
	gpuTimingEnd();
	TIMER_STOP(g_appDrawTime);
	framework.endDraw();
}

#if ENABLE_STEAM
// todo : move this to somewhere else

struct SteamUserAvatarInfo
{
	GLuint texture;
	int sx;
	int sy;
};

static std::map<CSteamID, SteamUserAvatarInfo> s_textureForSteamUser;
#endif

void App::debugDraw()
{
	if (g_devMode)
	{
		setColor(255, 255, 255);
		setDebugFont();
		drawText(GFX_SX - 5, GFX_SY - 25, 20, -1.f, -1.f, "build %08x", g_buildId);
	}

	if (g_devMode)
	{
		g_online->debugDraw();
	}

	if (g_devMode)
	{
		setColor(255, 255, 255);
		setDebugFont();
		drawText(GFX_SX/2, GFX_SY*4/5, 32, 0.f, 0.f, "NetState: %s", s_netStates[m_netState]);
	}

#if ENABLE_STEAM
	if (g_devMode && USE_STEAMAPI)
	{
		const int numFriends = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);

		int x = 0;

		for (int i = 0; i < numFriends; ++i)
		{
			const CSteamID friendId = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
			if (friendId.IsValid())
			{
				if (s_textureForSteamUser.count(friendId) == 0)
				{
					int avatarId = 0;

					if (avatarId == 0)
						avatarId = SteamFriends()->GetLargeFriendAvatar(friendId);
					if (avatarId < 0)
						continue;

					if (avatarId == 0)
						avatarId = SteamFriends()->GetMediumFriendAvatar(friendId);
					if (avatarId < 0)
						continue;

					if (avatarId != 0)
					{
						uint32_t sx, sy;

						if (SteamUtils()->GetImageSize(avatarId, &sx, &sy))
						{
							const int bufferSize = sx * sy * 4;
							uint8_t * buffer = (uint8_t*)alloca(bufferSize);

							if (SteamUtils()->GetImageRGBA(avatarId, buffer, bufferSize))
							{
								logDebug("got RGBA data for %llx. size=%ux%u", friendId.ConvertToUint64(), sx, sy);

								uint32_t * source = (uint32_t*)buffer;
								void * temp = alloca(sx * 4);

								for (uint32_t y = 0; y < sy/2; ++y)
								{
									const uint32_t y1 = y;
									const uint32_t y2 = sy - 1 - y;
									memcpy(temp,             source + sx * y1, sx * 4);
									memcpy(source + sx * y1, source + sx * y2, sx * 4);
									memcpy(source + sx * y2, temp,             sx * 4);
								}

								auto & info = s_textureForSteamUser[friendId];

								info.texture = createTextureFromRGBA8(buffer, sx, sy, false, true);
								info.sx = sx;
								info.sy = sy;
							}
						}
					}
				}

				auto & info = s_textureForSteamUser[friendId];

				if (info.texture != 0)
				{
					gxSetTexture(info.texture);
					setColor(colorWhite);
					drawRect(x, 0, x + info.sx, info.sy);
					gxSetTexture(0);

					x += info.sx;
				}
			}
		}
	}
#endif

	if (g_devMode && g_host)
	{
		g_host->debugDraw();

		{
			int y = 100;
			setDebugFont();
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
				drawText(0, y += 30, 24, +1, +1, "random seed=%08x, next pickup tick=%02.1f, crc=%08x, px=%g, py=%g, vx=%g, vy=%g",
					m_clients[i]->m_gameSim->m_randomSeed,
					m_clients[i]->m_gameSim->m_nextPickupSpawnTimeRemaining,
				#if ENABLE_GAMESTATE_DESYNC_DETECTION
					m_clients[i]->m_gameSim->calcCRC(),
				#else
					-1,
				#endif
					m_clients[i]->m_gameSim->m_playerInstanceDatas[0] ? m_clients[i]->m_gameSim->m_playerInstanceDatas[0]->m_player->m_pos[0] : 0.f,
					m_clients[i]->m_gameSim->m_playerInstanceDatas[0] ? m_clients[i]->m_gameSim->m_playerInstanceDatas[0]->m_player->m_pos[1] : 0.f,
					m_clients[i]->m_gameSim->m_playerInstanceDatas[0] ? m_clients[i]->m_gameSim->m_playerInstanceDatas[0]->m_player->m_vel[0] : 0.f,
					m_clients[i]->m_gameSim->m_playerInstanceDatas[0] ? m_clients[i]->m_gameSim->m_playerInstanceDatas[0]->m_player->m_vel[1] : 0.f);
			}
		}
	}

#if ENABLE_DEVMODE
	if (/*g_devMode && */keyboard.isDown(SDLK_F1))
	{
		setColor(0, 0, 0, 191);
		drawRect(0, 0, GFX_SX, GFX_SY);

		int x = 50;
		int y = 100;
		int sy = 40;
		int fontSize = 24;
		setColor(colorWhite);
		setDebugFont();
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F1: Show help");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F2: Leave Game");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F3: Toggle Animation Test Tool");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "        Left click = test");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "        Right click = next animation");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "        Shift + right click = previous animation");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F4: Toggle Blast Effect Test Tool");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "        Left click = test");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F5: Toggle Options Menu");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F6: Toggle Statistics Menu");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F9: (reserved)");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F10: Toggle Particle Editor");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F11: GIF Capture Tool");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "F12: New Round");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "T: Reload Options");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "Y: Reload sprites and textures");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "U: Reload sounds");
		drawText(x, y += sy, fontSize, +1.f, +1.f, "I: Reload shaders");
	}
#endif

	// draw desync notifier

	bool isDesync = false;

	for (size_t i = 0; i < m_clients.size(); ++i)
		isDesync |= m_clients[i]->m_isDesync;

	if (isDesync)
	{
		setColor(colorRed);
		drawRect(0, 0, GFX_SX, 40);
		setColor(colorWhite);
		setDebugFont();
		drawText(GFX_SX/2, 12, 30, 0.f, 0.f, "DESYNC");
	}

	//

#if ENABLE_DEVMODE
	animationTestDraw();
	blastEffectTestDraw();
	particleEditorDraw();
	gifCaptureTick_PostRender();
#endif

	//

#if ENABLE_OPTIONS
	if (m_optionMenuIsOpen)
	{
		const int sx = 500;
		const int sy = GFX_SY * 4 / 5;
		const int x = (GFX_SX - sx) / 2;
		const int y = (GFX_SY - sy) / 2;

		m_optionMenu->Draw(x, y, sx, sy);
	}
#endif

#if ENABLE_OPTIONS
	if (m_statTimerMenuIsOpen)
	{
		const int sx = 500;
		const int sy = GFX_SY / 2;
		const int x = (GFX_SX - sx) / 2;
		const int y = (GFX_SY - sy) / 2;

		m_statTimerMenu->Draw(x, y, sx, sy);
	}
#endif

	if (UI_DEBUG_VISIBLE)
	{
	#if ENABLE_NETWORKING
		std::vector<NetSessionDiscovery::ServerInfo> serverList;
	#endif

		if (USE_STEAMAPI)
		{
		#if ENABLE_NETWORKING
			uint64_t lobbyOwnerAddress;
			if (g_online->getLobbyOwnerAddress(lobbyOwnerAddress))
			{
				NetSessionDiscovery::ServerInfo serverInfo;
				serverInfo.m_address.Set(127, 0, 0, 1, NET_PORT);
				serverInfo.m_address.m_userData = lobbyOwnerAddress;
				serverList.push_back(serverInfo);
			}
		#endif
		}
		else
		{
		#if ENABLE_NETWORKING_DISCOVERY
			serverList = m_discoveryService->getServerList();
		#endif
		}

	#if ENABLE_NETWORKING
		m_discoveryUi->clear();
	
		for (size_t i = 0; i < serverList.size(); ++i)
		{
			const auto & serverInfo = serverList[i];

			char name[32];
			sprintf(name, "connect_%d", i);
			Dictionary & button = (*m_discoveryUi)[name];
			char props[1024];
			sprintf(props, "type:button name:%s x:%d y:0 action:connect address:%s userdata:%llu image:button.png image_over:button-over.png image_down:button-down.png text_color:000000 font:calibri.ttf font_size:24",
				name,
				i * 300,
				serverInfo.m_address.ToString(false).c_str(),
				serverInfo.m_address.m_userData);
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

		setColor(colorWhite);
		m_discoveryUi->draw();
	#endif
	}
}

void App::netAction(Channel * channel, NetAction action, uint8_t param1, uint8_t param2, const std::string & param3)
{
	LOG_DBG("netAction: channel=%p, action=%d, param1=%d, param2=%d, param3='%d'", channel, action, param1, param2, param3.c_str());

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
	LOG_DBG("netSyncGameSim: channel=%p", channel);
	Assert(m_isHost);

	// todo : when serializing the state, make sure all packets that are destined for the host are flushed
	//        first, to ensure the game sim is up to date and properly synced to the client
	debugSyncGameSims();

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
	LOG_DBG("netAddPlayerBroadcast: owningChannelId=%d, index=%d, characterIndex=%d, displayName=%s, controllerIndex=%d", owningChannelId, index, characterIndex, displayName.c_str(), controllerIndex);

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
	LOG_DBG("netRemovePlayer: channel=%p, index=%d", channel, index);

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
	LOG_DBG("netRemovePlayerBroadcast: index=%d", index);

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
		const int16_t analogX = (playerInstanceData ? playerInstanceData->m_input.m_lastRecv.analogX : 0);
		const int16_t analogY = (playerInstanceData ? playerInstanceData->m_input.m_lastRecv.analogY : 0);

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
		debugSyncGameSims();
		m_rpcMgr->Call(s_rpcBroadcastPlayerInputs, bs, ChannelPool_Server, 0, false, true);
	}
	else
	{
		m_rpcMgr->Call(s_rpcBroadcastPlayerInputs, bs, ChannelPool_Server, 0, true, true);
	}
}

void App::netSetPlayerCharacterIndex(uint16_t channelId, uint8_t playerId, uint8_t characterIndex)
{
	LOG_DBG("netSetPlayerCharacterIndex: channelId=%d, playerId=%d, characterIndex=%d", channelId, playerId, characterIndex);

	// client -> host

	BitStream bs;

	bs.Write(playerId);
	bs.Write(characterIndex);

	m_rpcMgr->Call(s_rpcSetPlayerCharacterIndex, bs, ChannelPool_Client, &channelId, false, false);
}

void App::netBroadcastCharacterIndex(uint8_t playerId, uint8_t characterIndex)
{
	LOG_DBG("netBroadcastCharacterIndex: playerId=%d, characterIndex=%d", playerId, characterIndex);

	// host -> clients

	BitStream bs;

	bs.Write(playerId);
	bs.Write(characterIndex);

	m_rpcMgr->Call(s_rpcSetPlayerCharacterIndexBroadcast, bs, ChannelPool_Server, 0, true, true);
}

void App::netDebugAction(const char * name, const char * param)
{
#if ENABLE_NETWORKING_DEBUGS
	BitStream bs;

	bs.WriteString(name);
	bs.WriteString(param ? param : "");

	m_rpcMgr->Call(s_rpcDebugAction, bs, ChannelPool_Server, 0, true, true);
#endif
}

int App::allocControllerIndex(int preferredControllerIndex)
{
	if (m_freeControllerList.empty())
		return -1;

	if (preferredControllerIndex != -1)
	{
		auto i = std::find(m_freeControllerList.begin(), m_freeControllerList.end(), preferredControllerIndex);
		if (i != m_freeControllerList.end())
		{
			m_freeControllerList.erase(i);
			return preferredControllerIndex;
		}
	}

	std::sort(m_freeControllerList.begin(), m_freeControllerList.end(), std::greater<int>());
	const int index = m_freeControllerList.back();
	m_freeControllerList.pop_back();

	return index;
}

void App::freeControllerIndex(int index)
{
	Assert(index != -1);
	if (index != -1)
	{
		Assert(std::find(m_freeControllerList.begin(), m_freeControllerList.end(), index) == m_freeControllerList.end());
		m_freeControllerList.push_back(index);
	}
}

int App::getControllerAllocationCount() const
{
	return (MAX_GAMEPAD + 1) - m_freeControllerList.size();
}

bool App::isControllerIndexAvailable(int index) const
{
	for (size_t i = 0; i < m_freeControllerList.size(); ++i)
		if (m_freeControllerList[i] == index)
			return true;
	return false;
}

void App::playSound(const char * filename, int volume)
{
	if (g_noSound || !m_userSettings->audio.soundEnabled)
		return;

	Sound(filename).play(volume * m_userSettings->audio.soundVolume);
}

void App::DialogQuit(void * arg, int dialogId, DialogResult result)
{
	App * self = (App*)arg;

	if (result == DialogResult_Yes)
	{
		if (self->m_clients.empty())
			self->quit();
		else
		{
			if (self->getSelectedClient()->m_channel->m_state >= ChannelState_Connecting)
				self->getSelectedClient()->m_channel->Disconnect();
		}
	}
}

#if ENABLE_STEAM

void App::OnSteamPersonaStateChange(PersonaStateChange_t * arg)
{
	logDebug("OnSteamPersonaStateChange");
}

void App::OnSteamAvatarImageLoaded(AvatarImageLoaded_t * arg)
{
	logDebug("OnSteamAvatarImageLoaded");
}

void App::OnSteamGameLobbyJoinRequested(GameLobbyJoinRequested_t * arg)
{
	logDebug("OnSteamGameLobbyJoinRequested");

	joinGame(arg->m_steamIDLobby.ConvertToUint64());
}

void App::OnSteamP2PSessionRequest(P2PSessionRequest_t * arg)
{
	logDebug("OnSteamP2PSessionRequest");

	SteamNetworking()->AcceptP2PSessionWithUser(arg->m_steamIDRemote);
}

#endif

//

static std::map<std::string, ParticleEffect> s_particleEffects;

const ParticleEffect & getParticleEffect(const char * name)
{
	auto i = s_particleEffects.find(name);

	if (i != s_particleEffects.end())
		return i->second;
	else
	{
		ParticleEffect & e = s_particleEffects[name];

		e.setup(name);

		return e;
	}
}

//

static std::vector<std::string> parseMapList(const std::string & list)
{
	std::vector<std::string> result;

	std::string mapList = list;

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
			result.push_back(file);
		}
	} while (!mapList.empty());

	return result;
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

#if ENABLE_STEAM
#ifdef _WIN32
static void MiniDumpFunction(unsigned int exceptionCode, EXCEPTION_POINTERS * exception )
{
	SteamAPI_SetMiniDumpComment("minidump for: riposte.exe\n");
	SteamAPI_WriteMiniDump(exceptionCode, exception, g_buildId);
}
#endif
#endif

//

static int RealMain(int argc, char * argv[]);

int main(int argc, char * argv[])
{
#ifdef _WIN32
	if (IsDebuggerPresent())
	{
		// we don't want to mask exceptions (or report them to Steam!) when debugging
		return RealMain(argc, argv);
	}
	else
	{
	#if ENABLE_STEAM
		_set_se_translator(MiniDumpFunction);
	#endif
		try
		{
			// this try block allows the SE translator to work
			return RealMain(argc, argv);
		}
		catch(...)
		{
			return -1;
		}
	}
#else
	return RealMain(argc, argv);
#endif
}

static int RealMain(int argc, char * argv[])
{
#if defined(__WIN32__)
	_CrtSetDebugFillThreshold(0);
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

	for (int i = 0; i < argc; ++i)
	{
	#if ENABLE_STEAM
		if (!strcmp(argv[i], "+connect_lobby"))
		{
			Assert(i + 1 < argc);
			if (i + 1 < argc)
			{
				g_connectLobby.SetFromUint64(_atoi64(argv[i + 1]));
				LOG_INF("connect_lobby is set to %llx", g_connectLobby.ConvertToUint64());
			}
		}
	#endif
	}

#if 0 // host IP prompt
	if (!g_devMode && !g_connectLocal && (std::string)g_connect == "")
	{
		std::cout << "host IP address: ";
		std::string ip;
		std::getline(std::cin, ip);
		g_connect = ip;
	}
#endif

	g_app = new App();

	if (!g_app->init())
	{
		//
	}
	else
	{
	#if 0 // spriter test
		//Spriter spriter("../../ArtistCave/JoyceTestcharacter/char0(Sword)/sprite/Sprite.scml");
		Spriter spriter("char4/sprite/Sprite.scml");

		//const int animIndex = 0;
		const int animIndex = spriter.getAnimIndexByName("Idle");
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
					setDebugFont();
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

			//g_app->m_host->m_gameSim.setGameState(kGameState_OnlineMenus);

			Verify(g_app->findGame());
		}
	#if ENABLE_STEAM
		else if (USE_STEAMAPI && g_connectLobby.IsLobby())
		{
			Verify(g_app->joinGame(g_connectLobby.ConvertToUint64()));
		}
	#if ENABLE_NETWORKING_DEBUGS
		else if (g_testSteamMatchmaking)
		{
			g_app->startHosting();
		}
	#endif
	#endif

		//

		for (;;)
		{
			cpuTimingBlock(frame);

			if (!g_app->tick())
				break;

			g_app->draw();
		}

		g_app->shutdown();
	}

	delete g_app;
	g_app = 0;

	return 0;
}
