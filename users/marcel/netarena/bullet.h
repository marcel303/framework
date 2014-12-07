#pragma once

#include <stdint.h>

#define MAX_BULLETS 1000

enum BulletType
{
	kBulletType_A,
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
};

class BulletPool
{
	uint16_t m_nextAllocIndex;

public:
	Bullet m_bullets[MAX_BULLETS];

	BulletPool();

	void tick(float dt);
	void anim(float dt);
	void draw();

	uint16_t alloc();
	void free(uint16_t id);
};
