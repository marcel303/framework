#include "Calc.h"
#include "FileStream.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamerules.h"
#include "gamestate.h"
#include "main.h"
#include "OptionMenu.h"
#include "StatTimerMenu.h"
#include "StatTimers.h"
#include "StreamReader.h"
#include "StringBuilder.h"
#include "Timer.h"

//

OPTION_DECLARE(bool, g_devMode, false);
OPTION_DEFINE(bool, g_devMode, "App/Developer Mode");
OPTION_ALIAS(g_devMode, "devmode");

OPTION_DECLARE(bool, g_monkeyMode, false);
OPTION_DEFINE(bool, g_monkeyMode, "App/Monkey Mode");
OPTION_ALIAS(g_monkeyMode, "monkeymode");

OPTION_EXTERN(int, g_playerCharacterIndex);

//

TIMER_DEFINE(g_appTickTime, PerFrame, "App/Tick");
TIMER_DEFINE(g_appDrawTime, PerFrame, "App/Draw");

//

#define CHARICON_SX 80
#define CHARICON_SY 100

#define NUM_VOTING_BUTTONS 4

#define CHARACTER_SELECT_OFFSET 200
#define TARGET_SELECT_OFFSET 400

//

App * g_app = 0;
GameState * g_gameState = 0;

class VotingScreen
{
public:
	class CharacterIcon
	{
	public:
		int x1, y1;
		int x2, y2;

		CharacterIcon()
		{
			memset(this, 0, sizeof(*this));
		}

		void draw()
		{
		}
	};

	class VotingButton
	{
	public:
		int x1, y1;
		int x2, y2;

		VotingButton()
		{
			memset(this, 0, sizeof(*this));
		}

		void setup(int x, int y)
		{
			x1 = x - 100;
			y1 = y;
			x2 = x + 100;
			y2 = y + 50;
		}

		bool isClicked() const
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				if (mouse.x >= x1 &&
					mouse.y >= y1 &&
					mouse.x <= x2 &&
					mouse.y <= y2)
				{
					return true;
				}
			}

