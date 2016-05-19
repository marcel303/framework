#pragma once

#include "menu.h"

class Button;
class ButtonLegend;
class MenuNav;

class HelpMenu : public Menu
{
	Button * m_back;
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

public:
	HelpMenu();
	virtual ~HelpMenu() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};
