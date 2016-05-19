#include "gamedefs.h"
#include "menu_newgame.h"
#include "uicommon.h"

Menu_NewGame::Menu_NewGame()
	: m_local(nullptr)
	, m_onlinePublic(nullptr)
	, m_onlinePrivate(nullptr)
{
	m_menuNav = new MenuNav();

	m_local = new Button(0, 0, "mainmenu-button.png", "mainmenu-button.png", "menu-newgame-local", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	m_onlinePublic = new Button(0, 0, "mainmenu-button.png", "mainmenu-button.png", "menu-newgame-online-public", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	m_onlinePrivate = new Button(0, 0, "mainmenu-button.png", "mainmenu-button.png", "menu-newgame-online-private", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	
	m_menuNav->addElem(m_local);
	m_menuNav->addElem(m_onlinePublic);
	m_menuNav->addElem(m_onlinePrivate);
}

Menu_NewGame::~Menu_NewGame()
{
	delete m_menuNav;
	
	delete m_local;
	delete m_onlinePublic;
	delete m_onlinePrivate;
}

void Menu_NewGame::onEnter()
{
}

void Menu_NewGame::onExit()
{
}

bool Menu_NewGame::tick(float dt)
{
	m_menuNav->tick(dt);
	
	if (m_local->isClicked())
	{
		return true;
	}
	else if (m_onlinePublic->isClicked())
	{
		return true;
	}
	else if (m_onlinePrivate->isClicked())
	{
		return true;
	}
	
	return false;
}

void Menu_NewGame::draw()
{
	m_local->draw();
	m_onlinePublic->draw();
	m_onlinePrivate->draw();
}
