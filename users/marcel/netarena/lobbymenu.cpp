#include "Channel.h"
#include "client.h"
#include "framework.h"
#include "lobbymenu.h"
#include "main.h"
#include "player.h"
#include "Timer.h" // for character anim time
#include "uicommon.h"

CharSelector::CharSelector(Client * client, int playerId)
	: m_client(client)
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

	if (player.m_isUsed)
	{
		setColorMode(COLOR_ADD);
		{
			const Color color = getCharacterColor(player.m_characterIndex);
			setColorf(
				color.r,
				color.g,
				color.b,
				1.f,
				.25f);

			Spriter spriter(makeCharacterFilename(player.m_characterIndex, "sprite/sprite.scml"));

			SpriterState spriterState;
			spriterState.animIndex = spriter.getAnimIndexByName("Idle");
			spriterState.animTime = g_TimerRT.Time_get();
			spriterState.x = UI_CHARSELECT_BASE_X + UI_CHARSELECT_PORTRAIT_X + m_playerId * UI_CHARSELECT_STEP_X;
			spriterState.y = UI_CHARSELECT_BASE_Y + UI_CHARSELECT_PORTRAIT_Y;
			spriterState.scale = .7f;

			spriter.draw(spriterState);
		}
		setColorMode(COLOR_MUL);

		setColor(player.m_isReadyUpped ? colorBlue : colorWhite);
		m_ready->draw();
	}

	setColor(colorWhite);
}

//

LobbyMenu::LobbyMenu(Client * client)
	: m_client(client)
	, m_prevGameMode(new Button(50,  GFX_SY - 80, "charselect-prev.png"))
	, m_nextGameMode(new Button(150, GFX_SY - 80, "charselect-next.png"))
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_charSelectors[i] = new CharSelector(client, i);
	}
}

LobbyMenu::~LobbyMenu()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		delete m_charSelectors[i];
	}

	delete m_prevGameMode;
	delete m_nextGameMode;
}

void LobbyMenu::tick(float dt)
{
	if (g_app->m_isHost)
	{
		if (m_prevGameMode->isClicked())
			g_app->netAction(m_client->m_channel, kPlayerInputAction_CycleGameMode, -1, 0);
		if (m_nextGameMode->isClicked())
			g_app->netAction(m_client->m_channel, kPlayerInputAction_CycleGameMode, +1, 0);
	}

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_charSelectors[i]->tick(dt);
	}
}

void LobbyMenu::draw()
{
	if (g_app->m_isHost)
	{
		setColor(colorWhite);
		m_prevGameMode->draw();
		m_nextGameMode->draw();
	}

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
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
				setColor(colorWhite);
				drawText((x1 + x2) / 2, y2 + 8, 32, 0.f, +1.f, "%s", m_client->m_gameSim->m_players[i].m_displayName.c_str());
			}
		}
	}

	setColor(colorWhite);
}
