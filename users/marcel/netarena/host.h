#pragma once

#include <vector>

class Arena;
class Player;

class Host
{
	Arena * m_arena;
	std::vector<Player*> m_players;

public:
	Host();
	~Host();

	void init();
	void shutdown();

	void tick(float dt);

	void addPlayer(Player * player);
	void removePlayer(Player * player);
	Player * findPlayerByChannelId(uint16_t channelId);
};

extern Host * g_host;
extern Arena * g_hostArena;
