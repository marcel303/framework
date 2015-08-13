#pragma once

#include "gamedefs.h"
#include "menu.h"

class Button;
class ButtonLegend;
class MenuNav;
class SpinButton;

class CustomizeMenu : public Menu
{
	Button * m_characters[MAX_CHARACTERS];
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

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
	int m_characterIndex;

	Button * m_effects;
	SpinButton * m_skin;
	SpinButton * m_emblem;
	Button * m_testGame;
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

public:
	CharacterMenu(int characterIndex);
	~CharacterMenu();

	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};
