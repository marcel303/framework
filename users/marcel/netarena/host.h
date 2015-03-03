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

	Host();
	~Host();

	void init();
	void shutdown();

	void tick(float dt);
	void debugDraw();

	PlayerInstanceData * allocPlayer(uint16_t owningChannelId);
	void freePlayer(PlayerInstanceData * player);
	PlayerInstanceData * findPlayerByPlayerId(uint8_t playerId);
	void setPlayerPtrs();
	void clearPlayerPtrs();
};

extern Host * g_host;
