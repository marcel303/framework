#pragma once

#include "menu.h"

class Button;
class ButtonLegend;
class CheckButton;
class MenuNav;
class Slider;

class OptioneMenu : public Menu
{
	Button * m_back;
	Button * m_audio;
	Button * m_display;
	Button * m_video;
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

public:
	OptioneMenu();
	virtual ~OptioneMenu() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};

class OptioneAudioMenu : public Menu
{
	Button * m_back;
	CheckButton * m_musicEnabled;
	Slider * m_musicVolume;
	CheckButton * m_soundEnabled;
	Slider * m_soundVolume;
	CheckButton * m_announcerEnabled;
	Slider * m_announcerVolume;
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

public:
	OptioneAudioMenu();
	virtual ~OptioneAudioMenu() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};

class OptioneDisplayMenu : public Menu
{
	Button * m_back;
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

public:
	OptioneDisplayMenu();
	virtual ~OptioneDisplayMenu() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};

class OptioneVideoMenu : public Menu
{
	Button * m_back;
	MenuNav * m_menuNav;
	ButtonLegend * m_buttonLegend;

public:
	OptioneVideoMenu();
	virtual ~OptioneVideoMenu() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};
