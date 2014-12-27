#include <algorithm>
#include "arena.h"
#include "bullet.h"
#include "Calc.h"
#include "Channel.h"
#include "client.h"
#include "Debugging.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "netsprite.h"
#include "player.h"

OPTION_DECLARE(bool, s_noBgm, false);
OPTION_DEFINE(bool, s_noBgm, "Sound/No BGM");
OPTION_ALIAS(s_noBgm, "nobgm");

static char s_bgm[64] = { };
static Music * s_bgmSound = 0;

Client::Client()
	: m_channel(0)
	, m_replicationId(0)
	, m_gameSim(0)
	, m_arena(0)
#if !ENABLE_CLIENT_SIMULATION
	, m_bulletPool(0)
#endif
	, m_particlePool(0)
	, m_spriteManager(0)
{
	m_gameSim = new GameSim();

#if !ENABLE_CLIENT_SIMULATION
	m_bulletPool = new BulletPool(true);
#endif
	m_particlePool = new BulletPool(true);

	m_spriteManager = new NetSpriteManager();
}

Client::~Client()
{
	delete m_spriteManager;
	m_spriteManager = 0;

#if !ENABLE_CLIENT_SIMULATION
	delete m_bulletPool;
	m_bulletPool = 0;
#endif

	delete m_gameSim;
	m_gameSim = 0;
}

void Client::initialize(Channel * channel)
{
	m_channel = channel;
}

void Client::tick(float dt)
{
	if (m_channel)
	{
		// see if there are more players than gamepads, to decide if we should use keyboard exclusively for one player, or also assign them a gamepad

		int numGamepads = 0;
		for (int i = 0; i < MAX_GAMEPAD; ++i)
			if (gamepad[i].isConnected)
				numGamepads++;

		const bool morePlayersThanControllers = (numGamepads < g_app->getControllerAllocationCount());

		for (auto i = m_players.begin(); i != m_players.end(); ++i)
		{
			PlayerNetObject * playerNetObject = *i;

			if (playerNetObject->getOwningChannelId() != m_channel->m_id)
				continue;

			PlayerInput input;

			bool useKeyboard = (playerNetObject->m_input.m_controllerIndex == 0) || (g_app->getControllerAllocationCount() == 1);
			bool useGamepad = !morePlayersThanControllers || (playerNetObject->m_input.m_controllerIndex != 0) || (g_app->getControllerAllocationCount() == 1);

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
					? playerNetObject->m_input.m_controllerIndex
					: (g_app->getControllerAllocationCount() == 1)
					? 0
					: (playerNetObject->m_input.m_controllerIndex - 1);

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

			if (input != playerNetObject->m_input.m_currState)
			{
				playerNetObject->m_input.m_currState = input;

				g_app->netSetPlayerInputs(m_channel->m_id, playerNetObject->getNetId(), input);
			}
		}
	}

#if !ENABLE_CLIENT_SIMULATION
	m_gameSim->anim(dt);

	m_bulletPool->anim(*m_gameSim, dt);
#endif

	m_particlePool->tick(*m_gameSim, dt);

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
	else if (m_arena && g_app->isSelectedClient(this))
	{
		char temp[64];

		switch (m_arena->m_gameState.m_gameState)
		{
		case kGameState_Lobby:
			strcpy_s(temp, sizeof(temp), "bgm-lobby.ogg");
			break;
		case kGameState_Play:
			strcpy_s(temp, sizeof(temp), "bgm-play.ogg");
			break;
		case kGameState_RoundComplete:
			strcpy_s(temp, sizeof(temp), "bgm-round-complete.ogg");
			break;
		}

		if (strcmp(temp, s_bgm) != 0)
		{
			strcpy_s(s_bgm, sizeof(s_bgm), temp);

			delete s_bgmSound;
			s_bgmSound = 0;

			s_bgmSound = new Music(s_bgm);
			s_bgmSound->play();
		}
	}
}

void Client::draw()
{
	if (!m_arena)
		return;

	//setPlayerPtrs(); // fixme : enable once full simulation runs on clients

	switch (m_arena->m_gameState.m_gameState)
	{
	case kGameState_Lobby:
		break;

	case kGameState_Play:
		drawPlay();
		break;

	case kGameState_RoundComplete:
		drawRoundComplete();
		break;
	}

	//clearPlayerPtrs();
}

void Client::drawPlay()
{
	const Vec2 shake = m_gameSim->getScreenShake();
	gxPushMatrix();
	gxTranslatef(shake[0], shake[1], 0.f);

#if 0
	m_gameSim->m_arena.drawBlocks();

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (m_gameSim->m_state.m_players[i].m_isAlive)
			m_gameSim->m_state.m_players[i].draw();
	}

	return;
#endif

	m_arena->m_arena->drawBlocks();

	m_spriteManager->draw();

	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		PlayerNetObject * playerNetObject = *i;
		Player * player = playerNetObject->m_player;

		player->draw();
	}

	m_particlePool->draw();

#if !ENABLE_CLIENT_SIMULATION
	m_bulletPool->draw();
#else
	m_gameSim->m_bulletPool->draw();
#endif

	gxPopMatrix();
}

void Client::drawRoundComplete()
{
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
		PlayerNetObject * playerNetObject = *p;
		Player * player = playerNetObject->m_player;

		drawText(GFX_SX/3*1, y, 40, +1.f, +1.f, "Player %d", index);
		drawText(GFX_SX/3*2, y, 40, -1.f, +1.f, "%d", player->m_score);

		index++;
		y += 50;
	}
}

void Client::addPlayer(PlayerNetObject * player)
{
	m_players.push_back(player);

	m_gameSim->m_players[player->getPlayerId()] = player;

	if (player->getOwningChannelId() == m_channel->m_id)
	{
		Assert(player->m_input.m_controllerIndex == -1);
		player->m_input.m_controllerIndex = g_app->allocControllerIndex();
	}
}

void Client::removePlayer(PlayerNetObject * player)
{
	auto i = std::find(m_players.begin(), m_players.end(), player);

	Assert(i != m_players.end());
	if (i != m_players.end())
	{
		if (player->getOwningChannelId() == m_channel->m_id)
		{
			Assert(player->m_input.m_controllerIndex != -1);
			if (player->m_input.m_controllerIndex != -1)
				g_app->freeControllerIndex(player->m_input.m_controllerIndex);
			player->m_input.m_controllerIndex = -1;
		}

		const int playerId = player->getPlayerId();

		if (playerId != -1)
		{
			Assert(m_gameSim->m_players[playerId]);
			m_gameSim->m_players[playerId] = 0;
		}

		m_players.erase(i);
	}
}

PlayerNetObject * Client::findPlayerByNetId(uint32_t netId)
{
	for (auto p = m_players.begin(); p != m_players.end(); ++p)
	{
		PlayerNetObject * player = *p;
		if (player->getNetId() == netId)
			return player;
	}

	return 0;
}

void Client::setPlayerPtrs()
{
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
		(*i)->m_player->m_netObject = (*i);
}

void Client::clearPlayerPtrs()
{
	for (auto i = m_players.begin(); i != m_players.end(); ++i)
		(*i)->m_player->m_netObject = 0;
}

void Client::spawnParticles(const ParticleSpawnInfo & spawnInfo)
{
	for (int i = 0; i < spawnInfo.count; ++i)
	{
		uint16_t id = m_particlePool->alloc();

		if (id != INVALID_BULLET_ID)
		{
			Bullet & b = m_particlePool->m_bullets[id];

			initBullet(*m_gameSim, b, spawnInfo);
		}
	}
}
