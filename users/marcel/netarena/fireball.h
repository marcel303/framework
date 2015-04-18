#pragma once

#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"

class GameSim;
class FireBall
{
public:
	void load(const char * name, GameSim& gameSim, int x = 600, int y = 0, float angle = 60.0f, float scale = 0.25f);

	void tick(GameSim & gameSim, float dt);
	void draw();

public:
	FixedString<64> m_name;
	SpriterState m_state;

	float m_x;
	float m_y;

	bool active;

	float m_speed;
};