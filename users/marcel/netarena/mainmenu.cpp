#include "Channel.h"
#include "client.h"
#include "customize.h"
#include "framework.h"
#include "gamedefs.h"
#include "host.h"
#include "main.h"
#include "mainmenu.h"
#include "optione.h"
#include "title.h"
#include "uicommon.h"

#ifdef WIN32
#include <Shellapi.h>
static void openBrowserWithUrl(const char * url)
{
	ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}
#else
static void openBrowserWithUrl(const char * url) { }
#endif

OPTION_EXTERN(std::string, g_connect);

static const float kMaxInactivityTime = 30.f;

static Mouse s_lastMouse;
static Gamepad s_lastGamepad;

MainMenu::MainMenu()
	: m_newGame(0)
	, m_findGame(0)
	, m_customize(0)
	, m_options(0)
	, m_quitApp(0)
	, m_menuNav(0)
	, m_socialFb(0)
	, m_socialTw(0)
{
	if (!PUBLIC_DEMO_BUILD || ((std::string)g_connect).empty())
		m_newGame = new Button(GFX_SX/2, GFX_SY/3, "mainmenu-button.png", "menu-newgame", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
#if ENABLE_NETWORKING
	if (!PUBLIC_DEMO_BUILD || !((std::string)g_connect).empty())
		m_findGame = new Button(GFX_SX/2, GFX_SY/3 + 150, "mainmenu-button.png", "menu-findgame", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
#endif
#if ENABLE_OPTIONS
	m_customize = new Button(GFX_SX/2, GFX_SY/3 + 300, "mainmenu-button.png", "menu-customize", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_options = new Button(GFX_SX/2, GFX_SY/3 + 450, "mainmenu-button.png", "menu-options", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
#endif
	m_quitApp = new Button(GFX_SX/2, GFX_SY/3 + 600, "mainmenu-button.png", "menu-quit", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);

#if ITCHIO_BUILD
	m_newGame->setPosition(670 + m_newGame->m_sprite->getWidth()/2, 610 + m_newGame->m_sprite->getHeight()/2);
	m_quitApp->setPosition(670 + m_newGame->m_sprite->getWidth()/2, 765 + m_newGame->m_sprite->getHeight()/2);
	const int socialSx = 385;
	const int socialSy = 85;
	m_socialFb = new Button(165 + socialSx/2, 925 + socialSy/2, "itch-social-fb.png", "", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_socialTw = new Button(165 + socialSx/2, 830 + socialSy/2, "itch-social-tw.png", "", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
#endif

	m_menuNav = new MenuNav();

	if (m_newGame)
		m_menuNav->addElem(m_newGame);
	if (m_findGame)
		m_menuNav->addElem(m_findGame);
	if (m_customize)
		m_menuNav->addElem(m_customize);
	if (m_options)
		m_menuNav->addElem(m_options);
	if (m_quitApp)
		m_menuNav->addElem(m_quitApp);

	if (m_socialFb)
		m_menuNav->addElem(m_socialFb);
	if (m_socialTw)
		m_menuNav->addElem(m_socialTw);
}

MainMenu::~MainMenu()
{
	delete m_menuNav;

	delete m_newGame;
	delete m_findGame;
	delete m_customize;
	delete m_options;
	delete m_quitApp;

	delete m_socialFb;
	delete m_socialTw;
}

void MainMenu::onEnter()
{
	m_inactivityTime = 0.f;
	s_lastMouse = mouse;
	s_lastGamepad = gamepad[0];
}

void MainMenu::onExit()
{
}

bool MainMenu::tick(float dt)
{
	// inactivity check

	const bool isInactive =
		memcmp(&s_lastMouse, &mouse, sizeof(Mouse)) == 0 &&
		memcmp(&s_lastGamepad, &gamepad[0], sizeof(Gamepad)) == 0 &&
		keyboard.isIdle();

	if (isInactive)
		m_inactivityTime += dt;
	else
	{
		m_inactivityTime = 0.f;

		s_lastMouse = mouse;
		s_lastGamepad = gamepad[0];
	}

	//

	m_menuNav->tick(dt);

	//

	if (m_newGame && m_newGame->isClicked())
	{
		logDebug("new game!");

		g_app->startHosting();

		g_app->m_host->m_gameSim.setGameState(kGameState_OnlineMenus);

		g_app->connect("127.0.0.1");
	}
	else if (m_findGame && m_findGame->isClicked())
	{
		logDebug("find game!");

		g_app->findGame();
	}
	else if (m_customize && m_customize->isClicked())
	{
		g_app->m_menuMgr->push(new CustomizeMenu());
	}
	else if (m_options && m_options->isClicked())
	{
		g_app->m_menuMgr->push(new OptioneMenu());
	}
	else if (m_quitApp->isClicked())
	{
		logDebug("exit game!");

		g_app->quit();
	}
	else if (m_socialFb && m_socialFb->isClicked())
	{
		openBrowserWithUrl("https://www.facebook.com/ripostegame");
	}
	else if (m_socialTw && m_socialTw->isClicked())
	{
		openBrowserWithUrl("https://twitter.com/damajogames");
	}
#if !ITCHIO_BUILD
	else if (m_inactivityTime >= (g_devMode ? 5.f : kMaxInactivityTime))
	{
		g_app->m_menuMgr->push(new Title());
	}
#endif

	return false;
}

void MainMenu::draw()
{
	setColor(colorWhite);
#if ITCHIO_BUILD
	Sprite("itch-mainmenu-back.png").draw();
#else
	Sprite("mainmenu-back.png").draw();
#endif

	if (m_newGame)
		m_newGame->draw();
	if (m_findGame)
		m_findGame->draw();
	if (m_customize)
		m_customize->draw();
	if (m_options)
		m_options->draw();
	if (m_quitApp)
		m_quitApp->draw();

	if (m_socialFb)
		m_socialFb->draw();
	if (m_socialTw)
		m_socialTw->draw();
}
