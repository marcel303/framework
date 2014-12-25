#include <algorithm>
#include <functional>
#include "arena.h"
#include "bullet.h"
#include "Debugging.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "netsprite.h"
#include "player.h"
#include "ReplicationManager.h"
#include "Timer.h"

COMMAND_OPTION(s_addSprite, "Debug/Add Sprite", [] { if (g_host) g_app->netAddSprite("block-spike.png", rand() % ARENA_SX_PIXELS, rand() % ARENA_SY_PIXELS); });
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
Arena * g_hostArena = 0;
BulletPool * g_hostBulletPool = 0;
NetSpriteManager * g_hostSpriteManager = 0;

Host::Host()
	: m_nextNetId(1) // 0 = unassigned
	, m_nextRoundNumber(0)
	, m_bulletPool(0)
	, m_spriteManager(0)
	, m_roundCompleteTimer(0)
{
}

Host::~Host()
{
	Assert(g_host == 0);
	Assert(g_hostArena == 0);
	Assert(g_hostBulletPool == 0);
	Assert(g_hostSpriteManager == 0);
}

void Host::init()
{
	m_freePlayerIds.clear();
	for (int i = 0; i < 4; ++i)
		m_freePlayerIds.push_back(i);

	g_app->getReplicationMgr()->SV_AddObject(&m_gameSim.m_arenaNetObject);

	m_bulletPool = new BulletPool(false);

	m_spriteManager = new NetSpriteManager();

	g_host = this;
	g_hostArena = &m_gameSim.m_state.m_arena;
	g_hostBulletPool = m_bulletPool;
	g_hostSpriteManager = m_spriteManager;
}

void Host::shutdown()
{
	g_hostSpriteManager = 0;
	g_hostBulletPool = 0;
	g_hostArena = 0;
	g_host = 0;

	delete m_spriteManager;
	m_spriteManager = 0;

	delete m_bulletPool;
	m_bulletPool = 0;

	g_app->getReplicationMgr()->SV_RemoveObject(m_gameSim.m_arenaNetObject.GetObjectID());
}

void Host::tick(float dt)
{
	//setPlayerPtrs(); // fixme : enable once full simulation runs on clients

	switch (m_gameSim.m_arenaNetObject.m_gameState.m_gameState)
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
	m_gameSim.tick();

	bool roundComplete = false;

	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		PlayerNetObject * playerNetObject = *i;
		Player * player = playerNetObject->m_player;

		player->tick(dt);

		if (playerNetObject->getScore() >= g_roundCompleteScore)
		{
			roundComplete = true;
		}
	}

	m_bulletPool->tick(dt);

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

	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		PlayerNetObject * playerNetObject = *i;
		Player * player = playerNetObject->m_player;

		player->debugDraw();
	}

	//clearPlayerPtrs();
}

uint32_t Host::allocNetId()
{
	return m_nextNetId++;
}

void Host::syncNewClient(Channel * channel)
{
	// sync bullet list

	// sync net sprites

	for (int i = 0; i < MAX_SPRITES; ++i)
		if (m_spriteManager->m_sprites[i].enabled)
			g_app->netSyncSprite(i, channel);
}

void Host::newGame()
{
	// reset players

	for (auto p = m_players.begin(); p != m_players.end(); ++p)
	{
		PlayerNetObject * playerNetObject = *p;
		Player * player = playerNetObject->m_player;

		player->handleNewGame();
	}

	newRound(0);
}

void Host::newRound(const char * mapOverride)
{
	// todo : send RPC call to host/clients

	// todo : remove bullets

	// remove net sprites
	
	for (int i = 0; i < MAX_SPRITES; ++i)
		if (m_spriteManager->m_sprites[i].enabled)
			g_app->netRemoveSprite(i);

	for (int i = 0; i < MAX_PICKUPS; ++i)
		m_gameSim.m_state.m_pickups[i].isAlive = false;

	// load arena

	std::string map = g_map;

	if (mapOverride)
	{
		m_gameSim.m_state.m_arena.load(mapOverride);
	}
	else if (!map.empty())
	{
		m_gameSim.m_state.m_arena.load(map.c_str());
	}
	else if (g_mapList.size() != 0)
	{
		const size_t index = m_nextRoundNumber % g_mapList.size();

		m_gameSim.m_state.m_arena.load(g_mapList[index].c_str());
	}
	else
	{
		m_gameSim.m_state.m_arena.load("arena.txt");
	}

	// respawn players

	for (auto p = m_players.begin(); p != m_players.end(); ++p)
	{
		PlayerNetObject * playerNetObject = *p;
		Player * player = playerNetObject->m_player;

		player->handleNewRound();

		player->respawn();
	}

	m_gameSim.m_arenaNetObject.m_gameState.m_gameState = kGameState_Play;
	m_gameSim.m_arenaNetObject.m_gameState.SetDirty();

	m_nextRoundNumber++;

	g_app->netPlaySound("round-begin.ogg");
}

void Host::endRound()
{
	m_gameSim.m_arenaNetObject.m_gameState.m_gameState = kGameState_RoundComplete;
	m_gameSim.m_arenaNetObject.m_gameState.SetDirty();

	m_roundCompleteTimer = g_TimerRT.TimeUS_get() + g_roundCompleteTimer * 1000000; // fixme, option
}

void Host::addPlayer(PlayerNetObject * player)
{
	// allocate player ID

	int playerId = -1;

	if (!m_freePlayerIds.empty())
	{
		std::sort(m_freePlayerIds.begin(), m_freePlayerIds.end(), std::greater<int>());
		playerId = m_freePlayerIds.back();
		m_freePlayerIds.pop_back();
	}

	player->setPlayerId(playerId);

	m_players.push_back(player);
}

void Host::removePlayer(PlayerNetObject * player)
{
	auto i = std::find(m_players.begin(), m_players.end(), player);

	Assert(i != m_players.end());
	if (i != m_players.end())
	{
		if (player->getPlayerId() != -1)
			m_freePlayerIds.push_back(player->getPlayerId());
		m_players.erase(i);
	}
}

PlayerNetObject * Host::findPlayerByNetId(uint32_t netId)
{
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		PlayerNetObject * player = *i;

		if (player->getNetId() == netId)
			return player;
	}

	return 0;
}

void Host::setPlayerPtrs()
{
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
		(*i)->m_player->m_netObject = (*i);
}

void Host::clearPlayerPtrs()
{
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
		(*i)->m_player->m_netObject = 0;
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
