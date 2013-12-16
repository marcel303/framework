#pragma once

#include <map>
#include "box_forward.h"
#include "deck_forward.h"
#include "Types.h"

class WorldObjectDesc
{
public:
	Vec2F position;
	float angle;
};

class World
{
public:
	World();
	~World();
	
	int Add(Shape* shape, Vec2F position, float angle);
	void Remove(int objectId);
	
	void SetPosition(int objectId, Vec2F position);
	void ApplyForce(int objectId, Vec2F position, Vec2F force, bool inBody);
	void SetLinearVelocity(int objectId, Vec2F speed);
	
	void Update(float dt);
	
	void Describe(int objectId, Vec2F& out_Position, float& out_Angle);
	WorldObjectDesc Describe(int objectId);
	
	int TestPoint(Vec2F position, Vec2F& out_PointInObject);
	Vec2F ObjectToWorld(int objectId, Vec2F position);
	
private:
	b2World* mWorld;
	int mCurrentId;
	std::map<int, b2Body*> mBodiesById;
};

extern World* gWorld;
