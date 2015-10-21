#include "Calc.h"
#include "framework.h"
#include "gamesim.h"
#include "gametypes.h"
#include "main.h"
#include "title.h"

OPTION_EXTERN(bool, g_noSound);

const float kLogoDuration = 1.5f;
const float kLogoX = 300.f;
const float kLogoBegin = -450.f;
const float kLogoEnd = 200.f;
const float kLogoFlashBegin = 1.7f;
const float kLogoFlashDuration = .4f;
const float kTitleDuration = 6.f;

static Curve s_logoCurve;
static Curve s_logoFlashCurve;

void Title::onEnter()
{
	m_logoSound = new Sound("title/laugh.ogg");
	if (!g_noSound)
		m_logoSound->play();

	m_logoAnim = 0.f;
	m_logoFlash = -1.f;

	s_logoCurve.makeLinear(kLogoBegin, kLogoEnd);
}

void Title::onExit()
{
	//m_logoSound->stop();
	delete m_logoSound;
	m_logoSound = 0;
}

bool Title::tick(float dt)
{
	m_logoAnim += dt;

	if (m_logoFlash >= 0.f)
		m_logoFlash = Calc::Max(0.f, m_logoFlash - dt);

	if (m_logoFlash == -1.f && m_logoAnim >= kLogoFlashBegin)
	{
		m_logoFlash = kLogoFlashDuration;
		g_app->playSound("title/flash.ogg");
	}

	if (m_logoAnim >= kTitleDuration)
		return true;

	if (g_keyboardLock == 0 && (g_uiInput->wentDown(INPUT_BUTTON_A) || g_uiInput->wentDown(INPUT_BUTTON_START)))
		return true;

	return false;
}

void Title::draw()
{
	setColorMode(COLOR_ADD);
	{
		setColor(255, 127, 63, 255, m_logoFlash >= 0.f ? m_logoFlash * 63.f : 0.f);
		Sprite("title/back.png").drawEx(0.f, 0.f);

		setColor(255, 127, 63, 255, m_logoFlash >= 0.f ? m_logoFlash * 255.f : 0.f);
		Sprite("title/logo.png").drawEx(
			kLogoX,
			s_logoCurve.eval(m_logoAnim / kLogoDuration));
	}
	setColorMode(COLOR_MUL);
}

//

SplashScreen::SplashScreen(const char * filename, float duration, float fadeInDuration, float fadeOutDuration)
	: m_filename(filename)
	, m_time(0.f)
	, m_duration(duration)
	, m_fadeInDuration(fadeInDuration)
	, m_fadeOutDuration(fadeOutDuration)
	, m_hasPlayedSound(false)
	, m_extraFrames(0)
	, m_skip(false)
{
}

void SplashScreen::onEnter()
{
	m_time = 0.f;
	m_hasPlayedSound = false;
	m_extraFrames = 0;
}

void SplashScreen::onExit()
{
}


bool SplashScreen::tick(float dt)
{
	m_time += dt * (m_skip ? 5.f : 1.f);

	if (m_time >= m_fadeInDuration/2.5f && !m_hasPlayedSound)
	{
		m_hasPlayedSound = true;
		g_app->playSound("title/splash.ogg");
	}

	if (m_time >= m_duration)
	{
		m_extraFrames++;
		if (m_extraFrames == 4)
			return true;
	}

	if (g_keyboardLock == 0 && mouse.wentDown(BUTTON_LEFT))
		m_skip = true;

	if (g_keyboardLock == 0 && (g_uiInput->wentDown(INPUT_BUTTON_A) || g_uiInput->wentDown(INPUT_BUTTON_START)))
		m_skip = true;

	if (m_skip && m_time > m_fadeInDuration && m_time < m_duration - m_fadeOutDuration)
		m_time = m_duration - m_fadeOutDuration;

	return false;
}

void SplashScreen::draw()
{
	float c = 1.f;
	if (m_time < m_fadeInDuration)
		c = saturate(m_time / m_fadeInDuration);
	if (m_time > m_duration - m_fadeOutDuration)
		c = saturate((m_duration - m_time) / m_fadeOutDuration);
	setColorf(c, c, c, 1.f);
	Sprite(m_filename.c_str()).drawEx(0.f, 0.f);
}
