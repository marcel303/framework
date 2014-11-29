#pragma once

#include <vector>

class Arena;
class Player;

class Host
{
	Arena * m_arena;
	std::vector<Player*> m_players;
	uint32_t m_nextNetId;

public:
	Host();
	~Host();

	void init();
	void shutdown();

	void tick(float dt);

	uint32_t allocNetId();

	void addPlayer(Player * player);
	void removePlayer(Player * player);
	Player * findPlayerByNetId(uint32_t netId);
};

extern Host * g_host;
extern Arena * g_hostArena;
