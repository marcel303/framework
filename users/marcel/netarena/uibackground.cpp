#include "Ease.h"
#include "framework.h"
#include "uibackground.h"

UiBackground * g_uiBackground = nullptr;

UiBackground::UiBackground()
	: m_state(kState_Hidden)
	, m_animTime(0.f)
{
}

UiBackground::~UiBackground()
{
}

void UiBackground::tick(float dt)
{
	m_animTime += dt;
}

void UiBackground::draw()
{
	const float kBackFadeinTime = .4f;
	const float kCharFadeinTime = 1.f;
	const float kLogoFadeinTime = .6f;

	const float kCharOffsetX = -150.f;

	const float backOpacity = saturate(m_animTime / kBackFadeinTime);
	const float charOpacity = saturate(m_animTime / kCharFadeinTime);
	const float logoOpacity = saturate(m_animTime / kLogoFadeinTime);

	setColorf(1.f, 1.f, 1.f, backOpacity);
	Sprite("ui/menus/menu-background.png").draw();

	setColorf(1.f, 1.f, 1.f, backOpacity);
	Sprite("ui/menus/menu-sidepanels.png").draw();

	setColorf(1.f, 1.f, 1.f, charOpacity);
	Sprite("ui/menus/menu-main-char.png").drawEx(kCharOffsetX * EvalEase(1.f - charOpacity, kEaseType_SineIn, 0.f), 0.f);

	setColorf(1.f, 1.f, 1.f, logoOpacity);
	Sprite("ui/menus/menu-main-logo.png").draw();
}

void UiBackground::setState(State state)
{
	m_state = state;
	m_animTime = 0.f;
}
