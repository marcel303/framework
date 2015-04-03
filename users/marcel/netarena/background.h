#pragma once

#include "gamedefs.h"
#include "gametypes.h"


class GameSim;
class Spriter;
class SpriterState;
class Background
{
public:
	Background();
	~Background();

	void clear();

	void reset();
	void load(const char * name);

	void tick(GameSim & gameSim);
	void draw();

	void doEvent();


public:
	Spriter* m_spriter;
	SpriterState* m_state;

};