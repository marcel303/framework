#pragma once

#include <stdint.h>
#include <vector>
#include "gamedefs.h"
#include "libnet_forward.h"

class Arena;
class BulletPool;
class GameSim;
class NetSpriteManager;
struct Player;
class PlayerNetObject;

class Client
{
public:
	Channel * m_channel;
	uint32_t m_replicationId;

	GameSim * m_gameSim;

	ArenaNetObject * m_arena;
	std::vector<PlayerNetObject*> m_players;

#if !ENABLE_CLIENT_SIMULATION
	BulletPool * m_bulletPool;

	BulletPool * m_particlePool;
#endif

	NetSpriteManager * m_spriteManager;

	Client();
	~Client();

	void initialize(Channel * channel);

	void tick(float dt);
	void draw();
	void drawPlay();
	void drawRoundComplete();

	void addPlayer(PlayerNetObject * player);
	void removePlayer(PlayerNetObject * player);
	PlayerNetObject * findPlayerByNetId(uint32_t netId);
	void setPlayerPtrs();
	void clearPlayerPtrs();

	void spawnParticles(const ParticleSpawnInfo & spawnInfo);
};
