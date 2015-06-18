#pragma once

#include "gamedefs.h"

class Button;
class Client;
class LobbyMenu;

#define GRIDBASED_CHARSELECT 1

#if GRIDBASED_CHARSELECT
class CharGrid
{
	Client * m_client;
	LobbyMenu * m_menu;

public:
	CharGrid(Client * client, LobbyMenu * menu);

	void tick(float dt);
	void draw();
};
#endif

#if !GRIDBASED_CHARSELECT
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

	bool isLocalPlayer() const;
};
#endif

class LobbyMenu
{
public:
	Client * m_client;
#if GRIDBASED_CHARSELECT
	CharGrid * m_charGrid;
#endif
#if !GRIDBASED_CHARSELECT
	CharSelector * m_charSelectors[MAX_PLAYERS];
#endif
	Button * m_prevGameMode;
	Button * m_nextGameMode;
	SpriterState m_joinSpriterState;

	LobbyMenu(Client * client);
	~LobbyMenu();

	void tick(float dt);
	void draw();
};
