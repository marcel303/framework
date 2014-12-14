#include "arena.h"
#include "bullet.h"
#include "Calc.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "player.h"

#define WRAP_AROUND_TOP_AND_BOTTOM 1
#define PLAYER_SCALE 1

// todo : m_isGrounded should be true when stickied too. review code and make change!

enum PlayerAnim
{
	kPlayerAnim_NULL,
	kPlayerAnim_Jump,
	kPlayerAnim_WallSlide,
	kPlayerAnim_Walk,
	kPlayerAnim_Attack,
	kPlayerAnim_Fire,
	kPlayerAnim_AirDash,
	kPlayerAnim_Spawn,
	kPlayerAnim_Die,
	kPlayerAnim_COUNT
};

struct PlayerAnimInfo
{
	const char * file;
	int prio;
} s_animInfos[kPlayerAnim_COUNT] =
{
	{ nullptr,                  0 },
	{ "zero-x3-jump.png",       1 },
	{ "zero-x3-wall-slide.png", 2 },
	{ "zero-x3-walk.png",       3 },
	{ "zero-x3-attack.png",     4 },
	{ "zero-x3-fire.png",       4 },
	{ "zero-x3-dash-air.png",   4 },
	{ "zero-x3-spawn.png",      5 },
	{ "zero-x3-die.png",        6 }
};

enum PlayerAttackType
{
	kPlayerAttackType_Sword,
	kPlayerAttackType_Fire
};

void PlayerAnim_NS::SerializeStruct()
{
	SerializeBits(m_anim, 4);
	Serialize(m_play);

	//

	Player * player = static_cast<Player*>(GetOwner());

	if (m_anim != kPlayerAnim_NULL)
	{
		delete player->m_sprite;
		player->m_sprite = 0;

		player->m_sprite = new Sprite(s_animInfos[m_anim].file);
		player->m_sprite->animActionHandler = Player::handleAnimationAction;
		player->m_sprite->animActionHandlerObj = player;
	}

	if (m_play)
		player->m_sprite->startAnim("anim");
	else
		player->m_sprite->stopAnim();
}

enum PlayerEvent
{
	kPlayerEvent_Spawn,
	kPlayerEvent_Die,
	kPlayerEvent_Jump,
	kPlayerEvent_WallJump,
	kPlayerEvent_LandOnGround,
	kPlayerEvent_StickyAttach,
	kPlayerEvent_StickyRelease,
	kPlayerEvent_StickyJump,
	kPlayerEvent_SpringJump,
	kPlayerEvent_SpikeHit,
	kPlayerEvent_ArenaWrap,
	kPlayerEvent_DashAir,
	kPlayerEvent_DestructibleDestroy
};

void Player::handleAnimationAction(const std::string & action, const Dictionary & args)
{
	Player * player = args.getPtrType<Player>("obj", 0);
	if (player && player->m_isAuthorative)
	{
		if (g_devMode)
		{
			log("action: %s", action.c_str());
		}

		if (action == "set_anim_vel")
		{
			player->m_animVel[0] = args.getFloat("x", player->m_animVel[0]);
			player->m_animVel[1] = args.getFloat("y", player->m_animVel[1]);
		}
		else if (action == "set_anim_vel_abs")
		{
			player->m_animVelIsAbsolute = args.getBool("abs", player->m_animVelIsAbsolute);
			player->m_vel = Vec2();
		}
		else if (action == "set_attack_vel")
		{
			player->m_attack.attackVel[0] = args.getFloat("x", player->m_attack.attackVel[0]);
			player->m_attack.attackVel[1] = args.getFloat("y", player->m_attack.attackVel[1]);
		}
		else if (action == "sound")
		{
			g_app->netPlaySound(args.getString("file", "").c_str(), args.getInt("volume", 100));
		}
	}
}

void Player::getPlayerCollision(CollisionInfo & collision)
{
	collision.x1 = m_pos[0] + m_collision.x1;
	collision.y1 = m_pos[1] + m_collision.y1;
	collision.x2 = m_pos[0] + m_collision.x2;
	collision.y2 = m_pos[1] + m_collision.y2;
}

