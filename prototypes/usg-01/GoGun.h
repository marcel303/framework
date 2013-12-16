#pragma once

#include "GoSun.h"
#include "IObject.h"
#include "PowerSink.h"
#include "Types.h"

class BlackHole;

class GunBullet : public IObject
{
public:
	GunBullet(Map* map);

	void Setup(Vec2F pos, Vec2F dir);

	virtual void Update(Map* map);
	virtual void Render(BITMAP* buffer);

	Vec2F Pos;
	Vec2F Dir;
};

class Gun : public IObject // Gun / main weapon against boss.
{
public:
	Gun(Map* map, Sun* sun);

	void Initialize(Sun* sun, float distance, float angle);

	virtual void Update(Map* map);
	virtual void Render(BITMAP* buffer);

	Vec2F HeliosPos();
	void Pull(Vec2F pos);

	void BlackHoleFeed_Begin(BlackHole* blackHole);
	void BlackHoleFeed_End();

	Sun* m_Sun;
	float m_Angle;    // Movement is helios based. Distance = distance from sun. Angle is rotation around sun.
	float m_Distance;

	//

	PowerSink m_PowerSink;
	PowerSink m_PowerSinkBlackHole;
	PowerSink m_PowerSinkFire;

	//

	BlackHole* m_CurrentBlackHole;
};
