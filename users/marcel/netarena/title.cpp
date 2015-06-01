#include "Calc.h"
#include "framework.h"
#include "gamesim.h"
#include "gametypes.h"
#include "main.h"
#include "title.h"

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
		Sound("title/flash.ogg").play();
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
