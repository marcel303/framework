#pragma once

#include "AngleController.h"
#include "libgg_forward.h"
#include "Types.h"

enum TowerType
{
	TowerType_Undefined,
	TowerType_Rocket,
	TowerType_Slow,
	TowerType_Vulcan
};

class TowerDesc
{
public:
	TowerType type;
	Vec2F position;
	int level;
};

class Tower
{
public:
	Tower();
	
	static float GetCost(TowerType type, int level);
	
	void Make(TowerDesc desc);
	
	void Update(float dt);
	void Render_Visibility();
	void Render();
	
	TowerType TowerType_get() const;
	Vec2F Position_get() const;
	float Radius_get() const;
	int Level_get() const;
	
	void Upgrade();
	bool HitTest(Vec2F location);
	
private:
	void Update_Rocket(float dt);
	void Update_Slow(float dt);
	void Update_Vulcan(float dt);
	
	void Render_Rocket();
	void Render_Slow();
	void Render_Vulcan();
	
	void RenderRadius(float radius, float alpha);
	
	void Recompute();
	
	TowerType mType;
	Vec2F mPosition;
	float mRadius;
	float mVisibility;
	int mLevel;
	
	struct
	{
		float angle;
		float fireTimer;
	} vulcan;
	
	struct
	{
		float angle;
		float fireTimer;
		bool rocketIsActive;
		Vec2F rocketPosition;
		AngleController rocketAngle;
		float rocketLife;
		int rocketTarget;
		bool rocketTargetIsAlive;
	} rocket;
};
