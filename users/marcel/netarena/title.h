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

class SplashScreen : public Menu
{
	std::string m_filename;
	float m_time;
	float m_duration;
	float m_fadeInDuration;
	float m_fadeOutDuration;
	bool m_hasPlayedSound;
	int m_extraFrames;

public:
	SplashScreen(const char * filename, float duration, float fadeInDuration, float fadeOutDuration);

	virtual void onEnter();
	virtual void onExit();

	virtual bool tick(float dt);
	virtual void draw();
};
