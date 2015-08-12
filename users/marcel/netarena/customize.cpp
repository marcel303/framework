#include "customize.h"
#include "gamedefs.h"
#include "main.h"
#include "uicommon.h"

CustomizeMenu::CustomizeMenu()
	: m_characters(0)
	, m_menuNav(0)
{
	m_characters = new Button(GFX_SX/2, GFX_SY/3, "mainmenu-button.png", "menu-characters", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);

	m_menuNav = new MenuNav();

	if (m_characters)
		m_menuNav->addElem(m_characters);
}

CustomizeMenu::~CustomizeMenu()
{
	delete m_menuNav;

	delete m_characters;
}

void CustomizeMenu::onEnter()
{
}

void CustomizeMenu::onExit()
{
}

bool CustomizeMenu::tick(float dt)
{
	m_menuNav->tick(dt);

	//

	if (m_characters && m_characters->isClicked())
	{
		g_app->m_menuMgr->push(new CharacterMenu());
	}
	else if (gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		return true;
	}

	return false;
}

void CustomizeMenu::draw()
{
	if (m_characters)
		m_characters->draw();
}

//

CharacterMenu::CharacterMenu()
	: m_effects(0)
	, m_skin(0)
	, m_emblem(0)
	, m_testGame(0)
	, m_menuNav(0)
{
	m_effects = new Button(GFX_SX/2, GFX_SY/3, "mainmenu-button.png", "menu-char-effects", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_skin = new Button(GFX_SX/2, GFX_SY/3 + 150, "mainmenu-button.png", "menu-char-skin", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_emblem = new Button(GFX_SX/2, GFX_SY/3 + 300, "mainmenu-button.png", "menu-char-emblem", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_testGame = new Button(GFX_SX/2, GFX_SY/3 + 450, "mainmenu-button.png", "menu-char-test", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);

	m_menuNav = new MenuNav();

	if (m_effects)
		m_menuNav->addElem(m_effects);
	if (m_skin)
		m_menuNav->addElem(m_skin);
	if (m_emblem)
		m_menuNav->addElem(m_emblem);
	if (m_testGame)
		m_menuNav->addElem(m_testGame);
}

CharacterMenu::~CharacterMenu()
{
	delete m_menuNav;

	delete m_effects;
	delete m_skin;
	delete m_emblem;
	delete m_testGame;
}

void CharacterMenu::onEnter()
{
}

void CharacterMenu::onExit()
{
}

bool CharacterMenu::tick(float dt)
{
	m_menuNav->tick(dt);

	// todo : check for button clicks

	if (gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		return true;
	}

	return false;
}

void CharacterMenu::draw()
{
	if (m_effects)
		m_effects->draw();
	if (m_skin)
		m_skin->draw();
	if (m_emblem)
		m_emblem->draw();
	if (m_testGame)
		m_testGame->draw();
}
