#pragma once

#include "enemy.h"
#include "libgg_forward.h"

enum WaveState
{
	WaveState_Idle,
	WaveState_Active,
	WaveState_Done
};

class Wave
{
public:
	Wave();
	
	void Update(Game* game, float dt);
	
	WaveState state;
	EnemyType type;
	float time;
	int count;
	float interval;
	EnemyDesc desc;
	
private:
	float spawnTime;
};

class WaveList
{
public:
	WaveList();
	~WaveList();
	
	void Load(Stream* stream);
	void Save(Stream* stream);
	
	int WaveCount_get() const;
	Wave* Wave_get(int index);
	
private:
	int mWaveCount;
	Wave* mWaves;
};
