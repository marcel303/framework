#include "background.h"
#include "framework.h"
#include "gamesim.h"
#include "main.h"

#define BACKGROUND_SPRITER Spriter(m_name.c_str())

void Background::load(const char * name)
{
	memset(this, 0, sizeof(Background));

	m_name = name;
	m_state = SpriterState();
	m_state.startAnim(BACKGROUND_SPRITER, "Idle");
}

void Background::tick(GameSim & gameSim, float dt)
{
	if (!m_isTriggered && gameSim.m_roundTime >= 20.f)
	{
		doEvent();
	}

	if (m_state.updateAnim(BACKGROUND_SPRITER, dt))
		m_state.startAnim(BACKGROUND_SPRITER, "Idle");
}

void Background::draw()
{
	BACKGROUND_SPRITER.draw(m_state);
}

void Background::doEvent()
{
	m_state.startAnim(BACKGROUND_SPRITER, "Erupt");

	m_isTriggered = true;
}
