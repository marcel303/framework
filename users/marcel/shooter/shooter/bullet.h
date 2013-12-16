#pragma once

#include "forward.h"
#include "handle.h"
#include "types2.h"

enum BulletType
{
	BulletType_Red,
	BulletType_Missile,
	BulletType_Swarm,
	BulletType_Orange,
	BulletType_Intersector
};

class Bullet
{
public:
	Bullet(xHandle handle);
	~Bullet();

	void Make_Red(float x, float y, float angle);
	void Make_Missile(float x, float y, int target);
	void Make_Swarm(float x, float y, float angle);
	void Make_Swarm(float x, float y, float angle, float angleSpeed, int hopCount);
	void Make_Orange(float x, float y);
	void Make_Intersector(float x, float y, float angle);
	
	void Update(float dt);
	void Render();

	xHandle Handle_get() const;
	bool IsDead_get() const;
	const Vec2F& Position_get() const;

	bool SelectMissileTarget();

	static void ForEachHandler_Damage(void* obj, Enemy* enemy);
	static void ForEachHandler_MissileTarget(void* obj, Enemy* enemy);

private:
	xHandle mHandle;
	bool mIsDead;
	BulletType mBulletType;
	Vec2F mPosition;
	float mDamage;
	Color* mColor;

	union
	{
		struct
		{
			xHandle target;
			bool isHunting;
			float angle;
			float huntTime;
		} mMissile;
		struct
		{
			float speed[2];
			float life;
		} mRed;
		struct
		{
			float angle;
			float angleSpeed;
			float life;
			bool hasHopped;
			int hopCount;
		} mSwarm;
		struct
		{
			float speed[2];
			float life;
		} mOrange;
		struct
		{
			float speed[2];
			float life;
		} mIntersector;
	};
};