void Player::getAttackCollision(CollisionInfo & collision)
{
	float x1 = m_attack.collision.x1 * m_pos.xFacing;
	float y1 = m_attack.collision.y1;
	float x2 = m_attack.collision.x2 * m_pos.xFacing;
	float y2 = m_attack.collision.y2;

	if (m_pos.yFacing < 0)
	{
		y1 = -PLAYER_COLLISION_SY - y1;
		y2 = -PLAYER_COLLISION_SY - y2;
	}

	if (x1 > x2)
		std::swap(x1, x2);
	if (y1 > y2)
		std::swap(y1, y2);

	collision.x1 = m_pos[0] + x1;
	collision.y1 = m_pos[1] + y1;
	collision.x2 = m_pos[0] + x2;
	collision.y2 = m_pos[1] + y2;
}

float Player::getAttackDamage(Player * other)
{
	float result = 0.f;

	if (m_attack.attacking)
	{
		CollisionInfo attackCollision;
		CollisionInfo otherCollision;

		getAttackCollision(attackCollision);
		other->getPlayerCollision(otherCollision);

		if (attackCollision.intersects(otherCollision))
		{
			result = 1.f;
		}
	}

	return result;
}

bool Player::isAnimOverrideAllowed(int anim) const
{
	return !m_isAnimDriven || (s_animInfos[anim].prio > s_animInfos[m_anim.m_anim].prio);
}

float Player::mirrorX(float x) const
{
	return m_pos.xFacing > 0 ? x : -x;
}

float Player::mirrorY(float y) const
{
	return m_pos.yFacing > 0 ? y : PLAYER_COLLISION_SY - y;
}

Player::Player(uint32_t netId, uint16_t owningChannelId)
	: m_pos(this)
	, m_state(this)
	, m_anim(this)
	, m_selectedWeapon(kPlayerWeapon_Sword)
	, m_isAuthorative(false)
	, m_blockMask(0)
	, m_isGrounded(false)
	, m_isAttachedToSticky(false)
	, m_isAnimDriven(false)
	, m_animVelIsAbsolute(false)
	, m_isAirDashCharged(false)
	, m_isWallSliding(false)
{
	setNetId(netId);
	setOwningChannelId(owningChannelId);

	if (netId != 0)
	{
		m_isAuthorative = true;
	}

	m_collision.x1 = -PLAYER_COLLISION_SX / 2.f;
	m_collision.x2 = +PLAYER_COLLISION_SX / 2.f;
	m_collision.y1 = -PLAYER_COLLISION_SY / 1.f;
	m_collision.y2 = 0.f;

	m_sprite = new Sprite("zero-x3-walk.png");
}

Player::~Player()
{
	delete m_sprite;
	m_sprite = 0;
}

static void PlaySecondaryEffects(PlayerEvent e)
{
	switch (e)
	{
	case kPlayerEvent_Spawn:
		g_app->netPlaySound("player-spawn.ogg");
		break;
	case kPlayerEvent_Die:
		g_app->netPlaySound("player-death.ogg");
		break;
	case kPlayerEvent_Jump:
		g_app->netPlaySound("player-jump.ogg");
		break;
	case kPlayerEvent_WallJump:
		g_app->netPlaySound("player-wall-jump.ogg");
		break;
	case kPlayerEvent_LandOnGround:
		g_app->netPlaySound("player-land-ground.ogg", 25);
		break;
	case kPlayerEvent_StickyAttach:
		g_app->netPlaySound("player-sticky-attach.ogg");
		break;
	case kPlayerEvent_StickyRelease:
		g_app->netPlaySound("player-sticky-release.ogg");
		break;
	case kPlayerEvent_StickyJump:
		g_app->netPlaySound("player-sticky-jump.ogg");
		break;
	case kPlayerEvent_SpringJump:
		g_app->netPlaySound("player-spring-jump.ogg");
		break;
	case kPlayerEvent_SpikeHit:
		g_app->netPlaySound("player-spike-hit.ogg");
		break;
	case kPlayerEvent_ArenaWrap:
		g_app->netPlaySound("player-arena-wrap.ogg");
		break;
	case kPlayerEvent_DashAir:
		break;
	case kPlayerEvent_DestructibleDestroy:
		g_app->netPlaySound("player-arena-wrap.ogg");
		break;
	}
}

