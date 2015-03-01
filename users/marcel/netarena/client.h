#pragma once

#include <list>
#include <stdint.h>
#include <string>
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
class TextField;

class Client
{
public:
	Channel * m_channel;

	LobbyMenu * m_lobbyMenu;

	TextField * m_textChat;

	GameSim * m_gameSim;

	BitStream * m_syncStream;

	bool m_isDesync;

	std::vector<PlayerInstanceData*> m_players;

	struct TextChatLog
	{
		int playerIndex;
		int characterIndex;
		std::string playerDisplayName;
		std::string message;
	};
	std::list<TextChatLog> m_textChatLog;
	float m_textChatFade;

	Client();
	~Client();

	void initialize(Channel * channel);

	void tick(float dt);
	void draw();
	void drawConnecting();
	void drawMenus();
	void drawPlay();
	void drawRoundComplete();
	void drawTextChat();
	void debugDraw();

	void addPlayer(PlayerInstanceData * player);
	void removePlayer(PlayerInstanceData * player);
	PlayerInstanceData * findPlayerByPlayerId(uint8_t playerId);
	void setPlayerPtrs();
	void clearPlayerPtrs();

	void spawnParticles(const ParticleSpawnInfo & spawnInfo);

	void addTextChat(int playerIndex, const std::string & text);
};
