#include "background.h"
#include "fireball.h"
#include "framework.h"
#include "gamesim.h"
#include "main.h"

#define BACKGROUND_SPRITER Spriter(m_name.c_str())

void Background::load(const char * name, GameSim& gameSim)
{
	memset(this, 0, sizeof(Background));

	m_volcanoState = VC_IDLE;

	m_name = name;
	m_state = SpriterState();
	m_state.startAnim(BACKGROUND_SPRITER, "Idle");

	m_startErupt = gameSim.RandomFloat(50.0f, 180.0f);
	//m_startErupt = 13.f;

	t1 = true;
	t2 = true;
	t3 = true;

	m_fireBall.active = false;
}

void Background::tick(GameSim & gameSim, float dt)
{
	if (t1 && !m_isTriggered && gameSim.m_roundTime >= (m_startErupt - 12.f))
	{
		gameSim.addScreenShake(-1.2f, 1.3f, 4000.f, 6.f);
		t1 = false;
	}

	if (t2 && !m_isTriggered && gameSim.m_roundTime >= (m_startErupt - 6.f))
	{
		gameSim.addScreenShake(-2.3f, 1.5f, 6000.f, 4.f);
		gameSim.playSound("volcano-rumble.ogg");
		
		t2 = false;
	}

	if (t3 && !m_isTriggered && gameSim.m_roundTime >= (m_startErupt - 2.f))
	{
		gameSim.addScreenShake(-3.f, 2.5f, 7000.f, 2.f);
		t3 = false;
	}

	if (!m_isTriggered && gameSim.m_roundTime >= m_startErupt)
	{
		doEvent(gameSim);
	}

	if (m_state.updateAnim(BACKGROUND_SPRITER, dt))
	{
		if (m_isTriggered)
		{
			m_state.startAnim(BACKGROUND_SPRITER, "IdleErupted");
			if (m_volcanoState == VC_ERUPT)
				m_volcanoState = VC_AFTER;
		}
		else
		{
			m_state.startAnim(BACKGROUND_SPRITER, "Idle");
		}
	}

	if (m_volcanoState == VC_ERUPT)
	{
		if (m_fireBall.m_y < -350.f)
		{
			m_fireBall.load("backgrounds/VolcanoTest/Fireball/fireball.scml", gameSim, 1280, 220, gameSim.RandomFloat(-120.0f, -80.0f), gameSim.RandomFloat(.1f, .2f));
			gameSim.addFireBall();
		}
		else
		{
			m_fireBall.tick(gameSim, dt);
		}
	}
	else if (m_volcanoState == VC_AFTER)
	{
		if (m_fireBall.m_y < -850.f)
		{
			if (m_fireBall.active)
			{
				gameSim.addFireBall();

				m_fireBall = FireBall();
			}

			if (gameSim.Random() % 1000 == 666)
				m_fireBall.load("backgrounds/VolcanoTest/Fireball/fireball.scml", gameSim, 1300, 340, gameSim.RandomFloat(-120.0f, -80.0f), gameSim.RandomFloat(.1f, .2f));
		}
		else
		{
			m_fireBall.tick(gameSim, dt);
		}
	}
}

void Background::draw()
{
	BACKGROUND_SPRITER.draw(m_state);

	m_fireBall.draw();
}

void Background::doEvent(GameSim & gameSim)
{
	m_state.startAnim(BACKGROUND_SPRITER, "Erupt");

	gameSim.playSound("volcano-eruption.ogg");
	gameSim.addScreenShake(-5.5f, 5.5f, 7500.f, 5.f);

	m_isTriggered = true;
	m_volcanoState = VC_ERUPT;

	m_fireBall.load("backgrounds/VolcanoTest/Fireball/fireball.scml", gameSim, 1300, 340, -100, 0.15f);
}

void Background::launchFireBall()
{
}
