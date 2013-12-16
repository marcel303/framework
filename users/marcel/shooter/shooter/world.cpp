#include "bullet.h"
#include "enemy.h"
#include "handle.h"
#include "player.h"
#include "powerup.h"
#include "world.h"

#define MAX_HANDLES 5000
//#define WORLD_SX 640.0f
//#define WORLD_SY 480.0f
#define WORLD_SX 480.0f
#define WORLD_SY 320.0f

World* gWorld = 0;

World::World()
{
	mPlayer = new Player();
	mHandlePool = new HandlePool();

	mHandlePool->Setup(MAX_HANDLES);
	mGoInfoList = new GoInfo[MAX_HANDLES];
	memset(mGoInfoList, 0, sizeof(GoInfo) * MAX_HANDLES);

	mSx = WORLD_SX;
	mSy = WORLD_SY;
}

World::~World()
{
	delete[] mGoInfoList;
	delete mHandlePool;
	delete mPlayer;
}

void World::Begin()
{
#define CLEAR_LIST(name, type) \
	for (std::list<type*>::iterator i = name.begin(); i != name.end(); ++i) \
	{ \
		type* element = *i; \
		GoInfo& goInfo = mGoInfoList[element->Handle_get()]; \
		goInfo.isDead = true; \
		mHandlePool->Free(element->Handle_get()); \
		delete element; \
	} \
	name.clear()
	
	CLEAR_LIST(mBulletList, Bullet);
	CLEAR_LIST(mEnemyList, Enemy);
	CLEAR_LIST(mPowerupList, Powerup);

	delete mPlayer;
	mPlayer = 0;
	mPlayer = new Player();
}

void World::Update(float dt)
{
#define UPDATE_LIST(name, type) \
	for (std::list<type*>::iterator i = name.begin(); i != name.end();) \
	{ \
		type* element = *i; \
 \
		element->Update(dt); \
 \
		GoInfo& goInfo = mGoInfoList[element->Handle_get()]; \
 \
		goInfo.position = element->Position_get(); \
		goInfo.isDead = element->IsDead_get(); \
 \
		if (goInfo.isDead) \
		{ \
			mHandlePool->Free(element->Handle_get()); \
			delete element; \
			i = name.erase(i); \
		} \
		else \
		{ \
			i++; \
		} \
	}

	UPDATE_LIST(mBulletList, Bullet);
	UPDATE_LIST(mEnemyList, Enemy);
	UPDATE_LIST(mPowerupList, Powerup);

	mPlayer->Update(dt);
}

void World::Render()
{
#define RENDER_LIST(name, type) \
	for (std::list<type*>::iterator i = name.begin(); i != name.end(); ++i) \
	{ \
		type* element = *i; \
 \
		element->Render(); \
	}

	RENDER_LIST(mPowerupList, Powerup);
	RENDER_LIST(mEnemyList, Enemy);
	RENDER_LIST(mBulletList, Bullet);

	mPlayer->Render();
}

Bullet* World::AllocateBullet()
{
	xHandle handle = mHandlePool->Allocate();

	Bullet* result = new Bullet(handle);

	mBulletList.push_back(result);

	return result;
}

Enemy* World::AllocateEnemy()
{
	xHandle handle = mHandlePool->Allocate();

	Enemy* result = new Enemy(handle);

	mEnemyList.push_back(result);

	return result;
}

Powerup* World::AllocatePowerup()
{
	xHandle handle = mHandlePool->Allocate();

	Powerup* result = new Powerup(handle);

	mPowerupList.push_back(result);

	return result;
}

bool World::IsOutside(const Vec2F& position) const
{
	return
		position.x < 0.0f ||
		position.x > WORLD_SX ||
		position.y < 0.0f ||
		position.y > WORLD_SY;
}

Player* World::Player_get()
{
	return mPlayer;
}

const GoInfo& World::GoInfo_get(int index) const
{
	return mGoInfoList[index];
}

void World::ForEachEnemy(void* obj, WorldForEachEnemyCb cb)
{
	for (std::list<Enemy*>::iterator i = mEnemyList.begin(); i != mEnemyList.end(); ++i)
	{
		Enemy* enemy = *i;

		cb(obj, enemy);
	}
}
