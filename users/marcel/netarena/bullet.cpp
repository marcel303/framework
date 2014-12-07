#pragma once

#include <cmath>
#include <string.h>
#include "bullet.h"
#include "Debugging.h"
#include "framework.h"
#include "main.h"

#include <stdlib.h> // fixme

BulletPool::BulletPool()
	: m_numFree(MAX_BULLETS)
{
	memset(m_bullets, 0, sizeof(m_bullets));

	for (int i = 0; i < MAX_BULLETS; ++i)
		m_freeList[i] = i;
}

void BulletPool::tick(float dt)
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		Bullet & b = m_bullets[i];

		if (b.isAlive)
		{
			anim(dt);

			// todo : evaluate collisions etc

			if ((rand() % 100) == 0) // fixme
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
			b.x += +std::cos(b.angle) * b.velocity * dt;
			b.y += -std::sin(b.angle) * b.velocity * dt;
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
			setColor(255, 0, 0);
			drawRect(
				b.x - 5,
				b.y - 5,
				b.x + 5,
				b.y + 5);
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
	if (m_bullets[id].isAlive)
	{
		m_bullets[id].isAlive = false;
		m_freeList[m_numFree++] = id;
	}
}
