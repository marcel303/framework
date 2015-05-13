#include <stdint.h>
#include <string.h>
#include "arena.h"
#include "Calc.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "NetSerializable.h"
#include "physobj.h"

PhysicsActor::PhysicsActor()
{
	memset(this, 0, sizeof(PhysicsActor));
}

void PhysicsActor::tick(GameSim & gameSim, float dt, PhysicsActorCBs & cbs)
{
	// physics

	if (!m_noGravity)
	{
		m_vel[1] += GRAVITY * dt;
	}

	// speed clamp

	if (std::abs(m_vel[1]) > PLAYER_SPEED_MAX)
	{
		m_vel[1] = PLAYER_SPEED_MAX * Calc::Sign(m_vel[1]);
	}

	uint32_t oldBlockMask = getIntersectingBlockMask(gameSim, Vec2(m_pos[0], m_pos[1]));
	const bool isInPassthrough = (oldBlockMask & kBlockMask_Passthrough) != 0;
	const bool passthroughMode = isInPassthrough || (m_vel[1] < 0.f);
	const uint32_t blockExcludeMask = passthroughMode ? ~kBlockMask_Passthrough : ~0;

	if (m_friction != 0.f)
	{
		uint32_t blockMask = getIntersectingBlockMask(gameSim, Vec2(m_pos[0], m_pos[1] + 1.f)) & blockExcludeMask;

		if (blockMask & kBlockMask_Solid)
		{
			m_vel[0] *= std::pow(m_friction, dt);
		}
	}

	if (m_airFriction)
	{
		m_vel *= std::pow(m_airFriction, dt);
	}

	// collision

	Vec2 delta = m_vel * dt;
	float deltaLen = delta.CalcSize();

	int numSteps = std::max<int>(1, (int)std::ceil(deltaLen));

	Vec2 step = delta / numSteps;

	const float wrapSizes[2] = { ARENA_SX_PIXELS, ARENA_SY_PIXELS };

	for (int i = 0; i < numSteps; ++i)
	{
		Vec2 newPos = m_pos;

		for (int j = 0; j < 2; ++j)
		{
			bool collision = false;

			cbs.axis = j;

			float oldPos = newPos[j];

			newPos[j] += step[j];

			if (newPos[j] < 0.f)
			{
				newPos[j] = wrapSizes[j];
				if (cbs.onWrap)
					cbs.onWrap(cbs, *this);
			}
			else if (newPos[j] > wrapSizes[j])
			{
				newPos[j] = 0.f;
				if (cbs.onWrap)
					cbs.onWrap(cbs, *this);
			}

			uint32_t blockMask = getIntersectingBlockMask(gameSim, newPos) & blockExcludeMask;

			if (j == 0)
				blockMask &= ~kBlockMask_Passthrough;

			if (blockMask & kBlockMask_Solid)
			{
				collision = true;
			}

			if (collision)
			{
				newPos[j] = oldPos;

				m_vel[j] *= -m_bounciness;
				step[j] *= -m_bounciness;

				if (cbs.onBounce)
					cbs.onBounce(cbs, *this);
			}
			else
			{
				m_pos[j] = newPos[j];
			}

			if (cbs.onBlockMask)
				cbs.onBlockMask(cbs, *this, blockMask);
		}

		if (m_doTeleport)
		{
			const uint32_t blockMask = getIntersectingBlockMask(gameSim, newPos) & blockExcludeMask;

			const bool oldTeleport = (oldBlockMask & (1 << kBlockType_Teleport)) != 0;
			const bool newTeleport = (   blockMask & (1 << kBlockType_Teleport)) != 0;

			if (!oldTeleport && newTeleport)
			{
				int sourceX = int(m_pos[0]) / BLOCK_SX;
				int sourceY = int(m_pos[1]) / BLOCK_SY;

				int destX;
				int destY;

				if (gameSim.m_arena.getTeleportDestination(gameSim, sourceX, sourceY, destX, destY))
				{
					int deltaX = destX - sourceX;
					int deltaY = destY - sourceY;

					m_pos[0] += deltaX * BLOCK_SX;
					m_pos[1] += deltaY * BLOCK_SY;

					oldBlockMask = blockMask;
				}
			}
		}

		if (cbs.onMove)
			cbs.onMove(cbs, *this);
	}

	if (cbs.onHitPlayer)
	{
		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (gameSim.m_players[i].m_isUsed && gameSim.m_players[i].m_isAlive)
			{
				CollisionInfo playerCollision;
				if (gameSim.m_players[i].getPlayerCollision(playerCollision))
				{
					CollisionInfo myCollision;
					getCollisionInfo(myCollision);

					if (myCollision.intersects(playerCollision))
					{
						cbs.onHitPlayer(cbs, *this, gameSim.m_players[i]);
					}
				}
			}
		}
	}
}

