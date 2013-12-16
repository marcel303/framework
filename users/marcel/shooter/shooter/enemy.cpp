#include <math.h>
#include "enemy.h"
#include "game.h"
#include "player.h"
#include "render.h"
#include "sounds.h"
#include "world.h"

#define SPINNER_SPEED 80.0f
#define SPINNER_ANGLE_SPEED 1.0f

Enemy::Enemy(xHandle handle)
{
	mHandle = handle;
	mIsDead = false;
	mHealth = 0.0f;
	mFlags = 0;
}

Enemy::~Enemy()
{
	gWorld->mPlayer->HandleKill();
	SoundPlay(Sound_Kill1);
}

void Enemy::Make_Spinner()
{
	mEnemyType = EnemyType_Spinner;
	mPosition.x = Random(0.0f, gWorld->mSx);
	mPosition.y = Random(0.0f, gWorld->mSy);
	mHealth = 10.0f;
	mSpinner.angle = Random(0.0f, (float)M_PI * 2.0f);
	mSpinner.radius = Random(10.0f, 20.0f);
	mSpinner.speed[0] = Random(-SPINNER_SPEED, +SPINNER_SPEED);
	mSpinner.speed[1] = Random(-SPINNER_SPEED, +SPINNER_SPEED);
}

void Enemy::Update(float dt)
{
	switch (mEnemyType)
	{
	case EnemyType_Spinner:
		{
			for (int i = 0; i < 2; ++i)
			{
				Vec2F temp = mPosition;
				temp[i] += mSpinner.speed[i] * dt;
				if (gWorld->IsOutside(temp))
					mSpinner.speed[i] *= -1.0f;
				else
					mPosition = temp;
			}
			mSpinner.angle += SPINNER_ANGLE_SPEED * dt;
			break;
		}
	}

	if (gWorld->mPlayer->CollidesWith(mPosition))
	{
		gWorld->mPlayer->Damage(150.0f * dt);
	}
}

void Enemy::Render()
{
	switch (mEnemyType)
	{
	case EnemyType_Spinner:
		{
			gRender->Quad(
				mPosition.x - mSpinner.radius,
				mPosition.y - mSpinner.radius,
				mPosition.x + mSpinner.radius,
				mPosition.y + mSpinner.radius,
				Color(1.0f, 0.0f, 1.0f));
			break;
		}
	}
}

xHandle Enemy::Handle_get() const
{
	return mHandle;
}

bool Enemy::IsDead_get() const
{
	return mIsDead;
}

Vec2F Enemy::Position_get() const
{
	return mPosition;
}

EntityType Enemy::EntityType_get()
{
	return EntityType_Enemy;
}

void Enemy::Flag_set(EnemyFlag flag)
{
	mFlags |= flag;
}

bool Enemy::Flag_get(EnemyFlag flag) const
{
	return (mFlags & flag) != 0;
}

void Enemy::Kill()
{
	mIsDead = true;
}

void Enemy::Damage(float amount)
{
	mHealth -= amount;

	if (mHealth <= 0.0f)
		mIsDead = true;
}

bool Enemy::CollidesWith(const Vec2F& position)
{
	return mPosition.DistanceTo(position) <= 10.0f;
}
