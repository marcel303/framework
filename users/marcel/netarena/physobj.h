#pragma once

#include "Vec2.h"

#define MAX_ACTOR_TYPES 32
#define MAX_PHYSICS_ACTORS 32 // todo : correct number = ?

struct BlockAndDistance;
struct CollisionInfo;
class GameSim;
struct PhysicsActor;
struct PhysicsActorCBs;
struct Player;
class SerializationContext;

struct PhysicsActorCBs
{
	PhysicsActorCBs()
	{
		memset(this, 0, sizeof(PhysicsActorCBs));
	}

	bool (*onBlockMask)(PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask);
	//bool (*onHitActor)(PhysicsActorCBs & cbs, PhysicsActor & actor, PhysicsActor & otherActor); // todo : check actors vs actor collisions once PhysicsScene is set up
	void (*onBounce)(PhysicsActorCBs & cbs, PhysicsActor & actor);
	void (*onWrap)(PhysicsActorCBs & cbs, PhysicsActor & actor);
	void (*onMove)(PhysicsActorCBs & cbs, PhysicsActor & actor);
};

//extern PhysicsActorCBs * g_physicsActorCBs[MAX_ACTOR_TYPES]; // todo : PhysicsScene and the object contained therein cannot contain pointers due to sync across the net. use type IDs indexing into this array to get to the CBs

// todo : add support for attaching to movers

#pragma pack(push)
#pragma pack(1)

struct PhysicsActor
{
	bool m_isActive;
	bool m_isTested; // true if this actor has been tested. since we need to test everything multiple times due to screen wrapping we must avoid testing objects positive multiple times
	uint8_t m_type; // used to index into g_physicsActorCBs

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

	bool test(const CollisionBox & box) const;
};

typedef uint8_t PhysicsActorId;

#define kPhysicsActorId_Invalid ((PhysicsActorId)-1)

typedef void (*CollisionCB)(void * arg, PhysicsActor * actor, BlockAndDistance * block, Player * player);

struct PhysicsScene
{
	PhysicsActor m_actors[MAX_PHYSICS_ACTORS];

	//

	PhysicsScene();

	void serialize(NetSerializationContext & ctx);

	PhysicsActorId allocActor();
	void freeActor(PhysicsActorId id);

	// test the given box against all of the registered physics actors and invoke the callback method for each actor that is hit
	void test(const CollisionBox & box, CollisionCB cb, void * arg, bool wrap);
	void testInternal(const CollisionBox & box, CollisionCB cb, void * arg);
};

#pragma pack(pop)
