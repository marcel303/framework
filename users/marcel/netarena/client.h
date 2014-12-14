#pragma once

#include <stdint.h>
#include <vector>
#include "libnet_forward.h"

class Arena;
class BulletPool;
class NetSpriteManager;
class Player;

class Client
{
public:
	Channel * m_channel;
	uint32_t m_replicationId;

	Arena * m_arena;
	std::vector<Player*> m_players;

	BulletPool * m_bulletPool;

	NetSpriteManager * m_spriteManager;

	Client();
	~Client();

	void initialize(Channel * channel);

	void tick(float dt);
	void draw();
	void drawPlay();
	void drawRoundComplete();

	void addPlayer(Player * player);
	void removePlayer(Player * player);
};