			return false;
		}
	};

	struct
	{
		int x1, y1;
		int x2, y2;
	} m_backButton;

	enum State
	{
		State_SelectCharacter,
		State_SelectOption,
		State_SelectTarget,
		State_ShowResults
	};

	State m_state;
	int m_selectedCharacter;
	int m_selectedOption;
	CharacterIcon m_characterIcons[MAX_PLAYERS];
	VotingButton m_votingButtons[NUM_VOTING_BUTTONS];

	VotingScreen()
		: m_state(State_SelectCharacter)
		, m_selectedCharacter(0)
		, m_selectedOption(0)
	{
		setupCharacterIcons();

		m_backButton.x1 = (GFX_SX - 200) / 2;
		m_backButton.y1 = GFX_SY - 100;
		m_backButton.x2 = (GFX_SX + 200) / 2;
		m_backButton.y2 = GFX_SY - 50;

		for (int i = 0; i < NUM_VOTING_BUTTONS; ++i)
		{
			m_votingButtons[i].setup(GFX_SX/2, GFX_SY/2 + i * 80);
		}
	}

	void setupCharacterIcons()
	{
		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			// todo : setup rect

			CharacterIcon & icon = m_characterIcons[i];

			const int sx = CHARICON_SX;
			const int sy = CHARICON_SY;

			const int x = GFX_SX / (MAX_PLAYERS + 1) * (i + 1);
			icon.x1 = x - sx / 2;
			icon.y1 = 0;
			icon.x2 = x + sx / 2;
			icon.y2 = CHARICON_SY;
		}
	}

	void tick()
	{
		if (m_state == State_SelectCharacter)
		{
			for (int i = 0; i < g_gameState->m_numPlayers; ++i)
			{
				// todo : see if mouse was down on character

				if (!g_gameState->m_players[i].m_hasVoted && mouse.wentDown(BUTTON_LEFT))
				{
					if (mouse.x >= m_characterIcons[i].x1 &&
						mouse.y >= m_characterIcons[i].y1 + CHARACTER_SELECT_OFFSET &&
						mouse.x <= m_characterIcons[i].x2 &&
						mouse.y <= m_characterIcons[i].y2 + CHARACTER_SELECT_OFFSET)
					{
						m_selectedCharacter = i;
						m_state = State_SelectOption;
						break;
					}
				}
			}
		}
		else if (m_state == State_SelectOption)
		{
			// check voting option

			for (int i = 0; i < NUM_VOTING_BUTTONS; ++i)
			{
				if (m_votingButtons[i].isClicked())
				{
					// commit vote

					// todo : base choice upon option info

					if (false)
					{
						if (g_gameState->m_players[m_selectedCharacter].vote(i))
							m_state = State_ShowResults;
						else
							m_state = State_SelectTarget;
					}
					else
					{
						m_selectedOption = i;
						m_state = State_SelectTarget;
					}
				}
			}

			if (mouse.wentDown(BUTTON_LEFT))
			{
				// todo : loop over voting options

				if (mouse.x >= m_backButton.x1 &&
					mouse.y >= m_backButton.y1 &&
					mouse.x <= m_backButton.x2 &&
					mouse.y <= m_backButton.y2)
				{
					m_state = State_SelectCharacter;
				}
			}
		}
		else if (m_state == State_SelectTarget)
		{
			for (int i = 0; i < g_gameState->m_numPlayers; ++i)
			{
				// todo : see if mouse was down on character

				if (i != m_selectedCharacter && mouse.wentDown(BUTTON_LEFT))
				{
					if (mouse.x >= m_characterIcons[i].x1 &&
						mouse.y >= m_characterIcons[i].y1 + TARGET_SELECT_OFFSET &&
						mouse.x <= m_characterIcons[i].x2 &&
						mouse.y <= m_characterIcons[i].y2 + TARGET_SELECT_OFFSET)
					{
						if (g_gameState->m_players[m_selectedCharacter].vote(m_selectedOption, i))
							m_state = State_ShowResults;
						else
							m_state = State_SelectCharacter;
						break;
					}
				}
			}
		}
		else if (m_state == State_ShowResults)
		{
			// todo : use a timer to transition to the next screen

			if (mouse.wentDown(BUTTON_LEFT))
			{
				m_state = State_SelectCharacter;
			}
		}
	}

	void draw()
	{
		// draw background

		setColor(colorWhite);
		Sprite("voting-back.png").draw();

		if (m_state == State_SelectCharacter)
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX / 2, 20, 40, 0.f, +1.f, "<Select Character>");

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				const CharacterIcon & icon = m_characterIcons[i];

				if (i < g_gameState->m_numPlayers && !g_gameState->m_players[i].m_hasVoted)
				{
					setColor(colorGreen);
					drawRect(
						icon.x1,
						icon.y1 + CHARACTER_SELECT_OFFSET,
						icon.x2,
						icon.y2 + CHARACTER_SELECT_OFFSET);
				}
				else
				{
					// todo : draw character icon in disabled state

					setColor(colorRed);
					drawRect(
						icon.x1,
						icon.y1 + CHARACTER_SELECT_OFFSET,
						icon.x2,
						icon.y2 + CHARACTER_SELECT_OFFSET);
				}
			}
		}
		else if (m_state == State_SelectOption)
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX / 2, 20, 40, 0.f, +1.f, "<Character Name>");

			// todo : draw character icon

			setColor(colorGreen);
			drawRect(
				(GFX_SX - CHARICON_SX) / 2,
				(GFX_SY/2 - CHARICON_SY) / 2,
				(GFX_SX + CHARICON_SX) / 2,
				(GFX_SY/2 + CHARICON_SY) / 2);

			// todo : draw voting buttons

			for (int i = 0; i < NUM_VOTING_BUTTONS;++i)
			{
				VotingButton & button = m_votingButtons[i];

				setColor(colorWhite);
				drawRect(
					button.x1,
					button.y1,
					button.x2,
					button.y2);

				setFont("calibri.ttf");
				setColor(colorBlack);
				drawText((button.x1 + button.x2) / 2, (button.y1 + button.y2) / 2, 40, 0.f, 0.f, "%c", 'A' + i);
			}

			// todo : draw back button

			setColor(colorWhite);
			drawRect(
				m_backButton.x1,
				m_backButton.y1,
				m_backButton.x2,
				m_backButton.y2);

			setFont("calibri.ttf");
			setColor(colorBlack);
			drawText((m_backButton.x1 + m_backButton.x2) / 2, (m_backButton.y1 + m_backButton.y2) / 2, 40, 0.f, 0.f, "Back");
		}
		else if (m_state == State_SelectTarget)
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX / 2, 20, 40, 0.f, +1.f, "<Select Target>");

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				const CharacterIcon & icon = m_characterIcons[i];

				if (i < g_gameState->m_numPlayers && i != m_selectedCharacter)
				{
					setColor(colorGreen);
					drawRect(
						icon.x1,
						icon.y1 + TARGET_SELECT_OFFSET,
						icon.x2,
						icon.y2 + TARGET_SELECT_OFFSET);
				}
				else
				{
					// todo : draw character icon in disabled state

					setColor(colorRed);
					drawRect(
						icon.x1,
						icon.y1 + TARGET_SELECT_OFFSET,
						icon.x2,
						icon.y2 + TARGET_SELECT_OFFSET);
				}
			}
		}
		else if (m_state == State_ShowResults)
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX / 2, 20, 40, 0.f, +1.f, "<Results>");

			// todo : show how end of round effects each player
		}
	}
};

