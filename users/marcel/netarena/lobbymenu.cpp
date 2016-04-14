#include "Channel.h"
#include "client.h"
#include "framework.h"
#include "host.h"
#include "lobbymenu.h"
#include "main.h"
#include "player.h"
#include "Timer.h" // for character anim time
#include "uicommon.h"

//#pragma optimize("", off)

OPTION_DECLARE(int, UI_CHARSELECT_GAMEMODE_TEXT_X, 350);
OPTION_DECLARE(int, UI_CHARSELECT_GAMEMODE_TEXT_Y, 616);
OPTION_DECLARE(int, UI_CHARSELECT_GAMEMODE_SELECT_X, 105);
OPTION_DECLARE(int, UI_CHARSELECT_GAMEMODE_SELECT_Y, 630);
OPTION_DEFINE(int, UI_CHARSELECT_GAMEMODE_TEXT_X, "UI/Character Select/GameMode Text X");
OPTION_DEFINE(int, UI_CHARSELECT_GAMEMODE_TEXT_Y, "UI/Character Select/GameMode Text Y");
OPTION_DEFINE(int, UI_CHARSELECT_GAMEMODE_SELECT_X, "UI/Character Select/GameMode Select X");
OPTION_DEFINE(int, UI_CHARSELECT_GAMEMODE_SELECT_Y, "UI/Character Select/GameMode Select Y");

OPTION_DECLARE(int, CHARCELL_INIT_X, 162);
OPTION_DECLARE(int, CHARCELL_INIT_Y, 155);
OPTION_DECLARE(int, CHARCELL_SPACING_X, 38);
OPTION_DECLARE(int, CHARCELL_SPACING_Y, 0);
OPTION_DECLARE(int, CHARCELL_SX, 166);
OPTION_DECLARE(int, CHARCELL_SY, 162);
OPTION_DECLARE(int, PLAYERCOL_SX, 36);
OPTION_DECLARE(int, PLAYERCOL_SY, 18);
OPTION_DEFINE(int, CHARCELL_INIT_X, "UI/Character Select/Grid/Base X");
OPTION_DEFINE(int, CHARCELL_INIT_Y, "UI/Character Select/Grid/Base Y");
OPTION_DEFINE(int, CHARCELL_SPACING_X, "UI/Character Select/Grid/Cell Spacing X");
OPTION_DEFINE(int, CHARCELL_SPACING_Y, "UI/Character Select/Grid/Cell Spacing Y");
OPTION_DEFINE(int, CHARCELL_SX, "UI/Character Select/Grid/Cell SX");
OPTION_DEFINE(int, CHARCELL_SY, "UI/Character Select/Grid/Cell SY");
OPTION_DEFINE(int, PLAYERCOL_SX, "UI/Character Select/Grid/Player Color SX");
OPTION_DEFINE(int, PLAYERCOL_SY, "UI/Character Select/Grid/Player Color SY");

OPTION_DECLARE(int, UI_CHARSELECT_PRESSTART_BASE_X, 290);
OPTION_DECLARE(int, UI_CHARSELECT_PRESSTART_BASE_Y, 420);
OPTION_DECLARE(int, UI_CHARSELECT_PRESSTART_STEP_X, 430);
OPTION_DEFINE(int, UI_CHARSELECT_PRESSTART_BASE_X, "UI/Character Select/PressStart Base X");
OPTION_DEFINE(int, UI_CHARSELECT_PRESSTART_BASE_Y, "UI/Character Select/PressStart Base Y");
OPTION_DEFINE(int, UI_CHARSELECT_PRESSTART_STEP_X, "UI/Character Select/PressStart Step X");

#define CHARGRID_SX 8
#define CHARGRID_SY 1

CharGrid::CharGrid(Client * client, LobbyMenu * menu)
	: m_client(client)
	, m_menu(menu)
{
}

void CharGrid::tick(float dt)
{
	// todo : update animations
}

