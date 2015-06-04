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
#include "Log.h"
#include "main.h"
#include "player.h"

#define WRAP_AROUND 1

static const char * s_bulletSpriteFiles[kBulletType_COUNT] =
{
	"fire-type-0.png",
	"fire-type-0.png",
	"particle-0.png",
	"particle-0.png",
	"particle-0.png",
	"particle-0.png"
};

static Sprite * s_bulletSprites[kBulletType_COUNT] = { };

static void getVelocityXY(float angle, float velocity, float & x, float & y)
{
	x = +std::cos(angle) * velocity;
	y = -std::sin(angle) * velocity;
}

static float toAngle(float dx, float dy)
{
	return std::atan2f(dy, dx);
}

static void mirrorAngle(float & angle, float x, float y)
{
	float dx;
	float dy;
	getVelocityXY(angle, 1.f, dx, dy);
	dx *= x;
	dy *= y;
	angle = toAngle(dx, dy);
}

//

Bullet::Bullet()
{
	memset(this, 0, sizeof(*this));

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_noGravity = true;
}

void Bullet::setVel(float angle, float velocity)
{
	getVelocityXY(angle, velocity, m_vel[0], m_vel[1]);
}

float Bullet::calcAge() const
{
	float result = 0.f;

	if (maxDistanceTravelled != 0.f)
		result = distanceTravelled / maxDistanceTravelled;

	if (result < 0.f)
		result = 0.f;
	if (result > 1.f)
		result = 1.f;

	return result;
}

//

BulletPool::BulletPool(bool localOnly)
{
	memset(m_bullets, 0, sizeof(m_bullets));
}

