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
#include "StringEx.h"
#include "Timer.h"

/*

todo:

- beamer display
- results screen
- add option select timer
- add agenda 'attack' type -> display
- add agenda discussion timer
- check player goal completion
- determine agenda success or failure

*/

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

#define PLAYER_STATS_TIME 5.f

//

void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

App * g_app = 0;
GameState * g_gameState = 0;

//

static void drawCharIcon(int x, int y, int index, float scale)
{
	char filename[64];
	sprintf_s(filename, sizeof(filename), "portrait%d.png", index + 1);
	Sprite(filename).drawEx(x, y, 0.f, scale);
}

//

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
			x1 = x;
			y1 = y;
			x2 = x + VOTING_OPTION_BUTTON_WIDTH;
			y2 = y + VOTING_OPTION_BUTTON_HEIGHT;
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
	} m_abstainButton;

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
	CharacterIcon m_targetIcons[MAX_PLAYERS];
	VotingButton m_votingButtons[NUM_VOTING_BUTTONS];
	int m_votingTimeStart;

	struct ResultsAnim
	{
		enum State
		{
			State_ShowPlayerResults,
			State_ShowGlobalResults,
			State_ShowVotingResults,
			State_Done
		};

		State state;
		float timer;
		int player;

		ResultsAnim()
			: state(State_ShowPlayerResults)
			, timer(0.f)
			, player(0)
		{
		}
	} m_resultsAnim;

	VotingScreen()
		: m_state(State_SelectCharacter)
		, m_selectedCharacter(0)
		, m_selectedOption(0)
		, m_votingTimeStart(0)
	{
		setupCharacterIcons();
		setupTargetIcons();

		m_abstainButton.x1 = VOTING_ABSTAIN_X;
		m_abstainButton.y1 = VOTING_ABSTAIN_Y;
		m_abstainButton.x2 = VOTING_ABSTAIN_X + VOTING_ABSTAIN_WIDTH;
		m_abstainButton.y2 = VOTING_ABSTAIN_Y + VOTING_ABSTAIN_HEIGHT;

		for (int i = 0; i < NUM_VOTING_BUTTONS; ++i)
		{
			m_votingButtons[i].setup(VOTING_OPTION_X, VOTING_OPTION_Y + VOTING_OPTION_DY * i);
		}
	}

	void setupCharacterIcons()
	{
		const int x[MAX_PLAYERS] =
		{
			VOTING_CHAR1_X,
			VOTING_CHAR2_X,
			VOTING_CHAR3_X,
			VOTING_CHAR4_X,
			VOTING_CHAR5_X,
			VOTING_CHAR6_X,
			VOTING_CHAR7_X,
			VOTING_CHAR8_X,
			VOTING_CHAR9_X,
			VOTING_CHAR10_X
		};

		const int y[MAX_PLAYERS] =
		{
			VOTING_CHAR1_Y,
			VOTING_CHAR2_Y,
			VOTING_CHAR3_Y,
			VOTING_CHAR4_Y,
			VOTING_CHAR5_Y,
			VOTING_CHAR6_Y,
			VOTING_CHAR7_Y,
			VOTING_CHAR8_Y,
			VOTING_CHAR9_Y,
			VOTING_CHAR10_Y
		};

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			CharacterIcon & icon = m_characterIcons[i];

			icon.x1 = x[i];
			icon.x2 = x[i] + CHARICON_SX;
			icon.y1 = y[i];
			icon.y2 = y[i] + CHARICON_SY;
		}
	}

	void setupTargetIcons()
	{
		const int x[MAX_PLAYERS] =
		{
			VOTING_TARGET_CHAR1_X,
			VOTING_TARGET_CHAR2_X,
			VOTING_TARGET_CHAR3_X,
			VOTING_TARGET_CHAR4_X,
			VOTING_TARGET_CHAR5_X,
			VOTING_TARGET_CHAR6_X,
			VOTING_TARGET_CHAR7_X,
			VOTING_TARGET_CHAR8_X,
			VOTING_TARGET_CHAR9_X,
			VOTING_TARGET_CHAR10_X
		};

		const int y[MAX_PLAYERS] =
		{
			VOTING_TARGET_CHAR1_Y,
			VOTING_TARGET_CHAR2_Y,
			VOTING_TARGET_CHAR3_Y,
			VOTING_TARGET_CHAR4_Y,
			VOTING_TARGET_CHAR5_Y,
			VOTING_TARGET_CHAR6_Y,
			VOTING_TARGET_CHAR7_Y,
			VOTING_TARGET_CHAR8_Y,
			VOTING_TARGET_CHAR9_Y,
			VOTING_TARGET_CHAR10_Y
		};

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			CharacterIcon & icon = m_targetIcons[i];

			icon.x1 = x[i];
			icon.x2 = x[i] + CHARICON_SX;
			icon.y1 = y[i];
			icon.y2 = y[i] + CHARICON_SY;
		}
	}

	int getVotingTimeLeft() const
	{
		return Calc::Max<int>(0, m_votingTimeStart + 180 - g_TimerRT.Time_get());
	}

	void tick()
	{
		if (m_state == State_SelectCharacter)
		{
			for (int i = 0; i < g_gameState->m_numPlayers; ++i)
			{
				Player & player = g_gameState->m_players[i];

				if (!player.m_hasVoted && mouse.wentDown(BUTTON_LEFT))
				{
					if (mouse.x >= m_characterIcons[i].x1 &&
						mouse.y >= m_characterIcons[i].y1 &&
						mouse.x <= m_characterIcons[i].x2 &&
						mouse.y <= m_characterIcons[i].y2)
					{
						m_selectedCharacter = i;
						m_votingTimeStart = g_TimerRT.Time_get();
						m_state = State_SelectOption;
						break;
					}
				}
			}
		}
		else if (m_state == State_SelectOption)
		{
			Player & player = g_gameState->m_players[m_selectedCharacter];

			// check voting option

			for (int i = 0; i < NUM_VOTING_BUTTONS; ++i)
			{
				if (m_votingButtons[i].isClicked())
				{
					// subtract cost

					AgendaOption & option = g_gameState->m_currentAgenda.m_options[i];

					player.m_resources.food -= option.m_cost.food;
					player.m_resources.wealth -= option.m_cost.wealth;
					player.m_resources.tech -= option.m_cost.tech;

					if (g_gameState->m_currentAgenda.m_options[i].m_isAttack)
					{
						m_selectedOption = i;
						m_state = State_SelectTarget;
					}
					else
					{
						if (g_gameState->m_players[m_selectedCharacter].vote(i, -1, false))
						{
							m_resultsAnim = ResultsAnim();
							m_state = State_ShowResults;
						}
						else
							m_state = State_SelectCharacter;
					}
				}
			}

			if (mouse.wentDown(BUTTON_LEFT))
			{
				if (mouse.x >= m_abstainButton.x1 &&
					mouse.y >= m_abstainButton.y1 &&
					mouse.x <= m_abstainButton.x2 &&
					mouse.y <= m_abstainButton.y2)
				{
					if (player.vote(-1, -1, true))
					{
						m_resultsAnim = ResultsAnim();
						m_state = State_ShowResults;
					}
					else
						m_state = State_SelectCharacter;
				}
			}

			if (m_state == State_SelectOption)
			{
				if (getVotingTimeLeft() == 0)
				{
					if (player.vote(-1, -1, true))
					{
						m_resultsAnim = ResultsAnim();
						m_state = State_ShowResults;
					}
					else
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
					if (mouse.x >= m_targetIcons[i].x1 &&
						mouse.y >= m_targetIcons[i].y1 &&
						mouse.x <= m_targetIcons[i].x2 &&
						mouse.y <= m_targetIcons[i].y2)
					{
						if (g_gameState->m_players[m_selectedCharacter].vote(m_selectedOption, i, false))
						{
							m_resultsAnim = ResultsAnim();
							m_state = State_ShowResults;
						}
						else
							m_state = State_SelectCharacter;
						break;
					}
				}
			}
		}
		else if (m_state == State_ShowResults)
		{
		}
	}

	void draw()
	{
		// draw background

		if (m_state == State_SelectCharacter)
		{
			setColor(colorWhite);
			Sprite("voting-back.png").draw();

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				Player & player = g_gameState->m_players[i];

				const CharacterIcon & icon = m_characterIcons[i];

				if (i < g_gameState->m_numPlayers && !player.m_hasVoted)
				{
					setColor(colorWhite);
					drawCharIcon(icon.x1, icon.y1, i, VOTING_CHAR_SCALE);

					if (g_devMode)
					{
						setColor(colorGreen);
						drawRectLine(
							icon.x1,
							icon.y1,
							icon.x2,
							icon.y2);
					}
				}
				else
				{
					// todo : draw character icon in disabled state

					setColor(colorRed);
					drawCharIcon(icon.x1, icon.y1, i, VOTING_CHAR_SCALE);

					if (g_devMode)
					{
						setColor(colorRed);
						drawRectLine(
							icon.x1,
							icon.y1,
							icon.x2,
							icon.y2);
					}
				}
			}
		}
		else if (m_state == State_SelectOption)
		{
			setColor(colorWhite);
			Sprite("voting-vote-back.png").draw();

			// agenda text box

			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(10, 10, 40, +1.f, +1.f, "Agenda");

			if (g_devMode)
			{
				setColor(colorGreen);
				drawRectLine(
					VOTING_AGENDA_TEXTBOX_X,
					VOTING_AGENDA_TEXTBOX_Y,
					VOTING_AGENDA_TEXTBOX_X + VOTING_AGENDA_TEXTBOX_WIDTH,
					VOTING_AGENDA_TEXTBOX_Y + VOTING_AGENDA_TEXTBOX_HEIGHT);

				setFont("calibri.ttf");
				setColor(colorWhite);
				drawTextArea(
						VOTING_AGENDA_TEXTBOX_X,
						VOTING_AGENDA_TEXTBOX_Y,
						VOTING_AGENDA_TEXTBOX_WIDTH,
						30,
						g_gameState->m_currentAgenda.m_description.c_str());
			}

			// character icon

			drawCharIcon(VOTING_CHAR_X, VOTING_CHAR_Y, m_selectedCharacter, VOTING_OPTION_CHAR_SCALE);

			// stats

			const int statSize = 50;

			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(VOTING_STAT_FOOD_X, VOTING_STAT_FOOD_Y, statSize, +1.f, +1.f, "%d",
				g_gameState->m_players[m_selectedCharacter].m_resources.food);
			drawText(VOTING_STAT_WEALTH_X, VOTING_STAT_WEALTH_Y, statSize, +1.f, +1.f, "%d",
				g_gameState->m_players[m_selectedCharacter].m_resources.wealth);
			drawText(VOTING_STAT_TECH_X, VOTING_STAT_TECH_Y, statSize, +1.f, +1.f, "%d",
				g_gameState->m_players[m_selectedCharacter].m_resources.tech);
			drawText(VOTING_STAT_HAPPINESS_X, VOTING_STAT_HAPPINESS_Y, statSize, +1.f, +1.f, "%d",
				-1);

			// goal

			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(VOTING_GOAL_X, VOTING_GOAL_Y, 40, +1.f, +1.f,
				"Your goal:");
			drawText(VOTING_GOAL_X, VOTING_GOAL_Y + 45, 40, +1.f, +1.f,
				g_gameState->m_players[m_selectedCharacter].m_goal.m_description.c_str());

			// todo : vote timer

			setFont("calibri.ttf");
			setColor(colorWhite);
			const int votingTimeLeft = getVotingTimeLeft();
			drawText(VOTING_TIMER_X, VOTING_TIMER_Y, VOTING_TIMER_SIZE, 0.f, 1.f, "%d:%02d", votingTimeLeft / 60, votingTimeLeft % 60);

			// todo : draw voting buttons

			for (int i = 0; i < NUM_VOTING_BUTTONS;++i)
			{
				AgendaOption & option = g_gameState->m_currentAgenda.m_options[i];

				VotingButton & button = m_votingButtons[i];

				setColor(colorGreen);
				drawRectLine(
					button.x1,
					button.y1,
					button.x2,
					button.y2);

				setFont("calibri.ttf");
				setColor(colorWhite);
				drawText(VOTING_OPTION_TEXT_X, VOTING_OPTION_TEXT_Y + i * VOTING_OPTION_DY,      40, 1.f, 1.f, option.m_caption.c_str());
				drawText(VOTING_OPTION_TEXT_X, VOTING_OPTION_TEXT_Y + i * VOTING_OPTION_DY + 40, 20, 1.f, 1.f, option.m_text.c_str());
			}

			if (g_devMode)
			{
				setColor(colorGreen);
				drawRectLine(
					m_abstainButton.x1,
					m_abstainButton.y1,
					m_abstainButton.x2,
					m_abstainButton.y2);
			}
		}
		else if (m_state == State_SelectTarget)
		{
			setColor(colorWhite);
			Sprite("voting-back.png").draw();

			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX / 2, 20, 40, 0.f, +1.f, "<Select Target>");

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				const CharacterIcon & icon = m_targetIcons[i];

				if (i < g_gameState->m_numPlayers && i != m_selectedCharacter)
				{
					setColor(colorWhite);
					drawCharIcon(icon.x1, icon.y1, i, VOTING_CHAR_SCALE);

					if (g_devMode)
					{
						setColor(colorGreen);
						drawRectLine(
							icon.x1,
							icon.y1,
							icon.x2,
							icon.y2);
					}
				}
				else
				{
					// draw character icon in disabled state

					setColor(colorRed);
					drawCharIcon(icon.x1, icon.y1, i, VOTING_CHAR_SCALE);

					if (g_devMode)
					{
						setColor(colorRed);
						drawRectLine(
							icon.x1,
							icon.y1,
							icon.x2,
							icon.y2);
					}
				}
			}
		}
		else if (m_state == State_ShowResults)
		{
			setColor(colorWhite);
			Sprite("voting-back.png").draw();

			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX / 2, 20, 40, 0.f, +1.f, "<Results %d/%d/%g>", m_resultsAnim.state, m_resultsAnim.player, m_resultsAnim.timer);

			switch (m_resultsAnim.state)
			{
			case ResultsAnim::State_ShowPlayerResults:
				{
					if (m_resultsAnim.timer == 0.f)
						m_resultsAnim.timer = g_TimerRT.Time_get() + PLAYER_STATS_TIME;

					const Player & player = g_gameState->m_players[m_resultsAnim.player];

					int y = GFX_SY/3;

					drawCharIcon(GFX_SX/2, y, m_resultsAnim.player, VOTING_CHAR_SCALE);
					y += VOTING_CHAR_HEIGHT;

					const int foodChange = player.m_resources.food - player.m_oldResources.food;
					const int wealthChange = player.m_resources.wealth - player.m_oldResources.wealth;
					const int techChange = player.m_resources.tech - player.m_oldResources.tech;

					setFont("calibri.ttf");
					setColor(colorWhite);

					if (foodChange != 0)
						drawText(GFX_SX/2, y += 50, 30, 0.f, +1.f, "food %+d", foodChange);
					if (wealthChange != 0)
						drawText(GFX_SX/2, y += 50, 30, 0.f, +1.f, "wealth %+d", wealthChange);
					if (techChange != 0)
						drawText(GFX_SX/2, y += 50, 30, 0.f, +1.f, "tech %+d", techChange);

					if (g_TimerRT.Time_get() >= m_resultsAnim.timer)
					{
						m_resultsAnim.timer = 0.f;

						if (m_resultsAnim.player + 1 < g_gameState->m_numPlayers)
							m_resultsAnim.player++;
						else
							m_resultsAnim.state = ResultsAnim::State_ShowGlobalResults;
					}
				}
				break;
			case ResultsAnim::State_ShowVotingResults:
				if (m_resultsAnim.timer == 0.f)
					m_resultsAnim.timer = g_TimerRT.Time_get() + 5.f;

				if (g_TimerRT.Time_get() >= m_resultsAnim.timer)
				{
					m_resultsAnim.timer = 0.f;
					m_resultsAnim.state = ResultsAnim::State_ShowGlobalResults;
				}
				break;
			case ResultsAnim::State_ShowGlobalResults:
				if (m_resultsAnim.timer == 0.f)
					m_resultsAnim.timer = g_TimerRT.Time_get() + 3.f;

				if (g_TimerRT.Time_get() >= m_resultsAnim.timer)
				{
					m_resultsAnim.timer = 0.f;
					m_resultsAnim.state = ResultsAnim::State_Done;
				}
				break;
			case ResultsAnim::State_Done:
				m_state = State_SelectCharacter;
				break;
			}

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
		framework.windowX = 0;
		framework.windowY = 0;
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
