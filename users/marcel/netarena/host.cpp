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
COMMAND_OPTION(s_addPickup, "Debug/Add Pickup", [] { if (g_host) g_host->spawnPickup(kPickupType_Ammo, rand() % ARENA_SX, rand() % ARENA_SY); });

COMMAND_OPTION(s_gameStateNewGame, "Game State/New Game", [] { if (g_host) g_host->newGame(); });
COMMAND_OPTION(s_gameStateNewRound, "Game State/New Round", [] { if (g_host) g_host->newRound(0); });
COMMAND_OPTION(s_gameStateEndRound, "Game State/End Round", [] { if (g_host) g_host->endRound(); });

OPTION_DECLARE(int, g_roundCompleteScore, 10);
OPTION_DEFINE(int, g_roundCompleteScore, "Game Round/Max Player Score");
OPTION_DECLARE(int, g_roundCompleteTimer, 6);
OPTION_DEFINE(int, g_roundCompleteTimer, "Game Round/Game Complete Time");

OPTION_DECLARE(int, g_pickupTimeBase, 10);
OPTION_DEFINE(int, g_pickupTimeBase, "Pickup/Spawn Interval (Sec)");
OPTION_DECLARE(int, g_pickupTimeRandom, 5);
OPTION_DEFINE(int, g_pickupTimeRandom, "Pickup/Spawn Interval Random (Sec)");
OPTION_DECLARE(int, g_pickupMax, 5);
OPTION_DEFINE(int, g_pickupMax, "Pickup/Maximum Pickup Count");

OPTION_EXTERN(std::string, g_map);

extern std::vector<std::string> g_mapList;

Host * g_host = 0;
Arena * g_hostArena = 0;
BulletPool * g_hostBulletPool = 0;
NetSpriteManager * g_hostSpriteManager = 0;

static const char * s_pickupSprites[kPickupType_COUNT] =
{
	"pickup-ammo.png"
};

Host::Host()
	: m_arena(0)
	, m_nextNetId(1) // 0 = unassigned
	, m_nextRoundNumber(0)
	, m_nextPickupSpawnTime(0)
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

	Assert(m_arena == 0);
}

void Host::init()
{
	m_freePlayerIds.clear();
	for (int i = 0; i < 4; ++i)
		m_freePlayerIds.push_back(i);

	m_arena = new Arena();

	g_app->getReplicationMgr()->SV_AddObject(m_arena);

	m_bulletPool = new BulletPool(false);

	m_spriteManager = new NetSpriteManager();

	g_host = this;
	g_hostArena = m_arena;
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

	g_app->getReplicationMgr()->SV_RemoveObject(m_arena->GetObjectID());

	delete m_arena;
	m_arena = 0;
}

void Host::tick(float dt)
{
	const uint64_t time = g_TimerRT.TimeMS_get();
	static uint64_t lastTime = time;
	if (time == lastTime)
	{
		SDL_Delay(1);
		return;
	}

	dt = (time - lastTime) / 1000.f;
	if (dt > 1.f / 60.f)
		dt = 1.f / 60.f;
	lastTime = time;

	switch (m_arena->m_gameState.m_gameState)
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
}

void Host::tickPlay(float dt)
{
	const uint64_t time = g_TimerRT.TimeUS_get();

	if (time >= m_nextPickupSpawnTime)
	{
		if ((int)m_pickups.size() < g_pickupMax)
		{
			int x;
			int y;

			if (m_arena->getRandomPickupLocation(x, y))
			{
				spawnPickup(kPickupType_Ammo, x, y);
			}
		}

		m_nextPickupSpawnTime = time + (g_pickupTimeBase + (rand() % g_pickupTimeRandom)) * 1000000;
	}

	bool roundComplete = false;

	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		Player * player = *i;

		player->tick(dt);

		if (player->getScore() >= g_roundCompleteScore)
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
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		Player * player = *i;

		player->debugDraw();
	}
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
		Player * player = *p;

		player->handleNewGame();
	}

	newRound(0);
}

void Host::newRound(const char * mapOverride)
{
	// todo : remove bullets

	// remove net sprites
	
	for (int i = 0; i < MAX_SPRITES; ++i)
		if (m_spriteManager->m_sprites[i].enabled)
			g_app->netRemoveSprite(i);

	m_pickups.clear();

	// load arena

	std::string map = g_map;

	if (mapOverride)
	{
		m_arena->load(mapOverride);
	}
	else if (!map.empty())
	{
		m_arena->load(map.c_str());
	}
	else if (g_mapList.size() != 0)
	{
		const size_t index = m_nextRoundNumber % g_mapList.size();

		m_arena->load(g_mapList[index].c_str());
	}
	else
	{
		m_arena->load("arena.txt");
	}

	// respawn players

	for (auto p = m_players.begin(); p != m_players.end(); ++p)
	{
		Player * player = *p;

		player->handleNewRound();

		player->respawn();
	}

	m_arena->m_gameState.m_gameState = kGameState_Play;
	m_arena->m_gameState.SetDirty();

	m_nextRoundNumber++;

	g_app->netPlaySound("round-begin.ogg");
}

void Host::endRound()
{
	m_arena->m_gameState.m_gameState = kGameState_RoundComplete;
	m_arena->m_gameState.SetDirty();

	m_roundCompleteTimer = g_TimerRT.TimeUS_get() + g_roundCompleteTimer * 1000000; // fixme, option
}

void Host::addPlayer(Player * player)
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

void Host::removePlayer(Player * player)
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

Player * Host::findPlayerByNetId(uint32_t netId)
{
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		Player * player = *i;

		if (player->getNetId() == netId)
			return player;
	}

	return 0;
}

void Host::spawnPickup(PickupType type, int blockX, int blockY)
{
	const char * filename = s_pickupSprites[type];

	Sprite sprite(filename);

	Pickup pickup;
	pickup.type = type;
	pickup.x1 = blockX * BLOCK_SX + (BLOCK_SX - sprite.getWidth()) / 2;
	pickup.y1 = blockY * BLOCK_SY + BLOCK_SY - sprite.getHeight();
	pickup.x2 = pickup.x1 + sprite.getWidth();
	pickup.y2 = pickup.y1 + sprite.getHeight();
	pickup.spriteId = g_app->netAddSprite(filename, pickup.x1, pickup.y1);

	m_pickups.push_back(pickup);
}

Pickup * Host::grabPickup(int x1, int y1, int x2, int y2)
{
	for (auto i = m_pickups.begin(); i != m_pickups.end(); ++i)
	{
		const Pickup & pickup = *i;

		if (x2 >= pickup.x1 && x1 < pickup.x2 &&
			y2 >= pickup.y1 && y1 < pickup.y2)
		{
			m_grabbedPickup = pickup;
			m_pickups.erase(i);

			g_app->netRemoveSprite(m_grabbedPickup.spriteId);

			g_app->netPlaySound("gun-pickup.ogg");

			return &m_grabbedPickup;
		}
	}

	return 0;
}
