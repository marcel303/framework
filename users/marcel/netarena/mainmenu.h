#pragma once

#include "menu.h"

class Button;
class MenuNav;

class MainMenu : public Menu
{
	Button * m_newGame;
	Button * m_findGame;
	Button * m_quitApp;
	MenuNav * m_menuNav;

	float m_inactivityTime;

public:
	MainMenu();
	~MainMenu();

	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};
