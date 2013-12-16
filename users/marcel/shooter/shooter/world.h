#pragma once

#include <list>
#include "forward.h"
#include "types2.h"

extern World* gWorld;

class GoInfo
{
public:
	Vec2F position;
	bool isDead;
};

typedef void (*WorldForEachEnemyCb)(void* obj, Enemy* enemy);

class World
{
public:
	World();
	~World();

	void Begin();
	
	void Update(float dt);
	void Render();

	Bullet* AllocateBullet();
	Enemy* AllocateEnemy();
	Powerup* AllocatePowerup();

	bool IsOutside(const Vec2F& position) const;

	Player* Player_get();

	const GoInfo& GoInfo_get(int index) const;

	void ForEachEnemy(void* obj, WorldForEachEnemyCb cb);

	std::list<Bullet*> mBulletList;
	std::list<Enemy*> mEnemyList;
	std::list<Powerup*> mPowerupList;
	Player* mPlayer;
	HandlePool* mHandlePool;
	GoInfo* mGoInfoList;

	float mSx;
	float mSy;
};