void CharGrid::draw()
{
	const GameSim * gameSim = m_client->m_gameSim;

	int py = CHARCELL_INIT_Y;

	for (int cy = 0; cy < CHARGRID_SY; ++cy)
	{
		int px = CHARCELL_INIT_X;

		for (int cx = 0; cx < CHARGRID_SX; ++cx)
		{
			const int x1 = px;
			const int y1 = py;
			const int x2 = px + CHARCELL_SX;
			const int y2 = py + CHARCELL_SY;

			if (g_devMode)
			{
				setColor(colorWhite);
				drawRectLine(x1, y1, x2, y2);
			}

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				const Player & player = gameSim->m_players[i];
				if (!player.m_isUsed)
					continue;
				int x, y;
				characterIndexToXY(player.m_characterIndex, x, y);
				if (x == cx && y == cy)
				{
				#if 0
					const int rx1 = x1 + i * PLAYERCOL_SX;
					const int ry1 = y1 + 0;
					const int rx2 = rx1 + PLAYERCOL_SX;
					const int ry2 = ry1 + PLAYERCOL_SY / (player.m_isReadyUpped ? 2 : 1);
					const float a = 1.f;
				#else
					const int rx1 = x1 + 0;
					const int ry1 = y1 + CHARCELL_SY * i / MAX_PLAYERS;
					const int rx2 = rx1 + CHARCELL_SX;
					const int ry2 = ry1 + CHARCELL_SY / MAX_PLAYERS;
					const float a = .5f;
				#endif

					const Color playerColor = getPlayerColor(i);
					const Color color =
						player.m_isReadyUpped
						 ? playerColor
						 : playerColor.interp(colorWhite, std::fmod(gameSim->m_physicalRoundTime * 2.f, 1.f) * .5f);

					setColorf(
						color.r,
						color.g,
						color.b,
						a);
					drawRect(rx1, ry1, rx2, ry2);
				}
			}

			px += CHARCELL_SX + CHARCELL_SPACING_X;
		}

		py += CHARCELL_SY + CHARCELL_SPACING_Y;
	}
}

void CharGrid::characterIndexToXY(int characterIndex, int & x, int & y)
{
	x = characterIndex;
	y = 0;
}

void CharGrid::modulateXY(int & x, int & y)
{
	x = (x + MAX_CHARACTERS) % MAX_CHARACTERS;
	Assert(x >= 0 && x < MAX_CHARACTERS);
	y = 0;
}

int CharGrid::xyToCharacterIndex(int x, int y)
{
	const int result = x;
	Assert(result >= 0 && result < MAX_CHARACTERS);
	return result;
}

bool CharGrid::isValidGridCell(int x, int y)
{
	return true;
}

//

LobbyMenu::LobbyMenu(Client * client)
	: m_client(client)
	, m_charGrid(0)
	, m_prevGameMode(new Button(50,  GFX_SY - 80, "charselect-prev.png", "charselect-prev.png", 0, 0, 0, 0, 0, MAINMENU_BUTTON_TEXT_COLOR))
	, m_nextGameMode(new Button(150, GFX_SY - 80, "charselect-next.png", "charselect-prev.png", 0, 0, 0, 0, 0, MAINMENU_BUTTON_TEXT_COLOR))
{
	m_charGrid = new CharGrid(client, this);
}

LobbyMenu::~LobbyMenu()
{
	delete m_charGrid;
	m_charGrid = 0;

	delete m_prevGameMode;
	delete m_nextGameMode;
}

void LobbyMenu::tick(float dt)
{
	cpuTimingBlock(lobbyMenuTick);

	m_prevGameMode->setPosition(
		UI_CHARSELECT_GAMEMODE_SELECT_X,
		UI_CHARSELECT_GAMEMODE_SELECT_Y);
	m_nextGameMode->setPosition(
		UI_CHARSELECT_GAMEMODE_SELECT_X + 60,
		UI_CHARSELECT_GAMEMODE_SELECT_Y);

	if (g_app->m_isHost && g_app->isSelectedClient(m_client) && UI_LOBBY_GAMEMODE_SELECT_ENABLE)
	{
		if (m_client->m_gameSim->m_gameStartTicks == 0)
		{
			if (m_prevGameMode->isClicked())
				g_app->netAction(m_client->m_channel, kPlayerInputAction_CycleGameMode, -1, 0);
			if (m_nextGameMode->isClicked())
				g_app->netAction(m_client->m_channel, kPlayerInputAction_CycleGameMode, +1, 0);
		}
	}

	m_charGrid->tick(dt);
}

