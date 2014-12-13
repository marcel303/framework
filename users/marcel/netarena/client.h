#pragma once

#include <stdint.h>
#include <vector>
#include "libnet_forward.h"

class Arena;
class BulletPool;
class Player;

class Client
{
public:
	Channel * m_channel;
	uint32_t m_replicationId;

	Arena * m_arena;
	std::vector<Player*> m_players;

	BulletPool * m_bulletPool;

	Client();
	~Client();

	void initialize(Channel * channel);

	void tick(float dt);
	void draw();

	void addPlayer(Player * player);
	void removePlayer(Player * player);
};
