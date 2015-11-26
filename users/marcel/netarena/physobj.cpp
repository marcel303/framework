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

	m_gravityMultiplier = 1.f;
}

void PhysicsActor::tick(GameSim & gameSim, float dt, PhysicsActorCBs & cbs)
{
	// physics

	if (!m_noGravity)
	{
		m_vel[1] += GRAVITY * m_gravityMultiplier * dt;
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
			Vec2 min, max;
			getAABB(min, max);

			if (m_portalCooldown)
			{
				int portalId;

				Portal * portal = gameSim.findPortal(
					min[0], min[1],
					max[0], max[1],
					false,
					true,
					portalId);

				if (!portal || portalId != m_lastPortalId)
					m_portalCooldown = false;
			}

			if (!m_portalCooldown)
			{
				int portalId;

				Portal * portal = gameSim.findPortal(
					min[0], min[1],
					max[0], max[1],
					true,
					false,
					portalId);

				if (portal)
				{
					int destinationId;
					Portal * destination;

					if (portal->doTeleport(gameSim, destination, destinationId))
					{
						const Vec2 offset = m_pos - portal->getDestinationPos(Vec2(0.f, 0.f));
						m_pos = destination->getDestinationPos(offset);
						m_portalCooldown = true;
						m_lastPortalId = destinationId;
					}
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

	m_isGrounded = (getIntersectingBlockMask(gameSim, Vec2(m_pos[0], m_pos[1] + 1.f)) & kBlockMask_Solid) != 0;
}

void PhysicsActor::drawBB() const
{
	switch (m_collisionShape.type)
	{
	case CollisionShape::kType_Poly:
		for (int i = 0; i < m_collisionShape.numPoints; ++i)
		{
			const Vec2 & p1 = m_pos + m_collisionShape.points[(i + 0) % m_collisionShape.numPoints];
			const Vec2 & p2 = m_pos + m_collisionShape.points[(i + 1) % m_collisionShape.numPoints];

			setColor(0, 255, 0, 63);
			drawRect(p1[0], p1[1], p2[0], p2[1]);
		}
		break;

	case CollisionShape::kType_Circle:
		// todo : draw circle shape
		break;

	default:
		Assert(false);
		break;
	}
}

void PhysicsActor::getAABB(Vec2 & min, Vec2 & max) const
{
	switch (m_collisionShape.type)
	{
	case CollisionShape::kType_Poly:
		min = m_collisionShape.points[0];
		max = m_collisionShape.points[0];
		for (int i = 1; i < m_collisionShape.numPoints; ++i)
		{
			min = min.Min(m_collisionShape.points[i]);
			max = max.Max(m_collisionShape.points[i]);
		}
		break;

	case CollisionShape::kType_Circle:
		min = m_collisionShape.points[0] - Vec2(m_collisionShape.radius, m_collisionShape.radius);
		max = m_collisionShape.points[0] + Vec2(m_collisionShape.radius, m_collisionShape.radius);
		break;

	default:
		Assert(false);
		break;
	}

	min += m_pos;
	max += m_pos;
}

uint32_t PhysicsActor::getIntersectingBlockMask(GameSim & gameSim, Vec2 pos)
{
	Vec2 min, max;
	getAABB(min, max);

	// todo : directly intersect collision shape with blocks

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
	getAABB(collisionInfo.min, collisionInfo.max);
}

void PhysicsActor::testCollision(const CollisionShape & shape, void * arg, CollisionCB cb)
{
	CollisionShape myShape(m_collisionShape, m_pos);

	if (myShape.intersects(shape))
	{
		cb(shape, arg, this, 0, 0);
	}
}

//

#if 0 // PhysicsScene
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
