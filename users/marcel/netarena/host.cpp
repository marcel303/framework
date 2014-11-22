#include "arena.h"
#include "Debugging.h"
#include "host.h"
#include "main.h"
#include "player.h"
#include "ReplicationManager.h"

Host * g_host = 0;
Arena * g_hostArena = 0;

Host::Host()
	: m_arena(0)
{
}

Host::~Host()
{
	Assert(g_host == 0);
	Assert(g_hostArena == 0);

	Assert(m_arena == 0);
}

void Host::init()
{
	m_arena = new Arena();
	m_arena->generate();

	g_app->getReplicationMgr()->SV_AddObject(m_arena);

	g_host = this;
	g_hostArena = m_arena;
}

void Host::shutdown()
{
	g_hostArena = 0;
	g_host = 0;

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
}

void Host::addPlayer(Player * player)
{
	m_players.push_back(player);
}

void Host::removePlayer(Player * player)
{
	auto i = std::find(m_players.begin(), m_players.end(), player);

	if (i != m_players.end())
		m_players.erase(i);
}

Player * Host::findPlayerByChannelId(uint16_t channelId)
{
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		Player * player = *i;

		if (player->getOwningChannelId() == channelId)
			return player;
	}

	return 0;
}