void BulletPool::tick(GameSim & gameSim, float _dt)
{
	Arena & arena = gameSim.m_arena;

	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		Bullet & b = m_bullets[i];

		if (b.isAlive)
		{
			const float distance = b.m_vel.CalcSize() * _dt;
			const int numSteps = std::max<int>(1, (int)std::ceil(std::abs(distance) / 4.f));

			uint32_t oldBlockMask = arena.getIntersectingBlocksMask(b.m_pos[0], b.m_pos[1]);

			const bool isInPassthrough = (oldBlockMask & kBlockMask_Passthrough) != 0;
			const bool passthroughMode = isInPassthrough || (b.m_vel[1] < 0.f);
			const uint32_t blockExcludeMask = passthroughMode ? ~kBlockMask_Passthrough : ~0;

			for (int step = 0; step < numSteps && b.isAlive; ++step)
			{
				const float dt = _dt / numSteps;

				Vec2 oldPos = b.m_pos;

				anim(gameSim, b, dt);

				bool kill = false;

				bool doReflection = true;

				if (b.type == kBulletType_Grenade)
				{
					doReflection = false;
				}

				// evaluate collisions

				if (!kill && !b.noCollide)
				{
					const uint32_t blockMask = arena.getIntersectingBlocksMask(b.m_pos[0], b.m_pos[1]) & blockExcludeMask;

					if (doReflection)
					{
						// reflection

						if (blockMask & kBlockMask_Solid & (~kBlockMask_Destructible))
						{
							if (b.maxReflectCount != 0)
							{
								b.reflectCount++;

								if (b.reflectCount > b.maxReflectCount)
								{
									kill = true;
								}
								else
								{
									b.m_vel *= -1.f;
								}
							}
							else
								kill = true;
						}
					}

					if (kill)
					{
						ParticleSpawnInfo spawnInfo(b.m_pos[0], b.m_pos[1], kBulletType_ParticleA, 10, 50, 200, 20);
						gameSim.spawnParticles(spawnInfo);
					}

					if (!kill && b.doBounce)
					{
						// eval collision in x direction

						uint32_t blockMask;

						blockMask = arena.getIntersectingBlocksMask(b.m_pos[0], oldPos[1]) & blockExcludeMask;

						if (blockMask & kBlockMask_Solid)
						{
							b.m_vel[0] *= -b.bounceAmount;
							b.m_pos[0] = oldPos[0];

							b.bounceCount++;
						}

						// eval collision in y direction

						blockMask = arena.getIntersectingBlocksMask(b.m_pos[0], b.m_pos[1]) & blockExcludeMask;

						if (blockMask & kBlockMask_Solid)
						{
							if (b.type == kBulletType_Grenade && !b.m_noGravity && b.m_vel[1] >= 0.f && b.m_vel.CalcSizeSq() < 50.f*50.f)
							{
								b.m_vel = Vec2(0.f, 0.f);
								b.m_noGravity = true;
								b.life = BULLET_GRENADE_NADE_LIFE_AFTER_SETTLE;
							}

							b.m_vel[0] *= +b.bounceAmount;
							b.m_vel[1] *= -b.bounceAmount;
							b.m_pos[1] = oldPos[1];

							b.bounceCount++;
						}
					}

					// teleport

					const bool oldTeleport = (oldBlockMask & (1 << kBlockType_Teleport)) != 0;
					const bool newTeleport = (   blockMask & (1 << kBlockType_Teleport)) != 0;

					if (!oldTeleport && newTeleport)
					{
						int sourceX = int(b.m_pos[0]) / BLOCK_SX;
						int sourceY = int(b.m_pos[1]) / BLOCK_SY;

						int destX;
						int destY;

						if (arena.getTeleportDestination(gameSim, sourceX, sourceY, destX, destY))
						{
							int deltaX = destX - sourceX;
							int deltaY = destY - sourceY;

							b.m_pos[0] += deltaX * BLOCK_SX;
							b.m_pos[1] += deltaY * BLOCK_SY;

							oldBlockMask = blockMask;
						}
					}
				}

				// wrap around

				if (b.wrapCount > b.maxWrapCount)
					kill = true;

				// max distance travelled

				if (b.maxDistanceTravelled != 0.f && b.distanceTravelled > b.maxDistanceTravelled)
					kill = true;

				Player * owner = 0;

				if (b.ownerPlayerId != -1)
				{
					for (int p = 0; p < MAX_PLAYERS; ++p)
					{
						Player & player = gameSim.m_players[p];
						if (!player.m_isUsed)
							continue;

						if (p == b.ownerPlayerId)
							owner = &player;
					}

					if (owner == 0)
						kill = true;
				}

				if (!kill && !b.noDamagePlayer)
				{
					// collide with players

					for (int p = 0; p < MAX_PLAYERS; ++p)
					{
						Player & player = gameSim.m_players[p];

						if (player.shieldSpecialReflect(b.m_pos, b.m_vel))
						{
							if (b.ownerPlayerId != -1)
								b.ownerPlayerId = p;
						}

						if (p == b.ownerPlayerId && !b.doDamageOwner)
							continue;
						if (!player.m_isUsed || !player.m_isAlive)
							continue;

						CollisionInfo collisionInfo;

						if (player.getPlayerCollision(collisionInfo))
						{
							if (collisionInfo.intersects(b.m_pos[0], b.m_pos[1]))
							{
								kill = true;

								switch (b.effect)
								{
								case kBulletEffect_Damage:
									player.handleDamage(1.f, b.m_vel, owner);
									break;
								case kBulletEffect_Ice:
									player.handleIce(b.m_vel, owner);
									break;
								case kBulletEffect_Bubble:
									player.handleBubble(b.m_vel, owner);
									break;

								default:
									Assert(false);
									break;
								}
							}
						}
					}
				}

				if (!kill && !b.noCollide && !b.noDamageMap)
				{
					// collide with map

					if (arena.handleDamageRect(
						gameSim,
						b.m_pos[0], b.m_pos[1],
						b.m_pos[0], b.m_pos[1],
						b.m_pos[0], b.m_pos[1],
						true))
					{
						b.blocksDestroyed++;

						if (b.blocksDestroyed == b.maxDestroyedBlocks)
							kill = true;
					}
				}

				if (!kill)
				{
					if (b.life != 0.f)
					{
						b.life -= dt;

						if (b.life <= 0.f)
							kill = true;
					}
				}

				if (kill)
				{
					if (b.type == kBulletType_Grenade)
					{
						if (DEBUG_RANDOM_CALLSITES)
							LOG_DBG("Random called from kBulletType_Grenade explode");
						for (int i = 0; i < BULLET_GRENADE_FRAG_COUNT; ++i)
						{
							gameSim.spawnBullet(
								b.m_pos[0],
								b.m_pos[1],
								gameSim.Random() % 256,
								kBulletType_GrenadeA,
								kBulletEffect_Damage,
								b.ownerPlayerId);
						}

						gameSim.playSound("grenade-explode.ogg");
						gameSim.doBlastEffect(
							b.m_pos,
							BULLET_GRENADE_BLAST_RADIUS,
							grenadeBlastCurve);
					}

					if (b.type == kBulletType_GrenadeA)
					{
						gameSim.playSound("grenade-frag.ogg");

						const float strength = 5.f;
						gameSim.addScreenShake(
							gameSim.RandomFloat(-strength, +strength),
							gameSim.RandomFloat(-strength, +strength), 2500.f, .3f,
							true);
					}

					free(i);
				}
			}
		}
	}
}

void BulletPool::anim(GameSim & gameSim, float dt)
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		Bullet & b = m_bullets[i];

		if (b.isAlive)
		{
			anim(gameSim, b, dt);
		}
	}
}

