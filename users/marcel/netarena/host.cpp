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

COMMAND_OPTION(s_gameStateNewGame, "Game State/New Game", [] { if (g_host) g_app->netDebugAction("newGame", ""); });
COMMAND_OPTION(s_gameStateNewRound, "Game State/New Round", [] { if (g_host) g_app->netDebugAction("newRound", ""); });
COMMAND_OPTION(s_gameStateEndRound, "Game State/End Round", [] { if (g_host) g_app->netDebugAction("endRound", ""); });

Host * g_host = 0;

Host::Host()
	: m_gameSim()
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
	switch (m_gameSim.m_gameState)
	{
	case kGameState_Initial:
		Assert(false);
		break;

	case kGameState_OnlineMenus:
	case kGameState_Play:
	case kGameState_RoundComplete:
		break;

	default:
		Assert(false);
		break;
	}

	if (PLAYER_INACTIVITY_KICK)
	{
		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_gameSim.m_playerInstanceDatas[i])
			{
				if (m_gameSim.m_playerInstanceDatas[i]->m_player->m_input.m_inactivityTime >= PLAYER_INACTIVITY_TIME)
				{
					g_app->netRemovePlayer(0, i);
				}
			}
		}
	}
}

void Host::debugDraw()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = m_gameSim.m_players[i];
		if (!player.m_isUsed)
			continue;

		player.debugDraw();
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
