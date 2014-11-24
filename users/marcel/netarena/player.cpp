#include "arena.h"
#include "Calc.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "player.h"

void PlayerAnim_NS::SerializeStruct()
{
	Serialize(anim);

	if (IsRecv())
	{
		Player * player = static_cast<Player*>(GetOwner());

		if (anim.empty())
			player->m_sprite->stopAnim();
		else
			player->m_sprite->startAnim(anim.c_str());
	}
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
	kPlayerEvent_ArenaWrap
};

Player::Player(uint16_t owningChannelId)
	: m_pos(this)
	, m_state(this)
	, m_anim(this)
	, m_isGrounded(false)
	, m_isAttachedToSticky(false)
{
	setOwningChannelId(owningChannelId);

	m_collision.x1 = -PLAYER_COLLISION_SX / 2.f;
	m_collision.x2 = +PLAYER_COLLISION_SX / 2.f;
	m_collision.y1 = -PLAYER_COLLISION_SY;
	m_collision.y2 = 0.f;

	m_sprite = new Sprite("player-walk.png");
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
	}
}

void Player::tick(float dt)
{
	if (!m_state.isAlive || m_input.wentDown(INPUT_BUTTON_X))
	{
		int x, y;

		if (g_hostArena->getRandomSpawnPoint(x, y))
		{
			m_state.isAlive = true;
			m_state.SetDirty();

			m_pos[0] = x;
			m_pos[1] = y;

			m_vel[0] = 0.f;
			m_vel[1] = 0.f;

			m_attack = AttackInfo();

			m_isAttachedToSticky = false;

			PlaySecondaryEffects(kPlayerEvent_Spawn);
		}
	}

	if (m_state.isAlive)
	{
		float surfaceFriction = 0.f;

		// attack

		if (m_attack.attacking)
		{
			m_attack.framesLeft--;

			if (m_attack.framesLeft == 0)
			{
				m_attack.attacking = false;
			}
			else
			{
			}
		}
		else
		{
			if (m_input.wentDown(INPUT_BUTTON_Y))
			{
				m_attack.attacking = true;
				m_attack.framesLeft = 60;

				g_app->netPlaySound("player-attack-sword-0.ogg"); // todo : weapon selection, etc
			}
		}

		//

		bool playerControl = true;

		if (m_attack.attacking)
		{
			playerControl = false;
		}

		// steering

		float steeringSpeed = 0.f;
		int numSteeringFrame = 1;

		if (playerControl && m_input.isDown(INPUT_BUTTON_LEFT))
			steeringSpeed -= 1.f;
		if (playerControl && m_input.isDown(INPUT_BUTTON_RIGHT))
			steeringSpeed += 1.f;

		if (playerControl && steeringSpeed != 0.f)
		{
			if (m_anim.anim.empty())
			{
				m_anim.anim = "walk";
				m_anim.SetDirty();
			}
		}
		else
		{
			if (!m_anim.anim.empty())
			{
				m_anim.anim.clear();
				m_anim.SetDirty();
			}
		}

		if (getIntersectingBlocksMask(m_pos[0], m_pos[1] + 1.f) & kBlockMask_Solid)
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

		//

		if (getIntersectingBlocksMask(m_pos[0], m_pos[1] - 1.f) & (1 << kBlockType_Sticky))
		{
			// sticky ceiling

			if (playerControl && m_input.wentDown(INPUT_BUTTON_A))
			{
				m_vel[1] = PLAYER_JUMP_SPEED / 2.f;

				PlaySecondaryEffects(kPlayerEvent_StickyJump);
			}
			else if (playerControl && m_input.wentDown(INPUT_BUTTON_DOWN))
			{
				m_vel[1] += GRAVITY * dt;

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

			m_vel[1] += GRAVITY * dt;
		}

		// collision

		for (int i = 0; i < 2; ++i)
		{
			float totalDelta = m_vel[i] * dt;

			const float deltaSign = totalDelta < 0.f ? -1.f : +1.f;

			while (totalDelta != 0.f)
			{
				const float delta = (std::abs(totalDelta) < 1.f) ? totalDelta : deltaSign;

				Vec2 newPos(m_pos.x, m_pos.y);

				newPos[i] += delta;

				const uint32_t newBlockMask = getIntersectingBlocksMask(newPos[0], newPos[1]);

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
						}
					}

					if (i == 1)
					{
						// grounded

						surfaceFriction = FRICTION_GROUNDED;

						if (m_vel[i] >= 0.f)
						{
							if (playerControl && m_input.wentDown(INPUT_BUTTON_A))
							{
								// jumping

								m_vel[i] = -PLAYER_JUMP_SPEED;

								PlaySecondaryEffects(kPlayerEvent_Jump);
							}
							else if (playerControl && newBlockMask & (1 << kBlockType_Spring))
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

				if (newBlockMask & (1 << kBlockType_Spike))
				{
					// todo : die

					PlaySecondaryEffects(kPlayerEvent_SpikeHit);
				}
			}
		}

		// grounded?

		if (getIntersectingBlocksMask(m_pos[0], m_pos[1] + 1.f) & kBlockMask_Solid)
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

		// facing

		if (playerControl && steeringSpeed != 0.f)
			m_pos.xFacing = steeringSpeed < 0.f ? -1 : +1;
		m_pos.yFacing = m_isAttachedToSticky ? -1 : +1;

		// wrapping

		if (m_pos[0] < 0)
			m_pos[0] = ARENA_SX * BLOCK_SX;
		if (m_pos[0] > ARENA_SX * BLOCK_SX)
			m_pos[0] = 0;
	}

	m_input.next();

	//printf("x: %g\n", m_pos[0]);

	m_pos.SetDirty();
}

void Player::draw()
{
	gxSetTexture(0);
	setColor(0, 255, 127, 127);
	drawRect(
		m_pos.x + m_collision.x1,
		m_pos.y + m_collision.y1,
		m_pos.x + m_collision.x2 + 1,
		m_pos.y + m_collision.y2 + 1);

	setColor(colorWhite);
	m_sprite->flipX = m_pos.xFacing < 0 ? false: true;
	m_sprite->flipY = m_pos.yFacing < 0 ? true : false;
	m_sprite->drawEx(m_pos.x, m_pos.y - (m_sprite->flipY ? PLAYER_COLLISION_SY : 0), 0.f, 3.f);
}

uint32_t Player::getIntersectingBlocksMask(float x, float y) const
{
	return g_hostArena->getIntersectingBlocksMask(
		x + m_collision.x1,
		y + m_collision.y1,
		x + m_collision.x2,
		y + m_collision.y2);
}
