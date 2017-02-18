#include "background.h"
#include "fireball.h"
#include "framework.h"
#include "gamesim.h"
#include "main.h"

#include "StringEx.h" // _s functions

OPTION_DECLARE(bool, s_debugBackground, false);
OPTION_DEFINE(bool, s_debugBackground, "Debug/Debug Background");

static char s_tempFilename[256];
static const char * makeTempFilename(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	vsprintf_s(s_tempFilename, sizeof(s_tempFilename), format, args);
	va_end(args);
	return s_tempFilename;
}

//#define LOBBY_SPRITER Spriter("backgrounds/lobby/background.scml")
//#define VOLCANO_SPRITER Spriter("backgrounds/VolcanoTest/background.scml")
//#define ICE_SPRITER(i) Spriter(makeTempFilename("backgrounds/ice/layer%d.scml", i + 1))
#define BACKGROUND_SPRITER(bg, i) Spriter(makeTempFilename((bg).backgroundBaseName, i + 1))

//

std::vector<Theme> g_themes;

static const char * getThemeName(LevelTheme type)
{
	if (type == kLevelTheme_Volcano)
		return "volcano";
	else if (type == kLevelTheme_Lobby)
		return "lobby";
	else if (type == kLevelTheme_Ice)
		return "ice";
	else
	{
		Assert(false);
		return "";
	}
}

static const Theme * getTheme(const char * name)
{
	for (size_t i = 0; i < g_themes.size(); ++i)
		if (g_themes[i].name == name)
			return &g_themes[i];
	return 0;
}

//

void Background::load(LevelTheme type, GameSim & gameSim)
{
	memset(this, 0, sizeof(Background));

	m_type = type;

	const char * themeName = getThemeName(m_type);
	const Theme * theme = getTheme(themeName);
	Assert(theme);

	if (!theme)
		return;

	for (int i = 0; i < Theme::kMaxBackgroundLayers; ++i)
		m_backgroundLayers[i] = SpriterState();

	strcpy_s(backgroundBaseName, sizeof(backgroundBaseName), theme->backgroundBaseName.c_str());

	switch (m_type)
	{
	case kLevelTheme_Lobby:
		m_lobbyState = LobbyState();
		m_lobbyState.init(*this);
		break;

	case kLevelTheme_Volcano:
		m_volcanoState = VolcanoState();
		m_volcanoState.init(*this);
		break;

	case kLevelTheme_Ice:
		m_iceState = IceState();
		m_iceState.init(*this);
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

	case kLevelTheme_Ice:
		m_iceState.tick(gameSim, *this, dt);
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

	// draw background layers
	for (int i = 0; i < theme->numBackgroundLayers; ++i)
	{
		gxPushMatrix();
		gameSim.applyCamParams(camParams, theme->backgroundParallax[i], BACKGROUND_SCREENSHAKE_MULTIPLIER);
		{
			gxScalef(1.f / gameSim.m_arena.getBaseZoom(), 1.f / gameSim.m_arena.getBaseZoom(), 1.f);
			setColor(colorWhite);
			BACKGROUND_SPRITER(*this, i).draw(m_backgroundLayers[i]);
		}
		gxPopMatrix();
	}

	switch (m_type)
	{
	case kLevelTheme_Lobby:
		break;

	case kLevelTheme_Volcano:
		gxPushMatrix();
		gameSim.applyCamParams(camParams, theme->backgroundParallax[0], BACKGROUND_SCREENSHAKE_MULTIPLIER);
		{
			//setBlend(BLEND_OPAQUE);
			gxScalef(1.f / gameSim.m_arena.getBaseZoom(), 1.f / gameSim.m_arena.getBaseZoom(), 1.f);
			setColor(colorWhite);
			if (m_volcanoState.m_fireBall.m_isActive)
				m_volcanoState.m_fireBall.draw();
		}
		gxPopMatrix();
		break;

	case kLevelTheme_Ice:
		break;

	default:
		Assert(false);
		break;
	}

	if (s_debugBackground)
	{
		for (int i = 0; i < theme->numBackgroundLayers; ++i)
		{
			setColor(colorWhite);
			drawText(0.f, 100.f, 24, +1.f, +1.f, "background: type=%d, layer=%d, animTime=%02.2f", m_type, i, m_backgroundLayers[i].animTime);

			if (m_type == kLevelTheme_Volcano) 
				drawText(0.f, 100.f, 24, +1.f, +1.f, "background: fireballIsActive=%d", m_volcanoState.m_fireBall.m_isActive);
		}
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
            
    case kLevelTheme_Ice:
    case kLevelTheme_Lobby:
        break;
        
    case kLevelTheme_COUNT:
        break;
	}
}

//

Background::LobbyState::LobbyState()
{
}

void Background::LobbyState::init(Background & background)
{
	memset(this, 0, sizeof(*this));

	background.m_backgroundLayers[0].startAnim(BACKGROUND_SPRITER(background, 0), "Idle");
}

void Background::LobbyState::tick(GameSim & gameSim, Background & background, float dt)
{
	background.m_backgroundLayers[0].updateAnim(BACKGROUND_SPRITER(background, 0), dt);
}

//

Background::VolcanoState::VolcanoState()
{
}

void Background::VolcanoState::init(Background & background)
{
	memset(this, 0, sizeof(*this));

	m_state = VC_IDLE;

	background.m_backgroundLayers[0].startAnim(BACKGROUND_SPRITER(background, 0), "Idle");

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
		doEvent(gameSim, background);
	}

	SpriterState & spriterState = background.m_backgroundLayers[0];

	if (spriterState.updateAnim(BACKGROUND_SPRITER(background, 0), dt))
	{
		if (m_isTriggered)
		{
			spriterState.startAnim(BACKGROUND_SPRITER(background, 0), "IdleErupted");
			if (m_state == VC_ERUPT)
				m_state = VC_AFTER;
		}
		else
		{
			spriterState.startAnim(BACKGROUND_SPRITER(background, 0), "Idle");
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

void Background::VolcanoState::doEvent(GameSim & gameSim, Background & background)
{
	background.m_backgroundLayers[0].startAnim(BACKGROUND_SPRITER(background, 0), "Erupt");

	gameSim.playSound("volcano-eruption.ogg");
	gameSim.addScreenShake(-5.5f, 5.5f, 7500.f, 5.f, true);

	m_isTriggered = true;
	m_state = VC_ERUPT;

	m_fireBall.load("backgrounds/VolcanoTest/Fireball/fireball.scml", gameSim, 1300, 340, -100, 0.15f);
}

//

Background::IceState::IceState()
{
}

void Background::IceState::init(Background & background)
{
	memset(this, 0, sizeof(*this));

	background.m_backgroundLayers[0].startAnim(BACKGROUND_SPRITER(background, 0), 0);
	background.m_backgroundLayers[1].startAnim(BACKGROUND_SPRITER(background, 1), 0);
}

void Background::IceState::tick(GameSim & gameSim, Background & background, float dt)
{
	background.m_backgroundLayers[0].updateAnim(BACKGROUND_SPRITER(background, 0), dt);
	background.m_backgroundLayers[1].updateAnim(BACKGROUND_SPRITER(background, 1), dt);
}
