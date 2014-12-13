#include <algorithm>
#include "arena.h"
#include "bullet.h"
#include "Channel.h"
#include "client.h"
#include "Debugging.h"
#include "framework.h"
#include "main.h"
#include "player.h"

Client::Client()
	: m_channel(0)
	, m_replicationId(0)
	, m_arena(0)
	, m_bulletPool(0)
{
	m_bulletPool = new BulletPool();
}

Client::~Client()
{
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
		for (auto i = m_players.begin(); i != m_players.end(); ++i)
		{
			Player * player = *i;

			if (player->getOwningChannelId() != m_channel->m_id)
				continue;

			uint16_t buttons = 0;

			bool useKeyboard = (player->m_input.m_controllerIndex == 0) || (g_app->getControllerAllocationCount() == 1);
			bool useGamepad = (player->m_input.m_controllerIndex != 0) || (g_app->getControllerAllocationCount() == 1);

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
			}

			if (useGamepad)
			{
				const int gamepadIndex =
					(g_app->getControllerAllocationCount() == 1)
					? 0
					: (player->m_input.m_controllerIndex - 1);

				if (gamepadIndex >= 0 && gamepadIndex < GAMEPAD_MAX && gamepad[gamepadIndex].isConnected)
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
}

void Client::draw()
{
	if (m_arena)
	{
		m_arena->drawBlocks();
	}

	for (auto i = m_players.begin(); i != m_players.end(); ++i)
	{
		Player * player = *i;

		player->draw();
	}

	m_bulletPool->draw();
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
			g_app->freeControllerIndex(player->m_input.m_controllerIndex);
			player->m_input.m_controllerIndex = -1;
		}

		m_players.erase(i);
	}
}
