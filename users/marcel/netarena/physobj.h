#pragma once

#include "gametypes.h"
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

	int axis;

	void * userData;

	bool (*onBlockMask)(PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask);
	bool (*onHitActor)(PhysicsActorCBs & cbs, PhysicsActor & actor, PhysicsActor & otherActor); // todo : check actors vs actor collisions once PhysicsScene is set up
	bool (*onHitPlayer)(PhysicsActorCBs & cbs, PhysicsActor & actor, Player & player); // fixme : use onHitActor at some point when player is converted
	void (*onBounce)(PhysicsActorCBs & cbs, PhysicsActor & actor);
	void (*onWrap)(PhysicsActorCBs & cbs, PhysicsActor & actor);
	void (*onMove)(PhysicsActorCBs & cbs, PhysicsActor & actor);
};

typedef void (*CollisionCB)(const CollisionShape & shape, void * arg, PhysicsActor * actor, BlockAndDistance * block, Player * player);

// PhysicsScene and the objects contained therein cannot contain pointers due to sync across the net. use type IDs indexing into this array to get to the CBs
//extern PhysicsActorCBs * g_physicsActorCBs[kObjectType_COUNT];

// todo : add support for attaching to movers

#pragma pack(push)
#pragma pack(1)

struct PhysicsActor
{
	bool m_isActive;
	bool m_isTested; // true if this actor has been tested. since we need to test everything multiple times due to screen wrapping we must avoid testing objects positive multiple times
	ObjectType m_type; // used to index into g_physicsActorCBs

	Vec2 m_pos;
	Vec2 m_vel;
	bool m_isGrounded;

	Vec2 m_bbMin;
	Vec2 m_bbMax;

	bool m_noGravity;
	float m_gravityMultiplier;
	bool m_doTeleport;
	float m_bounciness;
	float m_friction;
	float m_airFriction;

	PhysicsActor();

	void tick(GameSim & gameSim, float dt, PhysicsActorCBs & cbs);
	void drawBB() const;

	uint32_t getIntersectingBlockMask(GameSim & gameSim, Vec2 pos);
	void getCollisionInfo(CollisionInfo & collisionInfo);

	void testCollision(const CollisionShape & shape, void * arg, CollisionCB cb);

	//bool test(const CollisionBox & box) const;
};

typedef uint8_t PhysicsActorId;

#define kPhysicsActorId_Invalid ((PhysicsActorId)-1)

#if 0 // PhysicsScene
struct PhysicsScene
{
	PhysicsActor m_actors[MAX_PHYSICS_ACTORS];

	//

	PhysicsScene();

	void serialize(NetSerializationContext & ctx);

	PhysicsActorId allocActor();
	void freeActor(PhysicsActorId id);

	// test the given box against all of the registered physics actors and invoke the callback method for each actor that is hit
	void testCollision(const CollisionShape & shape, CollisionCB cb, void * arg, bool wrap);
	void testCollisionInternal(const CollisionShape & shape, CollisionCB cb, void * arg);
};
#endif

#pragma pack(pop)
