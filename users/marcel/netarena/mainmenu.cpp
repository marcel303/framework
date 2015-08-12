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
{
	if (!PUBLIC_DEMO_BUILD || ((std::string)g_connect).empty())
		m_newGame = new Button(GFX_SX/2, GFX_SY/3, "mainmenu-newgame.png", "menu-newgame");
	if (!PUBLIC_DEMO_BUILD || !((std::string)g_connect).empty())
		m_findGame = new Button(GFX_SX/2, GFX_SY/3 + 150, "mainmenu-findgame.png", "menu-findgame");
	if (!PUBLIC_DEMO_BUILD)
		m_customize = new Button(GFX_SX/2, GFX_SY/3 + 300, "mainmenu-customize.png", "menu-customize");
	if (!PUBLIC_DEMO_BUILD)
		m_options = new Button(GFX_SX/2, GFX_SY/3 + 450, "mainmenu-options.png", "menu-options");
	m_quitApp = new Button(GFX_SX/2, GFX_SY/3 + 600, "mainmenu-exitgame.png", "menu-quit");

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
}

MainMenu::~MainMenu()
{
	delete m_menuNav;

	delete m_newGame;
	delete m_findGame;
	delete m_customize;
	delete m_options;
	delete m_quitApp;
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
		memcmp(&s_lastGamepad, &gamepad[0], sizeof(Gamepad)) == 0;

	if (isInactive)
		m_inactivityTime += dt;
	else
	{
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
	else if (m_inactivityTime >= (g_devMode ? 5.f : kMaxInactivityTime))
	{
		g_app->m_menuMgr->push(new Title());
	}

	return false;
}

void MainMenu::draw()
{
	setColor(colorWhite);
	Sprite("mainmenu-back.png").draw();

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
}
