#pragma once

#include "menu.h"

class Sound;

class Title : public Menu
{
	Sound * m_logoSound;
	float m_logoAnim;
	float m_logoFlash;

public:
	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};

class SplashScreen : public Menu
{
	std::string m_filename;
	float m_time;
	float m_duration;
	float m_fadeInDuration;
	float m_fadeOutDuration;
	bool m_hasPlayedSound;
	int m_extraFrames;
	bool m_skip;

public:
	SplashScreen(const char * filename, float duration, float fadeInDuration, float fadeOutDuration);

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual bool tick(float dt) override;
	virtual void draw() override;
};
