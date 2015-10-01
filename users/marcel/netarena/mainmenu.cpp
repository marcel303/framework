#define NOMINMAX // grr windows

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
	, m_campaignGl(0)
	, m_campaignKs(0)
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
	const int glSx = 316;
	const int glSy = 171;
	const int ksSx = 300;
	const int ksSy = 86;
	m_socialFb = new Button(165 + socialSx/2, 925 + socialSy/2, "itch-social-fb.png", "", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_socialTw = new Button(165 + socialSx/2, 830 + socialSy/2, "itch-social-tw.png", "", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_campaignGl = new Button(1501 + glSx/2, 821 + glSy/2, "itch-campaign-gl.png", "", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_campaignKs = new Button(1501 + ksSx/2, 721 + ksSy/2, "itch-campaign-ks.png", "", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
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
	if (m_campaignGl)
		m_menuNav->addElem(m_campaignGl);
	if (m_campaignKs)
		m_menuNav->addElem(m_campaignKs);
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
	delete m_campaignGl;
	delete m_campaignKs;
}

void MainMenu::onEnter()
{
	m_animTime = 0.f;

	m_inactivityTime = 0.f;
	s_lastMouse = mouse;
	s_lastGamepad = gamepad[0];

	const int animX = 100;
	m_newGame->setAnimation(animX, 0, 0.f, .4f);
	m_quitApp->setAnimation(animX, 0, 0.f, .4f);
}

void MainMenu::onExit()
{
}

bool MainMenu::tick(float dt)
{
	m_animTime += dt;

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

		if (g_app->startHosting())
		{
			g_app->m_host->m_gameSim.setGameState(kGameState_OnlineMenus);

			g_app->connect("127.0.0.1");
		}
		else
		{
			// todo : show error dialog
		}
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
	else if (m_campaignGl && m_campaignGl->isClicked())
	{
		openBrowserWithUrl("http://steamcommunity.com/sharedfiles/filedetails/?id=499076298");
	}
	else if (m_campaignKs && m_campaignKs->isClicked())
	{
		openBrowserWithUrl("https://www.youtube.com/watch?v=dQw4w9WgXcQ");
	}
#if !ITCHIO_BUILD
	else if (m_inactivityTime >= (g_devMode ? 5.f : kMaxInactivityTime))
	{
		g_app->m_menuMgr->push(new Title());
	}
#endif

	return false;
}

#if ITCHIO_BUILD
static void drawParticles(float ballY)
{
	struct Part
	{
		Part()
		{
			memset(this, 0, sizeof(Part));
		}

		float x;
		float y;
		float angle;
		float life;
		float lifeRcp;
		int type;

		void tick(float dt)
		{
			life = life - dt;
			if (life < 0.f)
				life = 0.f;
		}

		void draw(float dy) const
		{
			if (life == 0.f)
				return;

			const float scale = 1.f - life * lifeRcp * .5f;
			const float opacity = std::sinf(life * lifeRcp * M_PI);

			setColorf(1.f, 1.f, 1.f, opacity);
			if (type == 0)
				Sprite("itch-ball-part1.png").drawEx(x, y + dy, angle, scale, scale, false, FILTER_LINEAR);
			else
				Sprite("itch-ball-part2.png").drawEx(x, y + dy, angle, scale, scale, false, FILTER_LINEAR);
		}
	};

	const int ballXBase = 1462;
	const int ballYBase = 426;

	const float dt = framework.timeStep;

	static const int numParts = 16;
	static Part parts[numParts];
	static float spawnTime = 0.f;
	static int spawnIdx = 0;

	spawnTime -= dt;
	if (spawnTime < 0.f)
		spawnTime = 0.f;

	if (spawnTime == 0.f)
	{
		bool large = (spawnIdx % 10) == 0;
		spawnIdx++;

		spawnTime = large ? 0.f : 1.f;

		for (int j = 0; j < numParts; ++j)
		{
			if (parts[j].life == 0.f)
			{
				parts[j].x = ballXBase;
				parts[j].y = ballYBase;
				parts[j].angle = rand() % 360;
				parts[j].type = large ? 0 : 1;
				parts[j].life = parts[j].type == 0 ? 12.f : 4.f;
				parts[j].lifeRcp = 1.f / parts[j].life;
				break;
			}
		}
	}

	for (int i = 0; i < numParts; ++i)
		parts[i].tick(dt);

	for (int i = 0; i < numParts; ++i)
		if (parts[i].type == 0)
			parts[i].draw(ballY);
	for (int i = 0; i < numParts; ++i)
		if (parts[i].type == 1)
			parts[i].draw(ballY);
}
#endif

void MainMenu::draw()
{
	const float kBackFadeinTime = .2f;
	const float kLogoFadeinTime = .5f;
	const float kBallFloatTime = 10.f;
	const float kBallFloatPixs = 30.f;
	const float kBallFloatOffs = -.5;

#if ITCHIO_BUILD
	const float backOpacity = saturate(m_animTime / kBackFadeinTime);
	const float logoOpacity = saturate(m_animTime / kLogoFadeinTime);
	const float ball1Y = std::sinf((m_animTime                 ) * 2.f * M_PI / kBallFloatTime) * kBallFloatPixs;
	const float ball2Y = std::sinf((m_animTime + kBallFloatOffs) * 2.f * M_PI / kBallFloatTime) * kBallFloatPixs;

	setColorf(1.f, 1.f, 1.f, 1.f, backOpacity);
	Sprite("itch-mainmenu-back.png").draw();

	drawParticles(ball1Y);

	setColorf(1.f, 1.f, 1.f, 1.f, backOpacity);
	Sprite("itch-mainmenu-ball1.png").drawEx(0.f, ball1Y, 0.f, 1.f, 1.f, false, FILTER_LINEAR);
	Sprite("itch-mainmenu-ball2.png").drawEx(0.f, ball2Y, 0.f, 1.f, 1.f, false, FILTER_LINEAR);

	setColorf(1.f, 1.f, 1.f, logoOpacity);
	Sprite("itch-mainmenu-logo.png").draw();
#else
	setColor(colorWhite);
	Sprite("mainmenu-back.png").draw();
#endif

	setColor(colorWhite);

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
	if (m_campaignGl)
		m_campaignGl->draw();
	if (m_campaignKs)
		m_campaignKs->draw();
}
