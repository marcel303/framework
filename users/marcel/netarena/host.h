#pragma once

#include <vector>

class Arena;
class BulletPool;
class NetSpriteManager;
class Player;

struct Pickup
{
	PickupType type;
	int blockX;
	int blockY;
	int x1;
	int y1;
	int x2;
	int y2;
	uint16_t spriteId;
};

class Host
{
	friend class BulletPool;
	friend class Player; // fixme, for m_players array

	Arena * m_arena;
	std::vector<Player*> m_players;
	uint32_t m_nextNetId;
	std::vector<int> m_freePlayerIds;

	int m_nextRoundNumber;

	std::vector<Pickup> m_pickups;
	Pickup m_grabbedPickup;
	uint64_t m_nextPickupSpawnTime;

	BulletPool * m_bulletPool;

	NetSpriteManager * m_spriteManager;

	// round complete game state
	uint64_t m_roundCompleteTimer;

public:
	Host();
	~Host();

	void init();
	void shutdown();

	void tick(float dt);
	void tickPlay(float dt);
	void tickRoundComplete(float dt);
	void debugDraw();

	uint32_t allocNetId();

	void syncNewClient(Channel * channel);

	void newGame();
	void newRound(const char * mapOverride);
	void endRound();

	void addPlayer(Player * player);
	void removePlayer(Player * player);
	Player * findPlayerByNetId(uint32_t netId);

	void spawnPickup(PickupType type, int blockX, int blockY);
	Pickup * grabPickup(int x1, int y1, int x2, int y2);
};

extern Host * g_host;
extern Arena * g_hostArena;
extern BulletPool * g_hostBulletPool;
extern NetSpriteManager * g_hostSpriteManager;
