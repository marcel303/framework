#pragma once

#include "menu.h"

class Button;
class MenuNav;

class OptioneMenu : public Menu
{
	Button * m_audio;
	Button * m_display;
	Button * m_video;
	MenuNav * m_menuNav;

public:
	OptioneMenu();
	~OptioneMenu();

	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};
