#include <ctime>
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

- agenda types:
	- stockpile
	- player attack
	- planet destruction
	- local

*/

//

OPTION_DECLARE(bool, g_devMode, false);
OPTION_DEFINE(bool, g_devMode, "App/Developer Mode");
OPTION_ALIAS(g_devMode, "devmode");

OPTION_DECLARE(bool, g_monkeyMode, false);
OPTION_DEFINE(bool, g_monkeyMode, "App/Monkey Mode");
OPTION_ALIAS(g_monkeyMode, "monkeymode");

OPTION_DECLARE(bool, g_flipScreen, false);
OPTION_DEFINE(bool, g_flipScreen, "App/Flip Screen");

OPTION_EXTERN(int, g_playerCharacterIndex);

COMMAND_OPTION(g_fakePlayerWin1, "App/Let Player 1 Win With Resources", []
{
	g_gameState->m_players[0].m_resources.food = 20;
	g_gameState->m_players[0].m_resources.wealth = 20;
	g_gameState->m_players[0].m_resources.tech = 20;
});

COMMAND_OPTION(g_fakePlayerWin2, "App/Let Player 1 Win As Last Man Standing", []
{
	for (int i = 1; i < g_gameState->m_numPlayers; ++i)
		g_gameState->m_players[i].m_isDead = true;
});

COMMAND_OPTION(g_fakePlayerDie1, "App/Let Player 1 Die Due To No Food", []
{
	g_gameState->m_players[0].m_resources.food = 0;
});

//

TIMER_DEFINE(g_appTickTime, PerFrame, "App/Tick");
TIMER_DEFINE(g_appDrawTime, PerFrame, "App/Draw");

//

//#define PLAYER_STATS_TIME (g_devMode ? 0.1f : 5.f)
#define PLAYER_STATS_TIME (2.f)

#define COLOR_CHAR_DISABLED Color(.5f, .5f, .5f, .5f)

//

void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

App * g_app = 0;
GameState * g_gameState = 0;

static class VotingScreen * g_votingScreen = 0;
static class StatsScreen * g_statsScreen = 0;

//

enum CharIcon
{
	CharIcon_CharSelect,
	CharIcon_Voting,
	CharIcon_Council
};

