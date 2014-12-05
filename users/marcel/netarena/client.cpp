#include "arena.h"
#include "Channel.h"
#include "client.h"
#include "framework.h"
#include "main.h"
#include "player.h"

Client::Client(uint8_t controllerIndex)
	: m_channel(0)
	, m_replicationId(0)
	, m_arena(0)
	, m_controllerIndex(controllerIndex)
{
}

Client::~Client()
{
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

			uint16_t buttons = 0;

			bool useKeyboard = (i == m_players.begin());
			bool useGamepad = (i != m_players.begin()) || (m_players.size() == 1);

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
				if (gamepad[0].isDown[DPAD_LEFT] || gamepad[0].getAnalog(0, ANALOG_X) < -0.5f)
					buttons |= INPUT_BUTTON_LEFT;
				if (gamepad[0].isDown[DPAD_RIGHT] || gamepad[0].getAnalog(0, ANALOG_X) > +0.5f)
					buttons |= INPUT_BUTTON_RIGHT;
				if (gamepad[0].isDown[DPAD_UP] || gamepad[0].getAnalog(0, ANALOG_Y) < -0.5f)
					buttons |= INPUT_BUTTON_UP;
				if (gamepad[0].isDown[DPAD_DOWN] || gamepad[0].getAnalog(0, ANALOG_Y) > +0.5f)
					buttons |= INPUT_BUTTON_DOWN;
				if (gamepad[0].isDown[GAMEPAD_A])
					buttons |= INPUT_BUTTON_A;
				if (gamepad[0].isDown[GAMEPAD_B])
					buttons |= INPUT_BUTTON_B;
				if (gamepad[0].isDown[GAMEPAD_X])
					buttons |= INPUT_BUTTON_X;
				if (gamepad[0].isDown[GAMEPAD_Y])
					buttons |= INPUT_BUTTON_Y;
			}

			if (buttons != player->m_input.m_currButtons)
			{
				player->m_input.m_currButtons = buttons;

				g_app->netSetPlayerInputs(m_channel->m_id, player->getNetId(), buttons);
			}
		}
	}
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
}
