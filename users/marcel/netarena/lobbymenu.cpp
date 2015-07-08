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

#define JOIN_SPRITER Spriter("ui/lobby/join.scml")

OPTION_DECLARE(int, UI_CHARSELECT_GAMEMODE_TEXT_X, 350);
OPTION_DECLARE(int, UI_CHARSELECT_GAMEMODE_TEXT_Y, 616);
OPTION_DECLARE(int, UI_CHARSELECT_GAMEMODE_SELECT_X, 105);
OPTION_DECLARE(int, UI_CHARSELECT_GAMEMODE_SELECT_Y, 630);
OPTION_DEFINE(int, UI_CHARSELECT_GAMEMODE_TEXT_X, "UI/Character Select/GameMode Text X");
OPTION_DEFINE(int, UI_CHARSELECT_GAMEMODE_TEXT_Y, "UI/Character Select/GameMode Text Y");
OPTION_DEFINE(int, UI_CHARSELECT_GAMEMODE_SELECT_X, "UI/Character Select/GameMode Select X");
OPTION_DEFINE(int, UI_CHARSELECT_GAMEMODE_SELECT_Y, "UI/Character Select/GameMode Select Y");

#if GRIDBASED_CHARSELECT

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

#endif

//

#if !GRIDBASED_CHARSELECT

OPTION_DECLARE(int, UI_CHARSELECT_BASE_X, 290);
OPTION_DECLARE(int, UI_CHARSELECT_BASE_Y, 270);
OPTION_DECLARE(int, UI_CHARSELECT_SIZE_X, 310);
OPTION_DECLARE(int, UI_CHARSELECT_SIZE_Y, 400);
OPTION_DECLARE(int, UI_CHARSELECT_STEP_X, 340);
OPTION_DECLARE(int, UI_CHARSELECT_PREV_X, 45);
OPTION_DECLARE(int, UI_CHARSELECT_PREV_Y, 300);
OPTION_DECLARE(int, UI_CHARSELECT_NEXT_X, 265);
OPTION_DECLARE(int, UI_CHARSELECT_NEXT_Y, 300);
OPTION_DECLARE(int, UI_CHARSELECT_READY_X, 155);
OPTION_DECLARE(int, UI_CHARSELECT_READY_Y, 350);
OPTION_DECLARE(int, UI_CHARSELECT_PORTRAIT_X, 155);
OPTION_DECLARE(int, UI_CHARSELECT_PORTRAIT_Y, 270);
OPTION_DEFINE(int, UI_CHARSELECT_BASE_X, "UI/Character Select/Base X");
OPTION_DEFINE(int, UI_CHARSELECT_BASE_Y, "UI/Character Select/Base Y");
OPTION_DEFINE(int, UI_CHARSELECT_SIZE_X, "UI/Character Select/Size X");
OPTION_DEFINE(int, UI_CHARSELECT_SIZE_Y, "UI/Character Select/Size Y");
OPTION_DEFINE(int, UI_CHARSELECT_STEP_X, "UI/Character Select/Step X");
OPTION_DEFINE(int, UI_CHARSELECT_PREV_X, "UI/Character Select/Prev X");
OPTION_DEFINE(int, UI_CHARSELECT_PREV_Y, "UI/Character Select/Prev Y");
OPTION_DEFINE(int, UI_CHARSELECT_NEXT_X, "UI/Character Select/Next X");
OPTION_DEFINE(int, UI_CHARSELECT_NEXT_Y, "UI/Character Select/Next Y");
OPTION_DEFINE(int, UI_CHARSELECT_READY_X, "UI/Character Select/Ready X");
OPTION_DEFINE(int, UI_CHARSELECT_READY_Y, "UI/Character Select/Ready Y");
OPTION_DEFINE(int, UI_CHARSELECT_PORTRAIT_X, "UI/Character Select/Portrait X");
OPTION_DEFINE(int, UI_CHARSELECT_PORTRAIT_Y, "UI/Character Select/Portrait Y");

CharSelector::CharSelector(Client * client, LobbyMenu * menu, int playerId)
	: m_client(client)
	, m_menu(menu)
	, m_playerId(playerId)
	, m_prevChar(new Button(0, 0, "charselect-prev.png"))
	, m_nextChar(new Button(0, 0, "charselect-next.png"))
	, m_ready(new Button(0, 0, "charselect-ready.png"))
{
	updateButtonLocations();
}

CharSelector::~CharSelector()
{
	delete m_prevChar;
	delete m_nextChar;
	delete m_ready;
}

