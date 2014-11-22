#include "arena.h"
#include "framework.h"
#include "host.h"
#include "player.h"

void Player::tick(float dt)
{
	m_pos.SetDirty();

	if (!m_state.isAlive)
	{
		int x, y;

		if (g_hostArena->getRandomSpawnPoint(x, y))
		{
			m_state.isAlive = true;
			m_state.SetDirty();

			m_pos.x = x * BLOCK_SX;
			m_pos.y = y * BLOCK_SY;
		}
	}

	if (m_state.isAlive)
	{
		int dx = 0;
		int dy = 0;

		if (m_buttons & INPUT_BUTTON_LEFT)
			dx--;
		if (m_buttons & INPUT_BUTTON_RIGHT)
			dx++;

		// todo : do collision detection

		m_pos.x += dx * 4;
	}
}

void Player::draw()
{
	setColor(rand() & 255, 0, 0);
	gxSetTexture(0);

	// todo : player rect. base should be at ground level

	drawRect(
		m_pos.x - PLAYER_SX/2,
		m_pos.y - PLAYER_SY/2,
		m_pos.x + PLAYER_SX/2,
		m_pos.y + PLAYER_SY/2);

	setColor(colorWhite);
}
