#include "background.h"


#include "main.h"
#include "framework.h"

Background::Background() : m_spriter(0), m_state(0)
{
}

Background::~Background()
{
	clear();
}

void Background::clear()
{
	if (m_spriter)
		delete m_spriter;
	if (m_state)
		delete m_state;

}

void Background::reset()
{
	m_state->startAnim(*m_spriter, "Idle");
}

void Background::load(const char * name)
{
	clear();

	m_spriter = new Spriter(name);
	m_state = new SpriterState();
	m_state->startAnim(*m_spriter, "Idle");

}

float t = 0.f;
void Background::tick(GameSim & gameSim)
{
	if (t > 20.f)
	{
		doEvent();
		t = 0.f;
	}

	if (m_state->updateAnim(*m_spriter, 1.f / 60.f))
		m_state->startAnim(*m_spriter, "Idle");

	t += 1.f / 60.f;
}

void Background::draw()
{
	m_spriter->draw(*m_state);
}

void Background::doEvent()
{
	m_state->startAnim(*m_spriter, "Erupt");
}