void CharSelector::updateButtonLocations()
{
	m_prevChar->setPosition(
		UI_CHARSELECT_BASE_X + UI_CHARSELECT_PREV_X + m_playerId * UI_CHARSELECT_STEP_X,
		UI_CHARSELECT_BASE_Y + UI_CHARSELECT_PREV_Y);

	m_nextChar->setPosition(
		UI_CHARSELECT_BASE_X + UI_CHARSELECT_NEXT_X + m_playerId * UI_CHARSELECT_STEP_X,
		UI_CHARSELECT_BASE_Y + UI_CHARSELECT_NEXT_Y);

	m_ready->setPosition(
		UI_CHARSELECT_BASE_X + UI_CHARSELECT_READY_X + m_playerId * UI_CHARSELECT_STEP_X,
		UI_CHARSELECT_BASE_Y + UI_CHARSELECT_READY_Y);
}

void CharSelector::tick(float dt)
{
	const Player & player = m_client->m_gameSim->m_players[m_playerId];

	if (player.m_isUsed)
	{
		if (player.m_owningChannelId == m_client->m_channel->m_id)
		{
			if (m_prevChar->isClicked())
				g_app->netAction(m_client->m_channel, kNetAction_PlayerInputAction, m_playerId, kPlayerInputAction_PrevChar);
			if (m_nextChar->isClicked())
				g_app->netAction(m_client->m_channel, kNetAction_PlayerInputAction, m_playerId, kPlayerInputAction_NextChar);
			if (m_ready->isClicked())
				g_app->netAction(m_client->m_channel, kNetAction_PlayerInputAction, m_playerId, kPlayerInputAction_ReadyUp);
		}
	}

	if (g_devMode)
	{
		updateButtonLocations();
	}
}

void CharSelector::draw()
{
	const Player & player = m_client->m_gameSim->m_players[m_playerId];

	if (player.m_isUsed && player.m_owningChannelId == m_client->m_channel->m_id)
	{
		if (!player.m_isReadyUpped)
		{
			setColor(colorWhite);
			m_prevChar->draw();
			m_nextChar->draw();
		}
	}

	const int characterX = UI_CHARSELECT_BASE_X + UI_CHARSELECT_PORTRAIT_X + m_playerId * UI_CHARSELECT_STEP_X;
	const int characterY = UI_CHARSELECT_BASE_Y + UI_CHARSELECT_PORTRAIT_Y;

	if (player.m_isUsed)
	{
		Spriter spriter(makeCharacterFilename(player.m_characterIndex, "sprite/sprite.scml"));

		SpriterState spriterState;
		spriterState.animIndex = spriter.getAnimIndexByName("Idle");
		spriterState.animTime = g_TimerRT.Time_get();
		spriterState.x = characterX;
		spriterState.y = characterY;
		spriterState.scale = .7f;

		setColor(colorWhite);
		spriter.draw(spriterState);

		setColor(player.m_isReadyUpped ? colorBlue : colorWhite);
		m_ready->draw();
	}
	else
	{
		SpriterState state = m_menu->m_joinSpriterState;
		state.x = characterX;
		state.y = characterY;

		setColor(colorWhite);
		JOIN_SPRITER.draw(state);
	}

	setColor(colorWhite);
}

bool CharSelector::isLocalPlayer() const
{
	const Player & player = m_client->m_gameSim->m_players[m_playerId];
	return player.m_isUsed && player.m_owningChannelId == m_client->m_channel->m_id;
}
#endif

//

LobbyMenu::LobbyMenu(Client * client)
	: m_client(client)
#if GRIDBASED_CHARSELECT
	, m_charGrid(0)
#endif
	, m_prevGameMode(new Button(50,  GFX_SY - 80, "charselect-prev.png"))
	, m_nextGameMode(new Button(150, GFX_SY - 80, "charselect-next.png"))
{
#if GRIDBASED_CHARSELECT
	m_charGrid = new CharGrid(client, this);
#else
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_charSelectors[i] = new CharSelector(client, this, i);
	}
#endif

	m_joinSpriterState.startAnim(JOIN_SPRITER, 0);
}

LobbyMenu::~LobbyMenu()
{
#if !GRIDBASED_CHARSELECT
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		delete m_charSelectors[i];
	}
#else
	delete m_charGrid;
	m_charGrid = 0;
#endif

	delete m_prevGameMode;
	delete m_nextGameMode;
}