void BulletPool::anim(GameSim & gameSim, Bullet & b, float dt)
{
	Assert(b.isAlive);

	// evaluate gravity

	if (!b.m_noGravity)
	{
		b.m_vel[1] += GRAVITY * (b.gravityModifier == 0.f ? 1.f : b.gravityModifier) * dt;
	}

	Vec2 delta = b.m_vel * dt;

	b.m_pos += delta;

	// distance travelled

	b.distanceTravelled += delta.CalcSize();

	// wrap

	float x = b.m_pos[0];
	float y = b.m_pos[1];

	if (b.m_pos[0] < 0.f)
		b.m_pos[0] += ARENA_SX_PIXELS;
	if (b.m_pos[0] > ARENA_SX_PIXELS)
		b.m_pos[0] -= ARENA_SX_PIXELS;

	if (b.m_pos[1] < 0.f)
		b.m_pos[1] += ARENA_SY_PIXELS;
	if (b.m_pos[1] > ARENA_SY_PIXELS)
		b.m_pos[1] -= ARENA_SY_PIXELS;

	if (x != b.m_pos[0] || y != b.m_pos[1])
	{
		b.wrapCount++;

		if (b.wrapCount > b.maxWrapCount)
		{
			b.m_pos[0] = x;
			b.m_pos[1] = y;
		}
	}
}

void BulletPool::draw() const
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
			const float ageAlpha = b.doAgeAlpha ? (1.f - b.calcAge()) : 1.f;

			const float cr = ((b.color >> 24) & 0xff) / 255.f;
			const float cg = ((b.color >> 16) & 0xff) / 255.f;
			const float cb = ((b.color >>  8) & 0xff) / 255.f;
			const float ca = ((b.color >>  0) & 0xff) / 255.f * ageAlpha;

			setColorf(cr, cg, cb, ca);
			s_bulletSprites[b.type]->drawEx(b.m_pos[0], b.m_pos[1], Calc::RadToDeg(toAngle(b.m_vel[0], b.m_vel[1])), s_bulletSprites[b.type]->scale);

			setColor(255, 255, 255);
		}
	}
}

void BulletPool::drawLight() const
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		const Bullet & b = m_bullets[i];

		if (b.isAlive)
		{
			const float age = b.calcAge();

			setColorf(1.f, 1.f, 1.f, 1.f - age);
			Sprite("player-light.png").drawEx(b.m_pos[0], b.m_pos[1], 0.f, .5f, .5f, false, FILTER_LINEAR);
		}
	}

	setColorf(1.f, 1.f, 1.f);
}

uint16_t BulletPool::alloc(uint16_t * ids, uint16_t count)
{
	uint16_t result = 0;

	for (int i = 0; i < MAX_BULLETS && result < count; ++i)
	{
		if (!m_bullets[i].isAlive)
			ids[result++] = i;
	}

	for (int i = result; i < count; ++i)
		ids[i] = INVALID_BULLET_ID;

	return result;
}

void BulletPool::free(uint16_t id)
{
	Assert(id != INVALID_BULLET_ID && m_bullets[id].isAlive);
	if (id != INVALID_BULLET_ID && m_bullets[id].isAlive)
	{
		memset(&m_bullets[id], 0, sizeof(Bullet));
	}
}

void BulletPool::serialize(NetSerializationContext & context)
{
	if (context.IsRecv())
	{
		memset(m_bullets, 0, sizeof(m_bullets));

		uint16_t bulletCount;

		context.Serialize(bulletCount);

		for (int i = 0; i < bulletCount; ++i)
		{
			uint16_t id;
			context.Serialize(id);

			Bullet & b = m_bullets[id];
			context.SerializeBytes(&b, sizeof(b));
		}
	}
	else
	{
		uint16_t numBullets = 0;

		for (int i = 0; i < MAX_BULLETS; ++i)
			if (m_bullets[i].isAlive)
				numBullets++;

		context.Serialize(numBullets);

		for (int i = 0; i < MAX_BULLETS; ++i)
		{
			Bullet & b = m_bullets[i];

			if (b.isAlive)
			{
				uint16_t id = i;
				context.Serialize(id);
				context.SerializeBytes(&b, sizeof(b));
			}
		}
	}
}

#if ENABLE_GAMESTATE_DESYNC_DETECTION

uint32_t BulletPool::calcCRC() const
{
	const uint8_t * bytes = (uint8_t*)m_bullets;
	const int numBytes = sizeof(m_bullets);
	
	uint32_t result = 0;

	for (int i = 0; i < numBytes; ++i)
	{
		result += bytes[i];
		result *= 13;
	}

	return result;
}

#endif

//

void initBullet(GameSim & gameSim, Bullet & b, const ParticleSpawnInfo & spawnInfo)
{
	if (DEBUG_RANDOM_CALLSITES)
		LOG_DBG("Random called from initBullet");
	const float angle = gameSim.Random() % 256;
	const float velocity = gameSim.RandomFloat(spawnInfo.minVelocity, spawnInfo.maxVelocity);

	b = Bullet();
	b.isAlive = true;
	b.type = (BulletType)spawnInfo.type;
	b.m_pos[0] = spawnInfo.x;
	b.m_pos[1] = spawnInfo.y;
	b.setVel(angle / 128.f * float(M_PI), velocity);

	b.noCollide = true;
	b.noDamagePlayer = true;
	b.maxWrapCount = 1;
	b.maxDistanceTravelled = spawnInfo.maxDistance;
	b.color = spawnInfo.color;
}
