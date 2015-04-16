#include "background.h"
#include "framework.h"
#include "gamesim.h"
#include "main.h"

#define BACKGROUND_SPRITER Spriter(m_name.c_str())

void Background::load(const char * name, GameSim& gameSim)
{
	memset(this, 0, sizeof(Background));

	m_name = name;
	m_state = SpriterState();
	m_state.startAnim(BACKGROUND_SPRITER, "Idle");

	m_startErupt = gameSim.RandomFloat(40.0f, 120.0f);

	t1 = true;
	t2 = true;
	t3 = true;
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
		if(m_isTriggered)
			m_state.startAnim(BACKGROUND_SPRITER, "IdleErupted");
		else
			m_state.startAnim(BACKGROUND_SPRITER, "Idle");
}

void Background::draw()
{
	BACKGROUND_SPRITER.draw(m_state);
}

void Background::doEvent(GameSim & gameSim)
{
	m_state.startAnim(BACKGROUND_SPRITER, "Erupt");

	gameSim.playSound("volcano-eruption.ogg");
	gameSim.addScreenShake(-5.5f, 5.5f, 7500.f, 5.f);

	m_isTriggered = true;
}
