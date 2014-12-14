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

COMMAND_OPTION(s_addSprite, "Debug/Add Sprite", [] { g_app->netAddSprite("block-spike.png", rand() % ARENA_SX_PIXELS, rand() % ARENA_SY_PIXELS); });
COMMAND_OPTION(s_addPickup, "Debug/Add Pickup", [] { g_host->spawnPickup(kPickupType_Ammo, rand() % ARENA_SX, rand() % ARENA_SY); });

OPTION_DECLARE(int, g_pickupTimeBase, 10);
OPTION_DEFINE(int, g_pickupTimeBase, "Pickup/Spawn Interval (Sec)");
OPTION_DECLARE(int, g_pickupTimeRandom, 5);
OPTION_DEFINE(int, g_pickupTimeRandom, "Pickup/Spawn Interval Random (Sec)");
OPTION_DECLARE(int, g_pickupMax, 5);
OPTION_DEFINE(int, g_pickupMax, "Pickup/Maximum Pickup Count");

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
	, m_nextPickupSpawnTime(0)
	, m_bulletPool(0)
	, m_spriteManager(0)
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
	m_arena = new Arena();
	m_arena->load("arena.txt");

	g_app->getReplicationMgr()->SV_AddObject(m_arena);

	m_bulletPool = new BulletPool();

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
	const uint64_t time = g_TimerRT.TimeUS_get();

	if (time >= m_nextPickupSpawnTime)
	{
		if (m_pickups.size() < g_pickupMax)
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

	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		Player * player = *i;

		player->tick(dt);
	}

	m_bulletPool->tick(dt);
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

void Host::addPlayer(Player * player)
{
	m_players.push_back(player);
}

void Host::removePlayer(Player * player)
{
	auto i = std::find(m_players.begin(), m_players.end(), player);

	Assert(i != m_players.end());
	if (i != m_players.end())
		m_players.erase(i);
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

			return &m_grabbedPickup;
		}
	}

	return 0;
}
