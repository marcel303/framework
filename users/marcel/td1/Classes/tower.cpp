#include "enemy.h"
#include "enemy_info.h"
#include "game.h"
#include "level.h"
#include "particle.h"
#include "Render.h"
#include "sounds.h"
#include "SpriteGfx.h"
#include "tower.h"

#define VULCAN_INTERVAL 0.2f
#define ROCKET_INTERVAL 15.0f

static float gVulcanCost[5] = { 15.0f, 25.0f, 50.0f, 100.0f, 250.0f };
static float gVulcanStrength[5] = { 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };
static float gVulcanRadius[5] = { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };

static float gSlowCost[5] = { 25.0f, 50.0f, 100.0f, 250.0f };
static float gSlowRadius[5] = { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };

static float gRocketCost[5] = { 40.0f, 75.0f, 125.0f, 250.0f, 350.0f };
static float gRocketRadius[5] = { 40.0f, 60.0f, 80.0f, 100.0f, 120.0f };
static float gRocketAngleSpeed[5] = { 2.0f, 2.0f, 2.0f, 2.0f, 2.0f };
static float gRocketSpeed[5] = { 70.0f, 70.0f, 70.0f, 70.0f, 70.0f };
static float gRocketLife[5] = { 3.0f, 3.0f, 3.0f, 3.0f, 3.0f };

Tower::Tower()
{
	mType = TowerType_Undefined;
	mRadius = 0.0f;
	vulcan.angle = 0.0f;
	vulcan.fireTimer = 0.0f;
	rocket.angle = 0.0f;
	rocket.fireTimer = 0.0f;
	rocket.rocketIsActive = false;
	rocket.rocketAngle.Setup(0.0f, 0.0f, 0.0f);
	rocket.rocketLife = 0.0f;
	rocket.rocketTarget = -1;
	rocket.rocketTargetIsAlive = false;
}

float Tower::GetCost(TowerType type, int level)
{
	switch (type) 
	{
		case TowerType_Rocket:
			return gRocketCost[level];
		case TowerType_Slow:
			return gSlowCost[level];
		case TowerType_Vulcan:
			return gVulcanCost[level];
		default:
			throw ExceptionNA();
	}
}

void Tower::Make(TowerDesc desc)
{
	mType = desc.type;
	mPosition = desc.position;
	mRadius = 15.0f;
	mLevel = desc.level;
	
	Recompute();
}

void Tower::Update(float dt)
{
	switch (mType)
	{
		case TowerType_Rocket:
			Update_Rocket(dt);
			break;
		case TowerType_Slow:
			Update_Slow(dt);
			break;
		case TowerType_Vulcan:
			Update_Vulcan(dt);
			break;
	}
}

void Tower::Render_Visibility()
{
	bool selected = gGame->SelectedTower_get() == this;
	
	if (selected)
		RenderRadius(mVisibility, 0.5f);
}

void Tower::Render()
{
	switch (mType)
	{
		case TowerType_Rocket:
			Render_Rocket();
			break;
		case TowerType_Slow:
			Render_Slow();
			break;
		case TowerType_Vulcan:
			Render_Vulcan();
			break;
	}
}

TowerType Tower::TowerType_get() const
{
	return mType;
}

Vec2F Tower::Position_get() const
{
	return mPosition;
}

float Tower::Radius_get() const
{
	return mRadius;
}

int Tower::Level_get() const
{
	return mLevel;
}

void Tower::Upgrade()
{
	Assert(mLevel < 4);
	
	if (mLevel == 4)
		return;
	
	mLevel++;
	
	Recompute();
}

bool Tower::HitTest(Vec2F location)
{
	return location.DistanceSq(mPosition) <= mRadius * mRadius;
}

void Tower::Update_Vulcan(float dt)
{
	// pick target
	
	Enemy* enemy = gGame->Level_get()->FindEnemyInRadius(mPosition, mVisibility);
	
	// target enemy
	
	if (enemy)
	{
		Vec2F p1 = mPosition;
		Vec2F p2 = enemy->Position_get();
		
		vulcan.angle = (p2 - p1).ToAngle();
	}
	
	// fire
	
	if (enemy)
	{
		if (vulcan.fireTimer <= 0.0f)
		{
			vulcan.fireTimer = VULCAN_INTERVAL;
			
			LOG_DBG("vulcan: fire", 0);
			
			enemy->HandleDamage(gVulcanStrength[mLevel]);
			
			for (int i = 0; i < 5; ++i)
			{
				Particle* p = gParticleMgr->Allocate();
				
				p->Setup(enemy->Position_get(), Vec2F::FromAngle(Calc::Random(Calc::m2PI)) * 50.0f, 0.5f, 3.0f, 1.0f, SpriteColor_Make(255, 255, 0, 255));
			}
			
			SoundPlay(Sound_Vulcan_Fire);
		}
	}
	
	vulcan.fireTimer -= dt;
}

