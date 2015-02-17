#include "Channel.h"
#include "client.h"
#include "framework.h"
#include "lobbymenu.h"
#include "main.h"
#include "uicommon.h"

CharSelector::CharSelector(Client * client, int playerId)
	: m_client(client)
	, m_playerId(playerId)
	, m_prevChar(new Button(0, 0, "charselect-prev.png"))
	, m_nextChar(new Button(0, 0, "charselect-next.png"))
{
	updateButtonLocations();
}

CharSelector::~CharSelector()
{
	delete m_prevChar;
	delete m_nextChar;
}

void CharSelector::updateButtonLocations()
{
	m_prevChar->setPosition(
		UI_CHARSELECT_BASE_X + UI_CHARSELECT_PREV_X + m_playerId * UI_CHARSELECT_STEP_X,
		UI_CHARSELECT_BASE_Y + UI_CHARSELECT_PREV_Y);

	m_nextChar->setPosition(
		UI_CHARSELECT_BASE_X + UI_CHARSELECT_NEXT_X + m_playerId * UI_CHARSELECT_STEP_X,
		UI_CHARSELECT_BASE_Y + UI_CHARSELECT_NEXT_Y);
}

void CharSelector::tick(float dt)
{
	Player & player = m_client->m_gameSim->m_players[m_playerId];

	if (player.m_isUsed && player.m_owningChannelId == m_client->m_channel->m_id)
	{
		// fixme : max characters = ?
		const int MAX_CHARACTERS = 4;

		if (m_prevChar->isClicked())
		{
			const int characterIndex = (player.m_characterIndex + MAX_CHARACTERS - 1) % MAX_CHARACTERS;

			g_app->netSetPlayerCharacterIndex(m_client->m_channel->m_id, m_playerId, characterIndex);
		}
		else if (m_nextChar->isClicked())
		{
			const int characterIndex = (player.m_characterIndex + MAX_CHARACTERS + 1) % MAX_CHARACTERS;

			g_app->netSetPlayerCharacterIndex(m_client->m_channel->m_id, m_playerId, characterIndex);
		}
	}

	if (g_devMode)
	{
		updateButtonLocations();
	}
}

void CharSelector::draw()
{
	Player & player = m_client->m_gameSim->m_players[m_playerId];

	if (player.m_isUsed && player.m_owningChannelId == m_client->m_channel->m_id)
	{
		setColor(colorWhite);
		m_prevChar->draw();
		m_nextChar->draw();
	}
}

//

LobbyMenu::LobbyMenu(Client * client)
	: m_client(client)
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
}

void LobbyMenu::tick(float dt)
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_charSelectors[i]->tick(dt);
	}
}

void LobbyMenu::draw()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_charSelectors[i]->draw();

		if (g_devMode)
		{
			const int x1 = UI_CHARSELECT_BASE_X + UI_CHARSELECT_STEP_X * i;
			const int y1 = UI_CHARSELECT_BASE_Y;
			const int x2 = UI_CHARSELECT_BASE_X + UI_CHARSELECT_STEP_X * i + UI_CHARSELECT_SIZE_X;
			const int y2 = UI_CHARSELECT_BASE_Y + UI_CHARSELECT_SIZE_Y;

			setColor(colorGreen);
			drawRectLine(x1, y1, x2, y2);
		}
	}

	setColor(colorWhite);
}
