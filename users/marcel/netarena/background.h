#pragma once

#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"

class GameSim;

class Background
{
public:
	void load(const char * name, GameSim& gameSim);

	void tick(GameSim & gameSim, float dt);
	void draw();

	void doEvent(GameSim & gameSim);

public:
	FixedString<64> m_name;
	SpriterState m_state;
	bool m_isTriggered;

	float m_startErupt;

	bool t1;
	bool t2;
	bool t3;
};