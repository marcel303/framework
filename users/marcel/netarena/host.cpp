#include "arena.h"
#include "bullet.h"
#include "Debugging.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "player.h"
#include "ReplicationManager.h"

Host * g_host = 0;
Arena * g_hostArena = 0;
BulletPool * g_hostBulletPool = 0;

Host::Host()
	: m_arena(0)
	, m_nextNetId(1) // 0 = unassigned
	, m_bulletPool(0)
{
}

Host::~Host()
{
	Assert(g_host == 0);
	Assert(g_hostArena == 0);
	Assert(g_hostBulletPool == 0);

	Assert(m_arena == 0);
}

void Host::init()
{
	m_arena = new Arena();
	m_arena->load("arena.txt");

	g_app->getReplicationMgr()->SV_AddObject(m_arena);

	m_bulletPool = new BulletPool();

	g_host = this;
	g_hostArena = m_arena;
	g_hostBulletPool = m_bulletPool;
}

void Host::shutdown()
{
	g_hostBulletPool = 0;
	g_hostArena = 0;
	g_host = 0;

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
