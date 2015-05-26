#pragma once

#include "menu.h"

class Title : public Menu
{
public:
	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};
