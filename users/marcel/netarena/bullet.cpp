#pragma once

#include <cmath>
#include <string.h>
#include "bullet.h"
#include "Debugging.h"
#include "framework.h"
#include "main.h"

#include <stdlib.h> // fixme

BulletPool::BulletPool()
	: m_nextAllocIndex(0)
{
	memset(m_bullets, 0, sizeof(m_bullets));
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
		Bullet & b = m_bullets[i];

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
	const uint16_t start = m_nextAllocIndex;

	do
	{
		const uint16_t index = m_nextAllocIndex;
		m_nextAllocIndex = (m_nextAllocIndex + 1) % MAX_BULLETS;
		
		if (!m_bullets[index].isAlive)
			return index;

	} while (m_nextAllocIndex != start);

	return -1;
}

void BulletPool::free(uint16_t id)
{
	Assert(id != -1 && m_bullets[id].isAlive);
	m_bullets[id].isAlive = false;
}
