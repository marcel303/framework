#pragma once

#include <stdint.h>
#include <vector>
#include "gamedefs.h"
#include "libnet_forward.h"

class Arena;
class BulletPool;
class GameSim;
class LobbyMenu;
class NetSpriteManager;
struct ParticleSpawnInfo;
struct Player;
class PlayerNetObject;

class Client
{
public:
	Channel * m_channel;

	LobbyMenu * m_lobbyMenu;

	GameSim * m_gameSim;

	BitStream * m_syncStream;

	bool m_isDesync;

	std::vector<PlayerNetObject*> m_players;

	Client();
	~Client();

	void initialize(Channel * channel);

	void tick(float dt);
	void draw();
	void drawConnecting();
	void drawMenus();
	void drawPlay();
	void drawRoundComplete();
	void debugDraw();

	void addPlayer(PlayerNetObject * player);
	void removePlayer(PlayerNetObject * player);
	PlayerNetObject * findPlayerByPlayerId(uint8_t playerId);
	void setPlayerPtrs();
	void clearPlayerPtrs();

	void spawnParticles(const ParticleSpawnInfo & spawnInfo);
};
