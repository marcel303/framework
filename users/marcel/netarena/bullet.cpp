#pragma once

#include <cmath>
#include <string.h>
#include "arena.h"
#include "bullet.h"
#include "Calc.h"
#include "Debugging.h"
#include "framework.h"
#include "gamedefs.h"
#include "host.h"
#include "main.h"
#include "player.h"

#define WRAP_AROUND 1

static void getVelocityXY(float angle, float velocity, float & x, float & y)
{
	x = +std::cos(angle) * velocity;
	y = -std::sin(angle) * velocity;
}

BulletPool::BulletPool()
	: m_numFree(MAX_BULLETS)
{
	memset(m_bullets, 0, sizeof(m_bullets));

	for (int i = 0; i < MAX_BULLETS; ++i)
		m_freeList[i] = i;
}

void BulletPool::tick(float dt)
{
	anim(dt);

	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		Bullet & b = m_bullets[i];

		if (b.isAlive)
		{
			// todo : evaluate collisions etc

			bool kill = false;

			const uint32_t blockMask = g_hostArena->getIntersectingBlocksMask(b.x, b.y, b.x, b.y);

			if (blockMask & kBlockMask_Solid)
				kill = true;
		#if !WRAP_AROUND
			else if (b.x < 0.f || b.y < 0.f || b.x > ARENA_SX * BLOCK_SX || b.y > ARENA_SY * BLOCK_SY)
				kill = true;
		#endif
			else
			{
				// collide with players

				for (auto p = g_host->m_players.begin(); p != g_host->m_players.end(); ++p)
				{
					Player * player = *p;

					if (player->getNetId() == b.ownerNetId)
						continue;

					CollisionInfo collisionInfo;

					player->getPlayerCollision(collisionInfo);

					if (collisionInfo.intersects(b.x, b.y))
					{
						float vx, vy;
						getVelocityXY(b.angle, b.velocity, vx, vy);

						player->handleDamage(1.f, Vec2(vx, vy));

						kill = true;
					}
				}
			}

			if (kill)
			{
				g_app->netKillBullet(i);

				free(i);
			}
		}
	}
}

void BulletPool::anim(float dt)
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		Bullet & b = m_bullets[i];

		if (b.isAlive)
		{
			float dx, dy;
			getVelocityXY(b.angle, b.velocity * dt, dx, dy);

			b.x += dx;
			b.y += dy;

		#if WRAP_AROUND
			if (b.x < 0.f)
				b.x += ARENA_SX * BLOCK_SX;
			if (b.x > ARENA_SX * BLOCK_SX)
				b.x -= ARENA_SX * BLOCK_SX;

			if (b.y < 0.f)
				b.y += ARENA_SY * BLOCK_SY;
			if (b.y > ARENA_SY * BLOCK_SY)
				b.y -= ARENA_SY * BLOCK_SY;
		#endif
		}
	}
}

void BulletPool::draw()
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		const Bullet & b = m_bullets[i];

		if (b.isAlive)
		{
			float vx, vy;
			getVelocityXY(b.angle, b.velocity, vx, vy);

			setColor(255, 255, 255);
			Sprite("fire-type-0.png").drawEx(b.x, b.y, Calc::RadToDeg(b.angle), 2.f);
		}
	}
}

uint16_t BulletPool::alloc()
{
	if (m_numFree == 0)
		return INVALID_BULLET_ID;
	return m_freeList[--m_numFree];
}

void BulletPool::free(uint16_t id)
{
	Assert(id != INVALID_BULLET_ID && m_bullets[id].isAlive);
	if (id != INVALID_BULLET_ID && m_bullets[id].isAlive)
	{
		m_bullets[id].isAlive = false;
		m_freeList[m_numFree++] = id;
	}
}
