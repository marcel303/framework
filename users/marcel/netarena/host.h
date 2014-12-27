#pragma once

#include <vector>
#include "gamedefs.h" // for ENABLE_CLIENT_SIMULATION
#include "gamesim.h"

class Arena;
class BulletPool;
class NetSpriteManager;
struct Player;
class PlayerNetObject;

class Host
{
	friend class App; // fixme, for m_gameSim
	friend class Arena;
	friend class BulletPool;
	friend struct Player; // fixme, for m_players array

	GameSim m_gameSim;

	std::vector<PlayerNetObject*> m_players;
	uint32_t m_nextNetId;
	std::vector<int> m_freePlayerIds;

	int m_nextRoundNumber;

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

	PlayerNetObject * allocPlayer(uint16_t owningChannelId);
	void addPlayer(PlayerNetObject * player);
	void removePlayer(PlayerNetObject * player);
	PlayerNetObject * findPlayerByNetId(uint32_t netId);
	void setPlayerPtrs();
	void clearPlayerPtrs();

	void trySpawnPickup(PickupType type);
	void spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY);
	Pickup * grabPickup(int x1, int y1, int x2, int y2);
};

extern Host * g_host;
extern NetSpriteManager * g_hostSpriteManager;
