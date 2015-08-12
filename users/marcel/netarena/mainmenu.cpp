#include "Channel.h"
#include "client.h"
#include "framework.h"
#include "gamedefs.h"
#include "host.h"
#include "main.h"
#include "mainmenu.h"
#include "title.h"
#include "uicommon.h"

OPTION_EXTERN(std::string, g_connect);

static const float kMaxInactivityTime = 30.f;

static Mouse s_lastMouse;
static Gamepad s_lastGamepad;

MainMenu::MainMenu()
	: m_newGame(0)
	, m_findGame(0)
	, m_quitApp(0)
	, m_menuNav(0)
{
	if (!PUBLIC_DEMO_BUILD || ((std::string)g_connect).empty())
		m_newGame = new Button(GFX_SX/2, GFX_SY/3, "mainmenu-newgame.png");
	if (!PUBLIC_DEMO_BUILD || !((std::string)g_connect).empty())
		m_findGame = new Button(GFX_SX/2, GFX_SY/3 + 200, "mainmenu-findgame.png");
	m_quitApp = new Button(GFX_SX/2, GFX_SY/3 + 400, "mainmenu-exitgame.png");

	m_menuNav = new MenuNav();
	m_menuNav->addElem(m_newGame);
	m_menuNav->addElem(m_findGame);
	m_menuNav->addElem(m_quitApp);
}

MainMenu::~MainMenu()
{
	delete m_menuNav;

	delete m_newGame;
	delete m_findGame;
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
	else if (m_quitApp->isClicked())
	{
		logDebug("exit game!");

		g_app->quit();
	}

	if (m_inactivityTime >= (g_devMode ? 5.f : kMaxInactivityTime))
		g_app->m_menuMgr->push(new Title());

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
	m_quitApp->draw();
}
