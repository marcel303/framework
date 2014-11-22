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
		uint16_t buttons = 0;

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

		g_app->netSetPlayerInputs(m_channel->m_id, buttons);
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
