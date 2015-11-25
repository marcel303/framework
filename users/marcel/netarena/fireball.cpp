#include "fireball.h"
#include "framework.h"
#include "gamesim.h"
#include "main.h"
#include "player.h"

#define FIREBALL_SPRITER Spriter(m_name.c_str())

FireBall::FireBall()
{
	memset(this, 0, sizeof(FireBall));
}

void FireBall::load(const char * name, GameSim & gameSim, int x, int y, float angle, float scale)
{
	memset(this, 0, sizeof(FireBall));

	m_name = name;
	m_state = SpriterState();
	
	m_state.x = m_x = x;
	m_state.y = m_y = y;
	m_state.angle = angle;
	m_state.scale = scale;

	m_isActive = true;

	m_speed = gameSim.RandomFloat(-10.f, 50.f) + FIREBALL_SPEED;

	m_state.startAnim(FIREBALL_SPRITER, "Fireball");
}

void FireBall::tick(GameSim & gameSim, float dt)
{
	if (m_isActive)
	{
		const float rad = (m_state.angle / 180.f * M_PI);

		float vx = (m_speed * cos(rad)) * dt;
		float vy = (m_speed * sin(rad)) * dt;

		m_x += vx;
		m_y += vy;

		m_state.x = m_x;
		m_state.y = m_y;

		m_state.updateAnim(FIREBALL_SPRITER, dt);

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			Player& other = gameSim.m_players[i];

			if (other.m_isAlive && other.m_isUsed)
			{
				CollisionShape shape1;
				CollisionInfo info1;

				if (getCollision(shape1) && other.getPlayerCollision(info1))
				{
					CollisionShape shape2(info1.min, info1.max);
					if (shape1.intersects(shape2))
					{
						m_isActive = false;

						other.handleDamage(1.f, Vec2(vx, vy), 0, true);
					}
				}
			}
		}
	}

	if (m_y > GFX_SY + 200)
	{
		// ball is definitely invisible now and can be reused.

		memset(this, 0, sizeof(FireBall));
	}
}

void FireBall::draw() const
{
	fassert(m_isActive);

	setColor(colorWhite);
	FIREBALL_SPRITER.draw(m_state);
}

void FireBall::drawLight() const
{
	fassert(m_isActive);

	setColor(colorWhite);
	Sprite("player-light.png").drawEx(m_state.x, m_state.y, m_state.angle, 2.f, 2.f, false, FILTER_MIPMAP);
}

bool FireBall::getCollision(CollisionShape & shape) const
{
	Vec2 points[4];

	if (!FIREBALL_SPRITER.getHitboxAtTime(m_state.animIndex, "Kill", m_state.animTime, points))
	{
		return false;
	}
	else
	{
		for (int i = 0; i < 4; ++i)
		{
			points[i][0] *= m_state.scale * 1.0f; // need to take angle into account? *(-m_facing[0]);
			points[i][1] *= m_state.scale * 1.0f;

			points[i][0] += m_x;
			points[i][1] += m_y;
		}

		shape.set(points[0], points[1], points[2], points[3]);

		return true;
	}
}
