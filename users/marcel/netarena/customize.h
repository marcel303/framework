#pragma once

#include "gamedefs.h"
#include "gametypes.h"
#include "menu.h"

class Button;
class ButtonLegend;
class MenuNav;
class SpinButton;

class CustomizeMenu : public Menu
{
	Button * m_back;
	Button * m_characters[MAX_CHARACTERS];
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

public:
	CustomizeMenu();
	virtual ~CustomizeMenu() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};

class CharacterMenu : public Menu
{
	int m_characterIndex;
	UserSettings::Char m_charSettings;

	Button * m_back;
	Button * m_effects;
	SpinButton * m_skin;
	SpinButton * m_emblem;
	Button * m_testGame;
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

public:
	CharacterMenu(int characterIndex);
	virtual ~CharacterMenu() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};
