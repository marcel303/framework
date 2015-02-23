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
class PlayerInstanceData;
class TextChat;

class Client
{
public:
	Channel * m_channel;

	LobbyMenu * m_lobbyMenu;

	TextChat * m_textChat;

	GameSim * m_gameSim;

	BitStream * m_syncStream;

	bool m_isDesync;

	std::vector<PlayerInstanceData*> m_players;

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

	void addPlayer(PlayerInstanceData * player);
	void removePlayer(PlayerInstanceData * player);
	PlayerInstanceData * findPlayerByPlayerId(uint8_t playerId);
	void setPlayerPtrs();
	void clearPlayerPtrs();

	void spawnParticles(const ParticleSpawnInfo & spawnInfo);
};
