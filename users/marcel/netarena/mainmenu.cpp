#include "Channel.h"
#include "client.h"
#include "framework.h"
#include "gamedefs.h"
#include "main.h"
#include "mainmenu.h"

class Button
{
	Sprite * m_sprite;
	int m_x;
	int m_y;
	bool m_isMouseDown;

public:
	Button(int x, int y, const char * filename)
		: m_sprite(new Sprite(filename))
		, m_isMouseDown(false)
	{
		m_x = x - m_sprite->getWidth() / 2;
		m_y = y - m_sprite->getHeight() / 2;
	}

	~Button()
	{
		delete m_sprite;
		m_sprite = 0;
	}

	bool isClicked()
	{
		const bool isInside =
			mouse.x >= m_x &&
			mouse.y >= m_y &&
			mouse.x < m_x + m_sprite->getWidth() &&
			mouse.y < m_y + m_sprite->getHeight();
		const bool isDown = isInside && mouse.isDown(BUTTON_LEFT);
		const bool isClicked = isInside && !isDown && m_isMouseDown;
		if (mouse.wentDown(BUTTON_LEFT))
			m_isMouseDown = isInside;
		if (!mouse.isDown(BUTTON_LEFT))
			m_isMouseDown = false;
		return isClicked;
	}

	void draw()
	{
		m_sprite->drawEx(m_x, m_y);
	}
};

// fixme, make members
static Button * s_newGame = 0;
static Button * s_findGame = 0;
static Button * s_exitGame = 0;

void MainMenu::tick(float dt)
{
	if (s_newGame == 0)
	{
		s_newGame = new Button(GFX_SX/2, GFX_SY/2, "mainmenu-newgame.png");
		s_findGame = new Button(GFX_SX/2, GFX_SY/2 + 200, "mainmenu-findgame.png");
		s_exitGame = new Button(GFX_SX/2, GFX_SY/2 + 400, "mainmenu-exitgame.png");
	}

	if (s_newGame->isClicked())
	{
		logDebug("new game!");

		g_app->startHosting();

		g_app->netSetGameState(kGameState_Menus);

		g_app->connect("127.0.0.1");
	}
	else if (s_findGame->isClicked())
	{
		logDebug("find game!");

		g_app->findGame();
	}
	else if (s_exitGame->isClicked())
	{
		logDebug("exit game!");

		g_app->quit();
	}
}

void MainMenu::draw()
{
	Sprite("mainmenu-back.png").draw();

	if (s_newGame != 0)
	{
		s_newGame->draw();
		s_findGame->draw();
		s_exitGame->draw();
	}
}
