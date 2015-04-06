#pragma once

#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"

class GameSim;

class Background
{
public:
	void load(const char * name);

	void tick(GameSim & gameSim, float dt);
	void draw();

	void doEvent();

public:
	FixedString<64> m_name;
	SpriterState m_state;
	bool m_isTriggered;
};