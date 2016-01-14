#pragma once

#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"
#include "fireball.h"
#include <vector>

struct CamParams;
class GameSim;

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
		
		void tick(GameSim & gameSim, Background & background, float dt);

		SpriterState m_spriterState;
	};

	struct VolcanoState
	{
		VolcanoState();

		void tick(GameSim & gameSim, Background & background, float dt);
		void doEvent(GameSim & gameSim);

		enum State
		{
			VC_IDLE,
			VC_ERUPT,
			VC_AFTER,
		} m_state;

		SpriterState m_spriterState;

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

		void tick(GameSim & gameSim, Background & background, float dt);

		SpriterState m_spriterState1;
		SpriterState m_spriterState2;
	};

	//

	LevelTheme m_type;

	LobbyState m_lobbyState;
	VolcanoState m_volcanoState;
	IceState m_iceState;
};

#pragma pack(pop)

struct Theme
{
	std::string name;
	float parallax1;
	float parallax2;
};

extern std::vector<Theme> g_themes;
