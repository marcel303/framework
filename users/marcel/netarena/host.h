#pragma once

#include <vector>
#include "gamesim.h"

class Arena;
class BulletPool;
class NetSpriteManager;
struct Player;
class PlayerInstanceData;

class Host
{
public:
	GameSim m_gameSim;

	int m_nextRoundNumber;

	// round complete game state
	uint64_t m_roundCompleteTimer;

	Host();
	~Host();

	void init();
	void shutdown();

	void tick(float dt);
	void tickMenus(float dt);
	void tickPlay(float dt);
	void tickRoundComplete(float dt);
	void debugDraw();

	void syncNewClient(Channel * channel);

	void newGame();
	void newRound(const char * mapOverride);
	void endRound();

	PlayerInstanceData * allocPlayer(uint16_t owningChannelId);
	void freePlayer(PlayerInstanceData * player);
	PlayerInstanceData * findPlayerByPlayerId(uint8_t playerId);
	void setPlayerPtrs();
	void clearPlayerPtrs();
};

extern Host * g_host;