static void drawCharIcon(int x, int y, int index, float scale, CharIcon icon)
{
	char filename[64];
	if (icon == CharIcon_CharSelect)
		sprintf_s(filename, sizeof(filename), "portrait%d_CharSelect.png", index + 1);
	if (icon == CharIcon_Voting)
		sprintf_s(filename, sizeof(filename), "portrait%d.png", index + 1);
	if (icon == CharIcon_Council)
		sprintf_s(filename, sizeof(filename), "portrait%d_Council.png", index + 1);
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
		State_ShowSponsor,
		State_TitleScreen,
		State_Discuss,
		State_SelectCharacter,
		State_SelectOption,
		State_SelectTarget,
		State_ShowResults,
		State_ShowWinner
	};

	State m_state;
	int m_stateTimer;
	int m_discussionTimeStart;
	int m_selectedCharacter;
	int m_selectedOption;
	CharacterIcon m_characterIcons[MAX_PLAYERS];
	CharacterIcon m_targetIcons[MAX_PLAYERS];
	VotingButton m_votingButtons[NUM_VOTING_BUTTONS];
	int m_votingTimeStart;

	VotingScreen()
		: m_state(g_devMode ? State_Discuss : State_ShowSponsor)
		, m_stateTimer(0.f)
		, m_discussionTimeStart(0)
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
			icon.x2 = x[i] + VOTING_CHAR_WIDTH;
			icon.y1 = y[i];
			icon.y2 = y[i] + VOTING_CHAR_HEIGHT;
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
			icon.x2 = x[i] + VOTING_CHAR_WIDTH;
			icon.y1 = y[i];
			icon.y2 = y[i] + VOTING_CHAR_HEIGHT;
		}
	}

	int getVotingTimeLeft() const
	{
		return Calc::Max<int>(-1, m_votingTimeStart + 20 - g_TimerRT.Time_get());
	}

	int getDiscussionTimeLeft() const
	{
		return Calc::Max<int>(-1, m_discussionTimeStart + (g_devMode ? 5 : 150) - g_TimerRT.Time_get());
	}

	bool canSelectCharacter(int player)
	{
		if (player >= g_gameState->m_numPlayers)
			return false;
		if (g_gameState->m_players[player].m_isDead)
			return false;
		if (g_gameState->m_players[player].m_hasVoted)
			return false;
		return true;
	}

	bool canSelectOption(int playerId, int optionId)
	{
		const Player & player = g_gameState->m_players[playerId];
		const AgendaOption & option = g_gameState->m_currentAgenda.m_options[optionId];
		if (!option.m_isEnabled)
			return false;
		if (-option.m_cost.food > player.m_resources.food)
			return false;
		if (-option.m_cost.wealth > player.m_resources.wealth)
			return false;
		if (-option.m_cost.tech > player.m_resources.tech)
			return false;
		return true;
	}

	bool canSelectTarget(int player, int target)
	{
		if (target == m_selectedCharacter)
			return false;
		if (g_gameState->m_players[player].m_isDead)
			return false;
		for (int i = 0; i < g_gameState->m_players[player].m_numSelectedTargets; ++i)
			if (g_gameState->m_players[player].m_targetSelection[i] == target)
				return false;
		return true;
	}

	void tick()
	{
		if (m_state == State_ShowSponsor)
		{
			if (m_stateTimer == 0.f && !g_devMode)
				m_stateTimer = g_TimerRT.Time_get() + 3.f;

			if (g_TimerRT.Time_get() >= m_stateTimer)
			{
				m_stateTimer = 0.f;
				m_state = State_TitleScreen;
			}
		}
		else if (m_state == State_TitleScreen)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				m_state = State_Discuss;
			}
		}
		else if (m_state == State_Discuss)
		{
			if (m_discussionTimeStart == 0)
				m_discussionTimeStart = g_TimerRT.Time_get();

			if (getDiscussionTimeLeft() < 0)
			{
				m_discussionTimeStart = 0;
				m_state = State_SelectCharacter;
			}
		}
		else if (m_state == State_SelectCharacter)
		{
			for (int i = 0; i < g_gameState->m_numPlayers; ++i)
			{
				Player & player = g_gameState->m_players[i];

				if (canSelectCharacter(i) && mouse.wentDown(BUTTON_LEFT))
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
				AgendaOption & option = g_gameState->m_currentAgenda.m_options[i];

				if (canSelectOption(m_selectedCharacter, i) && m_votingButtons[i].isClicked())
				{
					// subtract cost

					player.m_resources += option.m_cost;
					player.m_resourcesSpent = -option.m_cost;

					if (option.m_isSabotage)
						player.m_hasSabotaged = true;

					if (g_gameState->m_currentAgenda.m_options[i].m_isAttack || g_gameState->m_currentAgenda.m_options[i].m_numTargets > 0)
					{
						m_selectedOption = i;
						m_state = State_SelectTarget;
					}
					else
					{
						if (g_gameState->m_players[m_selectedCharacter].vote(i, false))
							m_state = State_ShowResults;
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
					if (player.vote(-1, true))
						m_state = State_ShowResults;
					else
						m_state = State_SelectCharacter;
				}
			}

			if (m_state == State_SelectOption)
			{
				if (getVotingTimeLeft() < 0)
				{
					if (player.vote(-1, true))
						m_state = State_ShowResults;
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

				if (canSelectTarget(m_selectedCharacter, i) && mouse.wentDown(BUTTON_LEFT))
				{
					if (mouse.x >= m_targetIcons[i].x1 &&
						mouse.y >= m_targetIcons[i].y1 &&
						mouse.x <= m_targetIcons[i].x2 &&
						mouse.y <= m_targetIcons[i].y2)
					{
						Player & player = g_gameState->m_players[m_selectedCharacter];

						player.m_targetSelection[player.m_numSelectedTargets++] = i;

						if (player.m_numSelectedTargets == g_gameState->m_currentAgenda.m_options[m_selectedOption].m_numTargets)
						{
							m_votingTimeStart = g_TimerRT.Time_get();
							m_state = State_SelectOption;

							if (g_gameState->m_players[m_selectedCharacter].vote(m_selectedOption, false))
								m_state = State_ShowResults;
							else
								m_state = State_SelectCharacter;
						}
						break;
					}
				}
			}
		}
		else if (m_state == State_ShowResults)
		{
		}
		else if (m_state == State_ShowWinner)
		{
			if (m_stateTimer == 0.f)
				m_stateTimer = g_TimerRT.Time_get() + 7.f;

			if (g_TimerRT.Time_get() >= m_stateTimer && mouse.wentDown(BUTTON_LEFT))
			{
				m_stateTimer = 0.f;
				m_state = State_TitleScreen;
			}
		}
		else
		{
			Assert(false);
		}
	}

	void draw()
	{
		gxMatrixMode(GL_MODELVIEW);
		gxPushMatrix();

		if (g_flipScreen)
			gxTranslatef(GFX_SX, 0, 0);

		// draw background

		if (m_state == State_ShowSponsor)
		{
			setColor(colorWhite);
			Sprite("sponsors.png").draw();
		}
		else if (m_state == State_TitleScreen)
		{
			setColor(colorWhite);
			Sprite("title-screen.jpg").draw();
		}
		else if (m_state == State_Discuss)
		{
			setColor(colorWhite);
			Sprite("voting-back.png").draw();
		}
		else if (m_state == State_SelectCharacter)
		{
			setColor(colorWhite);
			Sprite("voting-back.png").draw();

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				Player & player = g_gameState->m_players[i];

				const CharacterIcon & icon = m_characterIcons[i];

				if (canSelectCharacter(i))
				{
					setColor(colorWhite);
					drawCharIcon(icon.x1, icon.y1, i, VOTING_CHAR_SCALE, CharIcon_CharSelect);

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

					setColor(COLOR_CHAR_DISABLED);
					drawCharIcon(icon.x1, icon.y1, i, VOTING_CHAR_SCALE, CharIcon_CharSelect);

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

			setFont("orbi-bold.ttf");
			setColor(Color::fromHex("eaffff"));
			drawText(40, 70, 48, +1.f, +1.f, g_gameState->m_currentAgenda.m_title.c_str());

			if (!g_gameState->m_currentAgenda.m_requirement.empty())
			{
				setFont("orbi-bold.ttf");
				setColor(Color::fromHex("3bcac8"));
				drawText(VOTING_REQ_X, VOTING_REQ_Y, 34, +1.f, +1.f, "REQ.");

				setFont("orbi.ttf");
				setColor(Color::fromHex("3bcac8"));
				drawText(VOTING_REQ_X + 105, VOTING_REQ_Y, 34, +1.f, +1.f, "%s", g_gameState->m_currentAgenda.m_requirement.c_str());
			}

			if (g_devMode)
			{
				setColor(colorGreen);
				drawRectLine(
					VOTING_AGENDA_TEXTBOX_X,
					VOTING_AGENDA_TEXTBOX_Y,
					VOTING_AGENDA_TEXTBOX_X + VOTING_AGENDA_TEXTBOX_WIDTH,
					VOTING_AGENDA_TEXTBOX_Y + VOTING_AGENDA_TEXTBOX_HEIGHT);
			}

			setFont("orbi.ttf");
			setColor(Color::fromHex("3bcac8"));
			drawTextArea(
					VOTING_AGENDA_TEXTBOX_X,
					VOTING_AGENDA_TEXTBOX_Y,
					VOTING_AGENDA_TEXTBOX_WIDTH,
					30,
					g_gameState->m_currentAgenda.m_description.c_str());

			// character icon

			setColor(colorWhite);
			drawCharIcon(VOTING_CHAR_X, VOTING_CHAR_Y, m_selectedCharacter, VOTING_OPTION_CHAR_SCALE, CharIcon_Voting);

			// stats

			const int statSize = 48;

			setFont("electro.ttf");
			setColor(Color::fromHex("43e981"));
			drawText(VOTING_STAT_FOOD_X, VOTING_STAT_FOOD_Y, statSize, +1.f, +1.f, "%d",
				g_gameState->m_players[m_selectedCharacter].m_resources.food);

			setColor(Color::fromHex("e6e38f"));
			drawText(VOTING_STAT_WEALTH_X, VOTING_STAT_WEALTH_Y, statSize, +1.f, +1.f, "%d",
				g_gameState->m_players[m_selectedCharacter].m_resources.wealth);

			setColor(Color::fromHex("5ed8ee"));
			drawText(VOTING_STAT_TECH_X, VOTING_STAT_TECH_Y, statSize, +1.f, +1.f, "%d",
				g_gameState->m_players[m_selectedCharacter].m_resources.tech);

			// goal

			setFont("electro.ttf");
			setColor(Color::fromHex("d3f7ff"));
			drawText(VOTING_GOAL_X, VOTING_GOAL_Y + 45, 40, +1.f, +1.f,
				g_gameState->m_players[m_selectedCharacter].m_goal.m_description.c_str());

			// vote timer

			setFont("orbi.ttf");
			setColor(Color::fromHex("3bcac8"));
			const int votingTimeLeft = getVotingTimeLeft();
			drawText(VOTING_TIMER_X, VOTING_TIMER_Y, VOTING_TIMER_SIZE, 0.f, 1.f, "%d:%02d", votingTimeLeft / 60, votingTimeLeft % 60);

			// draw voting buttons

			for (int i = 0; i < NUM_VOTING_BUTTONS;++i)
			{
				AgendaOption & option = g_gameState->m_currentAgenda.m_options[i];

				VotingButton & button = m_votingButtons[i];

				if (g_devMode)
				{
					setColor(colorGreen);
					drawRectLine(
						button.x1,
						button.y1,
						button.x2,
						button.y2);
				}

				setFont("orbi.ttf");
				setColor(canSelectOption(m_selectedCharacter, i) ? Color::fromHex("3bcac8") : Color(1.f, 0.f, 0.f));
				drawText(VOTING_OPTION_TEXT_X, VOTING_OPTION_TEXT_Y + i * VOTING_OPTION_DY,      34, 1.f, 1.f, option.m_caption.c_str());
				drawText(VOTING_OPTION_TEXT_X, VOTING_OPTION_TEXT_Y + i * VOTING_OPTION_DY + 40, 20, 1.f, 1.f, option.m_text.c_str());

				// food, wealth, tech

				const Color colors[3] =
				{
					Color::fromHex("43e981"),
					Color::fromHex("e6e38f"),
					Color::fromHex("5ed8ee")
				};

				const char * boxes[3] =
				{
					"costbox-food.png",
					"costbox-wealth.png",
					"costbox-tech.png"
				};

				const int costs[3] =
				{
					option.m_cost.food,
					option.m_cost.wealth,
					option.m_cost.tech,
				};

				const int posX[2] =
				{
					VOTING_OPTION_COST_1_X,
					VOTING_OPTION_COST_2_X
				};

				const int boxX[2] =
				{
					VOTING_OPTION_COSTBOX_1_X,
					VOTING_OPTION_COSTBOX_2_X
				};

				int idx = 0;

				for (int c = 0; c < 3; ++c)
				{
					if (costs[c] != 0)
					{
						setColor(colorWhite);
						Sprite(boxes[c]).drawEx(boxX[idx], VOTING_OPTION_COSTBOX_Y + i * VOTING_OPTION_DY);

						setFont("electro.ttf");
						setColor(colors[c]);
						drawText(posX[idx], VOTING_OPTION_COST_Y + i * VOTING_OPTION_DY, 36, +1.f, +1.f, "%+d", costs[c]);

						idx++;

						if (idx == 2)
							break;
					}
				}
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

			setFont("orbi-bold.ttf");
			setColor(Color::fromHex("eaffff"));
			drawText(40, 70, 48, +1.f, +1.f, "Select target");

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				const CharacterIcon & icon = m_targetIcons[i];

				if (i < g_gameState->m_numPlayers && canSelectTarget(m_selectedCharacter, i))
				{
					Shader shader("focuspoint");
					shader.setImmediate("time", g_TimerRT.Time_get());
					setShader(shader);
					drawCharIcon(icon.x1, icon.y1, i, VOTING_CHAR_SCALE, CharIcon_CharSelect);
					clearShader();

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

					Shader shader("disabled");
					setShader(shader);
					drawCharIcon(icon.x1, icon.y1, i, VOTING_CHAR_SCALE, CharIcon_CharSelect);
					clearShader();

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
		}
		else if (m_state == State_ShowWinner)
		{
			setColor(colorWhite);
			Sprite("winner-back.png").draw();
		}

		gxPopMatrix();
	}
};

class StatsScreen
{
public:
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
		int numPlayers;

		ResultsAnim()
			: state(State_ShowPlayerResults)
			, timer(0.f)
			, numPlayers(1)
		{
		}
	} m_resultsAnim;

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

		if (!g_flipScreen)
			gxTranslatef(GFX_SX, 0, 0);

		// draw background

		if (g_votingScreen->m_state == VotingScreen::State_ShowSponsor)
		{
			setColor(colorWhite);
			Sprite("sponsors.png").draw();
		}
		else if (g_votingScreen->m_state == VotingScreen::State_TitleScreen)
		{
			setColor(colorWhite);
			Sprite("title-screen.jpg").draw();
		}
		else if (g_votingScreen->m_state == VotingScreen::State_ShowWinner)
		{
			setColor(colorWhite);
			Sprite("winner-back.png").draw();

			// todo : draw sprites for winning players

			// todo : show winning player goal
		}
		else
		{
			setColor(colorWhite);
			Sprite("overview-back.png").draw();
		}

		const int councilX[MAX_PLAYERS] =
		{
			COUNCIL_CHAR1_X,
			COUNCIL_CHAR2_X,
			COUNCIL_CHAR3_X,
			COUNCIL_CHAR4_X,
			COUNCIL_CHAR5_X,
			COUNCIL_CHAR6_X,
			COUNCIL_CHAR7_X,
			COUNCIL_CHAR8_X,
			COUNCIL_CHAR9_X,
			COUNCIL_CHAR10_X
		};

		const int councilY[MAX_PLAYERS] =
		{
			COUNCIL_CHAR1_Y,
			COUNCIL_CHAR2_Y,
			COUNCIL_CHAR3_Y,
			COUNCIL_CHAR4_Y,
			COUNCIL_CHAR5_Y,
			COUNCIL_CHAR6_Y,
			COUNCIL_CHAR7_Y,
			COUNCIL_CHAR8_Y,
			COUNCIL_CHAR9_Y,
			COUNCIL_CHAR10_Y
		};

		if (g_votingScreen->m_state == VotingScreen::State_Discuss)
		{
			// discuss timer

			setFont("orbi.ttf");
			setColor(Color::fromHex("3bcac8"));
			const int votingTimeLeft = g_votingScreen->getDiscussionTimeLeft();
			drawText(DISCUSSION_TIMER_X, DISCUSSION_TIMER_Y, DISCUSSION_TIMER_SIZE, 0.f, 1.f, "%d:%02d", votingTimeLeft / 60, votingTimeLeft % 60);
		}

		// draw council

		for (int layer = 0; layer < 2; ++layer)
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				// some characters need to be moved in front of others (layer 1)
				const int charLayer = (i == 0 || i == 1 || i == 3 || i == 8) ? 1 : 0;
				if (layer != charLayer)
					continue;

				Player & player = g_gameState->m_players[i];

				setColor(colorWhite);
				drawCharIcon(councilX[i], councilY[i], i, COUNCIL_CHAR_SCALE, CharIcon_Council);
			}
		}

		bool drawRoundStuff =
			g_votingScreen->m_state != VotingScreen::State_TitleScreen &&
			g_votingScreen->m_state != VotingScreen::State_ShowWinner;

		if (drawRoundStuff)
		{
			if (g_votingScreen->m_state == VotingScreen::State_Discuss)
			{
				setColor(colorWhite);
				Sprite("council-ui-agendaview.png").draw();

				setFont("orbi.ttf");
				setColor(Color::fromHex("245276"));
				drawTextArea(
					130,
					200,
					720,
					40,
					g_gameState->m_currentAgenda.m_description.c_str());
			}
			else
			{
				setColor(colorWhite);
				Sprite("council-ui-votingview.png").draw();
			}

			// draw agenda

			if (g_votingScreen->m_state != VotingScreen::State_Discuss)
			{
				setFont("orbi-bold.ttf");
				setColor(Color::fromHex("245276"));
				drawText(97, 665, 48, +1.f, +1.f, g_gameState->m_currentAgenda.m_title.c_str());
			}

			if (!g_gameState->m_currentAgenda.m_requirement.empty())
			{
				setFont("orbi-bold.ttf");
				setColor(Color::fromHex("3bcac8"));
				drawText(VOTING_REQ_X, VOTING_REQ_Y, 34, +1.f, +1.f, "REQ.");

				setFont("orbi.ttf");
				setColor(Color::fromHex("3bcac8"));
				drawText(VOTING_REQ_X + 105, VOTING_REQ_Y, 34, +1.f, +1.f, "%s", g_gameState->m_currentAgenda.m_requirement.c_str());
			}

			// draw round income

			const Color colors[3] =
			{
				Color::fromHex("e6e38f"),
				Color::fromHex("43e981"),
				Color::fromHex("5ed8ee")
			};

			const int income[3] =
			{
				g_gameState->m_perRoundIncome.wealth,
				g_gameState->m_perRoundIncome.food,
				g_gameState->m_perRoundIncome.tech,
			};

			for (int i = 0; i < 3; ++i)
			{
				setFont("electro.ttf");
				setColor(colors[i]);
				drawText(ROUND_INCOME_X + ROUND_INCOME_DX * i, ROUND_INCOME_Y, 32, +1.f, +1.f, "%+d", income[i]);
			}
		}

		if (g_votingScreen->m_state == VotingScreen::State_ShowResults)
		{
			switch (m_resultsAnim.state)
			{
			case ResultsAnim::State_ShowPlayerResults:
				{
					if (m_resultsAnim.timer == 0.f)
						m_resultsAnim.timer = g_TimerRT.Time_get() + PLAYER_STATS_TIME;

					int numAliveOrDiedPlayers = 0;
					for (int i = 0; i < g_gameState->m_numPlayers; ++i)
						if (!g_gameState->m_players[i].m_isDead || !g_gameState->m_players[i].m_wasDead)
							numAliveOrDiedPlayers++;

					int playerIdx = 0;

					for (int i = 0; i < m_resultsAnim.numPlayers; ++i, ++playerIdx)
					{
						while (g_gameState->m_players[playerIdx].m_isDead && g_gameState->m_players[playerIdx].m_wasDead)
							playerIdx++;

						if (i >= m_resultsAnim.numPlayers - 2)
						{
							const Player & player = g_gameState->m_players[playerIdx];

							if (player.m_isDead)
							{
								Assert(!player.m_wasDead);

								float y = councilY[i] - 50.f;

								setColor(colorWhite);
								Sprite bubble("Council_DeathBubble.png");
								bubble.drawEx(councilX[i], y, 0.f, 1.f);
							}
							else
							{
								const AgendaOption & option = g_gameState->m_previousAgenda.m_options[player.m_voteSelection];

								float y = councilY[i] - 50.f;

								setColor(colorWhite);
								Sprite bubble("Council_SpeechBubble.png");
								bubble.drawEx(councilX[i], y, 0.f, 1.f);

								const int foodChange = player.m_resourcesGainedThisRound.food;
								const int wealthChange = player.m_resourcesGainedThisRound.wealth;
								const int techChange = player.m_resourcesGainedThisRound.tech;

								y += RESEARCH_RESOURCE_OFFSET_Y;

								setFont("orbi.ttf");
								setColor(Color::fromHex("43e981"));
								drawText(councilX[i] + RESEARCH_FOOD_OFFSET, y, RESEARCH_FONT_SIZE, +1.f, +1.f, "%+d", foodChange);

								setFont("orbi.ttf");
								setColor(Color::fromHex("e6e38f"));
								drawText(councilX[i] + RESEARCH_WEALTH_OFFSET, y, RESEARCH_FONT_SIZE, +1.f, +1.f, "%+d", wealthChange);

								setFont("orbi.ttf");
								setColor(Color::fromHex("5ed8ee"));
								drawText(councilX[i] + RESEARCH_TECH_OFFSET, y, RESEARCH_FONT_SIZE, +1.f, +1.f, "%+d", techChange);

								setFont("orbi.ttf");
								setColor(colorBlack);
								drawTextArea(councilX[i] + RESEARCH_TEXT_OFFSET_X, councilY[i] + RESEARCH_TEXT_OFFSET_Y, bubble.getWidth() - RESEARCH_TEXT_OFFSET_X  * 2.f, RESEARCH_TEXT_FONT_SIZE, "I VOTE: %s", option.m_caption.c_str());
							}
						}
					}

					if (g_TimerRT.Time_get() >= m_resultsAnim.timer)
					{
						m_resultsAnim.timer = 0.f;

						if (m_resultsAnim.numPlayers + 1 <= numAliveOrDiedPlayers)
							m_resultsAnim.numPlayers++;
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
				m_resultsAnim = ResultsAnim();
				if (g_gameState->m_state == GameState::State_GameEnded)
					g_votingScreen->m_state = VotingScreen::State_ShowWinner;
				else
					g_votingScreen->m_state = VotingScreen::State_Discuss;
				break;
			}
		}

		if (drawRoundStuff)
		{
			// draw voting buttons

			for (int i = 0; i < NUM_VOTING_BUTTONS;++i)
			{
				AgendaOption & option = g_gameState->m_currentAgenda.m_options[i];

				VotingScreen::VotingButton & button = g_votingScreen->m_votingButtons[i];

				setFont("orbi.ttf");
				setColor(Color::fromHex("3bcac8"));
				drawText(COUNCIL_OPTION_TEXT_X, COUNCIL_OPTION_TEXT_Y + i * COUNCIL_OPTION_DY, 34, 1.f, 1.f, option.m_caption.c_str());
				drawText(COUNCIL_OPTION_TEXT_X, COUNCIL_OPTION_TEXT_Y + i * COUNCIL_OPTION_DY + 40, 20, 1.f, 1.f, option.m_text.c_str());

				// food, wealth, tech

				const Color colors[3] =
				{
					Color::fromHex("43e981"),
					Color::fromHex("e6e38f"),
					Color::fromHex("5ed8ee")
				};

				const char * boxes[3] =
				{
					"costbox-food.png",
					"costbox-wealth.png",
					"costbox-tech.png"
				};

				const int costs[3] =
				{
					option.m_cost.food,
					option.m_cost.wealth,
					option.m_cost.tech,
				};

				const int posX[2] =
				{
					COUNCIL_OPTION_COST_1_X,
					COUNCIL_OPTION_COST_2_X
				};

				const int boxX[2] =
				{
					COUNCIL_OPTION_COSTBOX_1_X,
					COUNCIL_OPTION_COSTBOX_2_X
				};

				int idx = 0;

				for (int c = 0; c < 3; ++c)
				{
					if (costs[c] != 0)
					{
						setColor(colorWhite);
						Sprite(boxes[c]).drawEx(boxX[idx], COUNCIL_OPTION_COSTBOX_Y + i * COUNCIL_OPTION_DY);

						setFont("electro.ttf");
						setColor(colors[c]);
						drawText(posX[idx], COUNCIL_OPTION_COST_Y + i * COUNCIL_OPTION_DY, 36, +1.f, +1.f, "%+d", costs[c]);

						idx++;

						if (idx == 2)
							break;
					}
				}
			}
		}
		
		gxPopMatrix();
	}
};

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
	time_t t = time(0);
	srand(t);

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
		g_gameState->m_numPlayers = g_devMode ? 4 : MAX_PLAYERS;

		g_votingScreen = new VotingScreen();
		g_statsScreen = new StatsScreen();

		// >> fixme : remove

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
