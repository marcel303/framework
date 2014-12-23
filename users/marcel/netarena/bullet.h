#pragma once

#include <stdint.h>

#define MAX_BULLETS 1000
#define INVALID_BULLET_ID 0xffff

enum BulletType
{
	kBulletType_A,
	kBulletType_ParticleA,
	kBulletType_COUNT
};

class Bullet
{
public:
	bool isAlive;

	float x;
	float y;
	float angle;
	float velocity;
	BulletType type;
	bool noCollide;

	float lastX;
	float lastY;

	uint8_t reflectCount;
	uint8_t maxReflectCount;

	uint8_t wrapCount;
	uint8_t maxWrapCount;

	float distanceTravelled;
	float maxDistanceTravelled;

	uint32_t ownerNetId;
};

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
