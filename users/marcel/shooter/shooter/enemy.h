#pragma once

#include "entity.h"
#include "handle.h"
#include "types2.h"

enum EnemyType
{
	EnemyType_Spinner
};

enum EnemyFlag
{
	EnemyFlag_Hunted = 1 << 0
};

class Enemy : public Entity
{
public:
	Enemy(xHandle handle);
	~Enemy();

	void Make_Spinner();

	void Update(float dt);
	void Render();

	xHandle Handle_get() const;
	bool IsDead_get() const;
	Vec2F Position_get() const;

	void Flag_set(EnemyFlag flag);
	bool Flag_get(EnemyFlag flag) const;

	// entity implementation
	virtual EntityType EntityType_get();
	virtual void Kill();
	virtual void Damage(float amount);
	virtual bool CollidesWith(const Vec2F& position);

private:
	EnemyType mEnemyType;
	xHandle mHandle;
	bool mIsDead;
	Vec2F mPosition;
	float mHealth;
	int mFlags;

	union
	{
		struct
		{
			float angle;
			float radius;
			float speed[2];
		} mSpinner;
	};
};
