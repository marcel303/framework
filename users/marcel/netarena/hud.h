#pragma once

#include "framework.h"
#include "Types.h"

class GameSim;

#pragma pack(push)
#pragma pack(1)

struct PlayerStatusHud
{
	enum State
	{
		kState_Idle,
		kState_Kill,
		kState_Death,
		kState_DeathLoop,
		kState_Spawn,
		kState_Score
	};

	State state;
	float stateTime;
	SpriterState m_spriterState;

	void setup(int playerIndex);

	void tick(GameSim & gameSim, int playerIndex, float dt);
	void draw(const GameSim & gameSim, int playerIndex) const;

	void handleCharacterIndexChange();
	void handleKill();
	void handleDeath();
	void handleSpawn();
	void handleRespawn();
	void handleNewRound();
	void handleScore();
};

#pragma pack(pop)