void PhysicsActor::drawBB() const
{
	const Vec2 min = m_pos + m_bbMin;
	const Vec2 max = m_pos + m_bbMax;

	setColor(0, 255, 0, 127);
	drawRect(min[0], min[1], max[0], max[1]);

	setColor(255, 255, 255);
}

uint32_t PhysicsActor::getIntersectingBlockMask(GameSim & gameSim, Vec2 pos)
{
	Vec2 min = pos + m_bbMin;
	Vec2 max = pos + m_bbMax;

	const int x1 = (int(min[0]         )     + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;
	const int x2 = (int(max[0]         )     + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;
	const int y1 = (int(min[1]         )     + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;
	const int y2 = (int(max[1]         )     + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;
	const int y3 = (int(min[1] + max[1]) / 2 + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;

	const Arena & arena = gameSim.m_arena;

	uint32_t result = 0;

	result |= arena.getIntersectingBlocksMask(x1, y1);
	result |= arena.getIntersectingBlocksMask(x2, y1);
	result |= arena.getIntersectingBlocksMask(x2, y2);
	result |= arena.getIntersectingBlocksMask(x1, y2);

	result |= arena.getIntersectingBlocksMask(x1, y3);
	result |= arena.getIntersectingBlocksMask(x2, y3);

	return result;
}

void PhysicsActor::getCollisionInfo(CollisionInfo & collisionInfo)
{
	collisionInfo.min = m_pos + m_bbMin;
	collisionInfo.max = m_pos + m_bbMax;
}

void PhysicsActor::testCollision(const CollisionShape & shape, void * arg, CollisionCB cb)
{
	CollisionShape myShape(
		m_pos + m_bbMin,
		m_pos + m_bbMax);

	if (myShape.intersects(shape))
	{
		cb(shape, arg, this, 0, 0);
	}
}

/*
bool PhysicsActor::test(const CollisionBox & box) const
{
	const Vec2 min = m_pos + m_bbMin;
	const Vec2 max = m_pos + m_bbMax;

	return
		min[0] <= box.max[0] &&
		min[1] <= box.max[1] &&
		max[0] >= box.min[0] &&
		max[1] >= box.min[1];
}
*/

//

#if 0
PhysicsScene::PhysicsScene()
{
}

void PhysicsScene::serialize(NetSerializationContext & ctx)
{
	ctx.SerializeBytes(m_actors, sizeof(m_actors));
}

PhysicsActorId PhysicsScene::allocActor()
{
	for (int i = 0; i < MAX_PHYSICS_ACTORS; ++i)
		if (!m_actors[i].m_isActive)
			return i;

	return kPhysicsActorId_Invalid;
}

void PhysicsScene::freeActor(PhysicsActorId id)
{
	Assert(id != kPhysicsActorId_Invalid);
	if (id != kPhysicsActorId_Invalid)
		m_actors[id] = PhysicsActor();
}

void PhysicsScene::testCollision(const CollisionShape & shape, CollisionCB cb, void * arg, bool wrap)
{
	if (wrap)
	{
		/* todo : 9x
		testInternal(b, cb, arg);
		*/
	}
	else
	{
		testCollisionInternal(shape, cb, arg);
	}

	for (int i = 0; i < MAX_PHYSICS_ACTORS; ++i)
		if (m_actors[i].m_isTested)
			m_actors[i].m_isTested = false;
}

void PhysicsScene::testCollisionInternal(const CollisionShape & shape, CollisionCB cb, void * arg)
{
	CollisionShape shape = box;

	for (int i = 0; i < MAX_PHYSICS_ACTORS; ++i)
	{
		if (m_actors[i].m_isActive && !m_actors[i].m_isTested)
		{
			if (m_actors[i].testCollision(shape))
			{
				m_actors[i].m_isTested = true;

				cb(shape, arg, &m_actors[i], 0, 0);
			}
		}
	}
}
#endif
