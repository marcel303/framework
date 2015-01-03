#include <algorithm>
#include <functional>
#include "arena.h"
#include "bullet.h"
#include "Debugging.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "player.h"
#include "ReplicationManager.h"
#include "Timer.h"

COMMAND_OPTION(s_addPickup, "Debug/Add Pickup", [] { if (g_host) g_host->trySpawnPickup((PickupType)(rand() % kPickupType_COUNT)); });

COMMAND_OPTION(s_gameStateNewGame, "Game State/New Game", [] { if (g_host) g_host->newGame(); });
COMMAND_OPTION(s_gameStateNewRound, "Game State/New Round", [] { if (g_host) g_host->newRound(0); });
COMMAND_OPTION(s_gameStateEndRound, "Game State/End Round", [] { if (g_host) g_host->endRound(); });

OPTION_DECLARE(int, g_roundCompleteScore, 10);
OPTION_DEFINE(int, g_roundCompleteScore, "Game Round/Max Player Score");
OPTION_DECLARE(int, g_roundCompleteTimer, 6);
OPTION_DEFINE(int, g_roundCompleteTimer, "Game Round/Game Complete Time");

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
	case kGameState_Lobby:
		break;

	case kGameState_Play:
		tickPlay(dt);
		break;

	case kGameState_RoundComplete:
		tickRoundComplete(dt);
		break;
	}

	//clearPlayerPtrs();
}

void Host::tickPlay(float dt)
{
	bool roundComplete = false;

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = m_gameSim.m_players[i];
		if (!player.m_isUsed)
			continue;

		if (player.m_score >= g_roundCompleteScore)
		{
			roundComplete = true;
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
	g_app->netSetGameState(kGameState_Play);

	m_nextRoundNumber++;
}

void Host::endRound()
{
	g_app->netSetGameState(kGameState_RoundComplete);

	m_roundCompleteTimer = g_TimerRT.TimeUS_get() + g_roundCompleteTimer * 1000000; // fixme, option
}

PlayerNetObject * Host::allocPlayer(uint16_t owningChannelId)
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = m_gameSim.m_players[i];
		if (player.m_isUsed)
			continue;

		player = Player(i, owningChannelId);
		player.m_isUsed = true;

		PlayerNetObject * netObject = new PlayerNetObject(&player, &m_gameSim);

		m_gameSim.m_playerNetObjects[i] = netObject;

		return netObject;
	}

	return 0;
}

void Host::freePlayer(PlayerNetObject * player)
{
	const int playerId = player->m_player->m_index;

	if (playerId != -1)
	{
		Assert(m_gameSim.m_playerNetObjects[playerId] != 0);
		if (m_gameSim.m_playerNetObjects[playerId] != 0)
		{
			m_gameSim.m_playerNetObjects[playerId]->m_player->m_isUsed = false;
			m_gameSim.m_playerNetObjects[playerId]->m_player->m_netObject = 0;
			m_gameSim.m_playerNetObjects[playerId] = 0;
		}
	}
}

PlayerNetObject * Host::findPlayerByPlayerId(uint8_t playerId)
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		PlayerNetObject * player = m_gameSim.m_playerNetObjects[i];
		if (player && player->m_player->m_index == playerId)
			return player;
	}

	return 0;
}

void Host::setPlayerPtrs()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		PlayerNetObject * player = m_gameSim.m_playerNetObjects[i];
		if (player)
			player->m_player->m_netObject = player;
	}
}

void Host::clearPlayerPtrs()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		PlayerNetObject * player = m_gameSim.m_playerNetObjects[i];
		if (player)
			player->m_player->m_netObject = 0;
	}
}

void Host::trySpawnPickup(PickupType type)
{
	return m_gameSim.trySpawnPickup(type);
}

void Host::spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY)
{
	m_gameSim.spawnPickup(pickup, type, blockX, blockY);
}

Pickup * Host::grabPickup(int x1, int y1, int x2, int y2)
{
	return m_gameSim.grabPickup(x1, y1, x2, y2);
}