void Player::tick(float dt)
{
	m_collision.x1 = -PLAYER_COLLISION_SX / 2.f;
	m_collision.x2 = +PLAYER_COLLISION_SX / 2.f;
	m_collision.y1 = -PLAYER_COLLISION_SY / 1.f;
	m_collision.y2 = 0.f;

	//

	if (m_isAnimDriven)
	{
		if (!m_anim.IsDirty() && !m_sprite->animIsActive)
		{
			m_isAnimDriven = false;
			m_animVel = Vec2();
			m_animVelIsAbsolute = false;

			switch (m_anim.m_anim)
			{
			case kPlayerAnim_Attack:
				m_attack.attacking = false;
				break;
			case kPlayerAnim_Fire:
				m_attack.attacking = false;
				break;
			case kPlayerAnim_AirDash:
				break;
			case kPlayerAnim_Jump:
				break;
			case kPlayerAnim_Spawn:
				m_state.isAlive = true;
				m_state.SetDirty();
				break;
			case kPlayerAnim_Die:
				m_state.isAlive = false;
				m_state.SetDirty();
				break;
			}
		}
	}

	//

	if (!m_state.isAlive || m_input.wentDown(INPUT_BUTTON_START))
	{
		respawn();
	}

	if (m_state.isAlive)
	{
		// see if we grabbed any pickup

		Pickup * pickup = g_host->grabPickup(
			m_pos[0] + m_collision.x1,
			m_pos[1] + m_collision.y1,
			m_pos[0] + m_collision.x2,
			m_pos[1] + m_collision.y2);

		if (pickup)
		{
			switch (pickup->type)
			{
			case kPickupType_Ammo:
				m_selectedWeapon = kPlayerWeapon_Fire;
				break;
			}
		}

		m_blockMask = ~0;

		const uint32_t currentBlockMask = getIntersectingBlocksMask(m_pos[0], m_pos[1]);
		
		const bool isInPassthough = (currentBlockMask & kBlockMask_Passthrough) != 0;
		if (isInPassthough || m_input.isDown(INPUT_BUTTON_DOWN))
			m_blockMask = ~kBlockMask_Passthrough;

		const uint32_t currentBlockMaskFloor = getIntersectingBlocksMask(m_pos[0], m_pos[1] + 1.f);
		const uint32_t currentBlockMaskCeil = getIntersectingBlocksMask(m_pos[0], m_pos[1] - 1.f);

		float surfaceFriction = 0.f;
		Vec2 animVel(0.f, 0.f);

		animVel[0] += m_animVel[0] * m_pos.xFacing;
		animVel[1] += m_animVel[1] * m_pos.yFacing;

		// attack

		if (m_attack.attacking)
		{
			animVel[0] += m_attack.attackVel[0] * m_pos.xFacing;
			animVel[1] += m_attack.attackVel[1] * m_pos.yFacing;

			// see if we hit anyone

			for (auto i = g_host->m_players.begin(); i != g_host->m_players.end(); ++i)
			{
				Player * other = *i;

				if (other == this)
					continue;

				float damage1 = getAttackDamage(other);
				float damage2 = other->getAttackDamage(this);

				if (damage1 != 0.f)
				{
					if (damage2 != 0.f)
					{
						//log("-> attack cancel");

						Player * players[2] = { this, other };

						for (int i = 0; i < 2; ++i)
						{
							players[i]->m_attack.attacking = false;
						}
					}
					else
					{
						//log("-> attack damage");

						other->handleDamage(1.f, Vec2(m_pos.xFacing * PLAYER_SWORD_PUSH_SPEED, 0.f), this);
					}
				}
			}

			// see if we've hit a block

			int x1, y1, x2, y2;

			CollisionInfo attackCollision;
			getAttackCollision(attackCollision);

			if (g_hostArena->getBlockRectFromPixels(
				attackCollision.x1,
				attackCollision.y1,
				attackCollision.x2,
				attackCollision.y2,
				x1, y1, x2, y2))
			{
				bool updated = false;

				for (int x = x1; x <= x2; ++x)
				{
					for (int y = y1; y <= y2; ++y)
					{
						Block & block = g_hostArena->getBlock(x, y);

						if (block.type == kBlockType_Destructible && !m_attack.hitDestructible)
						{
							block.type = kBlockType_Empty;

							PlaySecondaryEffects(kPlayerEvent_DestructibleDestroy);

							m_attack.hitDestructible = true;

							updated = true; // todo : more optimized way of making small changes to map
						}
					}
				}

				if (updated)
				{
					g_hostArena->setDirty();
				}
			}
		}
		else
		{
			if (m_input.wentDown(INPUT_BUTTON_B))
			{
				int attackAnim = -1;

				switch (m_selectedWeapon)
				{
				case kPlayerWeapon_Sword:
					attackAnim = kPlayerAnim_Attack;
					break;
				case kPlayerWeapon_Fire:
					attackAnim = kPlayerAnim_Fire;
					m_selectedWeapon = kPlayerWeapon_Sword;
					break;
				}

				Assert(attackAnim != -1);

				if (isAnimOverrideAllowed(attackAnim))
				{
					m_attack = AttackInfo();
					m_attack.attacking = true;

					m_attack.collision.x1 = PLAYER_SWORD_COLLISION_X1;
					m_attack.collision.y1 = PLAYER_SWORD_COLLISION_Y1;
					m_attack.collision.x2 = PLAYER_SWORD_COLLISION_X2;
					m_attack.collision.y2 = PLAYER_SWORD_COLLISION_Y2;

					m_anim.SetAnim(attackAnim, true, true);
					m_isAnimDriven = true;

					if (attackAnim == kPlayerAnim_Attack)
					{
						if (m_isWallSliding)
						{
							m_pos.xFacing *= -1;
							m_pos.SetDirty();
						}
					}

					if (attackAnim == kPlayerAnim_Fire)
					{
						g_app->netSpawnBullet(
							m_pos[0] + mirrorX(0.f),
							m_pos[1] - mirrorY(44.f),
							m_pos.xFacing < 0 ? 128 : 0,
							kBulletType_A, getNetId());
					}
				}
			}
		}

		if (m_isAirDashCharged && !m_isGrounded && !m_isAttachedToSticky && m_input.wentDown(INPUT_BUTTON_A))
		{
			if (isAnimOverrideAllowed(kPlayerAnim_AirDash))
			{
				if ((getIntersectingBlocksMask(m_pos[0] + m_pos.xFacing, m_pos[1]) & kBlockMask_Solid) == 0)
				{
					m_isAirDashCharged = false;

					m_anim.SetAnim(kPlayerAnim_AirDash, true, true);
					m_isAnimDriven = true;
				}
			}
		}

		// death by spike

		if (currentBlockMask & (1 << kBlockType_Spike))
		{
			if (isAnimOverrideAllowed(kPlayerAnim_Die))
			{
				m_anim.SetAnim(kPlayerAnim_Die, true, true);
				m_isAnimDriven = true;

				PlaySecondaryEffects(kPlayerEvent_SpikeHit);
			}
		}

		// teleport

		{
			int px = int(m_pos[0] + (m_collision.x1 + m_collision.x2) / 2) / BLOCK_SX;
			int py = int(m_pos[1] + (m_collision.y1 + m_collision.y2) / 2) / BLOCK_SY;

			if (px != m_teleport.x || py != m_teleport.y)
			{
				m_teleport.cooldown = false;
			}

			if (!m_teleport.cooldown && px >= 0 && px < ARENA_SX && py >= 0 && py < ARENA_SY)
			{
				const Block & block = g_hostArena->getBlock(px, py);

				if (block.type == kBlockType_Teleport)
				{
					// find a teleport destination

					std::vector< std::pair<int, int> > destinations;

					for (int x = 0; x < ARENA_SX; ++x)
						for (int y = 0; y < ARENA_SY; ++y)
							if (x != px && y != py && g_hostArena->getBlock(x, y).type == kBlockType_Teleport)
								destinations.push_back(std::make_pair(x, y));

					if (destinations.empty())
					{
						LOG_WRN("unable to find teleport destination");
					}
					else
					{
						const int idx = rand() % destinations.size();

						const int x = std::get<0>(destinations[idx]);
						const int y = std::get<1>(destinations[idx]);

						m_pos[0] = x * BLOCK_SX;
						m_pos[1] = y * BLOCK_SY;

						m_pos[0] += BLOCK_SX / 2;
						m_pos[1] += BLOCK_SY - 1;

						m_teleport.cooldown = true;
						m_teleport.x = x;
						m_teleport.y = y;
					}
				}
			}
		}

		bool playerControl = true;

		if (m_attack.attacking)
		{
			playerControl = false;
		}

		if (m_isAnimDriven)
		{
			playerControl = false;
		}

		// sticky ceiling

		if (currentBlockMaskCeil & (1 << kBlockType_Sticky))
		{
			if (playerControl && m_input.wentDown(INPUT_BUTTON_A))
			{
				m_vel[1] = PLAYER_JUMP_SPEED / 2.f;

				m_isAttachedToSticky = false;

				PlaySecondaryEffects(kPlayerEvent_StickyJump);
			}
			else if (playerControl && m_input.wentDown(INPUT_BUTTON_DOWN))
			{
				m_isAttachedToSticky = false;

				PlaySecondaryEffects(kPlayerEvent_StickyRelease);
			}
			else
			{
				surfaceFriction = FRICTION_GROUNDED;

				if (!m_isAttachedToSticky)
				{
					m_isAttachedToSticky = true;

					PlaySecondaryEffects(kPlayerEvent_StickyAttach);
				}
			}
		}
		else
		{
			m_isAttachedToSticky = false;
		}

		// steering

		float steeringSpeed = 0.f;

		if (playerControl)
		{
			int numSteeringFrame = 1;

			if (m_input.isDown(INPUT_BUTTON_LEFT))
				steeringSpeed -= 1.f;
			if (m_input.isDown(INPUT_BUTTON_RIGHT))
				steeringSpeed += 1.f;

			if (m_isGrounded || m_isAttachedToSticky)
			{
				if (steeringSpeed != 0.f)
					m_anim.SetAnim(kPlayerAnim_Walk, true, false);
				else
					m_anim.SetAnim(kPlayerAnim_Walk, false, false);
			}

			//

			bool isWalkingOnSolidGround = false;

			if (m_isAttachedToSticky)
				isWalkingOnSolidGround = (currentBlockMaskCeil & kBlockMask_Solid) != 0;
			else
				isWalkingOnSolidGround = (currentBlockMaskFloor & kBlockMask_Solid) != 0;

			if (isWalkingOnSolidGround)
			{
				steeringSpeed *= STEERING_SPEED_ON_GROUND;
				numSteeringFrame = 10;
			}
			else
			{
				steeringSpeed *= STEERING_SPEED_IN_AIR;
				numSteeringFrame = 10;
			}

			if (steeringSpeed != 0.f)
			{
				float maxSteeringDelta;

				if (steeringSpeed > 0)
					maxSteeringDelta = steeringSpeed - m_vel[0];
				if (steeringSpeed < 0)
					maxSteeringDelta = steeringSpeed - m_vel[0];

				if (Calc::Sign(steeringSpeed) == Calc::Sign(maxSteeringDelta))
					m_vel[0] += maxSteeringDelta * dt * 60.f / numSteeringFrame;
			}
		}

		// gravity

		float gravity;

		m_isWallSliding = false;

		if (m_isAttachedToSticky)
			gravity = 0.f;
		else if (m_animVelIsAbsolute)
			gravity = 0.f;
		else
		{
			if (currentBlockMask & (1 << kBlockType_GravityDisable))
				gravity = 0.f;
			else if (currentBlockMask & (1 << kBlockType_GravityReverse))
				gravity = GRAVITY * BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER;
			else if (currentBlockMask & (1 << kBlockType_GravityStrong))
				gravity = GRAVITY * BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER;
			else if (currentBlockMask & ((1 << kBlockType_GravityLeft) | (1 << kBlockType_GravityRight)))
				gravity = 0.f;
			else
				gravity = GRAVITY;

			// wall slide

			if (!m_isGrounded &&
				!m_isAttachedToSticky &&
				isAnimOverrideAllowed(kPlayerAnim_WallSlide) &&
				m_vel[0] != 0.f && Calc::Sign(m_pos.xFacing) == Calc::Sign(m_vel[0]) &&
				//Calc::Sign(m_vel[1]) == Calc::Sign(gravity) &&
				(getIntersectingBlocksMask(m_pos[0] + m_pos.xFacing, m_pos[1]) & kBlockMask_Solid) != 0)
			{
				m_isWallSliding = true;

				m_anim.SetAnim(kPlayerAnim_WallSlide, true, false);

				if (m_vel[1] > PLAYER_WALLSLIDE_SPEED && Calc::Sign(m_vel[1]) == Calc::Sign(gravity))
					m_vel[1] = PLAYER_WALLSLIDE_SPEED;
			}
		}

		m_vel[1] += gravity * dt;

		if (currentBlockMask & (1 << kBlockType_GravityLeft))
			m_vel[0] -= GRAVITY * dt;
		if (currentBlockMask & (1 << kBlockType_GravityRight))
			m_vel[0] += GRAVITY * dt;

		// converyor belt

		if (currentBlockMaskFloor & (1 << kBlockType_ConveyorBeltLeft))
			animVel[0] += -BLOCKTYPE_CONVEYOR_SPEED;
		if (currentBlockMaskFloor & (1 << kBlockType_ConveyorBeltRight))
			animVel[0] += +BLOCKTYPE_CONVEYOR_SPEED;

		// collision

		for (int i = 0; i < 2; ++i)
		{
			float totalDelta =
				(
					(m_animVelIsAbsolute ? 0.f : m_vel[i]) +
					animVel[i]
				) * dt;

			const float deltaSign = totalDelta < 0.f ? -1.f : +1.f;

			if (i == 1)
			{
				// update passthrough mode
				m_blockMask = ~0;
				if ((getIntersectingBlocksMask(m_pos[0], m_pos[1]) & kBlockMask_Passthrough) || m_input.isDown(INPUT_BUTTON_DOWN))
					m_blockMask = ~kBlockMask_Passthrough;
			}

			while (totalDelta != 0.f)
			{
				const float delta = (std::abs(totalDelta) < 1.f) ? totalDelta : deltaSign;

				Vec2 newPos(m_pos.x, m_pos.y);

				newPos[i] += delta;

				uint32_t newBlockMask = getIntersectingBlocksMask(newPos[0], newPos[1]);

				// ignore passthough blocks if we're moving horizontally or upwards
				// todo : update block mask each iteration. reset m_blockMask first
				//if (i != 1 || delta <= 0.f)
				//if ((!m_isGrounded || isInPassthough) && (i != 0 || delta <= 0.f))
				if ((i == 0 && isInPassthough) || (i == 1 && delta <= 0.f))
					newBlockMask &= ~kBlockMask_Passthrough;

				// make sure we stay grounded, within reason. allows the player to walk up/down slopes
				if (i == 0 && m_isGrounded)
				{
					if (newBlockMask & kBlockMask_Solid)
					{
						for (int dy = 1; dy <= 2; ++dy)
						{
							const uint32_t blockMask = getIntersectingBlocksMask(newPos[0], newPos[1] - dy);

							if ((blockMask & kBlockMask_Solid) == 0)
							{
								m_pos[1] -= dy;
								newPos[1] -= dy;
								newBlockMask = blockMask;
								break;
							}
						}
					}
					else
					{
						for (int dy = 1; dy <= 1; ++dy)
						{
							const uint32_t blockMask = getIntersectingBlocksMask(newPos[0], newPos[1] + 1.f);

							if ((blockMask & kBlockMask_Solid) == 0)
							{
								m_pos[1] += 1.f;
								newPos[1] += 1.f;
								newBlockMask = blockMask;
								break;
							}
						}
					}
				}

				if (newBlockMask & kBlockMask_Solid)
				{
					if (i == 0)
					{
						// colliding with solid object left/right of player

						if (!m_isGrounded && playerControl && m_input.wentDown(INPUT_BUTTON_A))
						{
							// wall jump

							m_vel[0] = -PLAYER_WALLJUMP_RECOIL_SPEED * deltaSign;
							m_vel[1] = -PLAYER_WALLJUMP_SPEED;

							PlaySecondaryEffects(kPlayerEvent_WallJump);
						}
						else
						{
							m_vel[0] = 0.f;

							if (m_isAnimDriven && m_anim.m_anim == kPlayerAnim_AirDash)
							{
								m_sprite->stopAnim();
							}
						}
					}

					if (i == 1)
					{
						// grounded

						if (newBlockMask & (1 << kBlockType_Slide))
							surfaceFriction = 0.f;
						else
							surfaceFriction = FRICTION_GROUNDED;

						if (m_vel[i] >= 0.f)
						{
							if (playerControl && m_input.wentDown(INPUT_BUTTON_A))
							{
								// jumping

								m_vel[i] = -PLAYER_JUMP_SPEED;

								PlaySecondaryEffects(kPlayerEvent_Jump);
							}
							else if (newBlockMask & (1 << kBlockType_Spring))
							{
								// spring

								m_vel[i] = -PLAYER_JUMP_SPEED;

								PlaySecondaryEffects(kPlayerEvent_SpringJump);
							}
							else
							{
								m_vel[i] = 0.f;
							}
						}
						else
						{
							m_vel[i] = 0.f;
						}
					}

					totalDelta = 0.f;
				}
				else
				{
					m_pos[i] = newPos[i];

					totalDelta -= delta;
				}
			}
		}

		// grounded?

		if (m_vel[1] >= 0.f && (getIntersectingBlocksMask(m_pos[0], m_pos[1] + 1.f) & kBlockMask_Solid) != 0)
		{
			if (!m_isGrounded)
			{
				m_isGrounded = true;

				PlaySecondaryEffects(kPlayerEvent_LandOnGround);
			}
		}
		else
		{
			m_isGrounded = false;
		}

		// breaking

		if (steeringSpeed == 0.f)
		{
			m_vel[0] *= powf(1.f - surfaceFriction, dt * 60.f);
		}

		// air dash

		if (m_isGrounded || m_isAttachedToSticky || m_isWallSliding)
		{
			m_isAirDashCharged = true;
		}

		// animation

		if (!m_isGrounded && !m_isAttachedToSticky && !m_isWallSliding && isAnimOverrideAllowed(kPlayerAnim_Jump))
		{
			m_anim.SetAnim(kPlayerAnim_Jump, true, false);
		}

	#if !WRAP_AROUND_TOP_AND_BOTTOM
		// death by fall

		if (m_pos[1] > ARENA_SY_PIXELS)
		{
			if (isAnimOverrideAllowed(kPlayerAnim_Die))
			{
				m_anim.SetAnim(kPlayerAnim_Die, true, true);
				m_isAnimDriven = true;
			}
		}
	#endif

		// facing

		if (playerControl && steeringSpeed != 0.f)
			m_pos.xFacing = steeringSpeed < 0.f ? -1 : +1;
		m_pos.yFacing = m_isAttachedToSticky ? -1 : +1;

		// wrapping

		if (m_pos[0] < 0)
		{
			m_pos[0] = ARENA_SX_PIXELS;
			PlaySecondaryEffects(kPlayerEvent_ArenaWrap);
		}
		if (m_pos[0] > ARENA_SX_PIXELS)
		{
			m_pos[0] = 0;
			PlaySecondaryEffects(kPlayerEvent_ArenaWrap);
		}

	#if WRAP_AROUND_TOP_AND_BOTTOM
		if (m_pos[1] > ARENA_SY_PIXELS)
		{
			m_pos[1] = 0;
			PlaySecondaryEffects(kPlayerEvent_ArenaWrap);
		}

		if (m_pos[1] < 0)
		{
			m_pos[1] = ARENA_SY_PIXELS;
			PlaySecondaryEffects(kPlayerEvent_ArenaWrap);
		}
	#endif

		// fixme : spawn bullet on fire
		//if (!m_isGrounded && !m_isAttachedToSticky && !m_isWallSliding)
		//	g_app->netSpawnBullet(m_pos[0], m_pos[1], rand(), 500, kBulletType_A);
	}

	m_input.next();

	//printf("x: %g\n", m_pos[0]);

	m_pos.SetDirty();
}

