#pragma once

#include "menu.h"

class Settings
{
public:
	void load(const char * filename);
	void save(const char * filename);
};

class SettingsMenu : public Menu
{
public:
	virtual void onEnter() { }
	virtual void onExit() { }

	virtual bool tick(float dt);
	virtual void draw();
};
