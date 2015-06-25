#pragma once

#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"
#include "fireball.h"

class GameSim;

#pragma pack(push)
#pragma pack(1)

enum BackgroundType
{
	kBackgroundType_Volcano,
	kBackgroundType_Lobby,
	kBackgroundType_COUNT
};

struct Background
{
	Background()
	{
		memset(this, 0, sizeof(Background));
	}

	void load(BackgroundType type, GameSim & gameSim);

	void tick(GameSim & gameSim, float dt);
	void draw();
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

	//

	BackgroundType m_type;

	LobbyState m_lobbyState;
	VolcanoState m_volcanoState;
};

#pragma pack(pop)
