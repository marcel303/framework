#pragma once

#include "menu.h"

class Sound;

class Title : public Menu
{
	Sound * m_logoSound;
	float m_logoAnim;
	float m_logoFlash;

public:
	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};
