#include "Channel.h"
#include "client.h"
#include "lobbymenu.h"
#include "main.h"
#include "uicommon.h"

CharSelector::CharSelector(Client * client, int playerId)
	: m_client(client)
	, m_playerId(playerId)
	, m_prevChar(new Button(playerId * 100 +  50, GFX_SY*3/4, "charselect-prev.png"))
	, m_nextChar(new Button(playerId * 100 + 100, GFX_SY*3/4, "charselect-next.png"))
{
}

CharSelector::~CharSelector()
{
	delete m_prevChar;
	delete m_nextChar;
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
}

void CharSelector::draw()
{
	Player & player = m_client->m_gameSim->m_players[m_playerId];

	if (player.m_isUsed && player.m_owningChannelId == m_client->m_channel->m_id)
	{
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
	}
}
