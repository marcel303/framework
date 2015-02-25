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
#include "textchat.h"
#include "Timer.h"

OPTION_DECLARE(bool, s_noBgm, false);
OPTION_DEFINE(bool, s_noBgm, "Sound/No BGM");
OPTION_ALIAS(s_noBgm, "nobgm");

static char s_bgm[64] = { };
static Music * s_bgmSound = 0;

Client::Client()
	: m_channel(0)
	, m_lobbyMenu(0)
	, m_textChat(0)
	, m_gameSim(0)
	, m_syncStream(0)
	, m_isDesync(false)
{
	m_lobbyMenu = new LobbyMenu(this);

	m_textChat = new TextChat(UI_TEXTCHAT_X, UI_TEXTCHAT_Y, UI_TEXTCHAT_SX, UI_TEXTCHAT_SY);

	m_gameSim = new GameSim();

	m_syncStream = new BitStream();

	m_gameSim->setGameState(kGameState_Connecting);
}

Client::~Client()
{
	Assert(m_players.empty());
	while (!m_players.empty())
	{
		PlayerInstanceData * player = m_players.front();
		removePlayer(player);
	}

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
				if (keyboard.isDown(SDLK_a))
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

		if (m_textChat->tick(dt))
		{
			g_app->netAction(m_channel, 	kNetAction_TextChat, 0, 0, m_textChat->getText());
		}

		if (g_keyboardLock == 0 && keyboard.wentDown(SDLK_t))
			m_textChat->setActive(true);
	}
	else
	{
		m_textChat->setActive(false);
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
			strcpy_s(temp, sizeof(temp), "bgm-play.ogg");
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

	m_textChat->draw();
}

void Client::drawConnecting()
{
	setColor(colorRed);
	drawRect(0, 0, GFX_SX, GFX_SY);
	setColor(colorWhite);
}

void Client::drawMenus()
{
	Sprite("mainmenu-back.png").draw();

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		const Player & player = m_gameSim->m_players[i];

		const int y = GFX_SY * 3 / 4 + i * 50;

		if (player.m_isUsed)
		{
			setFont("calibri.ttf");
			setColor(255, 255, 255);
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

		// fixme : options for position
		setFont("calibri.ttf");
		setColor(127, 255, 227);
		drawText(GFX_SX/2, GFX_SY - 75, 48, 0.f, 0.f, "ROUND START IN T-%02.2f", timeRemaining);
	}
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
		Sprite("back.png").draw();
		setBlend(BLEND_ALPHA);

		m_gameSim->m_arena.drawBlocks();

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

	for (auto p = m_players.begin(); p != m_players.end(); ++p)
	{
		PlayerInstanceData * playerInstanceData = *p;
		Player * player = playerInstanceData->m_player;

		drawText(GFX_SX/3*1, y, 40, +1.f, +1.f, "Player %d", index);
		drawText(GFX_SX/3*2, y, 40, -1.f, +1.f, "%d", player->m_score);

		index++;
		y += 50;
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

		const int playerId = player->m_player->m_index;

		if (playerId != -1)
		{
			Assert(m_gameSim->m_playerInstanceDatas[playerId] != 0);
			if (m_gameSim->m_playerInstanceDatas[playerId] != 0)
			{
				m_gameSim->m_playerInstanceDatas[playerId]->m_player->m_isUsed = false;
				m_gameSim->m_playerInstanceDatas[playerId]->m_player->m_instanceData = 0;
				m_gameSim->m_playerInstanceDatas[playerId] = 0;
			}
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
