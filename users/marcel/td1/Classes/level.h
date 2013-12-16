#pragma once

#include <vector>
#include "libgg_forward.h"
#include "td_forward.h"
#include "Types.h"

typedef void (*ForEachEnemyInRadiusCB)(void* obj, Enemy* enemy);

class Level
{
public:
	Level(Game* game);
	~Level();
	
	void Load(Stream* pathStream, Stream* waveStream);
	void Save(std::string path);
	
	void Update(float dt);
	void Render();
	
	bool IsFree(Vec2F location, float radius);
	Enemy* FindEnemyInRadius(Vec2F location, float radius);
	void ForEachEnemyInRadius(Vec2F location, float radius, ForEachEnemyInRadiusCB cb, void* obj);
	
	void AddEnemy(Enemy* enemy);
	void AddTower(Tower* tower);
	void RemoveTower(Tower* tower);
	
	float BuildMoney_get() const;
	void BuildMoney_increase(float amount);
	void BuildMoney_decrease(float amount);
	
	Tower* HitTest_Tower(Vec2F location);
	
private:
	Wave* NextWave();
	
	Game* mGame;
	RectF mPlacementRect;
	EnemyPath* mPath;
	WaveList* mWaveList;
	int mWaveIndex;
	std::vector<Tower*> mTowers;
	std::vector<Wave*> mActiveWaves;
	std::vector<Enemy*> mEnemies;
	float mBuildMoney;
};
