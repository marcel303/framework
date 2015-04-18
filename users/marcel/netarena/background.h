#pragma once

#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"
#include "fireball.h"

class GameSim;
class Background
{
public:
	void load(const char * name, GameSim& gameSim);

	void tick(GameSim & gameSim, float dt);
	void draw();

	void doEvent(GameSim & gameSim);

	void launchBall();

public:
	FixedString<64> m_name;
	SpriterState m_state;
	FireBall m_fb;
	bool m_isTriggered;

	float m_startErupt;

	bool t1;
	bool t2;
	bool t3;

	enum VolcanoState
	{
		VC_IDLE,
		VC_ERUPT,
		VC_AFTER,
	} m_volcanoState;
};

