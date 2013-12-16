#include <math.h>
#include "player.h"
#include "powerup.h"
#include "render.h"
#include "types2.h"
#include "world.h"

#define SPEED 10.0f
#define ANGLE_SPEED (0.1f * (float)M_PI * 2.0f)

Powerup::Powerup(xHandle handle)
{
	mHandle = handle;
	mIsDead = false;
	mColor = 0;
	mAngle = 0.0f;
}

Powerup::~Powerup()
{
	delete mColor;
}

void Powerup::Make(PowerupType type)
{
	mPowerupType = type;

	mPosition.x = Random(0.0f, gWorld->mSx);
	mPosition.y = Random(0.0f, gWorld->mSy);

	mSpeed.x = Random(-SPEED, +SPEED);
	mSpeed.y = Random(-SPEED, +SPEED);

	switch (type)
	{
	case PowerupType_Missiles:
		mColor = new Color(1.0f, 1.0f, 0.0f);
		break;
	case PowerupType_Orange:
		mColor = new Color(1.0f, 0.5f, 0.0f);
		break;
	case PowerupType_Red:
		mColor = new Color(1.0f, 0.0f, 0.0f);
		break;
	case PowerupType_Shockwave:
		mColor = new Color(0.0f, 0.0f, 1.0f);
		break;
	case PowerupType_Swarm:
		mColor = new Color(0.0f, 1.0f, 1.0f);
		break;
	case PowerupType_Wave:
		mColor = new Color(0.9f, 0.9f, 0.9f);
		break;
	}
}

void Powerup::Update(float dt)
{
	for (int i = 0; i < 2; ++i)
	{
		Vec2F temp = mPosition;

		temp[i] += mSpeed[i] * dt;

		if (gWorld->IsOutside(temp))
			mSpeed[i] *= -1.0f;
		else
			mPosition = temp;
	}

	mAngle += ANGLE_SPEED * dt;

	if (gWorld->Player_get()->CollidesWith(mPosition))
	{
		gWorld->Player_get()->HandlePowerup(this);

		mIsDead = true;
	}
}

void Powerup::Render()
{
	gRender->Circle(mPosition.x, mPosition.y, 3.0f, Color(0.9f, 0.9f, 1.0f));

	float angle1 = mAngle;
	float angle2 = mAngle - (float)M_PI * 2.0f / 4.0f * 3.0f;

	gRender->Arc(mPosition.x, mPosition.y, angle1, angle2, 6.0f, *mColor);
}

xHandle Powerup::Handle_get() const
{
	return mHandle;
}

bool Powerup::IsDead_get() const
{
	return mIsDead;
}

const Vec2F& Powerup::Position_get() const
{
	return mPosition;
}

PowerupType Powerup::PowerupType_get() const
{
	return mPowerupType;
}
