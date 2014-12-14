#include "arena.h"
#include "bullet.h"
#include "Debugging.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "netsprite.h"
#include "player.h"
#include "ReplicationManager.h"

COMMAND_OPTION(s_addSprite, "Debug/Add Sprite", [] { g_app->netAddSprite("block-spike.png", rand() % ARENA_SX_PIXELS, rand() % ARENA_SY_PIXELS); });
COMMAND_OPTION(s_addPickup, "Debug/Add Pickup", [] { g_host->spawnPickup(kPickupType_Ammo, rand() % ARENA_SX, rand() % ARENA_SY); });

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
	pickup.y1 = blockY * BLOCK_SY + (BLOCK_SX - sprite.getHeight()) / 2;
	pickup.x2 = pickup.x1 + sprite.getWidth();
	pickup.y2 = pickup.y1 + sprite.getHeight();
	pickup.spriteId = g_app->netAddSprite(filename, blockX * BLOCK_SX, blockY * BLOCK_SY);

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
