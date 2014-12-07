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

	uint8_t m_controllerIndex;

	Client(uint8_t controllerIndex);
	~Client();

	void initialize(Channel * channel);

	void tick(float dt);
	void draw();
};
