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
#include "textfield.h"
#include "Timer.h"

OPTION_DECLARE(bool, s_noBgm, false);
OPTION_DEFINE(bool, s_noBgm, "Sound/No BGM");
OPTION_ALIAS(s_noBgm, "nobgm");

OPTION_DECLARE(int, s_numLocalPlayersToAdd, 1);
OPTION_DEFINE(int, s_numLocalPlayersToAdd, "App/Num Local Players");
OPTION_ALIAS(s_numLocalPlayersToAdd, "numlocal");

OPTION_EXTERN(int, g_playerCharacterIndex);

static char s_bgm[64] = { };
static Music * s_bgmSound = 0;

Client::Client()
	: m_channel(0)
	, m_lobbyMenu(0)
	, m_textChat(0)
	, m_textChatFade(0.f)
	, m_gameSim(0)
	, m_syncStream(0)
	, m_isSynced(false)
	, m_isDesync(false)
	, m_hasAddedPlayers(false)
{
	m_lobbyMenu = new LobbyMenu(this);

	m_textChat = new TextField(UI_TEXTCHAT_X, UI_TEXTCHAT_Y, UI_TEXTCHAT_SX, UI_TEXTCHAT_SY);

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
	if (m_channel && m_channel->m_isConnected)
	{
		if (m_isSynced && !m_hasAddedPlayers)
		{
			m_hasAddedPlayers = true;

			for (int i = 0; i < s_numLocalPlayersToAdd; ++i)
			{
				g_app->netAddPlayer(m_channel, g_playerCharacterIndex, g_app->m_displayName);
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

			PlayerInput input;

			bool useKeyboard = ((playerInstanceData->m_input.m_controllerIndex == 0) || (g_app->getControllerAllocationCount() == 1)) && !m_textChat->isActive();
			bool useGamepad = !morePlayersThanControllers || (playerInstanceData->m_input.m_controllerIndex != 0) || (g_app->getControllerAllocationCount() == 1);

			if (useKeyboard)
			{
				if (keyboard.isDown(SDLK_LEFT))
				{
					input.buttons |= INPUT_BUTTON_LEFT;
					input.analogX -= 100;
				}
				if (keyboard.isDown(SDLK_RIGHT))
				{
					input.buttons |= INPUT_BUTTON_RIGHT;
					input.analogX += 100;
				}
				if (keyboard.isDown(SDLK_UP))
				{
					input.buttons |= INPUT_BUTTON_UP;
					input.analogY -= 100;
				}
				if (keyboard.isDown(SDLK_DOWN))
				{
					input.buttons |= INPUT_BUTTON_DOWN;
					input.analogY += 100;
				}
				if (keyboard.isDown(SDLK_a) || keyboard.isDown(SDLK_SPACE))
					input.buttons |= INPUT_BUTTON_A;
				if (keyboard.isDown(SDLK_s))
					input.buttons |= INPUT_BUTTON_B;
				if (keyboard.isDown(SDLK_z))
					input.buttons |= INPUT_BUTTON_X;
				if (keyboard.isDown(SDLK_x))
					input.buttons |= INPUT_BUTTON_Y;
				if (keyboard.isDown(SDLK_d))
					input.buttons |= INPUT_BUTTON_START;
			}

			if (useGamepad)
			{
				const int gamepadIndex =
					!morePlayersThanControllers
					? playerInstanceData->m_input.m_controllerIndex
					: (g_app->getControllerAllocationCount() == 1)
					? 0
					: (playerInstanceData->m_input.m_controllerIndex - 1);

				if (gamepadIndex >= 0 && gamepadIndex < MAX_GAMEPAD && gamepad[gamepadIndex].isConnected)
				{
					const Gamepad & g = gamepad[gamepadIndex];

					if (g.isDown(DPAD_LEFT) || g.getAnalog(0, ANALOG_X) < -0.4f)
						input.buttons |= INPUT_BUTTON_LEFT;
					if (g.isDown(DPAD_RIGHT) || g.getAnalog(0, ANALOG_X) > +0.4f)
						input.buttons |= INPUT_BUTTON_RIGHT;
					if (g.isDown(DPAD_UP) || g.getAnalog(0, ANALOG_Y) < -0.4f)
						input.buttons |= INPUT_BUTTON_UP;
					if (g.isDown(DPAD_DOWN) || g.getAnalog(0, ANALOG_Y) > +0.4f)
						input.buttons |= INPUT_BUTTON_DOWN;
					if (g.isDown(GAMEPAD_A))
						input.buttons |= INPUT_BUTTON_A;
					if (g.isDown(GAMEPAD_B))
						input.buttons |= INPUT_BUTTON_B;
					if (g.isDown(GAMEPAD_X))
						input.buttons |= INPUT_BUTTON_X;
					if (g.isDown(GAMEPAD_Y))
						input.buttons |= INPUT_BUTTON_Y;
					if (g.isDown(GAMEPAD_START))
						input.buttons |= INPUT_BUTTON_START;

					input.analogX = g.getAnalog(0, ANALOG_X) * 100;
					input.analogY = g.getAnalog(0, ANALOG_Y) * 100;
				}
			}

			if (g_monkeyMode)
			{
				if ((rand() % 2) == 0)
				{
					input.buttons |= INPUT_BUTTON_LEFT;
					input.analogX -= 100;
				}
				if ((rand() % 10) == 0)
				{
					input.buttons |= INPUT_BUTTON_RIGHT;
					input.analogX += 100;
				}
				if ((rand() % 10) == 0)
				{
					input.buttons |= INPUT_BUTTON_UP;
					input.analogY -= 100;
				}
				if ((rand() % 10) == 0)
				{
					input.buttons |= INPUT_BUTTON_DOWN;
					input.analogY += 100;
				}
				if ((rand() % 60) == 0)
					input.buttons |= INPUT_BUTTON_A;
				if ((rand() % 60) == 0)
					input.buttons |= INPUT_BUTTON_B;
				if ((rand() % 60) == 0)
					input.buttons |= INPUT_BUTTON_X;
				if ((rand() % 60) == 0)
					input.buttons |= INPUT_BUTTON_Y;
				if ((rand() % 60) == 0)
					input.buttons |= INPUT_BUTTON_START;
			}

			if (input != playerInstanceData->m_input.m_lastSent)
			{
				playerInstanceData->m_input.m_lastSent = input;

				g_app->netSetPlayerInputs(
					m_channel->m_id,
					player.m_index,
					input);
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

		//

		if (m_textChat->tick(dt))
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				const Player & player = m_gameSim->m_players[i];

				if (!player.m_isUsed || player.m_owningChannelId != m_channel->m_id)
					continue;

				g_app->netAction(m_channel, 	kNetAction_TextChat, i, 0, m_textChat->getText());

				logDebug("client %p owns player %d", this, i);
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

		if (g_keyboardLock == 0 && keyboard.wentDown(SDLK_t))
			m_textChat->open(UI_TEXTCHAT_MAX_TEXT_SIZE, true);
	}
	else
	{
		if (m_textChat->isActive())
		{
			m_textChat->close();
		}
	}

	if (s_noBgm)
	{
		if (s_bgmSound)
		{
			s_bgmSound->stop();

			delete s_bgmSound;
			s_bgmSound = 0;

			memset(s_bgm, 0, sizeof(s_bgm));
		}
	}
	else if (g_app->isSelectedClient(this))
	{
		char temp[64];

		temp[0] = 0;

		switch (m_gameSim->m_gameState)
		{
		case kGameState_MainMenus:
		case kGameState_Connecting:
		case kGameState_OnlineMenus:
			strcpy_s(temp, sizeof(temp), "bgm-menus.ogg");
			break;
		case kGameState_Play:
			sprintf_s(temp, sizeof(temp), "bgm-play%02d.ogg", m_gameSim->m_nextRoundNumber % 4);
			break;
		case kGameState_RoundComplete:
			strcpy_s(temp, sizeof(temp), "bgm-round-complete.ogg");
			break;

		default:
			Assert(false);
			break;
		}

		if (strcmp(temp, s_bgm) != 0)
		{
			strcpy_s(s_bgm, sizeof(s_bgm), temp);

			delete s_bgmSound;
			s_bgmSound = 0;

			if (strlen(s_bgm))
			{
				s_bgmSound = new Music(s_bgm);
				s_bgmSound->play();
			}
		}
	}
}

void Client::draw()
{
	// todo : move drawing code to game sim too?

	Assert(!g_gameSim);
	g_gameSim = m_gameSim;

	switch (m_gameSim->m_gameState)
	{
	case kGameState_MainMenus:
		Assert(false);
		break;

	case kGameState_Connecting:
		drawConnecting();
		break;

	case kGameState_OnlineMenus:
		drawMenus();
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

	g_gameSim = 0;
}

void Client::drawConnecting()
{
	setColor(colorRed);
	drawRect(0, 0, GFX_SX, GFX_SY);
	setColor(colorWhite);
}

void Client::drawMenus()
{
	setColor(colorWhite);
	Sprite("mainmenu-back.png").draw();

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		const Player & player = m_gameSim->m_players[i];

		const int y = GFX_SY * 3 / 4 + i * 50;

		if (player.m_isUsed)
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX/2, y, 24, 0.f, 0.f, "PLAYER %d CHAR %d", i, player.m_characterIndex);
		}
		else
		{
			setFont("calibri.ttf");
			setColor(127, 127, 127);
			drawText(GFX_SX/2, y, 24, 0.f, 0.f, "PLAYER %d NOT CONNECTED", i);
		}
	}

	if (m_gameSim->m_gameState == kGameState_OnlineMenus)
	{
		m_lobbyMenu->draw();
	}

	// draw current game mode selection

	setFont("calibri.ttf");
	setColor(127, 255, 227);
	drawText(GFX_SX/2, GFX_SY - 130, 48, 0.f, 0.f, "[%s]", g_gameModeNames[m_gameSim->m_gameMode]);

	// draw game start timer

	if (m_gameSim->m_gameStartTicks != 0)
	{
		const float timeRemaining = m_gameSim->m_gameStartTicks / float(TICKS_PER_SECOND);

		// todo : options for position
		setFont("calibri.ttf");
		setColor(127, 255, 227);
		drawText(GFX_SX/2, GFX_SY - 75, 48, 0.f, 0.f, "ROUND START IN T-%02.2f", timeRemaining);
	}

	setColor(colorWhite);
}

void Client::drawPlay()
{
	const Vec2 shake = m_gameSim->getScreenShake();

	Vec2 camTranslation = shake;

#if 0
	const float asx = ARENA_SX_PIXELS;
	const float asy = ARENA_SY_PIXELS;
	const float asx2 = asx / 2.f;
	const float asy2 = asy / 2.f;
	const float dx = m_gameSim->m_players[0].m_pos[0] - asx2;
	const float dy = m_gameSim->m_players[0].m_pos[1] - asy2;
	const float d = sqrt(dx * dx + dy * dy);
	const float t = d / sqrt(asx2 * asx2 + asy2 * asy2);
	const float scale = 1.f * t + 1.2f * (1.f - t);
	
	gxTranslatef(+ARENA_SX_PIXELS/2.f, +ARENA_SY_PIXELS/2.f, 0.f);
	gxScalef(scale, scale, 1.f);
	gxTranslatef(-ARENA_SX_PIXELS/2.f, -ARENA_SY_PIXELS/2.f, 0.f);
#endif

	pushSurface(g_colorMap);
	{
		gxPushMatrix();
		gxTranslatef(camTranslation[0], camTranslation[1], 0.f);

		// todo : background depends on level properties

		setBlend(BLEND_OPAQUE);
		//std::string background = std::string("levels/") + m_gameSim->m_arena.m_name.c_str() + "/Background.png";
		//Sprite(background.c_str()).draw();
		m_gameSim->m_background.draw();
		setBlend(BLEND_ALPHA);

		

		if (m_gameSim->m_levelEvents.gravityWell.endTimer.isActive())
		{
			setColor(255, 255, 255, 127);
			Sprite("gravitywell/well.png").drawEx(
				m_gameSim->m_levelEvents.gravityWell.m_x,
				m_gameSim->m_levelEvents.gravityWell.m_y,
				m_gameSim->m_roundTime * 90.f,
				1.f, true,
				FILTER_LINEAR);
			setColor(colorWhite);
		}

		m_gameSim->m_arena.drawBlocks();

		m_gameSim->m_floorEffect.draw();

		// torches

		for (int i = 0; i < MAX_TORCHES; ++i)
		{
			const Torch & torch = m_gameSim->m_torches[i];

			if (torch.m_isAlive)
				torch.draw();
		}

		// pickups

		for (int i = 0; i < MAX_PICKUPS; ++i)
		{
			const Pickup & pickup = m_gameSim->m_pickups[i];

			if (pickup.isAlive)
				pickup.draw();
		}

		// token

		m_gameSim->m_tokenHunt.m_token.draw();

		// coins

		for (int i = 0; i < MAX_COINS; ++i)
		{
			m_gameSim->m_coinCollector.m_coins[i].draw();
		}

		// movers

		for (int i = 0; i < MAX_MOVERS; ++i)
		{
			if (m_gameSim->m_movers[i].m_isActive)
				m_gameSim->m_movers[i].draw();
		}

		// players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			const Player & player = m_gameSim->m_players[i];

			if (player.m_isUsed)
				player.draw();
		}

		// bullets

		m_gameSim->m_bulletPool->draw();

		// particles

		setBlend(BLEND_ADD);
		m_gameSim->m_particlePool->draw();
		setBlend(BLEND_ALPHA);

		// spike walls

		m_gameSim->m_levelEvents.spikeWalls.draw();

		gxPopMatrix();
	}
	popSurface();

	pushSurface(g_lightMap);
	{
		gxPushMatrix();
		gxTranslatef(camTranslation[0], camTranslation[1], 0.f);

		const int lightingDebugMode = LIGHTING_DEBUG_MODE % 4;

		float v = 1.f;
		if (lightingDebugMode == 0)
			v = .1f + (std::sin(g_TimerRT.Time_get() / 5.f) + 1.f) / 2.f * .9f;
		else if (lightingDebugMode == 1)
			v = 1.f;
		else if (lightingDebugMode == 2)
			v = .5f;
		else if (lightingDebugMode == 3)
			v = 0.f;
		glClearColor(v, v, v, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		setBlend(BLEND_ADD);

		// torches

		for (int i = 0; i < MAX_TORCHES; ++i)
		{
			const Torch & torch = m_gameSim->m_torches[i];

			if (torch.m_isAlive)
				torch.drawLight();
		}

		// pickups

		for (int i = 0; i < MAX_PICKUPS; ++i)
		{
			const Pickup & pickup = m_gameSim->m_pickups[i];

			if (pickup.isAlive)
				pickup.drawLight();
		}

		// token

		m_gameSim->m_tokenHunt.m_token.drawLight();

		// coins

		for (int i = 0; i < MAX_COINS; ++i)
		{
			m_gameSim->m_coinCollector.m_coins[i].drawLight();
		}

		// players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			const Player & player = m_gameSim->m_players[i];

			if (player.m_isUsed)
				player.drawLight();
		}

		// bullets

		m_gameSim->m_bulletPool->drawLight();

		// particles

		m_gameSim->m_particlePool->drawLight();

		setBlend(BLEND_ALPHA);

		gxPopMatrix();
	}
	popSurface();

	// compose

	applyLightMap(*g_colorMap, *g_lightMap, *g_finalMap);

	// blit

	setBlend(BLEND_OPAQUE);
	setColor(255, 255, 255);
	glEnable(GL_TEXTURE_2D);
	{
		glBindTexture(GL_TEXTURE_2D, g_finalMap->getTexture());
		drawRect(0, 0, g_finalMap->getWidth(), g_finalMap->getHeight());
	}
	glDisable(GL_TEXTURE_2D);
	setBlend(BLEND_ALPHA);
}

void Client::drawRoundComplete()
{
	const int numTicks = TICKS_PER_SECOND * GAMESTATE_COMPLETE_TIME_DILATION_TIMER;
	const float t = 1.f - Calc::Saturate(m_gameSim->m_roundCompleteTimeDilationTicks / float(numTicks));
	setColorf(0.f, 0.f, 0.f, t);
	drawRect(0, 0, GFX_SX, GFX_SY);

	setColor(colorWhite);

	int y = GFX_SY / 4;

	setFont("calibri.ttf");
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
		setColor(0, 0, 255, 127);
		drawRect(0, y, GFX_SX, y + sy);

		setColor(colorWhite);
		setFont("calibri.ttf");
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

			setFont("calibri.ttf");

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

void Client::addPlayer(PlayerInstanceData * player)
{
	player->m_player->m_isUsed = true;

	m_players.push_back(player);

	m_gameSim->m_playerInstanceDatas[player->m_player->m_index] = player;

	if (player->m_player->m_owningChannelId == m_channel->m_id)
	{
		Assert(player->m_input.m_controllerIndex == -1);
		player->m_input.m_controllerIndex = g_app->allocControllerIndex();
	}
}

void Client::removePlayer(PlayerInstanceData * player)
{
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
