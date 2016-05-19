#include "framework.h"
#include "gamedefs.h"
#include "main.h"
#include "menu_controls.h"
#include "uicommon.h"

HelpMenu::HelpMenu()
	: m_back(0)
	, m_menuNav(0)
	, m_buttonLegend(0)
{
	m_back = new Button(0, 0, "mainmenu-button.png", "mainmenu-button.png", 0, MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);

	m_menuNav = new MenuNav();

	m_buttonLegend = new ButtonLegend();
	m_buttonLegend->addElem(ButtonLegend::kButtonId_B, m_back, "ui-legend-back");
	m_buttonLegend->addElem(ButtonLegend::kButtonId_ESCAPE, m_back, "ui-legend-back");
}

HelpMenu::~HelpMenu()
{
	delete m_buttonLegend;

	delete m_menuNav;

	delete m_back;
}

void HelpMenu::onEnter()
{
	inputLockAcquire(); // fixme : remove. needed for escape = quit hack for now
}

void HelpMenu::onExit()
{
	g_app->saveUserSettings();

	inputLockRelease(); // fixme : remove. needed for escape = quit hack for now
}

bool HelpMenu::tick(float dt)
{
	m_menuNav->tick(dt);

	m_buttonLegend->tick(dt, UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	//

	if (m_back->isClicked() || gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		g_app->playSound("ui/sounds/menu-back.ogg");
		return true;
	}

	return false;
}

void HelpMenu::draw()
{
	setColor(colorWhite);
#if ITCHIO_BUILD
	Sprite("itch-controls.png").draw();
#else
	Sprite("ui/controls.png").draw();
#endif

	m_buttonLegend->draw(UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);
}