class StatsScreen
{
public:
	StatsScreen()
	{
	}

	void tick()
	{
	}

	void draw()
	{
		gxMatrixMode(GL_MODELVIEW);
		gxPushMatrix();

		gxTranslatef(GFX_SX, 0, 0);

		// draw background

		setColor(colorWhite);
		Sprite("overview-back.png").draw();

		gxPopMatrix();
	}
};

static VotingScreen * g_votingScreen = 0;
static StatsScreen * g_statsScreen = 0;

//

static void HandleAction(const std::string & action, const Dictionary & args)
{
}

//

App::App()
	: m_optionMenu(0)
	, m_optionMenuIsOpen(false)
	, m_statTimerMenu(0)
	, m_statTimerMenuIsOpen(false)
{
}

App::~App()
{
}

bool App::init()
{
	Calc::Initialize();

	g_optionManager.Load("useroptions.txt");
	g_optionManager.Load("gameoptions.txt");

	if (g_devMode)
	{
		framework.minification = 2;
		framework.fullscreen = false;
	}
	else
	{
		framework.fullscreen = true;
	}

	framework.actionHandler = HandleAction;

	if (framework.init(0, 0, GFX_SX * 2, GFX_SY))
	{
		if (!g_devMode)
		{
			framework.fillCachesWithPath(".");
		}

		m_optionMenu = new OptionMenu();
		m_optionMenuIsOpen = false;

		m_statTimerMenu = new StatTimerMenu();
		m_statTimerMenuIsOpen = false;

		//

		g_gameState = new GameState();
		g_gameState->m_numPlayers = g_devMode ? 2 : MAX_PLAYERS;

		g_votingScreen = new VotingScreen();
		g_statsScreen = new StatsScreen();

		// >> fixme : remove

		AgendaEffect effect;

		effect.load("onresult:success target:everyone food:1 wealth:2 tech:3 special:incomemod special1:0 special2:-1");

		effect.apply(true, 0);

		try
		{
			FileStream stream;
			stream.Open("ruleset.txt", (OpenMode)(OpenMode_Read | OpenMode_Text));
			StreamReader reader(&stream, false);
			std::vector<std::string> lines = reader.ReadAllLines();
			g_gameState->loadAgendas(lines);
		}
		catch (std::exception & e)
		{
			logError(e.what());
		}

		// << fixme : remove

		g_gameState->newGame();

		return true;
	}

	return false;
}

void App::shutdown()
{
	delete m_statTimerMenu;
	m_statTimerMenu = 0;

	delete m_optionMenu;
	m_optionMenu = 0;

	framework.shutdown();
}

