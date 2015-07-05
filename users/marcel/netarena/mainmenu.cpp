#include "Channel.h"
#include "client.h"
#include "framework.h"
#include "gamedefs.h"
#include "host.h"
#include "main.h"
#include "mainmenu.h"
#include "title.h"
#include "uicommon.h"

static const float kMaxInactivityTime = 30.f;

static Mouse s_lastMouse;
static Gamepad s_lastGamepad;

MainMenu::MainMenu()
{
	m_newGame = new Button(GFX_SX/2, GFX_SY/3, "mainmenu-newgame.png");
	m_findGame = new Button(GFX_SX/2, GFX_SY/3 + 200, "mainmenu-findgame.png");
	m_quitApp = new Button(GFX_SX/2, GFX_SY/3 + 400, "mainmenu-exitgame.png");
}

MainMenu::~MainMenu()
{
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

	if (m_newGame->isClicked())
	{
		logDebug("new game!");

		g_app->startHosting();

		g_app->m_host->m_gameSim.setGameState(kGameState_OnlineMenus);

		g_app->connect("127.0.0.1");
	}
#if !PUBLIC_DEMO_BUILD
	else if (m_findGame->isClicked())
	{
		logDebug("find game!");

		g_app->findGame();
	}
#endif
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

	m_newGame->draw();
#if !PUBLIC_DEMO_BUILD
	m_findGame->draw();
#endif
	m_quitApp->draw();
}
