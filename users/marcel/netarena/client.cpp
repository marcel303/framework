#include <algorithm>
#include "arena.h"
#include "bullet.h"
#include "Calc.h"
#include "Channel.h"
#include "client.h"
#include "Debugging.h"
#include "framework.h"
#include "host.h"
#include "lobbymenu.h"
#include "main.h"
#include "player.h"
#include "quicklook.h"
#include "textfield.h"
#include "Timer.h"

OPTION_EXTERN(int, g_playerCharacterIndex);

Client::Client()
	: m_channel(0)
	, m_lobbyMenu(0)
	, m_textChat(0)
	, m_textChatFade(0.f)
	, m_quickLook(0)
	, m_gameSim(0)
	, m_syncStream(0)
	, m_isSynced(false)
	, m_isDesync(false)
	, m_hasAddedPlayers(false)
{
	m_lobbyMenu = new LobbyMenu(this);

	m_textChat = new TextField(UI_TEXTCHAT_X, UI_TEXTCHAT_Y, UI_TEXTCHAT_SX, UI_TEXTCHAT_SY);

	m_quickLook = new QuickLook();

	m_gameSim = new GameSim();

	m_syncStream = new BitStream();

	m_gameSim->setGameState(kGameState_Connecting);
}

Client::~Client()
{
	Assert(m_players.empty());

	delete m_syncStream;
	m_syncStream = 0;

	delete m_gameSim;
	m_gameSim = 0;

	delete m_quickLook;
	m_quickLook = 0;

	delete m_textChat;
	m_textChat = 0;

	delete m_lobbyMenu;
	m_lobbyMenu = 0;
}

void Client::initialize(Channel * channel)
{
	m_channel = channel;
}

void Client::tick(float dt)
{
	if (m_channel && m_channel->m_state == ChannelState_Connected)
	{
		if (m_isSynced && !m_hasAddedPlayers)
		{
			m_hasAddedPlayers = true;

			for (int i = 0; i < NUM_LOCAL_PLAYERS_TO_ADD; ++i)
			{
				g_app->netAddPlayer(m_channel, g_playerCharacterIndex != -1 ? g_playerCharacterIndex : g_validCharacterIndices[0], g_app->m_displayName, -1);
			}
		}

		// see if there are more players than gamepads, to decide if we should use keyboard exclusively for one player, or also assign them a gamepad

		int numGamepads = 0;
		for (int i = 0; i < MAX_GAMEPAD; ++i)
			if (gamepad[i].isConnected)
				numGamepads++;

		const bool morePlayersThanControllers = (numGamepads < g_app->getControllerAllocationCount());

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			const Player & player = m_gameSim->m_players[i];

			if (!player.m_isUsed || player.m_owningChannelId != m_channel->m_id)
				continue;

			PlayerInstanceData * playerInstanceData = player.m_instanceData;

			int highest = -1;
			for (int i = 0; i < MAX_GAMEPAD + 1; ++i)
			{
				bool used = true;
				for (size_t j = 0; j < g_app->m_freeControllerList.size(); ++j)
					if (g_app->m_freeControllerList[j] == i)
						used = false;
				if (used)
					highest = i;
			}

			const bool useKeyboard = playerInstanceData->m_input.m_controllerIndex == highest;
			const bool useGamepad = true;
			const int gamepadIndex = useGamepad ? playerInstanceData->m_input.m_controllerIndex : -1;

			PlayerInput input;

			input.gather(useKeyboard, gamepadIndex, g_monkeyMode);

			if (g_keyboardLock == 0)
			{
				if (input != playerInstanceData->m_input.m_lastSent)
				{
					playerInstanceData->m_input.m_lastSent = input;

					g_app->netSetPlayerInputs(
						m_channel->m_id,
						player.m_index,
						input);
				}
			}
		}

		//

		m_lobbyMenu->tick(dt);

		//

		for (auto i = m_gameSim->m_annoucements.begin(); i != m_gameSim->m_annoucements.end(); )
		{
			i->timeLeft -= dt;

			if (i->timeLeft <= 0.f)
				i = m_gameSim->m_annoucements.erase(i);
			else
				++i;
		}

		// text chat

		if (m_textChat->tick(dt))
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				const Player & player = m_gameSim->m_players[i];

				if (!player.m_isUsed || player.m_owningChannelId != m_channel->m_id)
					continue;

				g_app->netAction(m_channel, 	kNetAction_TextChat, i, 0, m_textChat->getText());

				LOG_DBG("client %p owns player %d", this, i);
			}
		}

		if (m_textChat->isActive())
			m_textChatFade = 1.f;
		else if (m_textChatFade > 0.f)
		{
			m_textChatFade -= dt * 3.f;
			if (m_textChatFade < 0.f)
				m_textChatFade = 0.f;
		}

		if (g_keyboardLock == 0 && keyboard.wentDown(SDLK_t) && UI_TEXTCHAT_ENABLE)
			m_textChat->open(UI_TEXTCHAT_MAX_TEXT_SIZE, true);

		// quicklook

		bool isQuickLookRequested = false;

		for (int i = 0; i < MAX_PLAYERS; ++i)
			isQuickLookRequested |= m_gameSim->m_players[i].m_isUsed && m_gameSim->m_players[i].m_input.isDown(INPUT_BUTTON_L1);
		
		isQuickLookRequested &= !DEMOMODE;

		if (isQuickLookRequested)
		{
			if (!m_quickLook->isActive())
				m_quickLook->open(true);
		}
		else
		{
			m_quickLook->close(true);
		}
	}
	else
	{
		if (m_textChat->isActive())
		{
			m_textChat->close();
		}

		if (m_quickLook->isActive())
		{
			m_quickLook->close(false);
		}
	}

	m_quickLook->tick(dt);
}

