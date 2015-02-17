#pragma once

#include "gamesim.h" // fixme, move MAX_PLAYERS to gamedefs

class Button;
class Client;

class CharSelector
{
public:
	Client * m_client;
	int m_playerId;

	Button * m_prevChar;
	Button * m_nextChar;

	CharSelector(Client * client, int playerId);
	~CharSelector();

	void tick(float dt);
	void draw();
};

class LobbyMenu
{
public:
	Client * m_client;
	CharSelector * m_charSelectors[MAX_PLAYERS];

	LobbyMenu(Client * client);
	~LobbyMenu();

	void tick(float dt);
	void draw();
};
