#pragma once

#include "menu.h"

class Button;
class MenuNav;

class CustomizeMenu : public Menu
{
	Button * m_characters;
	MenuNav * m_menuNav;

public:
	CustomizeMenu();
	~CustomizeMenu();

	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};

class CharacterMenu : public Menu
{
	Button * m_effects;
	Button * m_skin;
	Button * m_emblem;
	Button * m_testGame;
	MenuNav * m_menuNav;

public:
	CharacterMenu();
	~CharacterMenu();

	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};
