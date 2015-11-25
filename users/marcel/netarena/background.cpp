#include "background.h"
#include "fireball.h"
#include "framework.h"
#include "gamesim.h"
#include "main.h"

OPTION_DECLARE(bool, s_debugBackground, false);
OPTION_DEFINE(bool, s_debugBackground, "Debug/Debug Background");

#define LOBBY_SPRITER Spriter("backgrounds/lobby/background.scml")
#define VOLCANO_SPRITER Spriter("backgrounds/VolcanoTest/background.scml")

void Background::load(BackgroundType type, GameSim & gameSim)
{
	memset(this, 0, sizeof(Background));

	m_type = type;

	switch (m_type)
	{
	case kBackgroundType_Lobby:
		m_lobbyState = LobbyState();
		break;

	case kBackgroundType_Volcano:
		m_volcanoState = VolcanoState();
		break;

	default:
		Assert(false);
		break;
	}
}

void Background::tick(GameSim & gameSim, float dt)
{
	switch (m_type)
	{
	case kBackgroundType_Lobby:
		m_lobbyState.tick(gameSim, *this, dt);
		break;

	case kBackgroundType_Volcano:
		m_volcanoState.tick(gameSim, *this, dt);
		break;

	default:
		Assert(false);
		break;
	}
}

void Background::draw()
{
	switch (m_type)
	{
	case kBackgroundType_Lobby:
		setColor(colorWhite);
		LOBBY_SPRITER.draw(m_lobbyState.m_spriterState);

		if (s_debugBackground)
		{
			setColor(colorWhite);
			drawText(0.f, 100.f, 24, +1.f, +1.f, "background: type=lobby, animTime=%02.2f", m_lobbyState.m_spriterState.animTime);
		}
		break;

	case kBackgroundType_Volcano:
		setColor(colorWhite);
		VOLCANO_SPRITER.draw(m_volcanoState.m_spriterState);
		if (m_volcanoState.m_fireBall.m_isActive)
			m_volcanoState.m_fireBall.draw();

		if (s_debugBackground)
		{
			setColor(colorWhite);
			drawText(0.f, 100.f, 24, +1.f, +1.f, "background: type=volcano, animTime=%02.2f, fireballIsActive=%d", m_volcanoState.m_spriterState.animTime, m_volcanoState.m_fireBall.m_isActive);
		}
		break;

	default:
		Assert(false);
		break;
	}
}

void Background::drawLight()
{
	switch (m_type)
	{
	case kBackgroundType_Volcano:
		if (m_volcanoState.m_fireBall.m_isActive)
			m_volcanoState.m_fireBall.drawLight();
		break;
	}
}

//

Background::LobbyState::LobbyState()
{
	memset(this, 0, sizeof(*this));

	m_spriterState = SpriterState();
	m_spriterState.startAnim(LOBBY_SPRITER, "Idle");
}

void Background::LobbyState::tick(GameSim & gameSim, Background & background, float dt)
{
	m_spriterState.updateAnim(LOBBY_SPRITER, dt);
}

//

Background::VolcanoState::VolcanoState()
{
	memset(this, 0, sizeof(*this));

	m_state = VC_IDLE;

	m_spriterState = SpriterState();
	m_spriterState.startAnim(VOLCANO_SPRITER, "Idle");

	m_startErupt = 0;

	t1 = true;
	t2 = true;
	t3 = true;

	m_fireBall.m_isActive = false;
}

void Background::VolcanoState::tick(GameSim & gameSim, Background & background, float dt)
{
	if (m_startErupt == 0)
	{
		if (VOLCANO_LOOP)
			m_startErupt = gameSim.m_roundTime + VOLCANO_LOOP_TIME;
		else
			m_startErupt = gameSim.RandomFloat(50.0f, 180.0f);
	}

	if (t1 && !m_isTriggered && gameSim.m_roundTime >= (m_startErupt - 12.f))
	{
		gameSim.addScreenShake(-1.2f, 1.3f, 4000.f, 6.f, false);
		t1 = false;
	}

	if (t2 && !m_isTriggered && gameSim.m_roundTime >= (m_startErupt - 6.f))
	{
		gameSim.addScreenShake(-2.3f, 1.5f, 6000.f, 4.f, false);
		gameSim.playSound("volcano-rumble.ogg");
		
		t2 = false;
	}

	if (t3 && !m_isTriggered && gameSim.m_roundTime >= (m_startErupt - 2.f))
	{
		gameSim.addScreenShake(-3.f, 2.5f, 7000.f, 2.f, true);
		t3 = false;
	}

	if (!m_isTriggered && gameSim.m_roundTime >= m_startErupt)
	{
		doEvent(gameSim);
	}

	if (m_spriterState.updateAnim(VOLCANO_SPRITER, dt))
	{
		if (m_isTriggered)
		{
			m_spriterState.startAnim(VOLCANO_SPRITER, "IdleErupted");
			if (m_state == VC_ERUPT)
				m_state = VC_AFTER;
		}
		else
		{
			m_spriterState.startAnim(VOLCANO_SPRITER, "Idle");
		}
	}

	if (m_state == VC_ERUPT)
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
	else if (m_state == VC_AFTER)
	{
		if (VOLCANO_LOOP)
		{
			background.load(kBackgroundType_Volcano, gameSim);
			return;
		}

		if (m_fireBall.m_y < -850.f)
		{
			if (m_fireBall.m_isActive)
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

void Background::VolcanoState::doEvent(GameSim & gameSim)
{
	m_spriterState.startAnim(VOLCANO_SPRITER, "Erupt");

	gameSim.playSound("volcano-eruption.ogg");
	gameSim.addScreenShake(-5.5f, 5.5f, 7500.f, 5.f, true);

	m_isTriggered = true;
	m_state = VC_ERUPT;

	m_fireBall.load("backgrounds/VolcanoTest/Fireball/fireball.scml", gameSim, 1300, 340, -100, 0.15f);
}