void Player::draw()
{
	setColor(colorWhite);
	m_sprite->flipX = m_pos.xFacing < 0 ? true : false;
	m_sprite->flipY = m_pos.yFacing < 0 ? true : false;
	m_sprite->drawEx(m_pos.x, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_SY : 0), 0.f, PLAYER_SCALE);

	// render additional sprites for wrap around
	m_sprite->drawEx(m_pos.x + ARENA_SX_PIXELS, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_SY : 0), 0.f, PLAYER_SCALE);
	m_sprite->drawEx(m_pos.x - ARENA_SX_PIXELS, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_SY : 0), 0.f, PLAYER_SCALE);
	m_sprite->drawEx(m_pos.x, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_SY : 0) + ARENA_SY_PIXELS, 0.f, PLAYER_SCALE);
	m_sprite->drawEx(m_pos.x, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_SY : 0) - ARENA_SY_PIXELS, 0.f, PLAYER_SCALE);

	// draw score
	setFont("calibri.ttf");
	setColor(255, 255, 255);
	drawText(m_pos[0], m_pos[1] - 100, 20, 0.f, -1.f, "%d", m_state.score);
}

void Player::debugDraw()
{
	setColor(0, 31, 63, 63);
	drawRect(
		m_pos.x + m_collision.x1,
		m_pos.y + m_collision.y1,
		m_pos.x + m_collision.x2 + 1,
		m_pos.y + m_collision.y2 + 1);

	if (m_isAuthorative)
	{
		if (m_attack.attacking)
		{
			CollisionInfo attackCollision;
			getAttackCollision(attackCollision);

			Font font("calibri.ttf");
			setFont(font);

			setColor(colorWhite);
			drawText(m_pos[0], m_pos[1], 14, 0.f, 0.f, "attacking");

			setColor(255, 0, 0, 63);
			drawRect(
				attackCollision.x1,
				attackCollision.y1,
				attackCollision.x2,
				attackCollision.y2);
		}
	}

	setColor(colorWhite);
}

