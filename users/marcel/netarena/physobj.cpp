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

#if 1
	const float wrapSizes[2] = { gameSim.m_arena.m_sxPixels, gameSim.m_arena.m_syPixels };

	if (gameSim.m_arena.m_wrapAround)
	{
		for (int j = 0; j < 2; ++j)
		{
			if (m_pos[j] < 0.f)
			{
				m_pos[j] = wrapSizes[j];
				if (cbs.onWrap)
					cbs.onWrap(cbs, *this);
			}
			else if (m_pos[j] > wrapSizes[j])
			{
				m_pos[j] = 0.f;
				if (cbs.onWrap)
					cbs.onWrap(cbs, *this);
			}
		}
	}

	const CollisionShape shape(m_collisionShape, m_pos);

	const Arena & arena = gameSim.m_arena;

	struct CollisionArgs
	{
		PhysicsActor * self;
		GameSim * gameSim;
		PhysicsActorCBs * cbs;
		bool wasInPassthrough;
		bool enterPassThrough;
		Vec2 totalVel;
		bool hasBounced;
	};

	CollisionArgs args;
	args.self = this;
	args.gameSim = &gameSim;
	args.cbs = &cbs;
	args.wasInPassthrough = m_isInPassthrough;
	args.enterPassThrough = isInPassthrough;
	args.totalVel = m_vel;
	args.hasBounced = false;

	updatePhysics(gameSim, m_pos, m_vel, dt, m_collisionShape, &args,
		this,
		[](PhysicsUpdateInfo & updateInfo)
		{
			CollisionArgs * args = (CollisionArgs*)updateInfo.arg;
			PhysicsActor * self = args->self;
			GameSim & gameSim = *args->gameSim;
			PhysicsActorCBs & cbs = *args->cbs;
			const Vec2 & delta = updateInfo.delta;
			const Vec2 & totalVel = args->totalVel;
			const bool enterPassThrough = args->enterPassThrough;
			const int i = updateInfo.axis;

			updateInfo.contactRestitution = self->m_bounciness;

			cbs.axis = updateInfo.axis;

			if (updateInfo.actor == self || updateInfo.player)
			{
				return kPhysicsUpdateFlag_DontCollide | kPhysicsUpdateFlag_DontUpdateVelocity;
			}


			//

			int result = 0;

			//

			BlockAndDistance * blockAndDistance = updateInfo.blockInfo;

			if (blockAndDistance)
			{
				Block * block = blockAndDistance->block;

				if (block)
				{
					if ((1 << block->type) & kBlockMask_Passthrough)
					{
						if (i != 1 || delta[1] < 0.f || (args->wasInPassthrough || self->m_isInPassthrough || enterPassThrough))
						{
							result |= kPhysicsUpdateFlag_DontCollide;

							self->m_isInPassthrough = true;
						}
					}

					if (((1 << block->type) & kBlockMask_Solid) == 0)
						result |= kPhysicsUpdateFlag_DontCollide;

					if (cbs.onBlockMask)
						cbs.onBlockMask(cbs, *self, 1 << block->type);
				}

				if (!(result & kPhysicsUpdateFlag_DontCollide))
				{
					// screen shake

					const float sign = Calc::Sign(delta[i]);
					float strength = (Calc::Abs(totalVel[i]) - PLAYER_JUMP_SPEED) / 25.f;

					if (strength > PLAYER_SCREENSHAKE_STRENGTH_THRESHHOLD)
					{
						strength = sign * strength / 4.f;
						gameSim.addScreenShake(
							i == 0 ? strength : 0.f,
							i == 1 ? strength : 0.f,
							3000.f, .3f,
							true);
					}
				}
			}

			if (!(result & kPhysicsUpdateFlag_DontCollide) && updateInfo.contactNormal.CalcSizeSq() != 0.f)
			{
				args->hasBounced = true;
			}

			return result;
		});

	if (args.hasBounced && cbs.onBounce)
		cbs.onBounce(cbs, *this);

	if (m_doTeleport)
	{
		tickPortal(gameSim);
	}

	if (cbs.onMove)
		cbs.onMove(cbs, *this);
#else
	bool collision = false;

	Vec2 delta = m_vel * dt;
	float deltaLen = delta.CalcSize();

	int numSteps = std::max<int>(1, (int)std::ceil(deltaLen));

	Vec2 step = delta / numSteps;

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

			if (blockMask != 0)
			{
				if (cbs.onBlockMask)
					cbs.onBlockMask(cbs, *this, blockMask);
			}
		}

		if (m_doTeleport)
		{
			tickPortal();
		}

		if (cbs.onMove)
			cbs.onMove(cbs, *this);
	}
#endif

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

void PhysicsActor::tickPortal(GameSim & gameSim)
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

void PhysicsActor::drawBB() const
{
	switch (m_collisionShape.type)
	{
	case CollisionShape::kType_None:
		break;

	case CollisionShape::kType_Poly:
		for (int i = 0; i < m_collisionShape.numPoints; ++i)
		{
			const Vec2 & p1 = m_pos + m_collisionShape.points[(i + 0) % m_collisionShape.numPoints];
			const Vec2 & p2 = m_pos + m_collisionShape.points[(i + 1) % m_collisionShape.numPoints];

			setColor(0, 255, 0, 63);
			drawLine(p1[0], p1[1], p2[0], p2[1]);
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
	getAABB(m_pos, min, max);
}

void PhysicsActor::getAABB(Vec2Arg pos, Vec2 & min, Vec2 & max) const
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

	min += pos;
	max += pos;
}

uint32_t PhysicsActor::getIntersectingBlockMask(GameSim & gameSim, Vec2 pos)
{
	const CollisionShape shape(m_collisionShape, pos);
	const Arena & arena = gameSim.m_arena;

	return arena.getIntersectingBlocksMask(shape);
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
