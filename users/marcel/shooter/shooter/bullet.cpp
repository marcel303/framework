#include <math.h>
#include "bullet.h"
#include "enemy.h"
#include "render.h"
#include "sounds.h"
#include "world.h"

#define MISSILE_SPEED 200.0f
#define MISSILE_HUNT_TIME 3.0f
#define RED_SPEED 400.0f
#define RED_LIFE 1.5f
#define ORANGE_SPEED 300.0f
#define ORANGE_LIFE 2.5f
#define SWARM_SPEED 70.f
#define SWARM_ANGLE_SPEED ((float)M_PI / 3.0f)
#define SWARM_HOPS 4
#define SWARM_LIFE 3.0f
#define SWARM_HOP_TIME 1.5f
#define INTERSECTOR_SPEED 250.0f
#define INTERSECTOR_LIFE 3.0f

class MissileTargetState
{
public:
	MissileTargetState(Bullet* _self)
	{
		self = _self;
		target = 0;
		score = 1000000.0f;
	}

	Bullet* self;
	Enemy* target;
	float score;
};

Bullet::Bullet(xHandle handle)
{
	mHandle = handle;
	mIsDead = false;
	mDamage = 0.0f;
	mColor = 0;
}

Bullet::~Bullet()
{
	delete mColor;
}

void Bullet::Make_Red(float x, float y, float angle)
{
	mBulletType = BulletType_Red;
	mPosition.x = x;
	mPosition.y = y;
	//mDamage = 1.0f;
	mDamage = 6.0f;
	Vec2F direction = Vec2F::FromAngle(angle);
	mRed.speed[0] = direction.x * RED_SPEED;
	mRed.speed[1] = direction.y * RED_SPEED;
	mRed.life = RED_LIFE;
	mColor = new Color(1.0f, 0.0f, 0.0f);
}

void Bullet::Make_Missile(float x, float y, int target)
{
	mBulletType = BulletType_Missile;
	mPosition.x = x;
	mPosition.y = y;
	mDamage = 10.0f;
	mMissile.target = target;
	mMissile.isHunting = target != 0;
	mMissile.angle = Random(0.0f, (float)M_PI * 2.0f);
	mMissile.huntTime = MISSILE_HUNT_TIME;
	mColor = new Color(1.0f, 1.0f, 0.0f);

	if (!mMissile.target)
		mMissile.isHunting = SelectMissileTarget();
}

void Bullet::Make_Swarm(float x, float y, float angle)
{
	SoundPlay(Sound_Swarm2);

	//float angleSpeed = (rand() % 2) == 0 ? -SWARM_ANGLE_SPEED : +SWARM_ANGLE_SPEED;
	float angleSpeed = SWARM_ANGLE_SPEED;

	Make_Swarm(x, y, angle, angleSpeed, SWARM_HOPS);
}

void Bullet::Make_Swarm(float x, float y, float angle, float angleSpeed, int hopCount)
{
	mBulletType = BulletType_Swarm;
	mPosition.x = x;
	mPosition.y = y;
	mDamage = 100.0f;
	mSwarm.angle = angle;
	mSwarm.angleSpeed = angleSpeed;
	mSwarm.hasHopped = false;
	mSwarm.hopCount = hopCount;
	mSwarm.life = SWARM_LIFE;
	mColor = new Color(0.0f, 0.5f, 1.0f);
	SoundPlay(Sound_Swarm2);
}

void Bullet::Make_Orange(float x, float y)
{
	mBulletType = BulletType_Orange;
	mPosition.x = x;
	mPosition.y = y;
	mDamage = 10.0f;
	float angle = Random(0.0f, (float)M_PI * 2.0f);
	Vec2F direction = Vec2F::FromAngle(angle);
	mOrange.speed[0] = direction.x * ORANGE_SPEED;
	mOrange.speed[1] = direction.y * ORANGE_SPEED;
	mOrange.life = ORANGE_LIFE;
	mColor = new Color(1.0f, 0.5f, 0.0f);
}

void Bullet::Make_Intersector(float x, float y, float angle)
{
	mBulletType = BulletType_Intersector;
	mPosition.x = x;
	mPosition.y = y;
	mDamage = 20.0f;
	Vec2F direction = Vec2F::FromAngle(angle);
	mIntersector.speed[0] = direction.x * INTERSECTOR_SPEED;
	mIntersector.speed[1] = direction.y * INTERSECTOR_SPEED;
	mIntersector.life = INTERSECTOR_LIFE;
	mColor = new Color(0.9f, 0.9f, 0.9f);
	SoundPlay(Sound_Bubble1);
}

