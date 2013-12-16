#pragma once

#include "Types.h"

class EnemyInfo
{
public:
	inline EnemyInfo()
	{
		isAlive = false;
	}
	
	bool isAlive;
	Vec2F location;
};

class EnemyInfoMgr
{
public:
	EnemyInfoMgr(int maxEnemies);
	~EnemyInfoMgr();
	
	int Allocate();
	void Free(int index);
	void Update(int index, Vec2F location);
	
	EnemyInfo* EnemyInfo_get(int index);
	
private:
	EnemyInfo* mInfoList;
	int mInfoListSize;
	int mInfoListCursor;
};

extern EnemyInfoMgr* gEnemyInfoMgr;
