#pragma once

#include "config.h"
#include "gamedefs.h"
#include "menu.h"

class Button;
class ButtonLegend;
class MenuNav;

class MainMenu : public Menu
{
	Button * m_newGame;
	Button * m_findGame;
	Button * m_customize;
	Button * m_options;
	Button * m_quitApp;
	MenuNav * m_menuNav;

	Button * m_controls;
#if ITCHIO_SHOW_URLS
	Button * m_socialFb;
	Button * m_socialTw;
#endif
#if ITCHIO_SHOW_KICKSTARTER
	Button * m_campaignGl;
	Button * m_campaignKs;
#endif

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

class HelpMenu : public Menu
{
	Button * m_back;
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

public:
	HelpMenu();
	~HelpMenu();

	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};