void Bullet::Update(float dt)
{
	switch (mBulletType)
	{
	case BulletType_Missile:
		{
			if (mMissile.isHunting)
			{
				const GoInfo& goInfo = gWorld->GoInfo_get(mMissile.target);

				if (goInfo.isDead)
				{
					if (!SelectMissileTarget())
						mMissile.isHunting = false;
				}
				else
				{
					Vec2F delta = mPosition.DirectionTo(goInfo.position);
					Vec2F direction = Vec2F::FromAngle(mMissile.angle);
					float weight = 10.0f * dt;
					direction.x += delta.x * weight;
					direction.y += delta.y * weight;
					mMissile.angle = direction.ToAngle();
				}

				mMissile.huntTime -= dt;
				if (mMissile.huntTime < 0.0f)
					mMissile.isHunting = false;
			}
			Vec2F direction = Vec2F::FromAngle(mMissile.angle);
			mPosition.x += direction.x * MISSILE_SPEED * dt;
			mPosition.y += direction.y * MISSILE_SPEED * dt;
			if (gWorld->IsOutside(mPosition))
				mIsDead = true;
			break;
		}
	case BulletType_Red:
		{
			mPosition.x += mRed.speed[0] * dt;
			mPosition.y += mRed.speed[1] * dt;
			mRed.life -= dt;
			if (mRed.life < 0.0f)
				mIsDead = true;
			if (gWorld->IsOutside(mPosition))
				mIsDead = true;
			break;
		}
	case BulletType_Swarm:
		{
			if (mSwarm.hopCount > 0 && mSwarm.life < SWARM_HOP_TIME && !mSwarm.hasHopped)
			{
				mSwarm.hasHopped = true;
				Bullet* bullet = gWorld->AllocateBullet();
				bullet->Make_Swarm(mPosition.x, mPosition.y, mSwarm.angle, -mSwarm.angleSpeed, mSwarm.hopCount - 1);
			}
			mSwarm.angle += mSwarm.angleSpeed * dt;
			Vec2F direction = Vec2F::FromAngle(mSwarm.angle);
			mPosition.x += direction.x * SWARM_SPEED * dt;
			mPosition.y += direction.y * SWARM_SPEED * dt;
			mSwarm.life -= dt;
			if (mSwarm.life < 0.0f)
				mIsDead = true;
			if (gWorld->IsOutside(mPosition))
				mIsDead = true;
			mColor->a = mSwarm.life / SWARM_LIFE;
			break;
		}
	case BulletType_Orange:
		{
			mPosition.x += mRed.speed[0] * dt;
			mPosition.y += mRed.speed[1] * dt;
			mRed.life -= dt;
			if (mRed.life < 0.0f)
				mIsDead = true;
			if (gWorld->IsOutside(mPosition))
				mIsDead = true;
			break;
		}
	case BulletType_Intersector:
		{
			mPosition.x += mIntersector.speed[0] * dt;
			mPosition.y += mIntersector.speed[1] * dt;
			mIntersector.life -= dt;
			if (mIntersector.life < 0.0f)
				mIsDead = true;
			if (gWorld->IsOutside(mPosition))
				mIsDead = true;
			break;
		}
	}

	gWorld->ForEachEnemy(this, ForEachHandler_Damage);
}

void Bullet::Render()
{
	gRender->Circle(mPosition.x, mPosition.y, 3.0f, *mColor);
}

xHandle Bullet::Handle_get() const
{
	return mHandle;
}

bool Bullet::IsDead_get() const
{
	return mIsDead;
}

const Vec2F& Bullet::Position_get() const
{
	return mPosition;
}

bool Bullet::SelectMissileTarget()
{
	MissileTargetState state(this);

	gWorld->ForEachEnemy(&state, ForEachHandler_MissileTarget);

	if (state.target)
	{
		mMissile.target = state.target->Handle_get();
		state.target->Flag_set(EnemyFlag_Hunted);

		return true;
	}
	else
	{
		return false;
	}
}

void Bullet::ForEachHandler_Damage(void* obj, Enemy* enemy)
{
	Bullet* self = (Bullet*)obj;

	if (!enemy->CollidesWith(self->mPosition))
		return;

	switch (self->mBulletType)
	{
	case BulletType_Intersector:
		enemy->Damage(self->mDamage);
		break;
	case BulletType_Missile:
		enemy->Damage(self->mDamage);
		self->mIsDead = true;
		break;
	case BulletType_Orange:
		enemy->Damage(self->mDamage);
		self->mIsDead = true;
		break;
	case BulletType_Red:
		enemy->Damage(self->mDamage);
		self->mIsDead = true;
		break;
	case BulletType_Swarm:
		enemy->Damage(self->mDamage);
		break;
	}
}

void Bullet::ForEachHandler_MissileTarget(void* obj, Enemy* enemy)
{
	MissileTargetState* state = (MissileTargetState*)obj;

	const float distance = state->self->mPosition.DistanceTo(enemy->Position_get());
	const float hunted = enemy->Flag_get(EnemyFlag_Hunted) ? 200.0f : 0.0f;

	const float score = distance + hunted;

	if (score < state->score)
	{
		state->target = enemy;
		state->score = score;
	}
}
