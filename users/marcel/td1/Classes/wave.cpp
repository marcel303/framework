#include "game.h"
#include "level.h"
#include "Log.h"
#include "wave.h"

#ifndef DEPLOYMENT
#include <vector>

static Wave MakeWave(EnemyType type, float time, int count)
{
	Wave wave;
	wave.type = type;
	wave.time = time;
	wave.count = count;
	wave.interval = 0.3f;
	
	wave.desc.type = EnemyType_A;
	wave.desc.health = 4.0f;
	wave.desc.speed = 10 * count;
	
	return wave;
}
#endif

Wave::Wave()
{
	state = WaveState_Idle;
	type = EnemyType_Undefined;
	time = 0.0f;
	count = 0;
	interval = 0.0f;
	spawnTime = 0.0f;
}

void Wave::Update(Game* game, float dt)
{
	while (spawnTime <= 0.0f && state == WaveState_Active)
	{
		spawnTime += interval;
		
		LOG_DBG("wave: spawn", 0);
		
		Enemy* enemy = new Enemy();
		
		enemy->Make(desc);
		
		game->Level_get()->AddEnemy(enemy);
			
		count--;

		if (count == 0)
		{
			state = WaveState_Done;
		}
	}
	
	spawnTime -= dt;
}

WaveList::WaveList()
{
	mWaveCount = 0;
	mWaves = 0;
}

WaveList::~WaveList()
{
	delete[] mWaves;
	mWaves = 0;
}

void WaveList::Load(Stream* stream)
{
#ifndef DEPLOYMENT
	std::vector<Wave> waves;
	for (int i = 0; i < 10; ++i)
	{
	waves.push_back(MakeWave(EnemyType_A, 5.0f, 5));
	waves.push_back(MakeWave(EnemyType_B, 5.0f, 5));
	waves.push_back(MakeWave(EnemyType_A, 5.0f, 10));
	waves.push_back(MakeWave(EnemyType_B, 5.0f, 10));
	waves.push_back(MakeWave(EnemyType_A, 5.0f, 15));
	waves.push_back(MakeWave(EnemyType_B, 5.0f, 15));
	waves.push_back(MakeWave(EnemyType_A, 5.0f, 4));
	waves.push_back(MakeWave(EnemyType_B, 5.0f, 4));
	}
	
	mWaveCount = waves.size();
	mWaves = new Wave[mWaveCount];
	
	for (size_t i = 0; i < waves.size(); ++i)
		mWaves[i] = waves[i];
#else
#endif
}

void WaveList::Save(Stream* stream)
{
}

int WaveList::WaveCount_get() const
{
	return mWaveCount;
}

Wave* WaveList::Wave_get(int index)
{
	if (index < 0 || index >= mWaveCount)
		return 0;
	
	return &mWaves[index];
}
