#include "fireball.h"
#include "framework.h"
#include "gamesim.h"
#include "main.h"

#define FIREBALL_SPRITER Spriter(m_name.c_str())

FireBall::FireBall()
{
	memset(this, 0, sizeof(FireBall));
}

void FireBall::load(const char * name, GameSim & gameSim, int x, int y, float angle, float scale)
{
	Assert(!active);

	memset(this, 0, sizeof(FireBall));

	m_name = name;
	m_state = SpriterState();
	
	m_state.x = m_x = x;
	m_state.y = m_y = y;
	m_state.angle = angle;
	m_state.scale = scale;

	active = true;

	m_speed = gameSim.RandomFloat(-10.f, 50.f) + FIREBALL_SPEED;

	m_state.startAnim(FIREBALL_SPRITER, "Fireball");
}

void FireBall::tick(GameSim & gameSim, float dt)
{
	if (active)
	{
		const float rad = (m_state.angle / 180.f * M_PI);

		m_x += (m_speed * cos(rad)) * dt;
		m_y += (m_speed * sin(rad)) * dt;

		m_state.x = m_x;
		m_state.y = m_y;

		m_state.updateAnim(FIREBALL_SPRITER, dt);
	}

	if (m_y > GFX_SY + 200)
	{
		// ball is definitely invisible now and can be reused.

		memset(this, 0, sizeof(FireBall));
	}
}

void FireBall::draw()
{
	if (active)
	{
		FIREBALL_SPRITER.draw(m_state);
	}
}
