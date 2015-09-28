#pragma once

#include "gamedefs.h"
#include "menu.h"

class Button;
class MenuNav;

class MainMenu : public Menu
{
	Button * m_newGame;
	Button * m_findGame;
	Button * m_customize;
	Button * m_options;
	Button * m_quitApp;
	MenuNav * m_menuNav;

	Button * m_socialFb;
	Button * m_socialTw;
	Button * m_campaignGl;
	Button * m_campaignKs;

	float m_animTime;
	float m_inactivityTime;

public:
	MainMenu();
	~MainMenu();

	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};
