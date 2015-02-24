#pragma once

#include "gamesim.h" // fixme, move MAX_PLAYERS to gamedefs

class Button;
class Client;

class CharSelector
{
	void updateButtonLocations();

public:
	Client * m_client;
	int m_playerId;

	Button * m_prevChar;
	Button * m_nextChar;
	Button * m_ready;

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
	Button * m_prevGameMode;
	Button * m_nextGameMode;

	LobbyMenu(Client * client);
	~LobbyMenu();

	void tick(float dt);
	void draw();
};
