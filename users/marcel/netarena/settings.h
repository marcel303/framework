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
	virtual void onEnter() override { }
	virtual void onExit() override { }

	virtual bool tick(float dt) override;
	virtual void draw() override;
};
