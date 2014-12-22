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

static const char * s_bulletSpriteFiles[kBulletType_COUNT] =
{
	"fire-type-0.png",
	"particle-0.png"
};

static Sprite * s_bulletSprites[kBulletType_COUNT] = { };

static void getVelocityXY(float angle, float velocity, float & x, float & y)
{
	x = +std::cos(angle) * velocity;
	y = -std::sin(angle) * velocity;
}

BulletPool::BulletPool(bool localOnly)
	: m_numFree(MAX_BULLETS)
	, m_localOnly(localOnly)
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
			bool kill = false;

			// evaluate collisions

			if (!b.noCollide)
			{
				const uint32_t blockMask = g_hostArena->getIntersectingBlocksMask(b.x, b.y);

				// reflection

				if (blockMask & kBlockMask_Solid & (~(1 << kBlockType_Destructible)))
				{
					if (b.maxReflectCount != 0)
					{
						b.reflectCount++;

						if (b.reflectCount > b.maxReflectCount)
							kill = true;
						else
						{
							b.angle = b.angle + Calc::mPI;
							g_app->netUpdateBullet(i);
						}
					}
					else
						kill = true;
				}
			}

			// wrap around

			if (b.wrapCount > b.maxWrapCount)
				kill = true;

			// max distance travelled

			if (b.maxDistanceTravelled != 0.f && b.distanceTravelled > b.maxDistanceTravelled)
				kill = true;

			Player * owner = 0;

			if (b.ownerNetId != 0)
			{
				for (auto p = g_host->m_players.begin(); p != g_host->m_players.end(); ++p)
				{
					Player * player = *p;

					if (player->getNetId() == b.ownerNetId)
						owner = player;
				}

				if (owner == 0)
					kill = true;
			}

			if (!kill && !b.noCollide)
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

						player->handleDamage(1.f, Vec2(vx, vy), owner);

						kill = true;
					}
				}
			}

			if (!kill && !b.noCollide)
			{
				// collide with map

				if (g_hostArena->handleDamageRect(
					b.x, b.y,
					b.x, b.y,
					b.x, b.y,
					true))
				{
					kill = true;
				}
			}

			if (kill)
			{
				if (!m_localOnly)
				{
					g_app->netKillBullet(i);

					g_app->netSpawnParticles(b.x, b.y, kBulletType_ParticleA, 16, 100, 50);
				}

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

			// distance travelled

			b.distanceTravelled += std::sqrt(dx * dx + dy * dy);

			// wrap

			float x = b.x;
			float y = b.y;

			if (b.x < 0.f)
				b.x += ARENA_SX_PIXELS;
			if (b.x > ARENA_SX_PIXELS)
				b.x -= ARENA_SX_PIXELS;

			if (b.y < 0.f)
				b.y += ARENA_SY_PIXELS;
			if (b.y > ARENA_SY_PIXELS)
				b.y -= ARENA_SY_PIXELS;

			if (x != b.x || y != b.y)
			{
				b.wrapCount++;

				if (b.wrapCount > b.maxWrapCount)
				{
					b.x = x;
					b.y = y;
				}
			}
		}
	}
}

void BulletPool::draw()
{
	for (int i = 0; i < kBulletType_COUNT; ++i)
	{
		if (!s_bulletSprites[i])
		{
			s_bulletSprites[i] = new Sprite(s_bulletSpriteFiles[i]);
		}
	}

	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		const Bullet & b = m_bullets[i];

		if (b.isAlive)
		{
			float vx, vy;
			getVelocityXY(b.angle, b.velocity, vx, vy);

			setColor(255, 255, 255);
			s_bulletSprites[b.type]->drawEx(b.x, b.y, Calc::RadToDeg(b.angle), 2.f);
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