uint32_t Player::getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const
{
#if 1
	const int x1 = (x + (int)m_collision.x1) % ARENA_SX_PIXELS;
	const int y1 = (y + (int)m_collision.y1) % ARENA_SY_PIXELS;
	const int x2 = (x + (int)m_collision.x2) % ARENA_SX_PIXELS;
	const int y2 = (y + (int)m_collision.y2) % ARENA_SY_PIXELS;
#else
	const int x1 = x + m_collision.x1;
	const int y1 = y + m_collision.y1;
	const int x2 = x + m_collision.x2;
	const int y2 = y + m_collision.y2;
#endif

	uint32_t result = 0;
	
	result |= g_hostArena->getIntersectingBlocksMask(x1, y1);
	result |= g_hostArena->getIntersectingBlocksMask(x2, y1);
	result |= g_hostArena->getIntersectingBlocksMask(x2, y2);
	result |= g_hostArena->getIntersectingBlocksMask(x1, y2);

#if 0
	if (doWrap)
	{
		const int dx = x1 < 0 ? +ARENA_SX_PIXELS : x2 >= ARENA_SX_PIXELS ? -ARENA_SX_PIXELS : 0;
		const int dy = y1 < 0 ? +ARENA_SY_PIXELS : y2 >= ARENA_SY_PIXELS ? -ARENA_SY_PIXELS : 0;

		if ((dx | dy) != 0)
		{
			result |= getIntersectingBlocksMaskInternal(x + dx, y,      false);
			result |= getIntersectingBlocksMaskInternal(x,      y + dy, false);
			result |= getIntersectingBlocksMaskInternal(x + dx, y + dy, false);
		}
	}
#endif

	return result;
}