void Tower::Update_Rocket(float dt)
{
	while (rocket.fireTimer <= 0.0f)
	{
		// pick target
		
		Enemy* enemy = gGame->Level_get()->FindEnemyInRadius(mPosition, mVisibility);
		
		if (!enemy)
			break;
		
		LOG_DBG("rocket: fire", 0);
		
		rocket.fireTimer += ROCKET_INTERVAL;
		
		float angle = (enemy->Position_get() - mPosition).ToAngle();
		
		rocket.rocketIsActive = true;
		rocket.rocketPosition = mPosition;
		rocket.rocketAngle.Setup(angle, angle, gRocketAngleSpeed[mLevel]);
		rocket.rocketLife = gRocketLife[mLevel];
		rocket.rocketTarget = enemy->Id_get();
		rocket.rocketTargetIsAlive = true;
		SoundPlay(Sound_Missile_Fire);
	}
	
	rocket.fireTimer -= dt;
	
	if (rocket.rocketIsActive)
	{
		if (!gEnemyInfoMgr->EnemyInfo_get(rocket.rocketTarget)->isAlive)
			rocket.rocketTargetIsAlive = false;
		if (rocket.rocketTargetIsAlive)
		{
			float targetAngle = (gEnemyInfoMgr->EnemyInfo_get(rocket.rocketTarget)->location - mPosition).ToAngle();
			rocket.rocketAngle.TargetAngle_set(targetAngle);
		}
		rocket.rocketAngle.Update(dt);
		float angle = rocket.rocketAngle.Angle_get();
		Vec2F speed = Vec2F::FromAngle(angle) * gRocketSpeed[mLevel];
		rocket.rocketPosition += speed * dt;
		rocket.rocketAngle.Update(dt);
		rocket.rocketLife -= dt;
		if (rocket.rocketLife <= 0.0f)
			rocket.rocketIsActive = false;
	}
}

static void ApplySlow(void* obj, Enemy* enemy)
{
	enemy->HandleSlow();
}

void Tower::Update_Slow(float dt)
{
	gGame->Level_get()->ForEachEnemyInRadius(mPosition, mVisibility, ApplySlow, 0);
}

void Tower::Render_Rocket()
{
	gRender->Circle(mPosition[0], mPosition[1], mRadius, Color(1.0f, 0.0f, 0.0f));
	
	if (rocket.rocketIsActive)
	{
		gRender->Circle(rocket.rocketPosition[0], rocket.rocketPosition[1], 2.0f, Color(1.0f, 0.0f, 0.0f));
	}
}

void Tower::Render_Slow()
{
	gRender->Circle(mPosition[0], mPosition[1], mRadius, Color(1.0f, 1.0f, 0.0f));
}

void Tower::Render_Vulcan()
{
	gRender->Circle(mPosition[0], mPosition[1], mRadius, Color(0.0f, 1.0f, 0.0f));
	
	Vec2F p1 = mPosition;
	Vec2F p2 = p1 + Vec2F::FromAngle(vulcan.angle) * 10.0f;
	
	gRender->Line(p1[0], p1[1], p2[0], p2[1], Color(1.0f, 1.0f, 1.0f));
}

void Tower::RenderRadius(float radius, float alpha)
{
	gRender->Circle(mPosition[0], mPosition[1], radius, Color(0.3f, 0.3f, 1.0f, alpha));
}

void Tower::Recompute()
{
	Assert(mLevel >= 0 && mLevel <= 4);
	
	switch (mType)
	{
		case TowerType_Rocket:
			mVisibility = gRocketRadius[mLevel];
			break;
		case TowerType_Slow:
			mVisibility = gSlowRadius[mLevel];
			break;
		case TowerType_Vulcan:
			mVisibility = gVulcanRadius[mLevel];
			break;
		default:
			throw ExceptionNA();
	}
	
	mVisibility += mRadius;
}
