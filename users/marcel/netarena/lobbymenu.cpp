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

#if GRIDBASED_CHARSELECT

OPTION_DECLARE(int, CHARCELL_INIT_X, 162);
OPTION_DECLARE(int, CHARCELL_INIT_Y, 102);
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

			setColor(colorWhite);
			drawRectLine(x1, y1, x2, y2);

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				const Player & player = gameSim->m_players[i];
				if (!player.m_isUsed)
					continue;
				int x, y;
				characterIndexToXY(player.m_characterIndex, x, y);
				if (x == cx && y == cy)
				{
					const int rx1 = x1 + i * PLAYERCOL_SX;
					const int ry1 = y1 + 0;
					const int rx2 = rx1 + PLAYERCOL_SX;
					const int ry2 = ry1 + PLAYERCOL_SY;

					const Color playerColor = getPlayerColor(i);
					setColorf(
						playerColor.r,
						playerColor.g,
						playerColor.b,
						player.m_isReadyUpped ? 1.f : .5f);
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
	, m_charGrid(0)
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
	setColor(colorWhite);
	Sprite("ui/lobby/background.png").draw();

	if (g_app->m_isHost)
	{
		setColor(colorWhite);
		m_prevGameMode->draw();
		m_nextGameMode->draw();
	}

	m_charGrid->draw();

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

	setColor(colorWhite);
}
