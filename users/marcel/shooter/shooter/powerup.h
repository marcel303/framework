#pragma once

#include "handle.h"
#include "types2.h"

enum PowerupType
{
	PowerupType_Undefined = -1,
	PowerupType_Red,
	PowerupType_Missiles,
	PowerupType_Swarm,
	PowerupType_Orange,
	PowerupType_Shockwave,
	PowerupType_Wave,
	PowerupType__Count
};

class Powerup
{
public:
	Powerup(xHandle handle);
	~Powerup();

	void Make(PowerupType type);

	void Update(float dt);
	void Render();

	xHandle Handle_get() const;
	bool IsDead_get() const;
	const Vec2F& Position_get() const;
	PowerupType PowerupType_get() const;

private:
	xHandle mHandle;
	bool mIsDead;
	PowerupType mPowerupType;
	Vec2F mPosition;
	Vec2F mSpeed;
	float mAngle;
	Color* mColor;
};
