#include "Channel.h"
#include "client.h"
#include "framework.h"
#include "gamedefs.h"
#include "host.h"
#include "main.h"
#include "mainmenu.h"
#include "uicommon.h"

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

void MainMenu::tick(float dt)
{
	if (m_newGame->isClicked())
	{
		logDebug("new game!");

		g_app->startHosting();

		g_app->m_host->m_gameSim.setGameState(kGameState_OnlineMenus);

		g_app->connect("127.0.0.1");
	}
	else if (m_findGame->isClicked())
	{
		logDebug("find game!");

		g_app->findGame();
	}
	else if (m_quitApp->isClicked())
	{
		logDebug("exit game!");

		g_app->quit();
	}
}

void MainMenu::draw()
{
	setColor(colorWhite);
	Sprite("mainmenu-back.png").draw();

	m_newGame->draw();
	m_findGame->draw();
	m_quitApp->draw();
}
