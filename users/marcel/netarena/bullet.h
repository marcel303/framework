#pragma once

#include <stdint.h>
#include "framework.h"
#include "gametypes.h"
#include "NetSerializable.h"
#include "physobj.h"
#include "Vec2.h"

#define BULLET_IS_PHYSOBJ 1

#define MAX_BULLETS 1000
#define INVALID_BULLET_ID 0xffff

class GameSim;

class Bullet : public PhysicsActor
{
public:
	Bullet();

	void setVel(float angle, float velocity);
	float calcAge() const;

	bool isAlive;

#if !BULLET_IS_PHYSOBJ
	Vec2 pos;
	Vec2 vel;
#endif
	BulletType type;
	BulletEffect effect;
	bool noCollide;
	uint32_t color;

#if !BULLET_IS_PHYSOBJ
	bool doGravity;
#endif
	bool doBounce;
	bool noDamagePlayer;
	bool noDamageMap;
	bool doDamageOwner;

	bool doAgeAlpha;

	uint8_t reflectCount;
	uint8_t maxReflectCount;

	uint8_t wrapCount;
	uint8_t maxWrapCount;

	float distanceTravelled;
	float maxDistanceTravelled;

	int blocksDestroyed;
	int maxDestroyedBlocks;

	float bounceAmount;
	int bounceCount;

	float gravityModifier;

	float life;

	uint32_t ownerPlayerId;
};

#pragma pack(push)
#pragma pack(1)

struct ParticleSpawnInfo
{
	ParticleSpawnInfo()
		: type(kBulletType_ParticleA)
		, count(0)
		, x(0)
		, y(0)
		, minVelocity(100)
		, maxVelocity(100)
		, maxDistance(100)
		, color(0xffffffff)
		, blend(BLEND_ADD)
	{
	}

	ParticleSpawnInfo(int16_t _x, int16_t _y, BulletType _type, uint8_t _count, uint16_t _minVelocity, uint16_t _maxVelocity, uint16_t _maxDistance)
		: x(_x)
		, y(_y)
		, type(_type)
		, count(_count)
		, minVelocity(_minVelocity)
		, maxVelocity(_maxVelocity)
		, maxDistance(_maxDistance)
		, color(0xffffffff)
		, blend(BLEND_ADD)
	{
	}

	void serialize(NetSerializationContext & context)
	{
		context.SerializeBytes(this, sizeof(*this));
	}

	uint8_t type;
	uint8_t count;
	int16_t x;
	int16_t y;
	uint16_t minVelocity;
	uint16_t maxVelocity;
	uint16_t maxDistance;
	uint32_t color; // 0xrrggbbaa
	BLEND_MODE blend;
};

#pragma pack(pop)

class BulletPool
{
	uint16_t m_freeList[MAX_BULLETS];
	uint16_t m_numFree;
	bool m_localOnly;

public:
	Bullet m_bullets[MAX_BULLETS];

	BulletPool(bool localOnly);

	void tick(GameSim & gameSim, float dt);
	void anim(GameSim & gameSim, float dt);
	void anim(GameSim & gameSim, Bullet & b, float dt);
	void draw();
	void drawLight();

	uint16_t alloc();
	void free(uint16_t id);

	void serialize(NetSerializationContext & context);
};

void initBullet(GameSim & gameSim, Bullet & b, const ParticleSpawnInfo & spawnInfo);