void LobbyMenu::tick(float dt)
{
	m_joinSpriterState.updateAnim(JOIN_SPRITER, dt);

	m_prevGameMode->setPosition(
		UI_CHARSELECT_GAMEMODE_SELECT_X,
		UI_CHARSELECT_GAMEMODE_SELECT_Y);
	m_nextGameMode->setPosition(
		UI_CHARSELECT_GAMEMODE_SELECT_X + 60,
		UI_CHARSELECT_GAMEMODE_SELECT_Y);

	if (g_app->m_isHost && g_app->isSelectedClient(m_client))
	{
		if (m_client->m_gameSim->m_gameStartTicks == 0)
		{
			if (m_prevGameMode->isClicked())
				g_app->netAction(m_client->m_channel, kPlayerInputAction_CycleGameMode, -1, 0);
			if (m_nextGameMode->isClicked())
				g_app->netAction(m_client->m_channel, kPlayerInputAction_CycleGameMode, +1, 0);
		}
	}

#if GRIDBASED_CHARSELECT
	m_charGrid->tick(dt);
#else
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_charSelectors[i]->tick(dt);
	}
#endif
}

void LobbyMenu::draw()
{
	const GameSim * gameSim = m_client->m_gameSim;

#if GRIDBASED_CHARSELECT
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
#endif

	setColor(colorWhite);
	Sprite("ui/lobby/background.png").draw();

#if GRIDBASED_CHARSELECT
	m_charGrid->draw();

	setFont("calibri.ttf");
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

	if (g_app->m_isHost)
	{
		setColor(colorWhite);
		m_prevGameMode->draw();
		m_nextGameMode->draw();
	}

#if !GRIDBASED_CHARSELECT
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (g_devMode || true) // fixme
		{
			const int x1 = UI_CHARSELECT_BASE_X + UI_CHARSELECT_STEP_X * i;
			const int y1 = UI_CHARSELECT_BASE_Y;
			const int x2 = UI_CHARSELECT_BASE_X + UI_CHARSELECT_STEP_X * i + UI_CHARSELECT_SIZE_X;
			const int y2 = UI_CHARSELECT_BASE_Y + UI_CHARSELECT_SIZE_Y;

			const float v = m_client->isLocalPlayer(i) ? (std::sinf(g_TimerRT.Time_get() * M_PI * 2.f / 1.5f) + 1.f) / 10.f : 0.f;

			setColorf(v, v, v, .7f);
			drawRect(x1, y1, x2, y2);
		}

		m_charSelectors[i]->draw();

		if (g_devMode || true) // fixme
		{
			const int x1 = UI_CHARSELECT_BASE_X + UI_CHARSELECT_STEP_X * i;
			const int y1 = UI_CHARSELECT_BASE_Y;
			const int x2 = UI_CHARSELECT_BASE_X + UI_CHARSELECT_STEP_X * i + UI_CHARSELECT_SIZE_X;
			const int y2 = UI_CHARSELECT_BASE_Y + UI_CHARSELECT_SIZE_Y;

			setColor(colorGreen);
			drawRectLine(x1, y1, x2, y2);

			if (m_client->m_gameSim->m_players[i].m_isUsed)
			{
				int y = 8;

				setColor(colorWhite);
				drawText((x1 + x2) / 2, y2 + y, 32, 0.f, +1.f, "%s", m_client->m_gameSim->m_players[i].m_displayName.c_str());
				y += 34;

				const CharacterData * characterData = getCharacterData(m_client->m_gameSim->m_players[i].m_characterIndex);
				setColor(colorWhite);
				drawText((x1 + x2) / 2, y2 + y, 32, 0.f, +1.f, "%s", g_playerSpecialNames[characterData->m_special]);
				y += 34;

			#if 1 // todo : non final only
				if (g_host)
				{
					setColor(colorWhite);
					drawText((x1 + x2) / 2, y2 + y, 24, 0.f, +1.f, "%s", m_client->m_channel->m_address.ToString(true).c_str());
					y += 34;
				}
			#endif
			}
		}
	}
#endif

	// draw current game mode selection

	setFont("calibri.ttf");
	setColor(127, 255, 227);
	drawText(
		UI_CHARSELECT_GAMEMODE_TEXT_X,
		UI_CHARSELECT_GAMEMODE_TEXT_Y,
		48, 0.f, 0.f, "[%s]", g_gameModeNames[gameSim->m_desiredGameMode]);

	// draw game start timer

	if (gameSim->m_gameStartTicks != 0)
	{
		const float timeRemaining = gameSim->m_gameStartTicks / float(TICKS_PER_SECOND);

		// todo : options for position
		setFont("calibri.ttf");
		setColor(127, 255, 227);
		drawText(GFX_SX/2, 330, 48, 0.f, 0.f, "ROUND START IN T-%02.2f", timeRemaining);
	}

	setColor(colorWhite);
}
