#pragma once

#include "menu.h"

class Button;
class MenuNav;

class Menu_NewGame : public Menu
{
public:
	Button * m_local;
	Button * m_onlinePublic;
	Button * m_onlinePrivate;
	
	MenuNav * m_menuNav;
	
	Menu_NewGame();
	virtual ~Menu_NewGame();
	
	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};
