#pragma once

#include "config.h"
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

	float m_inactivityTime;

public:
	MainMenu();
	virtual ~MainMenu() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};