void Client::draw()
{
	// todo : move drawing code to game sim too?

	Assert(!g_gameSim);
	g_gameSim = m_gameSim;

	switch (m_gameSim->m_gameState)
	{
	case kGameState_Initial:
		Assert(false);
		break;

	case kGameState_Connecting:
		drawConnecting();
		break;

	case kGameState_OnlineMenus:
		{
			drawMenus();
			GameSim::CamParams camParams;
			camParams.shake.SetZero();
			camParams.zoom = 1.f;
			camParams.zoomFocus.Set(GFX_SX/2.f, GFX_SY/2.f);
			m_gameSim->drawPlayColor(camParams);
		}
		break;

	case kGameState_RoundBegin:
		drawPlay();
		break;

	case kGameState_Play:
		drawPlay();
		break;

	case kGameState_RoundComplete:
		drawPlay();
		drawRoundComplete();
		break;

	default:
		Assert(false);
		break;
	}

	drawAnnouncements();

	drawTextChat();

	drawQuickLook();

	g_gameSim = 0;
}

void Client::drawConnecting()
{
#if !ENABLE_NETWORKING
	setColor(colorBlack);
	drawRect(0, 0, GFX_SX, GFX_SY);
#else
	setColor(colorBlue);
	drawRect(0, 0, GFX_SX, GFX_SY);
	setColor(colorWhite);
	setMainFont();
	drawText(GFX_SX/2, GFX_SY/2, 32, 0.f, 0.f, "Connecting..");
	setColor(colorWhite);
#endif
}

void Client::drawMenus()
{
#if 0 // player character debug text
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		const Player & player = m_gameSim->m_players[i];

		const int y = GFX_SY * 3 / 4 + i * 50;

		if (player.m_isUsed)
		{
			setDebugFont();
			setColor(colorWhite);
			drawText(GFX_SX/2, y, 24, 0.f, 0.f, "PLAYER %d CHAR %d", i, player.m_characterIndex);
		}
		else
		{
			setDebugFont();
			setColor(127, 127, 127);
			drawText(GFX_SX/2, y, 24, 0.f, 0.f, "PLAYER %d NOT CONNECTED", i);
		}
	}
#endif

	if (m_gameSim->m_gameState == kGameState_OnlineMenus)
	{
		m_lobbyMenu->draw();
	}
	else
	{
		setColor(colorWhite);
		Sprite("mainmenu-back.png").draw();
	}

	setColor(colorWhite);
}

void Client::drawPlay()
{
	m_gameSim->drawPlay();
}

void Client::drawQuickLook()
{
	m_quickLook->drawWorld(*m_gameSim);
	m_quickLook->drawHud(*m_gameSim);
}

void Client::drawRoundComplete()
{
#if 0
	const int numTicks = TICKS_PER_SECOND * GAMESTATE_ROUNDCOMPLETE_TIMEDILATION_TIMER;
	const float t = 1.f - Calc::Saturate(m_gameSim->m_roundCompleteTimeDilationTicks / float(numTicks));
	setColorf(0.f, 0.f, 0.f, t);
	drawRect(0, 0, GFX_SX, GFX_SY);
#endif

	setColor(colorWhite);

	int y = GFX_SY / 4;

	setMainFont();
	drawText(GFX_SX / 2, y, 60, 0.f, 0.f, "Round Complete!");

	y += 80;
	int index = 0;

	drawText(GFX_SX/3*1, y, 45, +1.f, +1.f, "PLAYER");
	drawText(GFX_SX/3*2, y, 45, -1.f, +1.f, "SCORE");

	y += 35;

	drawText(GFX_SX/2, y, 45, 0.f, 0.f, "------------------------------------------");

	y += 70;

	auto players = m_players;
	std::sort(players.begin(), players.end(), [](PlayerInstanceData * p1, PlayerInstanceData * p2) { return p1->m_player->m_score > p2->m_player->m_score; });

	for (auto p = players.begin(); p != players.end(); ++p)
	{
		const PlayerInstanceData * playerInstanceData = *p;
		const Player * player = playerInstanceData->m_player;

		setColor(getPlayerColor(player->m_index));
		drawText(GFX_SX/3*1, y, 40, +1.f, +1.f, "%s", player->m_displayName.c_str());
		drawText(GFX_SX/3*2, y, 40, -1.f, +1.f, "%d", player->m_score);

		index++;
		y += 50;
	}

	setColor(colorWhite);
}

