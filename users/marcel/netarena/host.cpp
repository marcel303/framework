#include <algorithm>
#include <functional>
#include "arena.h"
#include "bullet.h"
#include "Debugging.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "player.h"
#include "Timer.h"

COMMAND_OPTION(s_addPickup, "Debug/Add Pickup", [] { if (g_host) g_host->m_gameSim.trySpawnPickup((PickupType)(rand() % kPickupType_COUNT)); });

COMMAND_OPTION(s_gameStateNewGame, "Game State/New Game", [] { if (g_host) g_host->newGame(); });
COMMAND_OPTION(s_gameStateNewRound, "Game State/New Round", [] { if (g_host) g_host->newRound(0); });
COMMAND_OPTION(s_gameStateEndRound, "Game State/End Round", [] { if (g_host) g_host->endRound(); });

OPTION_DECLARE(int, g_roundCompleteTimer, 6);
OPTION_DEFINE(int, g_roundCompleteTimer, "Menus/Results Screen Time");

OPTION_EXTERN(std::string, g_map);

extern std::vector<std::string> g_mapList;

Host * g_host = 0;

Host::Host()
	: m_gameSim()
	, m_nextRoundNumber(0)
	, m_roundCompleteTimer(0)
{
}

Host::~Host()
{
	Assert(g_host == 0);
}

void Host::init()
{
	g_host = this;
}

void Host::shutdown()
{
	g_host = 0;
}

void Host::tick(float dt)
{
	//setPlayerPtrs(); // fixme : enable once full simulation runs on clients

	switch (m_gameSim.m_gameState)
	{
	case kGameState_MainMenus:
		Assert(false);
		break;

	case kGameState_OnlineMenus:
		tickMenus(dt);
		break;

	case kGameState_Play:
		tickPlay(dt);
		break;

	case kGameState_RoundComplete:
		tickRoundComplete(dt);
		break;

	default:
		Assert(false);
		break;
	}

	//clearPlayerPtrs();
}

void Host::tickMenus(float dt)
{
	// check if all players are ready
	// todo : do this on the client also. makes it possible to show timer on screen

	bool allReady = true;

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_gameSim.m_players[i].m_isUsed)
			allReady &= m_gameSim.m_players[i].m_isReadyUpped;

	if (allReady)
	{
		newGame();
	}
}

void Host::tickPlay(float dt)
{
	bool roundComplete = false;

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = m_gameSim.m_players[i];
		if (!player.m_isUsed)
			continue;

		if (m_gameSim.m_gameMode == kGameMode_DeathMatch)
		{
			if (player.m_score >= DEATHMATCH_SCORE_LIMIT)
			{
				roundComplete = true;
			}
		}
		else if (m_gameSim.m_gameMode == kGameMode_TokenHunt)
		{
			if (player.m_score >= TOKENHUNT_SCORE_LIMIT)
			{
				roundComplete = true;
			}
		}
		else if (m_gameSim.m_gameMode == kGameMode_CoinCollector)
		{
			if (player.m_score >= COINCOLLECTOR_SCORE_LIMIT)
			{
				roundComplete = true;
			}
		}
		else
		{
			Assert(false);
		}
	}

	if (roundComplete)
	{
		endRound();
	}
}

void Host::tickRoundComplete(float dt)
{
	// todo : wait for all players to accept, and go the the next state

	const uint64_t time = g_TimerRT.TimeUS_get();

	if (time >= m_roundCompleteTimer)
	{
		newRound(0);
	}
}

void Host::debugDraw()
{
	//setPlayerPtrs(); // fixme : enable once full simulation runs on clients

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = m_gameSim.m_players[i];
		if (!player.m_isUsed)
			continue;

		player.debugDraw();
	}

	//clearPlayerPtrs();
}

void Host::syncNewClient(Channel * channel)
{
	g_app->netSyncGameSim(channel);
}

void Host::newGame()
{
	g_app->netSetGameState(kGameState_NewGame);

	newRound(0);
}

// todo : remove newRound from Host class, and move it to gamesim
void Host::newRound(const char * mapOverride)
{
	// load arena

	std::string map;

	if (mapOverride)
		map = mapOverride;
	else if (!((std::string)g_map).empty())
		map = g_map;
	else if (!g_mapList.empty())
		map = g_mapList[m_nextRoundNumber % g_mapList.size()];
	else
		map = "arena.txt";

	g_app->netLoadArena(map.c_str());

	g_app->netSetGameMode(kGameMode_TokenHunt);
	//g_app->netSetGameMode(kGameMode_CoinCollector);
	g_app->netSetGameState(kGameState_Play);

	m_nextRoundNumber++;
}

void Host::endRound()
{
	g_app->netSetGameState(kGameState_RoundComplete);

	m_roundCompleteTimer = g_TimerRT.TimeUS_get() + g_roundCompleteTimer * 1000000; // fixme, option
}

PlayerInstanceData * Host::allocPlayer(uint16_t owningChannelId)
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = m_gameSim.m_players[i];
		if (player.m_isUsed)
			continue;

		player = Player(i, owningChannelId);
		player.m_isUsed = true;

		PlayerInstanceData * playerInstanceData = new PlayerInstanceData(&player, &m_gameSim);

		m_gameSim.m_playerInstanceDatas[i] = playerInstanceData;

		return playerInstanceData;
	}

	return 0;
}

void Host::freePlayer(PlayerInstanceData * playerInstanceData)
{
	const int playerId = playerInstanceData->m_player->m_index;

	if (playerId != -1)
	{
		Assert(m_gameSim.m_playerInstanceDatas[playerId] != 0);
		if (m_gameSim.m_playerInstanceDatas[playerId] != 0)
		{
			m_gameSim.m_playerInstanceDatas[playerId]->m_player->m_isUsed = false;
			m_gameSim.m_playerInstanceDatas[playerId]->m_player->m_instanceData = 0;
			m_gameSim.m_playerInstanceDatas[playerId] = 0;
		}
	}
}

PlayerInstanceData * Host::findPlayerByPlayerId(uint8_t playerId)
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		PlayerInstanceData * playerInstanceData = m_gameSim.m_playerInstanceDatas[i];
		if (playerInstanceData && playerInstanceData->m_player->m_index == playerId)
			return playerInstanceData;
	}

	return 0;
}

void Host::setPlayerPtrs()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		PlayerInstanceData * playerInstanceData = m_gameSim.m_playerInstanceDatas[i];
		if (playerInstanceData)
			playerInstanceData->m_player->m_instanceData = playerInstanceData;
	}
}

void Host::clearPlayerPtrs()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		PlayerInstanceData * playerInstanceData = m_gameSim.m_playerInstanceDatas[i];
		if (playerInstanceData)
			playerInstanceData->m_player->m_instanceData = 0;
	}
}
