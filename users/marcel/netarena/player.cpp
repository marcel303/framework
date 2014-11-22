#include "arena.h"
#include "framework.h"
#include "host.h"
#include "player.h"

static const float STEERING_SPEED_ON_GROUND = 800.f;
static const float STEERING_SPEED_IN_AIR = STEERING_SPEED_ON_GROUND * 0.8f;

void Player::tick(float dt)
{
	if (!m_state.isAlive || m_input.wentDown(INPUT_BUTTON_X))
	{
		int x, y;

		if (g_hostArena->getRandomSpawnPoint(x, y))
		{
			m_state.isAlive = true;
			m_state.SetDirty();

			m_pos[0] = x;
			m_pos[1] = y;

			m_vel[0] = 0.f;
			m_vel[1] = 0.f;
		}
	}

	if (m_state.isAlive)
	{
		float steeringSpeed = 0.f;

		if (m_input.isDown(INPUT_BUTTON_LEFT))
			steeringSpeed -= 1.f;
		if (m_input.isDown(INPUT_BUTTON_RIGHT))
			steeringSpeed += 1.f;

		steeringSpeed *= STEERING_SPEED_ON_GROUND;

		//

		m_vel[1] += GRAVITY * (1.f / 60.f);

		for (int i = 0; i < 2; ++i)
		{
			float vel = m_vel[i];

			if (i == 0)
				vel += steeringSpeed;

			vel *= dt;

			float velSign = vel < 0.f ? -1.f : +1.f;

			while (vel != 0.f)
			{
				const float delta = (std::abs(vel) < 1.f) ? vel : velSign;

				Vec2 newPos(m_pos.x, m_pos.y);

				newPos[i] += delta;

				const uint32_t newBlockMask = g_hostArena->getIntersectingBlocksMask(
					newPos[0] + m_collision.x1,
					newPos[1] + m_collision.y1,
					newPos[0] + m_collision.x2,
					newPos[1] + m_collision.y2);

				if (newBlockMask & kBlockMask_Solid)
				{
					if (i == 1)
					{
						if (m_vel[i] > 0.f)
						{
							if (m_input.wentDown(INPUT_BUTTON_A))
							{
								m_vel[i] = -2000.f; // todo : jump speed constant
							}
							else
							{
								m_vel[i] = 0.f;
							}
						}
						else
						{
							m_vel[i] = 0.f;
						}
					}

					vel = 0.f;
				}
				else
				{
					m_pos[i] = newPos[i];

					vel -= delta;
				}

				if (newBlockMask & (1 << kBlockType_Spike))
				{
					// todo : die
				}

				if (newBlockMask & (1 << kBlockType_Spring))
				{
					if (i == 1)
					{
						m_vel[i] = -2000.f; // todo : auto jump speed constant
					}
				}
			}
		}
	}

	m_input.next();

	//printf("x: %g\n", m_pos[0]);

	m_pos.SetDirty();
}

void Player::draw()
{
	setColor(rand() & 255, 0, 0);
	gxSetTexture(0);

	// todo : player rect. base should be at ground level

	drawRect(
		m_pos.x + m_collision.x1,
		m_pos.y + m_collision.y1,
		m_pos.x + m_collision.x2 + 1,
		m_pos.y + m_collision.y2 + 1);

	setColor(colorWhite);
}