void Client::drawAnnouncements()
{
	int y = GFX_SY/3;
	const int sy = 60;

	for (auto i = m_gameSim->m_annoucements.begin(); i != m_gameSim->m_annoucements.end(); ++i)
	{
		//setColor(0, 0, 255, 127);
		setColorf(i->color.r, i->color.g, i->color.b, .5f);
		drawRect(0, y, GFX_SX, y + sy);

		setColor(colorWhite);
		setMainFont();
		drawText(GFX_SX/2, y + 5, 40, 0.f, +1.f, i->message.c_str());

		y += 60;
	}

	setColor(colorWhite);
}

void Client::drawTextChat()
{
	m_textChat->draw();

	if (m_textChatFade != 0.f)
	{
		int maxLines = 10;
		int x = GFX_SX - 500;
		int y = GFX_SY - 50;
		const int stepY = 40;
		const int sizeX = 400;

		setColor(0, 0, 0, 127 * m_textChatFade);
		drawRect(x, y - stepY * maxLines, x + sizeX, y);

		for (auto i = m_textChatLog.begin(); i != m_textChatLog.end(); ++i)
		{
			y -= 40;

			const TextChatLog & log = *i;

			setMainFont();

			const Color color = getPlayerColor(log.playerIndex);
			setColorf(color.r, color.g, color.b, m_textChatFade);
			drawText(x - 10, y, 24, -1.f, +1.f, "%s:", log.playerDisplayName.c_str());

			setColor(227, 227, 227, 255 * m_textChatFade);
			drawText(x, y, 24, +1.f, +1.f, "%s", log.message.c_str());
		}

		setColor(colorWhite);
	}
}

void Client::debugDraw()
{
	m_gameSim->m_tokenHunt.m_token.drawBB();
}

void Client::addPlayer(PlayerInstanceData * player, int controllerIndex)
{
	LOG_DBG("%s: player=%p, controllerIndex=%d", __FUNCTION__, player, controllerIndex);

	player->m_player->m_isUsed = true;

	Assert(std::find(m_players.begin(), m_players.end(), player) == m_players.end());
	m_players.push_back(player);

	m_gameSim->m_playerInstanceDatas[player->m_player->m_index] = player;
	//Assert(m_gameSim->m_playerInstanceDatas[player->m_player->m_index] == player);
	Assert(player->m_input.m_controllerIndex == -1);

	if (player->m_player->m_owningChannelId == m_channel->m_id)
	{
		player->m_input.m_controllerIndex = g_app->allocControllerIndex(controllerIndex);
	}
}

void Client::removePlayer(PlayerInstanceData * player)
{
	LOG_DBG("%s: player=%p", __FUNCTION__, player);

	auto i = std::find(m_players.begin(), m_players.end(), player);
	Assert(i != m_players.end());

	if (i != m_players.end())
	{
		if (player->m_player->m_owningChannelId == m_channel->m_id)
		{
			Assert(player->m_input.m_controllerIndex != -1);
			if (player->m_input.m_controllerIndex != -1)
				g_app->freeControllerIndex(player->m_input.m_controllerIndex);
			player->m_input.m_controllerIndex = -1;
		}

		m_players.erase(i);
	}
}

PlayerInstanceData * Client::findPlayerByPlayerId(uint8_t playerId)
{
	for (auto p = m_players.begin(); p != m_players.end(); ++p)
	{
		PlayerInstanceData * playerInstanceData = *p;
		if (playerInstanceData->m_player->m_index == playerId)
			return playerInstanceData;
	}

	return 0;
}

bool Client::isLocalPlayer(uint8_t playerId)
{
	const Player & player = m_gameSim->m_players[playerId];
	return player.m_isUsed && player.m_owningChannelId == m_channel->m_id;
}

void Client::setPlayerPtrs()
{
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
		(*i)->m_player->m_instanceData = (*i);
}

void Client::clearPlayerPtrs()
{
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
		(*i)->m_player->m_instanceData = 0;
}

void Client::spawnParticles(const ParticleSpawnInfo & spawnInfo)
{
	m_gameSim->spawnParticles(spawnInfo);
}

void Client::addTextChat(int playerIndex, const std::string & text)
{
	TextChatLog log;
	log.playerIndex = playerIndex;
	log.characterIndex = m_gameSim->m_players[playerIndex].m_characterIndex;
	log.message = text;
	log.playerDisplayName = m_gameSim->m_players[playerIndex].m_displayName.c_str();

	m_textChatLog.push_front(log);
	while (m_textChatLog.size() > 10) // todo : add max log size option
		m_textChatLog.pop_back();
}
