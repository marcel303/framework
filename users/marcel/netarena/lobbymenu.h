#pragma once

#include "gamedefs.h"

class Button;
class Client;
class LobbyMenu;

class CharSelector
{
	void updateButtonLocations();

public:
	Client * m_client;
	LobbyMenu * m_menu;
	int m_playerId;

	Button * m_prevChar;
	Button * m_nextChar;
	Button * m_ready;

	CharSelector(Client * client, LobbyMenu * menu, int playerId);
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
	SpriterState m_joinSpriterState;

	LobbyMenu(Client * client);
	~LobbyMenu();

	void tick(float dt);
	void draw();
};
