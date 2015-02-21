#pragma once

#include "Vec2.h"

struct CollisionInfo;
class GameSim;
struct PhysicsActor;
struct PhysicsActorCBs;

struct PhysicsActorCBs
{
	PhysicsActorCBs()
	{
		memset(this, 0, sizeof(*this));
	}

	bool (*onBlockMask)(PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask);
	void (*onBounce)(PhysicsActorCBs & cbs, PhysicsActor & actor);
	void (*onWrap)(PhysicsActorCBs & cbs, PhysicsActor & actor);
	void (*onMove)(PhysicsActorCBs & cbs, PhysicsActor & actor);
};

struct PhysicsActor
{
	Vec2 m_pos;
	Vec2 m_vel;

	Vec2 m_bbMin;
	Vec2 m_bbMax;

	bool m_noGravity;
	bool m_doTeleport;
	float m_bounciness;
	float m_friction;
	float m_airFriction;

	PhysicsActor();

	void tick(GameSim & gameSim, float dt, PhysicsActorCBs & cbs);
	void drawBB() const;

	uint32_t getIntersectingBlockMask(GameSim & gameSim, Vec2 pos);
	void getCollisionInfo(CollisionInfo & collisionInfo);
};
