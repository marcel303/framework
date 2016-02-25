#pragma once

#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"
#include "fireball.h"
#include <vector>

struct CamParams;
class GameSim;

struct Theme
{
	static const int kMaxBackgroundLayers = 5;

	Theme()
	{
		for (int i = 0; i < kMaxBackgroundLayers; ++i)
			backgroundParallax[i] = 1.f;
		numBackgroundLayers = 0;
	}

	std::string name;
	std::string backgroundBaseName;
	float backgroundParallax[kMaxBackgroundLayers];
	int numBackgroundLayers;
};

extern std::vector<Theme> g_themes;

#pragma pack(push)
#pragma pack(1)

struct Background
{
	Background()
	{
		memset(this, 0, sizeof(Background));
	}

	void load(LevelTheme theme, GameSim & gameSim);

	void tick(GameSim & gameSim, float dt);
	void draw(const GameSim & gameSim, const CamParams & camParams);
	void drawLight();

	struct LobbyState
	{
		LobbyState();
		
		void init(Background & background);
		void tick(GameSim & gameSim, Background & background, float dt);
	};

	struct VolcanoState
	{
		VolcanoState();

		void init(Background & background);
		void tick(GameSim & gameSim, Background & background, float dt);
		void doEvent(GameSim & gameSim, Background & background);

		enum State
		{
			VC_IDLE,
			VC_ERUPT,
			VC_AFTER,
		} m_state;

		FireBall m_fireBall;
		bool m_isTriggered;

		float m_startErupt;

		bool t1;
		bool t2;
		bool t3;
	};

	struct IceState
	{
		IceState();

		void init(Background & background);
		void tick(GameSim & gameSim, Background & background, float dt);
	};

	//

	LevelTheme m_type;
	char backgroundBaseName[128];

	LobbyState m_lobbyState;
	VolcanoState m_volcanoState;
	IceState m_iceState;

	SpriterState m_backgroundLayers[Theme::kMaxBackgroundLayers];
	int m_numBackgroundLayers;
};

#pragma pack(pop)