void LobbyMenu::draw()
{
	const GameSim * gameSim = m_client->m_gameSim;

#if ITCHIO_BUILD
	setColor(colorWhite);
	Sprite("itch-lobby-back.png").draw();

	// draw player bubbles and text
	setMainFont();

	const int bubbleBaseX = 186;
	const int bubbleBaseY = 170;
	const int bubbleDx = 310 - bubbleBaseX;
	const int bubbleDy = 290 - bubbleBaseY;
	const int charSpacing = 200;

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		auto & p = gameSim->m_players[i];
		if (p.m_isUsed)
		{
			Sprite back("itch-lobby-bubble-back.png");
			Sprite front("itch-lobby-bubble-front.png");

			int idx = 0;
			for (int o = 0; o < _countof(g_validCharacterIndices); ++o)
				if (p.m_characterIndex == g_validCharacterIndices[o])
					idx = o;

			const int bubbleX = bubbleBaseX + ((i == 0 || i == 2) ? 0 : bubbleDx);
			const int bubbleY = bubbleBaseY + ((i == 0 || i == 3) ? 0 : bubbleDy);

			setColor(colorWhite);
			back.drawEx(bubbleX + idx * charSpacing - back.getWidth() / 2, bubbleY - back.getHeight() / 2);

			if (p.m_isReadyUpped)
				setColor(255, 255, 255, 255, 200);
			else
				setColor(getPlayerColor(i));
			front.drawEx(bubbleX + idx * charSpacing - front.getWidth() / 2, bubbleY - front.getHeight() / 2);

			const int xOff = -2;
			const int yOff = -4;
		#if 0
			setColor(0, 0, 0, 255);
			for (int x = -1; x <= +1; ++x)
				for (int y = -1; y <= +1; ++y)
					drawText(xOff + bubbleX + idx * charSpacing + x * 2, yOff + bubbleY + y * 2, 24, 0.f, 0.f, "P%d", i);
		#endif
			setColor(colorWhite);
			drawText(xOff + bubbleX + idx * charSpacing, yOff + bubbleY, 24, 0.f, 0.f, "P%d", i + 1);
		}
	}

	const int baseX = 1040;
	const int baseY = 140;
	const int dy = 45;
	const int fontSize = 30;
	int numPressStart = 0;
	int numFreeControllers = 0;
	for (int i = 0; i < MAX_GAMEPAD; ++i)
		if (gamepad[i].isConnected && g_app->isControllerIndexAvailable(i))
			numFreeControllers++;

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		auto & p = gameSim->m_players[i];

		const char * text = 0;
		Color color;

		if (p.m_isUsed)
		{
			if (p.m_isReadyUpped)
			{
				text = "READY";
				color = Color(244, 244, 160);
			}
			else
			{
				text = "NOT READY";
				color = Color(130, 130, 130);
			}
		}
		else if (i >= MAX_GAMEPAD)
		{
		}
		else if (numPressStart < numFreeControllers)
		{
			text = "PRESS START";
			color = Color(1.f, 1.f, 1.f, std::fmod(gameSim->m_physicalRoundTime * 1.5f, .9f));

			numPressStart++;
		}
		else
		{
			text = "Please connect Xbox controller";
			color = Color(130, 130, 130, 100);
		}

		if (text)
		{
			setMainFont();
			setColor(color);

			drawText(
				baseX,
				baseY + dy * i,
				fontSize,
				+1.f, +1.f,
				"Player %d: %s",
				i + 1,
				text);
		}
	}
#else
	setColor(colorWhite);
	const float t = gameSim->m_physicalRoundTime;
	const float s = 0.05f;
	const float dx = std::sinf(t * 1.123f * s) * 100.f;
	const float dy = std::cosf(t * 1.234f * s) * 50.f;
	Sprite stars("ui/lobby/stars.png");
	stars.x = (GFX_SX - stars.getWidth()) / 2.f + dx;
	stars.y = (GFX_SY - stars.getHeight()) / 2.f + dy;
	stars.pixelpos = false;
	stars.filter = FILTER_LINEAR;
	stars.draw();

	setColor(colorWhite);
	Sprite("ui/lobby/background.png").draw();

	m_charGrid->draw();

	setMainFont();
	setColorf(1.f, 1.f, 1.f, std::fmod(gameSim->m_physicalRoundTime * 1.5f, 1.f));
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (i >= MAX_GAMEPAD || !gamepad[i].isConnected)
			continue;
		if (!g_app->isControllerIndexAvailable(i))
			continue;

		drawText(
			UI_CHARSELECT_PRESSTART_BASE_X + i * UI_CHARSELECT_PRESSTART_STEP_X,
			UI_CHARSELECT_PRESSTART_BASE_Y,
			48,
			0.f, 0.f,
			"P%d PRESS START",
			i + 1);
	}
#endif

	if (g_app->m_isHost && UI_LOBBY_GAMEMODE_SELECT_ENABLE)
	{
		setColor(colorWhite);
		m_prevGameMode->draw();
		m_nextGameMode->draw();
	}

	// draw current game mode selection

	if (UI_LOBBY_GAMEMODE_SELECT_ENABLE)
	{
		setMainFont();
		setColor(127, 255, 227);
		drawText(
			UI_CHARSELECT_GAMEMODE_TEXT_X,
			UI_CHARSELECT_GAMEMODE_TEXT_Y,
			48, 0.f, 0.f, "[%s]", g_gameModeNames[gameSim->m_desiredGameMode]);
	}

	// draw game start timer

	if (gameSim->m_gameStartTicks != 0)
	{
		const float timeRemaining = gameSim->m_gameStartTicks / float(TICKS_PER_SECOND);

		// todo : options for position
		setMainFont();
		setColor(127, 255, 227);
		drawText(GFX_SX/2, 450, 48, 0.f, 0.f, "ROUND START IN T-%02.2f", timeRemaining);
	}

	setColor(colorWhite);
}
