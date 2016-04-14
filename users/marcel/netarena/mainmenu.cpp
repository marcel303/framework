#include "Channel.h"
#include "client.h"
#include "customize.h"
#include "Ease.h"
#include "framework.h"
#include "gamedefs.h"
#include "host.h"
#include "main.h"
#include "mainmenu.h"
#include "optione.h"
#include "title.h"
#include "uicommon.h"

#define DRAW_BACK_V1 0
#define DRAW_BACK_V2 1

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
	, m_controls(0)
#if ITCHIO_SHOW_URLS
	, m_socialFb(0)
	, m_socialTw(0)
#endif
#if ITCHIO_SHOW_KICKSTARTER
	, m_campaignGl(0)
	, m_campaignKs(0)
#endif
{
#if DRAW_BACK_V1
	if (!PUBLIC_DEMO_BUILD || ((std::string)g_connect).empty())
		m_newGame = new Button(GFX_SX/2, GFX_SY/3, "mainmenu-button.png", "menu-newgame", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
#if ENABLE_NETWORKING
	if (!PUBLIC_DEMO_BUILD || !((std::string)g_connect).empty())
		m_findGame = new Button(GFX_SX/2, GFX_SY/3 + 150, "mainmenu-button.png", "menu-findgame", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
#endif
#if ENABLE_OPTIONS
	m_customize = new Button(GFX_SX/2, GFX_SY/3 + 300, "mainmenu-button.png", "menu-customize", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_options = new Button(GFX_SX/2, GFX_SY/3 + 450, "mainmenu-button.png", "menu-options", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
#endif
	m_quitApp = new Button(GFX_SX/2, GFX_SY/3 + 600, "mainmenu-button.png", "menu-quit", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);

#if ITCHIO_BUILD
	m_newGame->setPosition(670 + m_newGame->m_sprite->getWidth()/2, 610 + m_newGame->m_sprite->getHeight()/2);
	m_quitApp->setPosition(670 + m_newGame->m_sprite->getWidth()/2, 765 + m_newGame->m_sprite->getHeight()/2);
	const int socialSx = 385;
	const int socialSy = 85;
	const int glSx = 316;
	const int glSy = 171;
	const int ksSx = 300;
	const int ksSy = 86;
	
	m_controls = new Button(1340, 1010, "mainmenu-button-small.png", "Controls", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y/2, MAINMENU_BUTTON_TEXT_SIZE/2);
	m_controls->m_moveSet[std::pair<int, int>(0, -1)] = m_quitApp;
#if ITCHIO_SHOW_URLS
	m_socialFb = new Button(165 + socialSx/2, 925 + socialSy/2, "itch-social-fb.png", "", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_socialTw = new Button(165 + socialSx/2, 830 + socialSy/2, "itch-social-tw.png", "", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
#endif

#if ITCHIO_SHOW_KICKSTARTER
	m_campaignGl = new Button(1501 + glSx/2, 821 + glSy/2, "itch-campaign-gl.png", "", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_campaignKs = new Button(1501 + ksSx/2, 721 + ksSy/2, "itch-campaign-ks.png", "", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_campaignKs->m_moveSet[std::pair<int, int>(-1, 0)] = m_newGame;
#endif
#endif
#endif

#if DRAW_BACK_V2
	int x = 1510;
	int y = 420;
	const int stepX = 0;
	const int stepY = 110;

	m_newGame = new Button(x, y, "ui/menus/menu-main-button-back.png", "ui/menus/menu-main-button-selected.png", "menu-newgame", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	x += stepX; y += stepY;

	m_customize = new Button(x, y, "ui/menus/menu-main-button-back.png", "ui/menus/menu-main-button-selected.png", "menu-customize", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	x += stepX; y += stepY;

	m_options = new Button(x, y, "ui/menus/menu-main-button-back.png", "ui/menus/menu-main-button-selected.png", "menu-options", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	x += stepX; y += stepY;

	m_quitApp = new Button(x, y, "ui/menus/menu-main-button-back.png", "ui/menus/menu-main-button-selected.png", "menu-quit", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	x += stepX; y += stepY;
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

	if (m_controls)
		m_menuNav->addElem(m_controls);
#if ITCHIO_SHOW_URLS
	if (m_socialFb)
		m_menuNav->addElem(m_socialFb);
	if (m_socialTw)
		m_menuNav->addElem(m_socialTw);
#endif
#if ITCHIO_SHOW_KICKSTARTER
	if (m_campaignGl)
		m_menuNav->addElem(m_campaignGl);
	if (m_campaignKs)
		m_menuNav->addElem(m_campaignKs);
#endif
}

MainMenu::~MainMenu()
{
	delete m_menuNav;

	delete m_newGame;
	delete m_findGame;
	delete m_customize;
	delete m_options;
	delete m_quitApp;

	delete m_controls;
#if ITCHIO_SHOW_URLS
	delete m_socialFb;
	delete m_socialTw;
#endif
#if ITCHIO_SHOW_KICKSTARTER
	delete m_campaignGl;
	delete m_campaignKs;
#endif
}

void MainMenu::onEnter()
{
	m_animTime = 0.f;

	m_inactivityTime = 0.f;
	s_lastMouse = mouse;
	s_lastGamepad = gamepad[0];

#if DRAW_BACK_V1
	const int animX = 100;
	if (m_newGame)
		m_newGame->setAnimation(animX, 0, 0.f, .4f);
	if (m_quitApp)
		m_quitApp->setAnimation(animX, 0, 0.f, .4f);
#endif

	g_tileTransition->begin(.5f);
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

	if (g_app->m_netState > App::NetState_Offline && g_app->m_netState < App::NetState_Online)
	{
		// don't process menu interactions until we're done starting hosting
		return false;
	}

	//

	m_menuNav->tick(dt);

	//

	if (m_newGame && m_newGame->isClicked())
	{
		logDebug("new game!");

		g_app->startHosting();
	}
	else if (m_findGame && m_findGame->isClicked())
	{
		logDebug("find game!");

		Verify(g_app->findGame());
	}
	else if (m_customize && m_customize->isClicked())
	{
	#if ITCHIO_BUILD
		g_app->playSound("ui/sounds/menu-back.ogg");
	#else
		g_app->m_menuMgr->push(new CustomizeMenu());
	#endif
	}
	else if (m_options && m_options->isClicked())
	{
	#if ITCHIO_BUILD
		g_app->playSound("ui/sounds/menu-back.ogg");
	#else
		g_app->m_menuMgr->push(new OptioneMenu());
	#endif
	}
	else if (m_quitApp && m_quitApp->isClicked())
	{
		logDebug("exit game!");

		g_app->quit();
	}
	else if (m_controls && m_controls->isClicked())
	{
		g_app->m_menuMgr->push(new HelpMenu());
	}
#if ITCHIO_SHOW_URLS
	else if (m_socialFb && m_socialFb->isClicked())
	{
		openBrowserWithUrl("https://www.facebook.com/ripostegame");
	}
	else if (m_socialTw && m_socialTw->isClicked())
	{
		openBrowserWithUrl("https://twitter.com/damajogames");
	}
#endif
#if ITCHIO_SHOW_KICKSTARTER
	else if (m_campaignGl && m_campaignGl->isClicked())
	{
		openBrowserWithUrl("http://steamcommunity.com/sharedfiles/filedetails/?id=499076298");
	}
	else if (m_campaignKs && m_campaignKs->isClicked())
	{
		openBrowserWithUrl("https://www.kickstarter.com/projects/1766566023/riposte");
	}
#endif
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
		bool large = (spawnIdx % 20) == 0;
		spawnIdx++;

		spawnTime = large ? 0.f : .5f;

		for (int j = 0; j < numParts; ++j)
		{
			if (parts[j].life == 0.f)
			{
				parts[j].x = ballXBase;
				parts[j].y = ballYBase;
				parts[j].angle = rand() % 360;
				parts[j].type = large ? 0 : 1;
				parts[j].life = parts[j].type == 0 ? 12.f : 2.f;
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

static void drawSexyScroller(float x1, float y1, float x2, float y2, float xOffset, float yOffset, float scale, float a1, float a2)
{
	gxSetTexture(Sprite("itch-scroller.png").getTexture());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gxBegin(GL_QUADS);
	{
		/*gxColor4f(1.f, 1.f, 1.f, a1);*/ gxTexCoord2f(xOffset + x1 * scale, yOffset + y1 * scale); gxVertex2f(x1, y1);
		/*gxColor4f(1.f, 1.f, 1.f, a1);*/ gxTexCoord2f(xOffset + x2 * scale, yOffset + y1 * scale); gxVertex2f(x2, y1);
		/*gxColor4f(1.f, 1.f, 1.f, a2);*/ gxTexCoord2f(xOffset + x2 * scale, yOffset + y2 * scale); gxVertex2f(x2, y2);
		/*gxColor4f(1.f, 1.f, 1.f, a2);*/ gxTexCoord2f(xOffset + x1 * scale, yOffset + y2 * scale); gxVertex2f(x1, y2);
	}
	gxEnd();
	gxSetTexture(0);
}

void MainMenu::draw()
{
#if DRAW_BACK_V1
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
#endif

#if DRAW_BACK_V2
	const float kBackFadeinTime = .4f;
	const float kCharFadeinTime = 1.f;
	const float kLogoFadeinTime = .6f;

	const float kCharOffsetX = -150.f;

	const float backOpacity = saturate(m_animTime / kBackFadeinTime);
	const float charOpacity = saturate(m_animTime / kCharFadeinTime);
	const float logoOpacity = saturate(m_animTime / kLogoFadeinTime);

	setColorf(1.f, 1.f, 1.f, backOpacity);
	Sprite("ui/menus/menu-background.png").draw();

	setColorf(1.f, 1.f, 1.f, backOpacity);
	Sprite("ui/menus/menu-sidepanels.png").draw();

	setColorf(1.f, 1.f, 1.f, charOpacity);
	Sprite("ui/menus/menu-main-char.png").drawEx(kCharOffsetX * EvalEase(1.f - charOpacity, kEaseType_SineIn, 0.f), 0.f);

	setColorf(1.f, 1.f, 1.f, logoOpacity);
	Sprite("ui/menus/menu-main-logo.png").draw();
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

	if (m_controls)
		m_controls->draw();
#if ITCHIO_SHOW_URLS
	if (m_socialFb)
		m_socialFb->draw();
	if (m_socialTw)
		m_socialTw->draw();
#endif
#if ITCHIO_SHOW_KICKSTARTER
	if (m_campaignGl)
		m_campaignGl->draw();
	if (m_campaignKs)
		m_campaignKs->draw();
#endif

#if ITCHIO_BUILD && DRAW_BACK_V1
	const float offset = framework.time * .2f;
	const float scale = 1.f / 70.f;
	const float size = 8.f;
	static const Color scrollerColler = Color::fromHex("21c3f5");
	setColor(scrollerColler);
	drawSexyScroller(0.f,           0.f, GFX_SX,   size, +offset, -offset * .5f, scale, 1.f, 1.f);
	drawSexyScroller(0.f, GFX_SY - size, GFX_SX, GFX_SY, +offset, -offset * .5f, scale, 1.f, 1.f);
#endif
}

//

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
