#pragma once

#include <stdint.h>
#include "Vec2.h"

#define MAX_BULLETS 1000
#define INVALID_BULLET_ID 0xffff

enum BulletType
{
	kBulletType_A,
	kBulletType_B,
	kBulletType_Grenade,
	kBulletType_GrenadeA,
	kBulletType_ParticleA,
	kBulletType_COUNT
};

class Bullet
{
public:
	void setVel(float angle, float velocity);

	bool isAlive;

	Vec2 pos;
	Vec2 vel;
	BulletType type;
	bool noCollide;
	uint32_t color;

	bool doGravity;
	bool doBounce;
	bool noDamagePlayer;
	bool noDamageMap;

	Vec2 lastPos;

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

	uint32_t ownerNetId;
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

	void tick(float dt);
	void anim(float dt);
	void anim(Bullet & b, float dt);
	void draw();

	uint16_t alloc();
	void free(uint16_t id);
};
