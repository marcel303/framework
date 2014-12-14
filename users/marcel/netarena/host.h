#pragma once

#include <vector>

class Arena;
class BulletPool;
class NetSpriteManager;
class Player;

enum PickupType
{
	kPickupType_Ammo
};

struct Pickup
{
	PickupType type;
	uint8_t blockX;
	uint8_t blockY;
	uint16_t spriteId;
};

class Host
{
	friend class BulletPool;
	friend class Player; // fixme, for m_players array

	Arena * m_arena;
	std::vector<Player*> m_players;
	uint32_t m_nextNetId;

	std::vector<Pickup> m_pickups;
	Pickup m_grabbedPickup;

	BulletPool * m_bulletPool;

	NetSpriteManager * m_spriteManager;

public:
	Host();
	~Host();

	void init();
	void shutdown();

	void tick(float dt);
	void debugDraw();

	uint32_t allocNetId();

	void addPlayer(Player * player);
	void removePlayer(Player * player);
	Player * findPlayerByNetId(uint32_t netId);

	void spawnPickup(PickupType type, int blockX, int blockY);
	Pickup * grabPickup(int x, int y);
};

extern Host * g_host;
extern Arena * g_hostArena;
extern BulletPool * g_hostBulletPool;
extern NetSpriteManager * g_hostSpriteManager;
