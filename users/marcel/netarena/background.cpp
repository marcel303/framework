#include "background.h"
#include "fireball.h"
#include "framework.h"
#include "gamesim.h"
#include "main.h"

OPTION_DECLARE(bool, s_debugBackground, false);
OPTION_DEFINE(bool, s_debugBackground, "Debug/Debug Background");

#define LOBBY_SPRITER Spriter("backgrounds/lobby/background.scml")
#define VOLCANO_SPRITER Spriter("backgrounds/VolcanoTest/background.scml")
#define ICE_SPRITER1 Spriter("backgrounds/ice/layer1.scml") // todo
#define ICE_SPRITER2 Spriter("backgrounds/ice/layer2.scml")

//

std::vector<Theme> g_themes;

static const char * getThemeName(LevelTheme type)
{
	if (type == kLevelTheme_Volcano)
		return "volcano";
	else if (type == kLevelTheme_Lobby)
		return "ice";
	else if (type == kLevelTheme_Ice)
		return "lobby";
	else
	{
		Assert(false);
		return "";
	}
}

static const Theme * getTheme(const char * name)
{
	for (int i = 0; i < g_themes.size(); ++i)
		if (g_themes[i].name == name)
			return &g_themes[i];
	return 0;
}

//

void Background::load(LevelTheme type, GameSim & gameSim)
{
	memset(this, 0, sizeof(Background));

	m_type = type;

	switch (m_type)
	{
	case kLevelTheme_Lobby:
		m_lobbyState = LobbyState();
		break;

	case kLevelTheme_Volcano:
		m_volcanoState = VolcanoState();
		break;

	case kLevelTheme_Ice:
		m_iceState = IceState();
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
	case kLevelTheme_Lobby:
		m_lobbyState.tick(gameSim, *this, dt);
		break;

	case kLevelTheme_Volcano:
		m_volcanoState.tick(gameSim, *this, dt);
		break;

	default:
		Assert(false);
		break;
	}
}

void Background::draw(const GameSim & gameSim, const CamParams & camParams)
{
	const char * themeName = getThemeName(m_type);
	const Theme * theme = getTheme(themeName);
	Assert(theme);

	if (!theme)
		return;

	switch (m_type)
	{
	case kLevelTheme_Lobby:
		gxPushMatrix();
		gameSim.applyCamParams(camParams, theme->parallax1, BACKGROUND_SCREENSHAKE_MULTIPLIER);
		{
			//setBlend(BLEND_OPAQUE);
			gxScalef(1.f / gameSim.m_arena.getBaseZoom(), 1.f / gameSim.m_arena.getBaseZoom(), 1.f);
			setColor(colorWhite);
			LOBBY_SPRITER.draw(m_lobbyState.m_spriterState);
		}
		gxPopMatrix();

		if (s_debugBackground)
		{
			setColor(colorWhite);
			drawText(0.f, 100.f, 24, +1.f, +1.f, "background: type=lobby, animTime=%02.2f", m_lobbyState.m_spriterState.animTime);
		}
		break;

	case kLevelTheme_Volcano:
		gxPushMatrix();
		gameSim.applyCamParams(camParams, theme->parallax1, BACKGROUND_SCREENSHAKE_MULTIPLIER);
		{
			//setBlend(BLEND_OPAQUE);
			gxScalef(1.f / gameSim.m_arena.getBaseZoom(), 1.f / gameSim.m_arena.getBaseZoom(), 1.f);
			setColor(colorWhite);
			VOLCANO_SPRITER.draw(m_volcanoState.m_spriterState);
			if (m_volcanoState.m_fireBall.m_isActive)
				m_volcanoState.m_fireBall.draw();
		}
		gxPopMatrix();

		if (s_debugBackground)
		{
			setColor(colorWhite);
			drawText(0.f, 100.f, 24, +1.f, +1.f, "background: type=volcano, animTime=%02.2f, fireballIsActive=%d", m_volcanoState.m_spriterState.animTime, m_volcanoState.m_fireBall.m_isActive);
		}
		break;

	case kLevelTheme_Ice:
		gxPushMatrix();
		gameSim.applyCamParams(camParams, theme->parallax1, BACKGROUND_SCREENSHAKE_MULTIPLIER);
		{
			gxScalef(1.f / gameSim.m_arena.getBaseZoom(), 1.f / gameSim.m_arena.getBaseZoom(), 1.f);
			setColor(colorWhite);
			ICE_SPRITER1.draw(m_volcanoState.m_spriterState);
		}
		gxPopMatrix();

		gxPushMatrix();
		gameSim.applyCamParams(camParams, theme->parallax2, BACKGROUND_SCREENSHAKE_MULTIPLIER);
		{
			gxScalef(1.f / gameSim.m_arena.getBaseZoom(), 1.f / gameSim.m_arena.getBaseZoom(), 1.f);
			setColor(colorWhite);
			ICE_SPRITER2.draw(m_volcanoState.m_spriterState);
		}
		gxPopMatrix();
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
	case kLevelTheme_Volcano:
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
			background.load(kLevelTheme_Volcano, gameSim);
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

//

Background::IceState::IceState()
{
	memset(this, 0, sizeof(*this));

	m_spriterState1 = SpriterState();
	m_spriterState1.startAnim(ICE_SPRITER1, "Idle");

	m_spriterState2 = SpriterState();
	m_spriterState2.startAnim(ICE_SPRITER2, "Idle");
}

void Background::IceState::tick(GameSim & gameSim, Background & background, float dt)
{
	m_spriterState1.updateAnim(ICE_SPRITER1, dt);
	m_spriterState2.updateAnim(ICE_SPRITER2, dt);
}
