#include "enemy_info.h"

EnemyInfoMgr* gEnemyInfoMgr = 0;

EnemyInfoMgr::EnemyInfoMgr(int maxEnemies)
{
	mInfoList = new EnemyInfo[maxEnemies];
	mInfoListSize = maxEnemies;
	mInfoListCursor = 0;
}

EnemyInfoMgr::~EnemyInfoMgr()
{
	delete[] mInfoList;
	mInfoList = 0;
}

int EnemyInfoMgr::Allocate()
{
	mInfoListCursor++;
	mInfoListCursor %= mInfoListSize;
	mInfoList[mInfoListCursor].isAlive = true;
	return mInfoListCursor;
}

void EnemyInfoMgr::Free(int index)
{
	Assert(index >= 0 && index < mInfoListSize);
	
	mInfoList[mInfoListCursor].isAlive = false;
}

void EnemyInfoMgr::Update(int index, Vec2F location)
{
	Assert(index >= 0 && index < mInfoListSize);
	
	mInfoList[index].location = location;
}

EnemyInfo* EnemyInfoMgr::EnemyInfo_get(int index)
{
	Assert(index >= 0 && index < mInfoListSize);
	
	return mInfoList + index;
}
