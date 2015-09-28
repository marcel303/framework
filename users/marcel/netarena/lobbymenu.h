#pragma once

#include "gamedefs.h"

class Button;
class Client;
class LobbyMenu;

class CharGrid
{
	Client * m_client;
	LobbyMenu * m_menu;

public:
	CharGrid(Client * client, LobbyMenu * menu);

	void tick(float dt);
	void draw();

	static void characterIndexToXY(int characterIndex, int & x, int & y);
	static void modulateXY(int & x, int & y);
	static int xyToCharacterIndex(int x, int y);
	static bool isValidGridCell(int x, int y);
};

class LobbyMenu
{
public:
	Client * m_client;
	CharGrid * m_charGrid;
	Button * m_prevGameMode;
	Button * m_nextGameMode;
	SpriterState m_joinSpriterState;

	LobbyMenu(Client * client);
	~LobbyMenu();

	void tick(float dt);
	void draw();
};
