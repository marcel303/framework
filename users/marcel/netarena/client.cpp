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

Client::Client()
	: m_channel(0)
	, m_replicationId(0)
	, m_arena(0)
	, m_bulletPool(0)
	, m_particlePool(0)
	, m_spriteManager(0)
{
	m_bulletPool = new BulletPool(true);
	m_particlePool = new BulletPool(true);

	m_spriteManager = new NetSpriteManager();
}

Client::~Client()
{
	delete m_spriteManager;
	m_spriteManager = 0;

	delete m_bulletPool;
	m_bulletPool = 0;
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
			Player * player = *i;

			if (player->getOwningChannelId() != m_channel->m_id)
				continue;

			uint16_t buttons = 0;

			bool useKeyboard = (player->m_input.m_controllerIndex == 0) || (g_app->getControllerAllocationCount() == 1);
			bool useGamepad = !morePlayersThanControllers || (player->m_input.m_controllerIndex != 0) || (g_app->getControllerAllocationCount() == 1);

			if (useKeyboard)
			{
				if (keyboard.isDown(SDLK_LEFT))
					buttons |= INPUT_BUTTON_LEFT;
				if (keyboard.isDown(SDLK_RIGHT))
					buttons |= INPUT_BUTTON_RIGHT;
				if (keyboard.isDown(SDLK_UP))
					buttons |= INPUT_BUTTON_UP;
				if (keyboard.isDown(SDLK_DOWN))
					buttons |= INPUT_BUTTON_DOWN;
				if (keyboard.isDown(SDLK_a))
					buttons |= INPUT_BUTTON_A;
				if (keyboard.isDown(SDLK_s))
					buttons |= INPUT_BUTTON_B;
				if (keyboard.isDown(SDLK_z))
					buttons |= INPUT_BUTTON_X;
				if (keyboard.isDown(SDLK_x))
					buttons |= INPUT_BUTTON_Y;
				if (keyboard.isDown(SDLK_d))
					buttons |= INPUT_BUTTON_START;
			}

			if (useGamepad)
			{
				const int gamepadIndex =
					!morePlayersThanControllers
					? player->m_input.m_controllerIndex
					: (g_app->getControllerAllocationCount() == 1)
					? 0
					: (player->m_input.m_controllerIndex - 1);

				if (gamepadIndex >= 0 && gamepadIndex < MAX_GAMEPAD && gamepad[gamepadIndex].isConnected)
				{
					const Gamepad & g = gamepad[gamepadIndex];

					if (g.isDown(DPAD_LEFT) || g.getAnalog(0, ANALOG_X) < -0.5f)
						buttons |= INPUT_BUTTON_LEFT;
					if (g.isDown(DPAD_RIGHT) || g.getAnalog(0, ANALOG_X) > +0.5f)
						buttons |= INPUT_BUTTON_RIGHT;
					if (g.isDown(DPAD_UP) || g.getAnalog(0, ANALOG_Y) < -0.5f)
						buttons |= INPUT_BUTTON_UP;
					if (g.isDown(DPAD_DOWN) || g.getAnalog(0, ANALOG_Y) > +0.5f)
						buttons |= INPUT_BUTTON_DOWN;
					if (g.isDown(GAMEPAD_A))
						buttons |= INPUT_BUTTON_A;
					if (g.isDown(GAMEPAD_B))
						buttons |= INPUT_BUTTON_B;
					if (g.isDown(GAMEPAD_X))
						buttons |= INPUT_BUTTON_X;
					if (g.isDown(GAMEPAD_Y))
						buttons |= INPUT_BUTTON_Y;
					if (g.isDown(GAMEPAD_START))
						buttons |= INPUT_BUTTON_START;
				}
			}

			if (buttons != player->m_input.m_currButtons)
			{
				player->m_input.m_currButtons = buttons;

				g_app->netSetPlayerInputs(m_channel->m_id, player->getNetId(), buttons);
			}
		}
	}

	m_bulletPool->anim(dt);

	m_particlePool->tick(dt);
}

void Client::draw()
{
	if (!m_arena)
		return;

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
}

void Client::drawPlay()
{
	m_arena->drawBlocks();

	m_spriteManager->draw();

	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		Player * player = *i;

		player->draw();
	}

	m_particlePool->draw();

	m_bulletPool->draw();
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
		Player * player = *p;

		drawText(GFX_SX/3*1, y, 40, +1.f, +1.f, "Player %d", index);
		drawText(GFX_SX/3*2, y, 40, -1.f, +1.f, "%d", player->getScore());

		index++;
		y += 50;
	}
}

void Client::addPlayer(Player * player)
{
	m_players.push_back(player);

	if (player->getOwningChannelId() == m_channel->m_id)
	{
		Assert(player->m_input.m_controllerIndex == -1);
		player->m_input.m_controllerIndex = g_app->allocControllerIndex();
	}
}

void Client::removePlayer(Player * player)
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

		m_players.erase(i);
	}
}

void Client::spawnParticles(BulletType type, uint8_t count, int16_t x, int16_t y, uint16_t velocity, uint16_t maxDistance)
{
	for (int i = 0; i < count; ++i)
	{
		float angle = rand() % 256;

		uint16_t id = m_particlePool->alloc();

		if (id != INVALID_BULLET_ID)
		{
			Bullet & b = m_particlePool->m_bullets[id];

			memset(&b, 0, sizeof(b));
			b.isAlive = true;
			b.type = type;
			b.x = x;
			b.y = y;
			b.angle = angle / 128.f * float(M_PI);
			b.velocity = velocity;

			b.noCollide = true;
			b.maxWrapCount = 1;
			b.maxDistanceTravelled = maxDistance;
		}
	}
}
