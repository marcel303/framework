#include "enemy_path.h"
#include "game.h"
#include "level.h"
#include "tower.h"
#include "wave.h"

Level::Level(Game* game)
{
	mGame = 0;
	mPath = 0;
	mWaveList = 0;
	mWaveIndex = 0;
	mBuildMoney = 0.0f;
	
	//
	
	mGame = game;
}

Level::~Level()
{
	delete mPath;
	mPath = 0;
	
	delete mWaveList;
	mWaveList = 0;
}

void Level::Load(Stream* pathStream, Stream* waveStream)
{
	Assert(!mPath);
	Assert(!mWaveList);
	
	mPlacementRect.m_Position[0] = 0.0f;
	mPlacementRect.m_Position[1] = 0.0f;
	mPlacementRect.m_Size[0] = 320.0f;
	mPlacementRect.m_Size[1] = 480.0f;
	
	mPath = new EnemyPath();
	mPath->Load(pathStream);
	
	mWaveList = new WaveList();
	mWaveList->Load(waveStream);
	
	mBuildMoney = 100.0f;
}

void Level::Save(std::string path)
{
	Assert(mPath);
	Assert(mWaveList);
	
	throw ExceptionNA();
	
	mPath->Save(0);
	mWaveList->Save(0);
}

void Level::Update(float dt)
{
	mPath->Update(dt);
	
	// update towers
	// note: must be updated first, because of single-cycle effects such as slow
	
	for (std::vector<Tower*>::iterator i = mTowers.begin(); i != mTowers.end(); ++i)
	{
		Tower* tower = *i;
		
		tower->Update(dt);
	}
	
	// update enemies
	
	for (std::vector<Enemy*>::iterator i = mEnemies.begin(); i != mEnemies.end();)
	{
		Enemy* enemy = *i;
		
		enemy->Update(dt);
		
		if (enemy->Flags_get() & EnemyFlag_Dead)
		{
			delete enemy;
			
			i = mEnemies.erase(i);
		}
		else
			++i;
	}
	
	// check if waves should become active
	
	if (mActiveWaves.size() == 0)
	{
		Wave* wave = NextWave();
		
		if (wave)
		{
			wave->time -= dt;
			
			if (wave->time <= 0.0f)
			{
				LOG_DBG("wave: idle -> active", 0);
				
				wave->state = WaveState_Active;
				
				mActiveWaves.push_back(wave);
				
				mWaveIndex++;
			}
		}
	}
	
/*	while ((wave = NextWave()) != 0)
	{
		LOG_DBG("wave: idle -> active", 0);
		
		wave->state = WaveState_Active;
		
		mActiveWaves.push_back(wave);
	}*/
	
	// update active waves
	
	for (std::vector<Wave*>::iterator i = mActiveWaves.begin(); i != mActiveWaves.end();)
	{
		Wave* wave = *i;
		
		wave->Update(mGame, dt);
		
		if (wave->state == WaveState_Done)
			i = mActiveWaves.erase(i);
		else
			++i;
	}
}

void Level::Render()
{
	// render tower visibility radii
	
	for (std::vector<Tower*>::iterator i = mTowers.begin(); i != mTowers.end(); ++i)
	{
		Tower* tower = *i;
		
		tower->Render_Visibility();
	}
	
	// render path
	
	mPath->Render();
	
	// render enemies
	
	for (std::vector<Enemy*>::iterator i = mEnemies.begin(); i != mEnemies.end(); ++i)
	{
		Enemy* enemy = *i;
		
		enemy->Render();
	}
	
	// render towers
	
	for (std::vector<Tower*>::iterator i = mTowers.begin(); i != mTowers.end(); ++i)
	{
		Tower* tower = *i;
		
		tower->Render();
	}
}

bool Level::IsFree(Vec2F location, float radius)
{
	// check against path
	
	if (mPath->CalculateDistance(location) <= radius)
		return false;
	
	// check against towers
	
	for (std::vector<Tower*>::iterator i = mTowers.begin(); i != mTowers.end(); ++i)
	{
		Tower* tower = *i;
		
		float distance = (tower->Position_get() - location).Length_get();
		
		if (distance <= radius + tower->Radius_get())
		{
			LOG_DBG("free: no: blocked by tower", 0);
			
			return false;
		}
	}
	
	// check against level border
	
	if (!mPlacementRect.IsInside(location + Vec2F(-radius, -radius)) || !mPlacementRect.IsInside(location + Vec2F(+radius, +radius)))
	{
		LOG_DBG("free: no: outside level", 0);
		return false;
	}
	
	LOG_DBG("free: yes", 0);
	
	return true;
}

Enemy* Level::FindEnemyInRadius(Vec2F location, float radius)
{
	for (std::vector<Enemy*>::iterator i = mEnemies.begin(); i != mEnemies.end(); ++i)
	{
		Enemy* enemy = *i;
		
		Vec2F p1 = location;
		Vec2F p2 = enemy->Position_get();
		
		if (p1.DistanceTo(p2) > radius)
			continue;
		
		return enemy;
	}
	
	return 0;
}

void Level::ForEachEnemyInRadius(Vec2F location, float radius, ForEachEnemyInRadiusCB cb, void* obj)
{
	for (std::vector<Enemy*>::iterator i = mEnemies.begin(); i != mEnemies.end(); ++i)
	{
		Enemy* enemy = *i;
		
		Vec2F p1 = location;
		Vec2F p2 = enemy->Position_get();
		
		if (p1.DistanceTo(p2) > radius)
			continue;
		
		cb(obj, enemy);
	}
}

void Level::AddEnemy(Enemy* enemy)
{
	enemy->Attach(mPath);
	
	mEnemies.push_back(enemy);
}

void Level::AddTower(Tower* tower)
{
	mTowers.push_back(tower);
}

void Level::RemoveTower(Tower* tower)
{
	std::vector<Tower*>::iterator i = std::find(mTowers.begin(), mTowers.end(), tower);
	
	Assert(i != mTowers.end());
	
	if (i == mTowers.end())
		return;
	
	gGame->DeselectTower(tower);
	
	mTowers.erase(i);
	
	delete tower;
}

float Level::BuildMoney_get() const
{
	return mBuildMoney;
}

void Level::BuildMoney_increase(float amount)
{
	Assert(amount >= 0.0f);
	
	mBuildMoney += amount;
	
	if (mBuildMoney < 0.0f)
		mBuildMoney = 0.0f;
}

void Level::BuildMoney_decrease(float amount)
{
	Assert(amount >= 0.0f);
	Assert(amount <= mBuildMoney);
	
	mBuildMoney -= amount;
	
	if (mBuildMoney < 0.0f)
		mBuildMoney = 0.0f;
}

Tower* Level::HitTest_Tower(Vec2F location)
{
	for (std::vector<Tower*>::iterator i = mTowers.begin(); i != mTowers.end(); ++i)
	{
		Tower* tower = *i;
		
		if (tower->HitTest(location))
			return tower;
	}
	
	return 0;
}

Wave* Level::NextWave()
{
	if (mActiveWaves.size() > 0)
		return 0;
	
	if (mWaveIndex >= mWaveList->WaveCount_get())
		return 0;
	
	return mWaveList->Wave_get(mWaveIndex);
	
	/*for (int i = 0; i < mWaveList->WaveCount_get(); ++i)
	{
		Wave* wave = mWaveList->Wave_get(i);
		
		if (wave->state != WaveState_Idle)
			continue;
		
		if (wave->time > mGame->Time_get())
			continue;
		
		return wave;
	}*/
	
//	return 0;
}