bool App::tick()
{
	TIMER_SCOPE(g_appTickTime);

	// calculate time step

	const uint64_t time = g_TimerRT.TimeMS_get();
	static uint64_t lastTime = time;
	if (time == lastTime)
	{
		SDL_Delay(1);
		return true;
	}

	float dt = (time - lastTime) / 1000.f;
	if (dt > 1.f / 60.f)
		dt = 1.f / 60.f;
	lastTime = time;

	framework.process();

	g_votingScreen->tick();

	g_statsScreen->tick();

	// debug

	if (keyboard.wentDown(SDLK_F5))
	{
		m_optionMenuIsOpen = !m_optionMenuIsOpen;
		m_statTimerMenuIsOpen = false;
	}

	if (keyboard.wentDown(SDLK_F6))
	{
		m_optionMenuIsOpen = false;
		m_statTimerMenuIsOpen = !m_statTimerMenuIsOpen;
	}

	if (m_optionMenuIsOpen || m_statTimerMenuIsOpen)
	{
		MultiLevelMenuBase * menu = 0;

		if (m_optionMenuIsOpen)
			menu = m_optionMenu;
		else if (m_statTimerMenuIsOpen)
			menu = m_statTimerMenu;

		menu->Update();

		if (keyboard.isDown(SDLK_UP) || gamepad[0].isDown(DPAD_UP))
			menu->HandleAction(OptionMenu::Action_NavigateUp, dt);
		if (keyboard.isDown(SDLK_DOWN) || gamepad[0].isDown(DPAD_DOWN))
			menu->HandleAction(OptionMenu::Action_NavigateDown, dt);
		if (keyboard.wentDown(SDLK_RETURN) || gamepad[0].wentDown(GAMEPAD_A))
			menu->HandleAction(OptionMenu::Action_NavigateSelect);
		if (keyboard.wentDown(SDLK_BACKSPACE) || gamepad[0].wentDown(GAMEPAD_B))
		{
			if (menu->HasNavParent())
				menu->HandleAction(OptionMenu::Action_NavigateBack);
			else
			{
				m_optionMenuIsOpen = false;
				m_statTimerMenuIsOpen = false;
			}
		}
		if (keyboard.isDown(SDLK_LEFT) || gamepad[0].isDown(DPAD_LEFT))
			menu->HandleAction(OptionMenu::Action_ValueDecrement, dt);
		if (keyboard.isDown(SDLK_RIGHT) || gamepad[0].isDown(DPAD_RIGHT))
			menu->HandleAction(OptionMenu::Action_ValueIncrement, dt);
	}

	if (keyboard.wentDown(SDLK_t))
	{
		g_optionManager.Load("options.txt");
	}

#if GG_ENABLE_TIMERS
	TIMER_STOP(g_appTickTime);
	g_statTimerManager.Update();
	TIMER_START(g_appTickTime);
#endif

	if (keyboard.wentDown(SDLK_ESCAPE))
	{
		exit(0);
	}

	return true;
}

void App::draw()
{
	TIMER_SCOPE(g_appDrawTime);

	framework.beginDraw(10, 15, 10, 0);
	{
		g_votingScreen->draw();

		g_statsScreen->draw();

		if (m_optionMenuIsOpen)
		{
			const int sx = 500;
			const int sy = GFX_SY / 3;
			const int x = (GFX_SX - sx) / 2;
			const int y = (GFX_SY - sy) / 2;

			m_optionMenu->Draw(x, y, sx, sy);
		}

		if (m_statTimerMenuIsOpen)
		{
			const int sx = 500;
			const int sy = GFX_SY / 3;
			const int x = (GFX_SX - sx) / 2;
			const int y = (GFX_SY - sy) / 2;

			m_statTimerMenu->Draw(x, y, sx, sy);
		}
	}
	TIMER_STOP(g_appDrawTime);
	framework.endDraw();
	TIMER_START(g_appDrawTime);
}

//

int main(int argc, char * argv[])
{
	changeDirectory("data");

	g_optionManager.LoadFromCommandLine(argc, argv);

	g_app = new App();

	if (!g_app->init())
	{
		//
	}
	else
	{
		while (g_app->tick())
		{
			g_app->draw();
		}

		g_app->shutdown();
	}

	delete g_app;
	g_app = 0;

	return 0;
}