uint32_t Player::getIntersectingBlocksMask(int x, int y) const
{
	return (getIntersectingBlocksMaskInternal(x, y, true) & m_blockMask);
}

void Player::handleNewGame()
{
	m_state.score = 0;
	m_state.totalScore = 0;
	m_state.SetDirty();
}

void Player::handleNewRound()
{
	m_state.totalScore += m_state.score;
	m_state.score = 0;
	m_state.SetDirty();
}

void Player::respawn()
{
	int x, y;

	if (g_hostArena->getRandomSpawnPoint(x, y))
	{
		m_pos[0] = (float)x;
		m_pos[1] = (float)y;

		m_vel[0] = 0.f;
		m_vel[1] = 0.f;

		m_state.isAlive = true;
		m_state.SetDirty();

		m_anim.SetAnim(kPlayerAnim_Walk, false, true);

		m_attack = AttackInfo();

		m_selectedWeapon = kPlayerWeapon_Sword;

		m_blockMask = 0;

		m_isGrounded = false;
		m_isAttachedToSticky = false;
		m_isAnimDriven = false;
		m_animVelIsAbsolute = false;
		m_isAirDashCharged = false;
		m_isWallSliding = false;

		PlaySecondaryEffects(kPlayerEvent_Spawn);
	}
}

void Player::handleDamage(float amount, Vec2Arg velocity, Player * attacker)
{
	if (m_state.isAlive)
	{
		if (isAnimOverrideAllowed(kPlayerAnim_Die))
		{
			Vec2 velDelta = velocity - m_vel;

			for (int i = 0; i < 2; ++i)
			{
				if (Calc::Sign(velDelta[i]) == Calc::Sign(velocity[i]))
					m_vel[i] += velDelta[i];
			}

			m_anim.SetAnim(kPlayerAnim_Die, true, true);
			m_isAnimDriven = true;

			if (attacker)
			{
				attacker->awardScore(1);
			}
		}
	}
}

void Player::awardScore(int score)
{
	m_state.score += score;
	m_state.SetDirty();
}
