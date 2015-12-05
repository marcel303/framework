#include "bullet.h"
#include "Calc.h"
#include "client.h"
#include "Debugging.h"
#include "FileStream.h"
#include "framework.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "host.h"
#include "lobbymenu.h"
#include "Log.h"
#include "main.h"
#include "Parse.h"
#include "Path.h"
#include "player.h"
#include "StreamReader.h"
#include "Timer.h"

#include "BinaryDiff.h" // fixme : remove

#include <tinyxml2.h>
using namespace tinyxml2;

//#pragma optimize("", off)

OPTION_DECLARE(bool, g_checkCRCs, true);
OPTION_DEFINE(bool, g_checkCRCs, "App/Check CRCs");
OPTION_ALIAS(g_checkCRCs, "checkcrc");

OPTION_DECLARE(bool, g_noSound, false);
OPTION_DEFINE(bool, g_noSound, "Sound/Disable Sound Effects");
OPTION_ALIAS(g_noSound, "nosound");

OPTION_DECLARE(int, g_gameModeNextRound, -1);
OPTION_DEFINE(int, g_gameModeNextRound, "Game State/Game Mode Next Round");
OPTION_STEP(g_gameModeNextRound, -1, kGameMode_COUNT - 1, 1);
OPTION_ALIAS(g_gameModeNextRound, "gamemode");
OPTION_VALUE_ALIAS(g_gameModeNextRound, deathmatch, kGameMode_DeathMatch);
OPTION_VALUE_ALIAS(g_gameModeNextRound, tokenhunt, kGameMode_TokenHunt);
OPTION_VALUE_ALIAS(g_gameModeNextRound, coincollector, kGameMode_CoinCollector);

OPTION_DECLARE(bool, g_autoReadyUp, false);
OPTION_DEFINE(bool, g_autoReadyUp, "App/Auto Ready Up");
OPTION_ALIAS(g_autoReadyUp, "readyup");

OPTION_DECLARE(int, s_pickupTest, -1);
OPTION_DEFINE(int, s_pickupTest, "Pickups/Debug Spawn Type");
OPTION_ALIAS(s_pickupTest, "pickuptest");

OPTION_EXTERN(std::string, g_map);

extern std::vector<std::string> g_mapRotationList;

//

void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

GameSim * g_gameSim = 0;

//

struct PickupSprite
{
	bool isSpriter;
	const char * filename;
} s_pickupSprites[kPickupType_COUNT] =
{
	true,  "objects/pickups/deathray/deathray.scml",//false, "pickup-ammo.png",
	true,  "objects/pickups/bomb/bomb.scml",//false, "pickup-nade.png"
	true,  "objects/pickups/shield/shield.scml",//false, "pickup-shield.png",
	true,  "objects/pickups/freezeray/freezeray.scml",
	true,  "objects/pickups/bubble/bubble.scml",//false, "pickup-bubble.png",
	true,  "objects/pickups/time/time.scml"//false, "pickup-time.png"
};

#define TOKEN_SPRITER "objects/token/Token.scml"
#define TOKEN_SX 38
#define TOKEN_SY 38
#define COIN_SPRITE "coin.png"
#define AXE_SPRITER Spriter("objects/axe/sprite.scml")
#define PIPEBOMB_SPRITER Spriter("objects/pipebomb/sprite.scml")
#define FOOTBALL_SPRITER Spriter("objects/football/sprite.scml")

//

uint32_t GameStateData::Random()
{
	m_randomSeed += 1;
	m_randomSeed *= 16807;
	m_randomSeed = ~m_randomSeed;
	m_randomSeed -= 1;
	return m_randomSeed;
}

uint32_t GameStateData::GetTick() const
{
	return m_tick;
}

float GameStateData::getRoundTime() const
{
	return m_roundTime;
}

void GameStateData::addTimeDilationEffect(float multiplier1, float multiplier2, float duration)
{
	int minIndex = 0;

	for (int i = 1; i < MAX_TIMEDILATION_EFFECTS; ++i)
	{
		if (m_timeDilationEffects[i].ticksRemaining < m_timeDilationEffects[minIndex].ticksRemaining)
			minIndex = i;
	}

	m_timeDilationEffects[minIndex].multiplier1 = multiplier1;
	m_timeDilationEffects[minIndex].multiplier2 = multiplier2;
	m_timeDilationEffects[minIndex].ticks = duration * TICKS_PER_SECOND;
	m_timeDilationEffects[minIndex].ticksRemaining = m_timeDilationEffects[minIndex].ticks;
}

LevelEvent GameStateData::getRandomLevelEvent()
{
	LevelEvent e = (LevelEvent)(Random() % kLevelEvent_COUNT);

	if ((DEMOMODE || RECORDMODE) && e == kLevelEvent_GravityWell)
		return getRandomLevelEvent();
	else
		return e;
}

//

void Pickup::setup(PickupType type, float x, float y)
{
	int spriteSx;
	int spriteSy;

	if (s_pickupSprites[type].isSpriter)
	{
		spriteSx = PICKUP_SPRITE_SX;
		spriteSy = PICKUP_SPRITE_SY;
	}
	else
	{
		const char * filename = s_pickupSprites[type].filename;

		Sprite sprite(filename);
		spriteSx = sprite.getWidth();
		spriteSy = sprite.getHeight();
	}

	m_pickupType = type;

	//

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_isActive = true;
	m_type = kObjectType_Pickup;
	m_pos.Set(x, y);
	m_vel.Set(0.f, 0.f);

	m_collisionShape.set(
		Vec2(-spriteSx / 2.f, -spriteSy / 2.f),
		Vec2(+spriteSx / 2.f, -spriteSy / 2.f),
		Vec2(+spriteSx / 2.f, +spriteSy / 2.f),
		Vec2(-spriteSx / 2.f, +spriteSy / 2.f));

	m_doTeleport = true;
	m_bounciness = .25f;
	m_friction = .1f;
	m_airFriction = .5f;
}

void Pickup::tick(GameSim & gameSim, float dt)
{
	PhysicsActorCBs cbs;
	cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
	{
		if (std::abs(actor.m_vel[cbs.axis]) >= TOKEN_BOUNCE_SOUND_TRESHOLD) // fixme : add gamedef
			g_gameSim->playSound("pickup-bounce.ogg");
	};

	PhysicsActor::tick(gameSim, dt, cbs);
}

void Pickup::draw(const GameSim & gameSim) const
{
	gpuTimingBlock(pickupDraw);
	cpuTimingBlock(pickupDraw);

	if (s_pickupSprites[m_pickupType].isSpriter)
	{
		const char * filename = s_pickupSprites[m_pickupType].filename;

		Spriter spriter(filename);

		SpriterState state;
		state.startAnim(spriter, 0);
		state.animTime = gameSim.m_roundTime;
		state.x = m_pos[0];
		state.y = m_pos[1] + PICKUP_SPRITE_SY / 2.f;

		setColor(colorWhite);
		spriter.draw(state);
	}
	else
	{
		Vec2 min, max;
		getAABB(min, max);

		const char * filename = s_pickupSprites[m_pickupType].filename;

		Sprite sprite(filename);

		setColor(colorWhite);
		sprite.drawEx(min[0], min[1]);
	}

	if (g_devMode)
	{
		drawBB();
	}
}

void Pickup::drawLight() const
{
	setColor(colorWhite);
	Sprite("player-light.png").drawEx(m_pos[0], m_pos[1] + PICKUP_LIGHT_OFFSET_Y, 0.f, 1.f, 1.f, false, FILTER_MIPMAP);
}

//

void Token::setup(int blockX, int blockY)
{
	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_isActive = true;
	m_type = kObjectType_Token;
	m_collisionShape.set(
		Vec2(-TOKEN_SX / 2.f, -TOKEN_SY / 2.f),
		Vec2(+TOKEN_SX / 2.f, -TOKEN_SY / 2.f),
		Vec2(+TOKEN_SX / 2.f, +TOKEN_SY / 2.f),
		Vec2(-TOKEN_SX / 2.f, +TOKEN_SY / 2.f));
	m_pos.Set(
		(blockX + .5f) * BLOCK_SX,
		(blockY + .5f) * BLOCK_SY);
	m_vel.Set(0.f, 0.f);
	m_doTeleport = true;
	m_bounciness = TOKEN_BOUNCINESS;
	m_noGravity = false;
	m_friction = 0.1f;
	m_airFriction = 0.9f;

	m_isDropped = true;
	m_dropTimer = 0.f;
}

void Token::tick(GameSim & gameSim, float dt)
{
	if (m_isDropped)
	{
		PhysicsActorCBs cbs;
		cbs.onBlockMask = [](PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask) 
		{
			if (blockMask & kBlockMask_Spike)
			{
				if (actor.m_vel[1] > 0.f)
				{
					actor.m_vel.Set(g_gameSim->RandomFloat(-TOKEN_FLEE_SPEED, +TOKEN_FLEE_SPEED), -TOKEN_FLEE_SPEED);
					g_gameSim->playSound("token-bounce.ogg");
				}
			}
			return false;
		};
		cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
		{
			if (std::abs(actor.m_vel[cbs.axis]) >= TOKEN_BOUNCE_SOUND_TRESHOLD)
				g_gameSim->playSound("token-bounce.ogg");
		};

		PhysicsActor::tick(gameSim, dt, cbs);

		m_dropTimer -= dt;
	}
}

void Token::draw() const
{
	if (m_isDropped)
	{
		setColor(colorWhite);
		Spriter spriter(TOKEN_SPRITER);
		SpriterState spriterState;
		spriterState.startAnim(spriter, 0);
		spriterState.x = m_pos[0];
		spriterState.y = m_pos[1];
		spriter.draw(spriterState);
	}

	if (g_devMode)
	{
		drawBB();
	}
}

void Token::drawLight() const
{
	if (m_isDropped)
	{
		setColor(colorWhite);
		Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f, 1.5f, false, FILTER_MIPMAP);
	}
}

//

void FootBall::setup(int x, int y)
{
	const int kRadius = 50;

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_isActive = true;
	m_type = kObjectType_FootBall;
	m_collisionShape.setCircle(Vec2(0.f, 0.f), kRadius);
	m_pos.Set(x, y);
	m_vel.Set(0.f, 0.f);
	m_doTeleport = true;
	m_bounciness = FOOTBALL_BOUNCINESS;
	m_noGravity = false;
	m_friction = 0.1f;
	m_airFriction = 0.9f;

	m_hasBeenTouched = false;
	m_isDropped = true;
	m_spriterState = SpriterState();
	m_spriterState.startAnim(FOOTBALL_SPRITER, "idle");
}

void FootBall::tick(GameSim & gameSim, float dt)
{
	if (m_isDropped)
	{
		if ((gameSim.m_tick % 120) == 0)
		{
			m_vel[0] = gameSim.RandomFloat(-500.f, +500.f);
			m_vel[1] = gameSim.RandomFloat(-1000.f, 0.f);
		}

		PhysicsActorCBs cbs;
		cbs.onBlockMask = [](PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask) 
		{
			if (blockMask & kBlockMask_Spike)
			{
				if (actor.m_vel[1] > 0.f)
				{
					actor.m_vel.Set(g_gameSim->RandomFloat(-FOOTBALL_FLEE_SPEED, +FOOTBALL_FLEE_SPEED), -FOOTBALL_FLEE_SPEED);
					g_gameSim->playSound("football-bounce.ogg");
				}
			}
			return false;
		};
		cbs.onHitPlayer = [](PhysicsActorCBs & cbs, PhysicsActor & actor, Player & player)
		{
			FootBall * self = static_cast<FootBall*>(&actor);
			self->m_hasBeenTouched = true;
			return false;
		};
		cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
		{
			//if (std::abs(actor.m_vel[1]) >= FOOTBALL_BOUNCE_SOUND_TRESHOLD)
			//	g_gameSim->playSound("football-bounce.ogg");
		};

		m_noGravity = !m_hasBeenTouched;

		PhysicsActor::tick(gameSim, dt, cbs);

		m_spriterState.updateAnim(FOOTBALL_SPRITER, dt);
	}
}

void FootBall::draw() const
{
	for (int ox = -1; ox <= +1; ++ox)
	{
		for (int oy = -1; oy <= +1; ++oy)
		{
			gxPushMatrix();
			gxTranslatef(ox * ARENA_SX_PIXELS, oy * ARENA_SY_PIXELS, 0.f);

			if (m_isDropped)
			{
				setColor(colorWhite);

				SpriterState spriterState = m_spriterState;
				spriterState.x = m_pos[0];
				spriterState.y = m_pos[1];
				FOOTBALL_SPRITER.draw(spriterState);
			}

			if (g_devMode)
			{
				drawBB();
			}

			gxPopMatrix();
		}
	}
}

void FootBall::drawLight() const
{
	for (int ox = -1; ox <= +1; ++ox)
	{
		for (int oy = -1; oy <= +1; ++oy)
		{
			gxPushMatrix();
			gxTranslatef(ox * ARENA_SX_PIXELS, oy * ARENA_SY_PIXELS, 0.f);

			if (m_isDropped)
			{
				setColor(colorWhite);
				Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f, 1.5f, false, FILTER_MIPMAP);
			}

			gxPopMatrix();
		}
	}
}

//

void FootBallGoal::setup(int x, int y)
{
	m_isActive = true;
}

void FootBallGoal::tick(GameSim & gameSim, float dt)
{
}

void FootBallGoal::draw() const
{
}

void FootBallGoal::drawLight() const
{
}

//

void Coin::setup(int blockX, int blockY)
{
	Sprite sprite(COIN_SPRITE);

	*static_cast<PhysicsActor*>(this) = PhysicsActor();

	m_isActive = true;
	m_type = kObjectType_Coin;
	m_collisionShape.set(
		Vec2(-sprite.getWidth() / 2.f, -sprite.getHeight() / 2.f),
		Vec2(+sprite.getWidth() / 2.f, -sprite.getHeight() / 2.f),
		Vec2(+sprite.getWidth() / 2.f, +sprite.getHeight() / 2.f),
		Vec2(-sprite.getWidth() / 2.f, +sprite.getHeight() / 2.f));
	m_pos.Set(
		(blockX + .5f) * BLOCK_SX,
		(blockY + .5f) * BLOCK_SY);
	m_vel.Set(0.f, 0.f);
	m_doTeleport = true;
	m_bounciness = TOKEN_BOUNCINESS;
	m_noGravity = false;
	m_friction = 0.1f;
	m_airFriction = 0.9f;

	m_isDropped = true;
	m_dropTimer = 0.f;
}

void Coin::tick(GameSim & gameSim, float dt)
{
	if (m_isDropped)
	{
		PhysicsActorCBs cbs;
		cbs.onBlockMask = [](PhysicsActorCBs & cbs, PhysicsActor & actor, uint32_t blockMask) 
		{
			if (blockMask & kBlockMask_Spike)
			{
				if (actor.m_vel[1] > 0.f)
				{
					actor.m_vel.Set(g_gameSim->RandomFloat(-COIN_FLEE_SPEED, +COIN_FLEE_SPEED), -COIN_FLEE_SPEED);
					g_gameSim->playSound("coin-bounce.ogg");
				}
			}
			return false;
		};
		cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
		{
			if (std::abs(actor.m_vel[1]) >= COIN_BOUNCE_SOUND_TRESHOLD)
				g_gameSim->playSound("coin-bounce.ogg");
		};

		PhysicsActor::tick(gameSim, dt, cbs);

		m_dropTimer -= dt;
	}
}

void Coin::draw() const
{
	if (m_isDropped)
	{
		setColor(colorWhite);
		Sprite(COIN_SPRITE).drawEx(m_pos[0], m_pos[1]);
	}

	if (g_devMode)
	{
		drawBB();
	}
}

void Coin::drawLight() const
{
	if (m_isDropped)
	{
		setColor(colorWhite);
		Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f, 1.5f, false, FILTER_MIPMAP);
	}
}

//

void Mover::setSprite(const char * filename)
{
	m_sprite = filename;

	Sprite sprite(m_sprite.c_str());
	m_sx = sprite.getWidth();
	m_sy = sprite.getHeight();
}

void Mover::tick(GameSim & gameSim, float dt)
{
	if (m_moveMultiplier == 0.f)
	{
		const int dx = m_x2 - m_x1;
		const int dy = m_y2 - m_y1;

		m_moveMultiplier = m_speed / std::sqrtf(dx * dx + dy * dy) / 2.f;
	}

	m_moveAmount = std::fmodf(m_moveAmount + m_moveMultiplier * dt, 1.f);

	CollisionInfo collision;
	getCollisionInfo(collision);

	const Vec2 pos = getPosition();
	const Vec2 speed = getSpeed();

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = gameSim.m_players[i];

		if (player.m_isUsed && player.m_isAlive)
		{
			bool grounded = false;

			CollisionInfo playerCollision;

			if (player.m_isGrounded && player.m_vel[1] >= 0.f)
			{
				player.getPlayerCollision(playerCollision);
				playerCollision.min[1] += 2.f;
				playerCollision.max[1] += 2.f;

				if (playerCollision.intersects(collision))
				{
					player.m_pos += speed * dt;

					grounded = true;
				}
			}

			player.getPlayerCollision(playerCollision);

			if (playerCollision.intersects(collision))
			{
				const float d = + playerCollision.max[1] - collision.min[1];

				if (d >= 0.f && grounded)
				{
					player.m_pos[1] -= d + 1.f;
				}
			}

			if (grounded)
			{
				for (int offset = 1; offset < 4; ++offset)
				{
					if (player.getIntersectingBlocksMask(player.m_pos[0], player.m_pos[1] + offset) & kBlockMask_Solid)
					{
						if (offset != 0)
						{
							player.m_pos[1] += offset - 1.f;
							Assert((player.getIntersectingBlocksMask(player.m_pos[0], player.m_pos[1] + 1.f) & kBlockMask_Solid) != 0);
						}
						break;
					}
				}
			}

			//Assert((player.getIntersectingBlocksMask(player.m_pos[0], player.m_pos[1]) & kBlockMask_Solid) == 0);
		}
	}
}

void Mover::draw() const
{
	CollisionInfo collision;
	getCollisionInfo(collision);

	setColor(colorWhite);
	drawRect(collision.min[0], collision.min[1], collision.max[0], collision.max[1]);
}

void Mover::drawLight() const
{
	setColor(colorWhite);
}

Vec2 Mover::getPosition() const
{
	const float t = m_moveAmount < .5f ? m_moveAmount * 2.f : (1.f - m_moveAmount) * 2.f;
	const float x = Calc::Lerp(m_x1, m_x2, t);
	const float y = Calc::Lerp(m_y1, m_y2, t);

	return Vec2(x, y);
}

Vec2 Mover::getSpeed() const
{
	const float t = m_moveAmount < .5f ? +2.f : -2.f;
	const float dx = m_x2 - m_x1;
	const float dy = m_y2 - m_y1;

	return Vec2(
		dx * t * m_moveMultiplier,
		dy * t * m_moveMultiplier);
}

void Mover::getCollisionInfo(CollisionInfo & collisionInfo) const
{
	const Vec2 pos = getPosition();

	collisionInfo.min[0] = pos[0] - m_sx / 2.f;
	collisionInfo.min[1] = pos[1] - m_sy / 2.f;
	collisionInfo.max[0] = pos[0] + m_sx / 2.f;
	collisionInfo.max[1] = pos[1] + m_sy / 2.f;
}

bool Mover::intersects(CollisionInfo & collisionInfo) const
{
	CollisionInfo myCollision;
	getCollisionInfo(myCollision);

	return myCollision.intersects(collisionInfo);
}

//

void Axe::setup(Vec2Arg pos, Vec2Arg vel, int playerIndex)
{
	*static_cast<PhysicsActor*>(this) = PhysicsActor();
	m_spriterState = SpriterState();

	m_isActive = true;
	m_type = kObjectType_Axe;
	m_collisionShape.set(
		Vec2(-AXE_COLLISION_SX / 2.f, -AXE_COLLISION_SY / 2.f),
		Vec2(+AXE_COLLISION_SX / 2.f, -AXE_COLLISION_SY / 2.f),
		Vec2(+AXE_COLLISION_SX / 2.f, +AXE_COLLISION_SY / 2.f),
		Vec2(-AXE_COLLISION_SX / 2.f, +AXE_COLLISION_SY / 2.f));
	m_pos = pos;
	m_vel = vel;
	m_doTeleport = true;
	m_bounciness = 0.2f;
	m_noGravity = false;
	m_gravityMultiplier = AXE_GRAVITY_MULTIPLIER_ACTIVE;
	m_friction = 0.01f;
	m_airFriction = 0.9f;

	m_playerIndex = playerIndex;
	m_throwDone = false;
	m_travelTime = AXE_THROW_TIME;

	g_gameSim->playSound("objects/axe/activate.ogg");
	m_spriterState.startAnim(AXE_SPRITER, "active");
}

void Axe::tick(GameSim & gameSim, float dt)
{
	PhysicsActorCBs cbs;
	cbs.userData = &gameSim;
	cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
	{
		GameSim * gameSim = (GameSim*)cbs.userData;
		Axe & self = static_cast<Axe&>(actor);

		if (!self.m_throwDone)
		{
			self.endThrow();
		}

		if (std::abs(actor.m_vel[cbs.axis]) >= TOKEN_BOUNCE_SOUND_TRESHOLD) // fixme : add gamedef
			g_gameSim->playSound("objects/axe/bounce.ogg");
	};
	cbs.onHitPlayer = [](PhysicsActorCBs & cbs, PhysicsActor & actor, Player & player)
	{
		GameSim * gameSim = (GameSim*)cbs.userData;
		Axe & self = static_cast<Axe&>(actor);

		if (!self.m_hasLanded)
		{
			// filter collision with owning player
			if (player.m_index == self.m_playerIndex)
				return false;

			player.handleDamage(1.f, self.m_vel, (self.m_playerIndex == -1) ? 0 : &gameSim->m_players[self.m_playerIndex]);
		}

		return false;
	};

	PhysicsActor::tick(gameSim, dt, cbs);

	//

	if (!m_throwDone && m_travelTime > 0.f)
	{
		m_travelTime -= dt;

		if (m_travelTime < 0.f)
		{
			m_travelTime = 0.f;
			endThrow();
		}
	}

	//

	if (!m_hasLanded && m_throwDone)
	{
		m_hasLanded = true;

		m_fadeTime = AXE_FADE_TIME;

		if (m_playerIndex != -1 && gameSim.m_players[m_playerIndex].m_axe.recoveryTime == 0.f)
		{
			gameSim.m_players[m_playerIndex].m_axe.recoveryTime = AXE_FADE_TIME;
		}
	}

	//

	if (m_hasLanded)
	{
		m_fadeTime -= dt;

		if (m_fadeTime <= 0.f)
		{
			*this = Axe();
		}
	}

	if (m_vel[0] != 0.f)
	{
		m_spriterState.flipX = m_vel[0] < 0.f;
	}

	if (!m_throwDone)
	{
		m_spriterState.angle += AXE_ROTATION_SPEED * dt;
	}

	if (m_spriterState.animIsActive)
	{
		m_spriterState.updateAnim(AXE_SPRITER, dt);
	}
}

void Axe::endThrow()
{
	m_throwDone = true;
	m_vel *= AXE_SPEED_MULTIPLIER_ON_DIE;
	m_gravityMultiplier = AXE_GRAVITY_MULTIPLIER_INACTIVE;

	g_gameSim->playSound("objects/axe/deactivate.ogg");
	m_spriterState.startAnim(AXE_SPRITER, "inactive");
}

void Axe::draw() const
{
	SpriterState state = m_spriterState;
	state.x = m_pos[0];
	state.y = m_pos[1];
	setColor(colorWhite);
	AXE_SPRITER.draw(state);

	if (g_devMode)
	{
		Vec2 min, max;
		getAABB(min, max);

		setColor(m_hasLanded ? 0 : 255, m_hasLanded ? 255 : 0, 0, 63);
		drawRectLine(
			min[0],
			min[1],
			max[0],
			max[1]);
	}
}

void Axe::drawLight() const
{
	setColor(colorWhite);
	Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f, 1.5f, false, FILTER_MIPMAP);
}

//

void PipeBomb::setup(Vec2Arg pos, Vec2Arg vel, int playerIndex)
{
	*static_cast<PhysicsActor*>(this) = PhysicsActor();
	m_spriterState = SpriterState();

	m_isActive = true;
	m_type = kObjectType_PipeBomb;
	m_collisionShape.set(
		Vec2(-PIPEBOMB_COLLISION_SX / 2.f, -PIPEBOMB_COLLISION_SY / 2.f),
		Vec2(+PIPEBOMB_COLLISION_SX / 2.f, -PIPEBOMB_COLLISION_SY / 2.f),
		Vec2(+PIPEBOMB_COLLISION_SX / 2.f, +PIPEBOMB_COLLISION_SY / 2.f),
		Vec2(-PIPEBOMB_COLLISION_SX / 2.f, +PIPEBOMB_COLLISION_SY / 2.f));
	m_pos = pos + Vec2(0.f, -1.f);
	m_vel = vel;
	m_doTeleport = true;
	m_bounciness = 0.f;
	m_noGravity = false;
	m_friction = 0.1f;
	m_airFriction = 0.9f;

	m_playerIndex = playerIndex;
	m_exploded = false;
	m_activationTime = PIPEBOMB_ACTIVATION_TIME;

	m_spriterState.startAnim(PIPEBOMB_SPRITER, "thrown");
}

void PipeBomb::tick(GameSim & gameSim, float dt)
{
	if (m_activationTime > 0.f)
	{
		m_activationTime -= dt;
		if (m_activationTime < 0.f)
		{
			m_activationTime = 0.f;
			gameSim.playSound("objects/pipebomb/activate.ogg");
			m_spriterState.startAnim(PIPEBOMB_SPRITER, "activated");
		}
	}

	PhysicsActorCBs cbs;
	cbs.userData = &gameSim;
	cbs.onBounce = [](PhysicsActorCBs & cbs, PhysicsActor & actor)
	{
		GameSim * gameSim = (GameSim*)cbs.userData;
		PipeBomb & self = static_cast<PipeBomb&>(actor);

		if (!self.m_hasLanded && cbs.axis == 1)
		{
			self.m_hasLanded = true;
			self.m_vel.Set(0.f, 0.f);

			gameSim->playSound("objects/pipebomb/land.ogg");
			self.m_spriterState.startAnim(PIPEBOMB_SPRITER, "landed");
		}
	};
	cbs.onHitPlayer = [](PhysicsActorCBs & cbs, PhysicsActor & actor, Player & player)
	{
		PipeBomb & self = static_cast<PipeBomb&>(actor);

		// filter collision with owning player
		if (player.m_index == self.m_playerIndex)
			return false;

		self.explode();

		return false;
	};

	PhysicsActor::tick(gameSim, dt, cbs);

	if (m_spriterState.animIsActive)
	{
		m_spriterState.updateAnim(PIPEBOMB_SPRITER, dt);
	}

	if (m_exploded)
	{
		gameSim.playSound("objects/pipebomb/explode.ogg");
		gameSim.addAnimationFx(kDrawLayer_Game, "fx/PipeBomb_Explode.scml", m_pos[0], m_pos[1]);
		gameSim.doBlastEffect(
			m_pos,
			PIPEBOMB_BLAST_RADIUS,
			pipebombBlastCurve);

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (gameSim.m_players[i].m_isUsed && gameSim.m_players[i].m_isAlive)
			{
				if (i != m_playerIndex)
				{
					const Vec2 delta = gameSim.m_players[i].m_pos - m_pos;
					const float distance = delta.CalcSize() + 0.001f;
					if (distance <= PIPEBOMB_BLAST_RADIUS)
					{
						const float d = distance / PIPEBOMB_BLAST_RADIUS;
						const float s = Calc::Lerp(PIPEBOMB_BLAST_STRENGTH_NEAR, PIPEBOMB_BLAST_STRENGTH_FAR, d);
						const Vec2 dir = delta.CalcNormalized();
						gameSim.m_players[i].handleDamage(1.f, dir * s, (m_playerIndex == -1) ? 0 : &gameSim.m_players[m_playerIndex]);
					}
				}
			}
		}

		*this = PipeBomb();
	}
}

void PipeBomb::draw() const
{
	SpriterState state = m_spriterState;
	state.x = m_pos[0];
	state.y = m_pos[1];
	setColor(colorWhite);
	PIPEBOMB_SPRITER.draw(state);

	if (g_devMode)
	{
		drawBB();
		setColor(0, 255, 0, 63);
		drawRectLine(
			m_pos[0] - PIPEBOMB_BLAST_RADIUS,
			m_pos[1] - PIPEBOMB_BLAST_RADIUS,
			m_pos[0] + PIPEBOMB_BLAST_RADIUS,
			m_pos[1] + PIPEBOMB_BLAST_RADIUS);
	}
}

void PipeBomb::drawLight() const
{
	setColor(colorWhite);
	Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f, 1.5f, false, FILTER_MIPMAP);
}

bool PipeBomb::explode()
{
	if (!m_exploded && m_activationTime == 0.f)
	{
		logDebug("PipeBomb::explode");

		m_exploded = true;

		return true;
	}

	return false;
}

//

void Barrel::setup(Vec2Arg pos)
{
}

void Barrel::tick(GameSim & gameSim, float dt)
{
	// todo : check if we are hit by a player using melee, if > GFX_SY, disable
}

void Barrel::draw() const
{
	if (g_devMode)
	{
		drawBB();
	}
}

void Barrel::drawLight() const
{
	setColor(colorWhite);
}

//

void Light::setup(float x, float y, const Color & color)
{
	m_isActive = true;
	m_pos.Set(x, y);
	m_color[0] = color.r;
	m_color[1] = color.g;
	m_color[2] = color.b;
	m_color[3] = color.a;
}

void Light::tick(GameSim & gameSim, float dt)
{
}

void Light::draw() const
{
}

void Light::drawLight() const
{
	float a = 0.f;
	a += std::sin(g_gameSim->getRoundTime() * Calc::m2PI * LIGHT_FLICKER_FREQ_A);
	a += std::sin(g_gameSim->getRoundTime() * Calc::m2PI * LIGHT_FLICKER_FREQ_B);
	a += std::sin(g_gameSim->getRoundTime() * Calc::m2PI * LIGHT_FLICKER_FREQ_C);
	a = (a + 3.f) / 6.f;
	a = Calc::Lerp(1.f, a, LIGHT_FLICKER_STRENGTH);

	Color color;
	color.r = m_color[0];
	color.g = m_color[1];
	color.b = m_color[2];
	color.a = a;
	setColor(color);

	Sprite("player-light.png").drawEx(m_pos[0], m_pos[1], 0.f, 1.5f, 1.5f, false, FILTER_MIPMAP);
}

//

void ParticleEffect::setup(const char * filename, int x, int y)
{
	XMLDocument d;

	if (d.LoadFile(filename) == XML_NO_ERROR)
	{
		for (int i = 0; i < kMaxParticleSystems; ++i)
		{
			m_system[i] = ParticleSystem();
		}

		int emitterInfoIdx = 0;
		for (XMLElement * emitterElem = d.FirstChildElement("emitter"); emitterElem; emitterElem = emitterElem->NextSiblingElement("emitter"))
		{
			if (emitterInfoIdx < kMaxParticleSystems)
				m_system[emitterInfoIdx++].emitterInfo.load(emitterElem);
		}

		int particleInfoIdx = 0;
		for (XMLElement * particleElem = d.FirstChildElement("particle"); particleElem; particleElem = particleElem->NextSiblingElement("particle"))
		{
			if (particleInfoIdx < kMaxParticleSystems)
				m_system[particleInfoIdx++].particleInfo.load(particleElem);
		}

		m_data.m_isActive = true;
		m_data.m_filename = filename;
		m_data.m_x = x;
		m_data.m_y = y;
	}
}

void ParticleEffect::draw()
{
	gxPushMatrix();
	{
		gxTranslatef(m_data.m_x, m_data.m_y, 0.f);

		for (int i = 0; i < kMaxParticleSystems; ++i)
		{
			ParticleSystem & ps = m_system[i];

			gxSetTexture(Sprite(ps.emitterInfo.materialName).getTexture());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			if (ps.particleInfo.blendMode == ParticleInfo::kBlendMode_AlphaBlended)
				setBlend(BLEND_ALPHA);
			else if (ps.particleInfo.blendMode == ParticleInfo::kBlendMode_Additive)
				setBlend(BLEND_ADD);
			else
				fassert(false);

			gxBegin(GL_QUADS);
			{
				for (Particle * p = (ps.particleInfo.sortMode == ParticleInfo::kSortMode_OldestFirst) ? ps.pool.head : ps.pool.tail;
								p; p = (ps.particleInfo.sortMode == ParticleInfo::kSortMode_OldestFirst) ? p->next : p->prev)
				{
					const float particleLife = 1.f - p->life;
					//const float particleSpeed = std::sqrtf(p->speed[0] * p->speed[0] + p->speed[1] * p->speed[1]);
					const float particleSpeed = p->speedScalar;

					ParticleColor color(true);
					computeParticleColor(ps.emitterInfo, ps.particleInfo, particleLife, particleSpeed, color);
					const float size_div_2 = computeParticleSize(ps.emitterInfo, ps.particleInfo, particleLife, particleSpeed) / 2.f;

					const float s = std::sinf(-p->rotation * float(M_PI) / 180.f);
					const float c = std::cosf(-p->rotation * float(M_PI) / 180.f);

					gxColor4fv(color.rgba);
					gxTexCoord2f(0.f, 1.f); gxVertex2f(p->position[0] + (- c - s) * size_div_2, p->position[1] + (+ s - c) * size_div_2);
					gxTexCoord2f(1.f, 1.f); gxVertex2f(p->position[0] + (+ c - s) * size_div_2, p->position[1] + (- s - c) * size_div_2);
					gxTexCoord2f(1.f, 0.f); gxVertex2f(p->position[0] + (+ c + s) * size_div_2, p->position[1] + (- s + c) * size_div_2);
					gxTexCoord2f(0.f, 0.f); gxVertex2f(p->position[0] + (- c + s) * size_div_2, p->position[1] + (+ s + c) * size_div_2);
				}
			}
			gxEnd();

			gxSetTexture(0);
		}
	}
	gxPopMatrix();
}

//

static const float kPortalSafeZoneSize = 10.f;

void Portal::setup(float x1, float y1, float x2, float y2, int key)
{
	m_isActive = true;

	Assert(x1 < x2);
	Assert(y1 < y2);
	m_x1 = x1;
	m_y1 = y1;
	m_x2 = x2;
	m_y2 = y2;

	m_key = key;
}

bool Portal::intersects(float x1, float y1, float x2, float y2, bool mustEncapsulate, bool applySafeZone) const
{
	if (mustEncapsulate)
	{
		if (m_x1 >= m_x2 || m_y1 >= m_y2)
			return false;

		const float safeZone = applySafeZone ? kPortalSafeZoneSize : 0.f;

		if (x1 < m_x1 - safeZone || x2 > m_x2 + safeZone)
			return false;
		if (y1 < m_y1 - safeZone || y2 > m_y2 + safeZone)
			return false;
		return true;
	}
	else
	{
		if (m_x1 >= m_x2 || m_y1 >= m_y2)
			return false;

		const float safeZone = applySafeZone ? kPortalSafeZoneSize : 0.f;

		if (x2 < m_x1 - safeZone || x1 > m_x2 + safeZone)
			return false;
		if (y2 < m_y1 - safeZone || y1 > m_y2 + safeZone)
			return false;
		return true;
	}
}

bool Portal::doTeleport(GameSim & gameSim, Portal *& destination, int & destinationId)
{
	// find a destination portal

	int numPortals = 0;
	int portals[MAX_PORTALS];

	for (int i = 0; i < MAX_PORTALS; ++i)
	{
		Portal & portal = gameSim.m_portals[i];

		if (portal.m_isActive &&
			portal.m_key == m_key &&
			&portal != this)
		{
			// found one!

			portals[numPortals++] = i;
		}
	}

	if (numPortals > 0)
	{
		// make a random selection

		const int index = portals[gameSim.Random() % numPortals];
		Portal & portal = gameSim.m_portals[index];

		// trigger tile sprite animation

		const Vec2 pos = portal.getDestinationPos(Vec2(0.f, 0.f));
		TileSprite * tileSprite = gameSim.findTileSpriteAtPos(pos[0], pos[1]);
		if (tileSprite)
			tileSprite->startAnim("Activate");

		// todo : make a sound?

		destination = &portal;
		destinationId = index;

		return true;
	}
	else
	{
		return false;
	}
}

Vec2 Portal::getDestinationPos(Vec2Arg offset) const
{
	return Vec2(
		offset[0] + (m_x1 + m_x2) / 2.f,
		offset[1] + m_y2);
}

void Portal::tick(GameSim & gameSim, float dt)
{
}

void Portal::draw() const
{
	if (g_devMode || 1) // fixme
	{
		setColor(colorGreen);
		drawRectLine(
			m_x1,
			m_y1,
			m_x2,
			m_y2);

		setColor(colorRed);
		drawRectLine(
			m_x1 - kPortalSafeZoneSize,
			m_y1 - kPortalSafeZoneSize,
			m_x2 + kPortalSafeZoneSize,
			m_y2 + kPortalSafeZoneSize);
	}
}

//

void PickupSpawner::setup(float x1, float y1, float x2, float y2, PickupType type, float interval)
{
	m_isActive = true;
	m_x = (x1 + x2) / 2.f;
	m_y = (y1 + y2) / 2.f;
	m_type = type;
	m_interval = interval;
	m_timeLeft = interval;
}

void PickupSpawner::tick(GameSim & gameSim, float dt)
{
	if (m_interval > 0.f)
	{
		m_timeLeft -= dt;

		if (m_timeLeft <= 0.f)
		{
			Pickup * pickup = gameSim.allocPickup();

			if (pickup)
			{
				pickup->setup(m_type, m_x, m_y);

				m_timeLeft = m_interval;
			}
		}
	}
}

void PickupSpawner::draw() const
{
	if (g_devMode || 1) // fixme
	{
		static const float kBorderSize = 8.f;

		setColor(colorGreen);
		drawRectLine(
			m_x - kBorderSize,
			m_y - kBorderSize,
			m_x + kBorderSize,
			m_y + kBorderSize);
	}
}

//

void TileSprite::setup(const char * name, int pivotX, int pivotY, int x1, int y1, int x2, int y2)
{
	m_isActive = true;

	m_pivotX = pivotX;
	m_pivotY = pivotY;

	m_spriter = name;
	m_spriterState = SpriterState();
	m_spriterState.startAnim(Spriter(m_spriter.c_str()), 0);

	Assert(x1 < x2);
	Assert(y1 < y2);
	m_x1 = x1;
	m_y1 = y1;
	m_x2 = x2;
	m_y2 = y2;
}

void TileSprite::tick(GameSim & gameSim, float dt)
{
	if (m_spriterState.updateAnim(Spriter(m_spriter.c_str()), dt))
	{
		m_spriterState.startAnim(Spriter(m_spriter.c_str()), 0);
	}
}

void TileSprite::draw(const GameSim & gameSim) const
{
	gpuTimingBlock(tileSpriteDraw);
	cpuTimingBlock(tileSpriteDraw);

	setColor(colorWhite);
	const Vec2 offset = m_transition.eval(gameSim.m_physicalRoundTime);
	SpriterState state = m_spriterState;
	state.x = (m_x1 + m_x2) / 2 + m_pivotX + offset[0];
	state.y = m_y2 + m_pivotY + offset[1];
	Spriter(m_spriter.c_str()).draw(state);

	if (g_devMode)
	{
		setColor(colorGreen);
		drawRectLine(
			m_x1,
			m_y1,
			m_x2,
			m_y2);
	}
}

void TileSprite::drawLight() const
{
	// todo : let object define light map?

	setColor(colorWhite);
}

void TileSprite::startAnim(const char * name)
{
	m_spriterState.startAnim(
		Spriter(m_spriter.c_str()),
		name);
}

bool TileSprite::intersects(int x, int y) const
{
	if (m_x1 < 0 || m_y1 < 0)
		return false;
	if (x < m_x1 || x > m_x2)
		return false;
	if (y < m_y1 || y > m_y2)
		return false;
	return true;
}

//

void Decal::tick(GameSim & gameSim, float dt)
{
}

void Decal::draw() const
{
	if (DECAL_ENABLED)
	{
		// note: should draw at all combinations of +1/-1, but drawing it four times will do 99% of the time
		drawAt(x, y);
		drawAt(x - ARENA_SX_PIXELS, y);
		drawAt(x + ARENA_SX_PIXELS, y);
		drawAt(x, y - ARENA_SY_PIXELS);
		drawAt(x, y + ARENA_SY_PIXELS);
	}
}

void Decal::drawAt(int x, int y) const
{
	setColor(color[0], color[1], color[2]);
	Sprite sprite("decals/0.png");
	sprite.drawEx(
		x - sprite.getWidth() * scale / 2.f,
		y - sprite.getHeight() * scale / 2.f,
		0.f, scale, scale,
		false, FILTER_MIPMAP);
}

Color getDecalColor(int playerIndex, Vec2Arg direction)
{
	const int colorIndex = (DECAL_COLOR >= 0) ? (DECAL_COLOR % MAX_PLAYERS) : playerIndex;
	const Color color1 = (DECAL_COLOR == MAX_PLAYERS) ? Color(92, 173, 255) : getPlayerColor(colorIndex);
	const Color color2 = color1.interp(colorBlack, 0.f);  // todo : modify decal color in some way?
	const Color color3 = direction.CalcSize() == 0.f ? color2 : color2.hueShift(Bullet::toAngle(direction[0], direction[1]) / (2.f * float(M_PI)));
	return color3;
}

//

void ScreenShake::tick(GameSim & gameSim, float dt)
{
	Vec2 force = pos * (-stiffness);
	vel += force * dt;
	pos += vel * dt;

	life -= dt;

	if (life <= 0.f)
	{
		*this = ScreenShake();
	}
}

//

void ZoomEffect::tick(GameSim & gameSim, float dt)
{
	life = Calc::Max(0.f, life - lifeRcp * dt);

	if (life <= 0.f)
	{
		*this = ZoomEffect();
	}
}

//

void LightEffect::setDarken(float time, float amount)
{
	this->m_isActive = true;
	this->type = kType_Darken;
	this->life = time;
	this->lifeRcp = 1.f / time;
	this->amount = amount;
}

void LightEffect::setLighten(float time, float amount)
{
	this->m_isActive = true;
	this->type = kType_Lighten;
	this->life = time;
	this->lifeRcp = 1.f / time;
	this->amount = amount;
}

void LightEffect::tick(GameSim & gameSim, float dt)
{
	life = Calc::Max(0.f, life - dt);

	if (life <= 0.f)
	{
		*this = LightEffect();
	}
}

//

void FloorEffect::ActiveTile::getCollisionInfo(CollisionInfo & collisionInfo) const
{
	collisionInfo.min[0] = x - STOMP_EFFECT_DAMAGE_HITBOX_SX/2;
	collisionInfo.max[0] = x + STOMP_EFFECT_DAMAGE_HITBOX_SX/2;
	collisionInfo.min[1] = y - STOMP_EFFECT_DAMAGE_HITBOX_SY;
	collisionInfo.max[1] = y;
}

void FloorEffect::tick(GameSim & gameSim, float dt)
{
	for (int i = 0; i < MAX_FLOOR_EFFECT_TILES; ++i)
	{
		bool isAlive = false;

		if (m_tiles[i].damageTime > 0.f && m_tiles[i].damageSize > 0)
		{
			isAlive = true;

			CollisionInfo collisionInfo;
			m_tiles[i].getCollisionInfo(collisionInfo);

			for (int p = 0; p < MAX_PLAYERS; ++p)
			{
				if (p != m_tiles[i].playerId)
				{
					Player & player = gameSim.m_players[p];

					if (!player.m_isUsed)
						continue;
					if (!player.m_isAlive)
						continue;

					CollisionInfo playerCollision;
					if (player.getPlayerCollision(playerCollision))
					{
						if (collisionInfo.intersects(playerCollision))
						{
							player.handleDamage(1.f, Vec2(m_tiles[i].dx * PLAYER_SWORD_PUSH_SPEED/10.f /* todo : speed */, -1.f), &gameSim.m_players[m_tiles[i].playerId]);
						}
					}
				}
			}

			m_tiles[i].damageTime -= dt;
		}

		if (m_tiles[i].time > 0.f)
		{
			m_tiles[i].time -= dt;

			if (m_tiles[i].time <= 0.f && m_tiles[i].size != 0)
			{
				const int playerId = m_tiles[i].playerId;
				const int x = m_tiles[i].x + m_tiles[i].dx;
				const int y = m_tiles[i].y;
				const int dx = m_tiles[i].dx;
				const int size = m_tiles[i].size - 1;
				const int damageSize = m_tiles[i].damageSize > 0 ? m_tiles[i].damageSize - 1 : 0;

				trySpawnAt(gameSim, playerId, x, y, dx, size, damageSize);
			}
			else
			{
				isAlive = true;
			}
		}

		if (!isAlive)
		{
			memset(&m_tiles[i], 0, sizeof(m_tiles[i]));
		}
	}
}

void FloorEffect::draw()
{
	if (g_devMode)
	{
		for (int i = 0; i < MAX_FLOOR_EFFECT_TILES; ++i)
		{
			if (m_tiles[i].damageTime > 0.f && m_tiles[i].damageSize > 0)
			{
				CollisionInfo collisionInfo;
				m_tiles[i].getCollisionInfo(collisionInfo);

				setColor(255, 127, 63, 127);
				drawRect(
					collisionInfo.min[0],
					collisionInfo.min[1],
					collisionInfo.max[0],
					collisionInfo.max[1]);
			}
		}
	}
}

void FloorEffect::trySpawnAt(GameSim & gameSim, int playerId, int x, int y, int dx, int size, int damageSize)
{
	x = (x + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;

	const Arena & arena = gameSim.m_arena;

	// try to move up

	bool foundEmpty = false;

	for (int dy = 0; dy < BLOCK_SY * 3 / 2; ++dy)
	{
		const uint32_t blockMask = arena.getIntersectingBlocksMask(x, y - dy);

		if (!(blockMask & kBlockMask_Solid))
		{
			y = y - dy;
			foundEmpty = true;
			break;
		}
	}

	if (!foundEmpty)
		return;

	// try to move down

	bool hitGround = false;

	for (int dy = 0; dy < BLOCK_SY * 3 / 2; ++dy)
	{
		const uint32_t blockMask = arena.getIntersectingBlocksMask(x, y + dy + 1);

		if (blockMask & kBlockMask_Solid)
		{
			y = y + dy;
			hitGround = true;
			break;
		}
	}

	if (hitGround)
	{
		for (int i = 0; i < MAX_FLOOR_EFFECT_TILES; ++i)
		{
			if (m_tiles[i].time == 0)
			{
				m_tiles[i].playerId = playerId;
				m_tiles[i].x = x;
				m_tiles[i].y = y;
				m_tiles[i].dx = dx;
				m_tiles[i].size = size;
				m_tiles[i].damageSize = damageSize;
				m_tiles[i].time = STOMP_EFFECT_PROPAGATION_INTERVAL;
				m_tiles[i].damageTime = damageSize ? STOMP_EFFECT_DAMAGE_DURATION : 0.f;

				ParticleSpawnInfo spawnInfo(
					x,
					y,
					kBulletType_ParticleA, 10,
					50.f, 100.f, 50.f);
				spawnInfo.color = damageSize > 0 ? 0xff8000a0 : 0x0000ffa0;
				g_gameSim->spawnParticles(spawnInfo);

				gameSim.playSound("grenade-frag.ogg");

				break;
			}
		}
	}
}

//

void BlindsEffect::setup(float time, int x, int y, int size, bool vertical, const char * text)
{
	m_isActive = true;
	m_duration = time;
	m_time = time;
	m_x = x;
	m_y = y;
	m_size = size;
	m_vertical = vertical;
	m_text = text;
}

void BlindsEffect::tick(GameSim & gameSim, float dt)
{
	if (m_time > 0.f)
	{
		m_time -= dt;

		if (m_time <= 0.f)
		{
			*this = BlindsEffect();
		}
	}
}


void BlindsEffect::drawLight()
{
	const int kBorderSize = 1024;

	if (m_time > 0.f)
	{
		setBlend(BLEND_SUBTRACT);
		{
			setColorf(.8f, .8f, .8f, m_time / m_duration * 4.f);
			if (m_vertical)
			{
				drawRect(-kBorderSize, -kBorderSize, m_x - m_size, GFX_SY + kBorderSize);
				drawRect(m_x + m_size, -kBorderSize, GFX_SX + kBorderSize, GFX_SY + kBorderSize);
			}
			else
			{
				drawRect(-kBorderSize, -kBorderSize, GFX_SX + kBorderSize, m_y - m_size);
				drawRect(-kBorderSize, m_y + m_size, GFX_SX + kBorderSize, GFX_SY + kBorderSize);
			}
		}
		setBlend(BLEND_ADD);
	}
}

void BlindsEffect::drawHud()
{
	if (m_vertical)
	{
	}
	else
	{
		setColorf(1.f, 1.f, 1.f, m_time / m_duration * 4.f);
		drawText(150, m_y - m_size - 20, 40, +1.f, -1.f, "%s", m_text.c_str());
	}
}


//

void AnimationFxState::tick(GameSim & gameSim, float dt)
{
	if (m_isActive)
	{
		if (m_state.updateAnim(Spriter(m_fileName.c_str()), dt))
		{
			memset(this, 0, sizeof(AnimationFxState));
		}
	}
}

void AnimationFxState::draw(DrawLayer layer)
{
	if (m_isActive && layer == m_layer)
	{
		gpuTimingBlock(animationFxDraw);
		cpuTimingBlock(animationFxDraw);

		setColor(colorWhite);
		Spriter(m_fileName.c_str()).draw(m_state);
	}
}

//

GameSim::GameSim()
	: GameStateData()
	, m_bulletPool(0)
{
	m_arena.init();

	for (int i = 0; i < MAX_PLAYERS; ++i)
		m_playerInstanceDatas[i] = 0;

	m_bulletPool = new BulletPool(false);

	m_particlePool = new BulletPool(true);
}

GameSim::~GameSim()
{
	delete m_particlePool;
	m_particlePool = 0;

	delete m_bulletPool;
	m_bulletPool = 0;
}

#if ENABLE_GAMESTATE_DESYNC_DETECTION
uint32_t GameSim::calcCRC() const
{
	if (!g_checkCRCs)
		return 0;

	clearPlayerPtrs();

	uint32_t result = 0;

	const uint8_t * bytes = (const uint8_t*)static_cast<const GameStateData*>(this);
	const uint32_t numBytes = sizeof(GameStateData);

	for (uint32_t i = 0; i < numBytes; ++i)
		result = result * 13 + bytes[i];

	result = result * 13 + m_arena.calcCRC();

	result = result * 13 + m_bulletPool->calcCRC();

	// todo : add screen shakes, particle pool, bullet pool

	setPlayerPtrs();

	return result;
}
#endif

void GameSim::serialize(NetSerializationContext & context)
{
#if ENABLE_GAMESTATE_DESYNC_DETECTION
	uint32_t crc;

	if (context.IsSend())
		crc = calcCRC();

	context.Serialize(crc);
#endif

	clearPlayerPtrs();
	{
		GameStateData * data = static_cast<GameStateData*>(this);

		context.SerializeBytes(data, sizeof(GameStateData));

	#if 0 // todo : use zero compression to keep serialization size down
		int numZeroes = 0;
		uint8_t * bytes = (uint8_t*)data;
		for (int i = 0; i < sizeof(GameStateData); ++i)
			if (!bytes[i])
				numZeroes++;
		logDebug("GameStateData: size=%u, numZeroes=%u", sizeof(GameStateData), numZeroes);
		//unsigned char temp[sizeof(GameStateData)];
		//memset(temp, 0, sizeof(temp));
		//BinaryDiffResult diff = BinaryDiff(data, temp, sizeof(GameStateData), 4);
	#endif
	}
	setPlayerPtrs();

	// todo : serialize player animation state

	m_arena.serialize(context);

	m_bulletPool->serialize(context);

	for (int i = 0; i < MAX_PARTICLE_EFFECTS; ++i)
	{
		context.SerializeBytes(&m_particleEffects[i].m_data, sizeof(m_particleEffects[i].m_data));

		if (context.IsRecv() && m_particleEffects[i].m_data.m_isActive)
		{
			m_particleEffects[i].setup(
				m_particleEffects[i].m_data.m_filename.c_str(),
				m_particleEffects[i].m_data.m_x,
				m_particleEffects[i].m_data.m_y);
		}
	}

	// todo : serialize player animation state, since it affects game play

#if ENABLE_GAMESTATE_DESYNC_DETECTION
	if (context.IsRecv())
		Assert(crc == calcCRC());
#endif
}

void GameSim::clearPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerInstanceDatas[i])
			m_playerInstanceDatas[i]->m_player->m_instanceData = 0;
}

void GameSim::setPlayerPtrs() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_playerInstanceDatas[i])
			m_playerInstanceDatas[i]->m_player->m_instanceData = m_playerInstanceDatas[i];
}

PlayerInstanceData * GameSim::allocPlayer(uint16_t owningChannelId)
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = m_players[i];

		if (player.m_isUsed)
			continue;

		player = Player(i, owningChannelId);
		player.m_isUsed = true;

		PlayerInstanceData * playerInstanceData = new PlayerInstanceData(&player, this);

		m_playerInstanceDatas[i] = playerInstanceData;

		return playerInstanceData;
	}

	return 0;
}

void GameSim::freePlayer(PlayerInstanceData * instanceData)
{
	const int playerIndex = instanceData->m_player->m_index;
	Assert(playerIndex != -1);
	if (playerIndex != -1)
	{
		Assert(m_players[playerIndex].m_isUsed);
		m_players[playerIndex].handleLeave();
		m_players[playerIndex] = Player();

		Assert(m_playerInstanceDatas[playerIndex] != 0);
		m_playerInstanceDatas[playerIndex] = 0;
	}
}

int GameSim::getNumPlayers() const
{
	int result = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_players[i].m_isUsed)
			result++;
	return result;
}

void GameSim::setGameState(::GameState gameState)
{
	m_gameState = gameState;

	switch (gameState)
	{
	case kGameState_Initial:
		Assert(false);
		break;

	case kGameState_Connecting:
		break;

	case kGameState_OnlineMenus:
		resetGameWorld();

		// reset players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_playerInstanceDatas[i])
			{
				Player * player = m_playerInstanceDatas[i]->m_player;

				player->m_isActive = false;
				player->m_isReadyUpped = false;
			}
		}

	#if ITCHIO_BUILD
		load("itch-lobby");
	#else
		load("lobby");
	#endif

		setGameMode(kGameMode_Lobby);

		break;

	case kGameState_NewGame:
		m_consecutiveRoundCount = 0;

		resetGameWorld(); // todo : why was this moved?

		// reset players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_playerInstanceDatas[i])
			{
				Player * player = m_playerInstanceDatas[i]->m_player;

				player->handleNewGame();
			}
		}

		setGameMode(m_desiredGameMode);
		break;

	case kGameState_RoundBegin:
		{
			m_roundBegin = RoundBegin();
			if (GAMESTATE_ROUNDBEGIN_SHOW_CONTROLS && m_consecutiveRoundCount == 0)
			{
				m_roundBegin.m_delayTimeRcp = 1.f / 4.f;
				m_roundBegin.m_delay = 1.f;
				m_roundBegin.m_state = RoundBegin::kState_ShowControls;
			}
			else
			{
				m_roundBegin.m_delayTimeRcp = 1.f / GAMESTATE_ROUNDBEGIN_TRANSITION_TIME;
				m_roundBegin.m_delay = 1.f;
				m_roundBegin.m_state = RoundBegin::kState_LevelTransition;
				playSound("round-begin.ogg");
			}

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				if (m_playerInstanceDatas[i])
				{
					Player * player = m_playerInstanceDatas[i]->m_player;

					player->despawn(false);

					player->handleNewRound();
				}
			}
		}
		break;

	case kGameState_Play:
		{
			// game modes

			if (m_gameMode == kGameMode_FootBrawl)
			{
				spawnFootball();
			}

			if (m_gameMode == kGameMode_TokenHunt)
			{
				spawnToken();
			}

			if (m_gameMode == kGameMode_CoinCollector)
			{
				for (int i = 0; i < COINCOLLECTOR_INITIAL_COIN_DROP; ++i)
					spawnCoin();
			}
		}
		break;

	case kGameState_RoundComplete:
		{
			m_roundEnd = RoundEnd();
			m_roundEnd.m_delayTimeRcp = 1.f / GAMESTATE_ROUNDCOMPLETE_SHOWWINNER_TIME;
			m_roundEnd.m_delay = 1.f;
		}
		break;

	default:
		Assert(false);
		break;
	}
}

void GameSim::setGameMode(GameMode gameMode)
{
	m_gameMode = gameMode;
}

static Color parseColor(const char * str)
{
	const uint32_t hex = std::stoul(str, 0, 16);
	const float r = ((hex >> 24) & 0xff) / 255.f;
	const float g = ((hex >> 16) & 0xff) / 255.f;
	const float b = ((hex >>  8) & 0xff) / 255.f;
	const float a = ((hex >>  0) & 0xff) / 255.f;
	return Color(r, g, b, a);
}

void GameSim::newGame()
{
	setGameState(kGameState_NewGame);

	newRound(0);

	framework.blinkTaskbarIcon(3); // todo : option
}

void GameSim::newRound(const char * mapOverride)
{
	// load arena

	std::string map;

	if (mapOverride)
		map = mapOverride;
	else if (!((std::string)g_map).empty())
		map = g_map;
	else if (!g_mapRotationList.empty())
		map = g_mapRotationList[m_nextRoundNumber % g_mapRotationList.size()];
	else
		map = "testArena";

	load(map.c_str());

	// set game mode

	if (g_gameModeNextRound >= 0 && g_gameModeNextRound < kGameMode_COUNT)
	{
		setGameMode((GameMode)(int)g_gameModeNextRound);
	}

	// and start playing!

	setGameState(kGameState_RoundBegin);

	m_nextRoundNumber++;
	m_consecutiveRoundCount++;

	addAnnouncement(colorRed, "Map: %s", m_arena.m_displayName.c_str());
}

void GameSim::endRound()
{
	setGameState(kGameState_RoundComplete);
}

template <typename T> T * allocObject(T * objects, int numObjects)
{
	for (int i = 0; i < numObjects; ++i)
		if (!objects[i].m_isActive)
			return &objects[i];
	return 0;
}

void GameSim::load(const char * name)
{
	resetGameWorld();

	// load arena

	m_arena.load(name);

	// load background

	if (m_gameState == kGameState_OnlineMenus)
		m_background.load(kBackgroundType_Lobby, *this);
	else
		m_background.load(kBackgroundType_Volcano, *this);

	// load objects

#if 0 // todo : remove (mover test)
	{ Mover & mover = m_movers[0]; mover.setup(400, 50, GFX_SX*1/4, GFX_SY/2-200, GFX_SX*3/4, GFX_SY/2-300, 100); }
	{ Mover & mover = m_movers[1]; mover.setup(400, 50, GFX_SX*1/4, GFX_SY/2,     GFX_SX*3/4, GFX_SY/2-100, 111); }
	{ Mover & mover = m_movers[2]; mover.setup(400, 50, GFX_SX*1/4, GFX_SY/2+200, GFX_SX*3/4, GFX_SY/2+100, 121); }
#endif

	const std::string baseName = std::string("levels/") + name + "/";
	const std::string objFilename = baseName + "Obj.txt";

	//std::string objectsFilename = baseName + "-objects.txt";

	try
	{
		FileStream stream;
		stream.Open(objFilename.c_str(), (OpenMode)(OpenMode_Read | OpenMode_Text));
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();

		for (size_t i = 0; i < lines.size(); ++i)
		{
			if (lines[i].empty() || lines[i][0] == '#')
				continue;

			Dictionary d;

			if (!d.parse(lines[i]))
			{
				LOG_ERR("failed to parse object definition: %s", lines[i].c_str());
				continue;
			}

			std::string type = d.getString("type", "");

			if (type == "mover")
			{
				auto * mover = allocObject(m_movers, MAX_MOVERS);

				if (mover == 0)
					LOG_ERR("too many movers!");
				else
				{
					mover->m_isActive = true;

					mover->setSprite(d.getString("sprite", "").c_str());
					mover->m_x1 = d.getInt("x1", 0);
					mover->m_y1 = d.getInt("y1", 0);
					mover->m_x2 = d.getInt("x2", 0);
					mover->m_y2 = d.getInt("y2", 0);
					mover->m_speed = d.getInt("speed", 0);
				}
			}
			else if (type == "light")
			{
				auto * light = allocObject(m_lights, MAX_LIGHTS);

				if (light == 0)
					LOG_ERR("too many torches!");
				else
				{
					light->m_isActive = true;

					light->m_pos[0] = d.getInt("x", 0);
					light->m_pos[1] = d.getInt("y", 0);
	
					const Color color = parseColor(d.getString("color", "ffffffff").c_str());
					light->m_color[0] = color.r;
					light->m_color[1] = color.g;
					light->m_color[2] = color.b;
					light->m_color[3] = color.a;
				}
			}
			else if (type == "tilesprite")
			{
				auto * tileSprite = allocObject(m_tileSprites, MAX_TILE_SPRITES);

				if (tileSprite == 0)
					LOG_ERR("too many tile sprites!");
				else
				{
					tileSprite->setup(
						d.getString("sprite", "").c_str(),
						d.getInt("px", 0),
						d.getInt("py", 0),
						d.getInt("x1", 0),
						d.getInt("y1", 0),
						d.getInt("x2", 0),
						d.getInt("y2", 0));
					tileSprite->m_transition.parse(d);
				}
			}
			else if (type == "tiletransition")
			{
				if (m_arena.m_numTileTransitions < MAX_TILE_TRANSITIONS)
				{
					Arena::TileTransition & tileTransition = m_arena.m_tileTransitions[m_arena.m_numTileTransitions++];

					tileTransition.setup(
						d.getInt("x1", 0) / BLOCK_SX,
						d.getInt("y1", 0) / BLOCK_SY,
						d.getInt("x2", 0) / BLOCK_SX,
						d.getInt("y2", 0) / BLOCK_SY,
						d);
				}
				else
				{
					LOG_ERR("too many tile transitions!");
				}
			}
			else if (type == "particleeffect")
			{
				ParticleEffect * particleEffect = 0;

				for (int i = 0; i < MAX_PARTICLE_EFFECTS; ++i)
				{
					if (!m_particleEffects[i].m_data.m_isActive)
					{
						particleEffect = &m_particleEffects[i];
						break;
					}
				}

				if (particleEffect == 0)
					LOG_ERR("too many particle effects!");
				else
				{
					particleEffect->setup(
						d.getString("file", "").c_str(),
						d.getInt("x", 0),
						d.getInt("y", 0));
				}
			}
			else if (type == "portal")
			{
				auto * portal = allocObject(m_portals, MAX_PORTALS);

				if (portal == 0)
					LOG_ERR("too many portals!");
				else
				{
					portal->setup(
						d.getInt("x1", 0),
						d.getInt("y1", 0),
						d.getInt("x2", 0),
						d.getInt("y2", 0),
						d.getInt("key", 0));
				}
			}
			else if (type == "pickupspawn")
			{
				auto * spawner = allocObject(m_pickupSpawners, MAX_PICKUP_SPAWNERS);

				if (spawner == 0)
					LOG_ERR("too many pickup spawners!");
				else
				{
					PickupType type;
					if (parsePickupType(d.getString("pickup", 0).c_str(), type))
					{
						spawner->setup(
							d.getInt("x1", 0),
							d.getInt("y1", 0),
							d.getInt("x2", 0),
							d.getInt("y2", 0),
							type,
							d.getInt("interval", 0));
					}
				}
			}
			else if (type == "footballspawn")
			{
				m_footBrawl.ballSpawnPoint[0] = (d.getInt("x1", 0) + d.getInt("x2", 0)) / 2;
				m_footBrawl.ballSpawnPoint[1] = (d.getInt("y1", 0) + d.getInt("y2", 0)) / 2;
			}
			else if (type == "footballgoal")
			{
				auto * goal = allocObject(m_footBallGoals, MAX_FOOTBALL_GOALS);

				if (goal == 0)
					LOG_ERR("too many football goals!");
				else
				{
					goal->setup(
						(d.getInt("x1", 0) + d.getInt("x2", 0)) / 2,
						(d.getInt("y1", 0) + d.getInt("y2", 0)) / 2);
				}
			}
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR(e.what());
	}
}

template <typename T> void resetObjects(T * objects, int numObjects)
{
	for (int i = 0; i < numObjects; ++i)
		objects[i] = T();
}

void GameSim::resetGameWorld()
{
	// reset round stuff

	m_roundTime = 0.f;
	m_physicalRoundTime = 0.f;
	m_isFirstKill = true;

	// reset map

	m_arena.reset();

	resetObjects(m_pickups, MAX_PICKUPS);

	m_nextPickupSpawnTimeRemaining = 0.f;

	resetObjects(m_movers, MAX_MOVERS);
	resetObjects(m_axes, MAX_AXES);
	resetObjects(m_pipebombs, MAX_PIPEBOMBS);
	resetObjects(m_footBalls, MAX_FOOTBALLS);
	resetObjects(m_footBallGoals, MAX_FOOTBALL_GOALS);

	// reset floor effect

	m_floorEffect = FloorEffect();

	resetObjects(m_blindsEffects, MAX_BLINDS_EFFECTS);
	resetObjects(m_lights, MAX_LIGHTS);
	resetObjects(m_particleEffects, MAX_PARTICLE_EFFECTS);
	resetObjects(m_portals, MAX_PORTALS);
	resetObjects(m_pickupSpawners, MAX_PICKUP_SPAWNERS);
	resetObjects(m_tileSprites, MAX_TILE_SPRITES);

	// reset bullets

	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		if (m_bulletPool->m_bullets[i].isAlive)
			m_bulletPool->free(i);

		if (m_particlePool->m_bullets[i].isAlive)
			m_particlePool->free(i);
	}

	resetObjects(m_decals, MAX_DECALS);
	g_decalMap->clear();

	resetObjects(m_screenShakes, MAX_SCREEN_SHAKES);
	resetObjects(m_zoomEffects, MAX_ZOOM_EFFECTS);

	m_desiredZoom = 1.f;
	m_desiredZoomFocus.SetZero();
	m_desiredZoomFocusIsSet = false;
	m_effectiveZoom = 1.f;
	m_effectiveZoomFocus.Set(ARENA_SX_PIXELS/2.f, ARENA_SY_PIXELS/2.f);

	resetObjects(m_lightEffects, MAX_LIGHT_EFFECTS);
	resetObjects(m_fireballs, MAX_FIREBALLS);

	// reset time dilation effects

	memset(m_timeDilationEffects, 0, sizeof(m_timeDilationEffects));

	// reset level events

	m_levelEvents = LevelEvents();
	m_timeUntilNextLevelEvent = 0.f;

	// reset footbrawl game mode

	m_footBrawl = FootBrawl();

	// reset token hunt game mode

	m_tokenHunt = TokenHunt();

	// reset coin collector game mode

	m_coinCollector = CoinCollector();
}

void GameSim::resetPlayers()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_players[i] = Player();
	}
}

void GameSim::resetGameSim()
{
	resetGameWorld();

	resetPlayers();
}

void GameSim::tick()
{
	cpuTimingBlock(gameSimTick);

	Assert(!g_gameSim);
	g_gameSim = this;

#if ENABLE_GAMESTATE_CRC_LOGGING
	const uint32_t oldCRC = g_logCRCs ? calcCRC() : 0;
#endif

	// options support

	static bool curvesAreInitialized = false;
	if (!curvesAreInitialized || g_devMode)
	{
		curvesAreInitialized = true;

		pipebombBlastCurve.makeLinear(PIPEBOMB_BLAST_STRENGTH_NEAR, PIPEBOMB_BLAST_STRENGTH_FAR);
		grenadeBlastCurve.makeLinear(BULLET_GRENADE_BLAST_STRENGTH_NEAR, BULLET_GRENADE_BLAST_STRENGTH_FAR);
		jetpackAnalogCurveX.makeLinear(JETPACK_NEW_STEERING_CURVE_MIN, JETPACK_NEW_STEERING_CURVE_MAX);
		jetpackAnalogCurveY.makeLinear(JETPACK_NEW_STEERING_CURVE_MIN, JETPACK_NEW_STEERING_CURVE_MAX);
		gravityWellFalloffCurve.makeLinear(1.f, 0.f);
	}

	const float dt = 1.f / TICKS_PER_SECOND;

	m_physicalTimeStep = dt;

	switch (m_gameState)
	{
	case kGameState_Initial:
		Assert(false);
		break;

	case kGameState_OnlineMenus:
		tickPlay();
		tickMenus();
		break;

	case kGameState_RoundBegin:
		tickPlay();
		tickRoundBegin(dt);
		break;

	case kGameState_Play:
		tickPlay();
		break;

	case kGameState_RoundComplete:
		tickPlay();
		tickRoundComplete(dt);
		break;

	default:
		Assert(false);
		break;
	}

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (m_playerInstanceDatas[i])
		{
			const bool doInactivityCheck =
				m_gameState == kGameState_Play ||
				m_gameState == kGameState_OnlineMenus && !m_playerInstanceDatas[i]->m_player->m_isReadyUpped;

			const float dt = 1.f / TICKS_PER_SECOND;

			m_playerInstanceDatas[i]->m_player->m_input.next(doInactivityCheck, dt);
		}
	}

#if ENABLE_GAMESTATE_CRC_LOGGING
	if (g_logCRCs)
	{
		int numPlayers = 0;
		for (int i = 0; i < MAX_PLAYERS; ++i)
			if (m_playerInstanceDatas[i])
				numPlayers++;

		const uint32_t newCRC = calcCRC();

		LOG_DBG("gamesim %p: tick=%u, oldCRC=%08x, newCRC=%08x, numPlayers=%d [%s]", this, m_tick, oldCRC, newCRC, numPlayers, g_host && &g_host->m_gameSim == this ?  "server" : "client");
		/*
		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_playerInstanceDatas[i])
			{
				LOG_DBG("\tplayer %d: (%04x,%02d,%02d) - (%04x,%02d,%02d)", i,
					(int)m_playerInstanceDatas[i]->m_player->m_input.m_prevState.buttons, (int)m_playerInstanceDatas[i]->m_player->m_input.m_prevState.analogX, (int)m_playerInstanceDatas[i]->m_player->m_input.m_prevState.analogY,
					(int)m_playerInstanceDatas[i]->m_player->m_input.m_currState.buttons, (int)m_playerInstanceDatas[i]->m_player->m_input.m_currState.analogX, (int)m_playerInstanceDatas[i]->m_player->m_input.m_currState.analogY);
			}
		}
		*/
	}
#endif

	g_gameSim = 0;
}

void GameSim::tickMenus()
{
	// wait for the host to enter the next game state

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = m_players[i];

		if (player.m_isUsed)
		{
			PlayerInstanceData * playerInstanceData = m_playerInstanceDatas[i];
			Assert(playerInstanceData);
			if (playerInstanceData)
			{
				// character select

				if (!player.m_isReadyUpped)
				{
					int dx = 0;
					int dy = 0;

					if (player.m_input.wentDown(INPUT_BUTTON_LEFT))
						dx--;
					if (player.m_input.wentDown(INPUT_BUTTON_RIGHT))
						dx++;
					if (player.m_input.wentDown(INPUT_BUTTON_UP))
						dy--;
					if (player.m_input.wentDown(INPUT_BUTTON_DOWN))
						dy++;

				#if ITCHIO_BUILD
					if (dx)
					{
						const int oldCharacterIndex = player.m_characterIndex;
						bool valid = false;
						do
						{
							//player.m_characterIndex = (player.m_characterIndex + dx + MAX_CHARACTERS) % MAX_CHARACTERS;
							player.m_characterIndex = player.m_characterIndex + dx;
							if (player.m_characterIndex < 0 || player.m_characterIndex >= MAX_CHARACTERS)
								break;
							for (int o = 0; o < _countof(g_validCharacterIndices); ++o)
								valid |= player.m_characterIndex == g_validCharacterIndices[o];
						} while (!valid);
						if (!valid)
							player.m_characterIndex = oldCharacterIndex;
					}
				#else
					if (dx || dy)
					{
						int x, y;

						CharGrid::characterIndexToXY(player.m_characterIndex, x, y);
						x += dx;
						y += dy;
						CharGrid::modulateXY(x, y);

						if (CharGrid::isValidGridCell(x, y))
						{
							player.m_instanceData->setCharacterIndex(CharGrid::xyToCharacterIndex(x, y));

							playSound("ui/sounds/charselect-change.ogg");
						}
					}
				#endif
				}

				// ready up

				if (!player.m_isReadyUpped)
				{
					if (player.m_input.wentDown(INPUT_BUTTON_A) || (player.m_input.m_actions & (1 << kPlayerInputAction_ReadyUp)) || g_autoReadyUp)
					{
						//if (!DEMOMODE || (player.m_characterIndex != 4 && player.m_characterIndex != 7))
						{
							playSound("ui/sounds/charselect-select.ogg");

							player.m_isReadyUpped = true;

							Vec2 pos(560, 900);
							player.respawn(&pos);
						}
					}
				}
				else
				{
					if (player.m_input.wentDown(INPUT_BUTTON_Y) || (player.m_input.m_actions & (1 << kPlayerInputAction_ReadyUp)))
					{
						playSound("ui/sounds/charselect-deselect.ogg");

						player.m_isReadyUpped = false;

						player.despawn(false);
					}
				}
			}
		}
	}

	bool allReady = true;

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		const Player & p = m_players[i];
		if (p.m_isUsed)
		{
			const bool isInStartZone =
				g_devMode ||
				!GAMESTATE_LOBBY_STARTZONE_ENABLED ||
				(
					p.m_pos[0] >= GAMESTATE_LOBBY_STARTZONE_X &&
					p.m_pos[1] >= GAMESTATE_LOBBY_STARTZONE_Y &&
					p.m_pos[0] <= GAMESTATE_LOBBY_STARTZONE_X + GAMESTATE_LOBBY_STARTZONE_SX &&
					p.m_pos[1] <= GAMESTATE_LOBBY_STARTZONE_Y + GAMESTATE_LOBBY_STARTZONE_SY
				);

			allReady &= p.m_isReadyUpped && isInStartZone;
		}
	}

	allReady &= getNumPlayers() >= MIN_PLAYER_COUNT;

	if (allReady)
	{
		if (m_gameStartTicks == 0)
		{
			m_gameStartTicks = g_devMode ? 1 : (TICKS_PER_SECOND * 4);
		}
		else
		{
			m_gameStartTicks--;

			if (m_gameStartTicks == 0)
			{
				newGame();
			}
		}
	}
	else
	{
		m_gameStartTicks = 0;
	}
}

void GameSim::tickRoundBegin(float dt)
{
	if (m_roundBegin.m_state == RoundBegin::kState_ShowControls)
	{
		Assert(m_roundBegin.m_delay > 0.f);
		m_roundBegin.m_delay -= dt * m_roundBegin.m_delayTimeRcp;

		if (m_roundBegin.m_delay <= 0.f)
		{
			m_roundBegin.m_state = RoundBegin::kState_LevelTransition;
			m_roundBegin.m_delayTimeRcp = 1.f / GAMESTATE_ROUNDBEGIN_TRANSITION_TIME;
			m_roundBegin.m_delay = 1.f;
			playSound("round-begin.ogg");
		}
	}
	else if (m_roundBegin.m_state == RoundBegin::kState_LevelTransition)
	{
		Assert(m_roundBegin.m_delay > 0.f);
		m_roundBegin.m_delay -= dt * m_roundBegin.m_delayTimeRcp;

		if (m_roundBegin.m_delay <= 0.f)
		{
			m_roundBegin.m_state = RoundBegin::kState_SpawnPlayers;
			m_roundBegin.m_delayTimeRcp = 1.f / GAMESTATE_ROUNDBEGIN_SPAWN_DELAY;
			m_roundBegin.m_delay = 1.f;
		}
	}
	else if (m_roundBegin.m_state == RoundBegin::kState_SpawnPlayers)
	{
		Assert(m_roundBegin.m_delay > 0.f);
		m_roundBegin.m_delay -= dt * m_roundBegin.m_delayTimeRcp;

		if (m_roundBegin.m_delay <= 0.f)
		{
			// respawn players

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				if (m_playerInstanceDatas[i])
				{
					Player * player = m_playerInstanceDatas[i]->m_player;

					if (!player->isSpawned())
					{
						if (player->respawn(0))
						{
							break;
						}
					}
				}
			}

			bool allDone = true;

			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				if (m_playerInstanceDatas[i])
				{
					Player * player = m_playerInstanceDatas[i]->m_player;

					allDone &= player->isSpawned();
				}
			}

			if (allDone)
			{
				m_roundBegin.m_state = RoundBegin::kState_FightMessage;
				m_roundBegin.m_delayTimeRcp = 1.f / GAMESTATE_ROUNDBEGIN_MESSAGE_DELAY;
				m_roundBegin.m_delay = 1.f;

				addAnimationFx(kDrawLayer_HUD, "ui/fight/fight.scml", GAMESTATE_ROUNDBEGIN_MESSAGE_X, GAMESTATE_ROUNDBEGIN_MESSAGE_Y);
				playSound("round-begin-fight.ogg");
			}
			else
			{
				m_roundBegin.m_delayTimeRcp = 1.f / GAMESTATE_ROUNDBEGIN_SPAWN_DELAY;
				m_roundBegin.m_delay = 1.f;
			}
		}
	}
	else if (m_roundBegin.m_state == RoundBegin::kState_FightMessage)
	{
		Assert(m_roundBegin.m_delay > 0.f);
		m_roundBegin.m_delay -= dt * m_roundBegin.m_delayTimeRcp;

		if (m_roundBegin.m_delay <= 0.f)
		{
			setGameState(kGameState_Play);
		}
	}
}

template <typename T>
static void tickObjects(GameSim & gameSim, float dt, T * objects, int numObjects)
{
	for (int i = 0; i < numObjects; ++i)
		if (objects[i].m_isActive)
			objects[i].tick(gameSim, dt);
}

void GameSim::tickPlay()
{
	const float physicalDt = 1.f / TICKS_PER_SECOND;

	float timeDilation;
	bool playerAttackTimeDilation;

	getCurrentTimeDilation(timeDilation, playerAttackTimeDilation);

	const float dt =
		(1.f / TICKS_PER_SECOND) *
		timeDilation;

	const uint32_t tick = GetTick();

	// time dilation effects

	for (int i = 0; i < MAX_TIMEDILATION_EFFECTS; ++i)
		if (m_timeDilationEffects[i].ticksRemaining != 0)
			m_timeDilationEffects[i].ticksRemaining--;

	// arena update

	m_arena.tick(*this);

	// background update

	m_background.tick(*this, dt);

	tickObjects(*this, dt, m_fireballs, MAX_FIREBALLS);

	// player update

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (m_playerInstanceDatas[i])
		{
			float playerTimeMultiplier = 1.f;

			if (playerAttackTimeDilation && (m_players[i].m_timeDilationAttack.isActive() || (m_players[i].m_attack.attacking && m_players[i].m_attack.allowCancelTimeDilationAttack)))
				playerTimeMultiplier /= PLAYER_EFFECT_TIMEDILATION_MULTIPLIER;

			m_playerInstanceDatas[i]->m_player->tick(*this, dt * playerTimeMultiplier);
		}
	}

	tickObjects(*this, dt, m_pickups, MAX_PICKUPS);
	tickObjects(*this, dt, m_movers, MAX_MOVERS);
	tickObjects(*this, dt, m_axes, MAX_AXES);
	tickObjects(*this, dt, m_pipebombs, MAX_PIPEBOMBS);
	tickObjects(*this, dt, m_footBalls, MAX_FOOTBALLS);
	tickObjects(*this, dt, m_footBallGoals, MAX_FOOTBALL_GOALS);

	tickPlayPickupSpawn(dt);
	tickPlayLevelEvents(dt);

	// token

	if (m_gameMode == kGameMode_TokenHunt)
	{
		m_tokenHunt.m_token.tick(*this, dt);
	}

	// coins

	if (m_gameMode == kGameMode_CoinCollector && m_gameState == kGameState_Play)
	{
		tickObjects(*this, dt, m_coinCollector.m_coins, MAX_COINS);

		if (tick >= m_coinCollector.m_nextSpawnTick)
		{
			if (m_coinCollector.m_nextSpawnTick != 0)
			{
				int numCoins = 0;

				for (int i = 0; i < MAX_COINS; ++i)
					if (m_coinCollector.m_coins[i].m_isDropped)
						numCoins++;

				for (int i = 0; i < MAX_PLAYERS; ++i)
					if (m_players[i].m_isAlive)
						numCoins += m_players[i].m_score;

				if (numCoins < COINCOLLECTOR_COIN_LIMIT)
				{
					spawnCoin();
				}
			}

			m_coinCollector.m_nextSpawnTick = tick + (COIN_SPAWN_INTERVAL + (Random() % COIN_SPAWN_INTERVAL_VARIANCE)) * TICKS_PER_SECOND;
		}
	}

	// floor effect

	m_floorEffect.tick(*this, dt);

	tickObjects(*this, dt, m_blindsEffects, MAX_BLINDS_EFFECTS);
	tickObjects(*this, dt, m_lights, MAX_LIGHTS);

	// particle effects

	for (int i = 0; i < MAX_PARTICLE_EFFECTS; ++i)
	{
		if (m_particleEffects[i].m_data.m_isActive)
		{
			cpuTimingBlock(particleEffectTick);

			struct UserData
			{
				GameSim * gameSim;
				ParticleEffect * particleEffect;
			};

			UserData userData;
			userData.gameSim = this;
			userData.particleEffect = &m_particleEffects[i];

			ParticleCallbacks cbs;
			cbs.userData = &userData;
			cbs.randomInt = [](void * userData, int min, int max) { return min + (rand() % (max - min + 1)); };
			cbs.randomFloat = [](void * userData, float min, float max) { return min + (rand() & 4095) / 4095.f * (max - min); };
			//cbs.randomInt = [](void * userData, int min, int max) { return (int)static_cast<UserData*>(userData)->gameSim->RandomInt(min, max); };
			//cbs.randomFloat = [](void * userData, float min, float max) { return static_cast<UserData*>(userData)->gameSim->RandomFloat(min, max); };
			cbs.getEmitterByName = [](void * _userData, const char * name, const ParticleEmitterInfo *& pei, const ParticleInfo *& pi, ParticlePool *& pool, ParticleEmitter *& pe)
			{
				UserData * userData = static_cast<UserData*>(_userData);
				for (int i = 0; i < ParticleEffect::kMaxParticleSystems; ++i)
				{
					if (!strcmp(userData->particleEffect->m_system[i].emitterInfo.name, name))
					{
						pei = &userData->particleEffect->m_system[i].emitterInfo;
						pi = &userData->particleEffect->m_system[i].particleInfo;
						pool = &userData->particleEffect->m_system[i].pool;
						pe = &userData->particleEffect->m_system[i].emitter;
						return true;
					}
				}
				return false;
			};
			// todo : add collision callback
			//bool (*checkCollision)(void * userData, float x1, float y1, float x2, float y2, float & t, float & nx, float & ny);

			for (int j = 0; j < ParticleEffect::kMaxParticleSystems; ++j)
			{
				m_particleEffects[i].m_system[j].tick(cbs, 0.f, GRAVITY, dt);
			}
		}
	}

	tickObjects(*this, dt, m_portals, MAX_PORTALS);
	tickObjects(*this, dt, m_pickupSpawners, MAX_PICKUP_SPAWNERS);
	tickObjects(*this, dt, m_tileSprites, MAX_TILE_SPRITES);
	tickObjects(*this, dt, m_animationEffects, MAX_ANIM_EFFECTS);
	tickObjects(*this, dt, m_decals, MAX_DECALS);
	tickObjects(*this, dt, m_screenShakes, MAX_SCREEN_SHAKES);
	tickObjects(*this, dt, m_zoomEffects, MAX_ZOOM_EFFECTS);

	tickZoom(dt);

	// light effects

	tickObjects(*this, dt, m_lightEffects, MAX_LIGHT_EFFECTS);

	// particles

	m_particlePool->tick(*this, dt);

	// bullets

	m_bulletPool->tick(*this, dt);

	m_tick++;

	m_roundTime += dt;

	m_physicalRoundTime += physicalDt;

	if (m_gameState == kGameState_Play)
	{
		// check if the round has ended

		bool roundComplete = false;

		if (m_gameMode == kGameMode_FootBrawl)
		{
			// todo : check team scores
		}
		else if (m_gameMode == kGameMode_DeathMatch)
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				Player & player = m_players[i];
				if (!player.m_isUsed)
					continue;

				if (player.m_score >= DEATHMATCH_SCORE_LIMIT)
				{
					roundComplete = true;
				}
			}
		}
		else if (m_gameMode == kGameMode_CoinCollector)
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				Player & player = m_players[i];
				if (!player.m_isUsed)
					continue;

				if (player.m_score >= COINCOLLECTOR_SCORE_LIMIT)
				{
					roundComplete = true;
				}
			}
		}
		else if (m_gameMode == kGameMode_TokenHunt)
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				Player & player = m_players[i];
				if (!player.m_isUsed)
					continue;

				if (player.m_score >= TOKENHUNT_SCORE_LIMIT)
				{
					roundComplete = true;
				}
			}
		}
		else
		{
			fassert(false);
		}

		if (DEMOMODE && getNumPlayers() < MIN_PLAYER_COUNT)
		{
			roundComplete = true;
		}

		if (roundComplete)
		{
			endRound();
		}
	}
}

void GameSim::tickPlayPickupSpawn(float dt)
{
	if (m_nextPickupSpawnTimeRemaining > 0.f && m_gameMode != kGameMode_Lobby)
	{
		m_nextPickupSpawnTimeRemaining -= dt;

		if (m_nextPickupSpawnTimeRemaining < 0.f)
		{
			m_nextPickupSpawnTimeRemaining = 0.f;

			int numPickups = 0;
			for (int i = 0; i < MAX_PICKUPS; ++i)
				if (m_pickups[i].m_isActive)
					numPickups++;

			if (numPickups < MAX_PICKUP_COUNT)
			{
				int weights[kPickupType_COUNT] =
				{
					PICKUP_GUN_WEIGHT,
					PICKUP_NADE_WEIGHT,
					PICKUP_SHIELD_WEIGHT,
					PICKUP_ICE_WEIGHT,
					PICKUP_BUBBLE_WEIGHT,
					PICKUP_TIMEDILATION_WEIGHT
				};

				int totalWeight = 0;

				for (int i = 0; i < kPickupType_COUNT; ++i)
				{
					totalWeight += weights[i];
					weights[i] = totalWeight;
				}

				if (DEBUG_RANDOM_CALLSITES)
					LOG_DBG("Random called from pre trySpawnPickup");
				int value = Random() % totalWeight;

				PickupType type = kPickupType_COUNT;

				if (s_pickupTest >= 0 && s_pickupTest < kPickupType_COUNT)
					type = (PickupType)(int)s_pickupTest;
				else
				{
					for (int i = 0; type == kPickupType_COUNT; ++i)
						if (value < weights[i])
							type = (PickupType)i;
				}

				trySpawnPickup(type);
			}
		}
	}

	if (m_nextPickupSpawnTimeRemaining == 0.f)
	{
		const float multipliers[MAX_PLAYERS] =
		{
			PICKUP_RATE_MULTIPLIER_1,
			PICKUP_RATE_MULTIPLIER_2,
			PICKUP_RATE_MULTIPLIER_3,
			PICKUP_RATE_MULTIPLIER_4
		};
		const float multiplier = multipliers[Calc::Max(0, getNumPlayers() - 1)];

		m_nextPickupSpawnTimeRemaining = (PICKUP_INTERVAL + (Random() % PICKUP_INTERVAL_VARIANCE)) / multiplier;
	}
}

void GameSim::tickPlayLevelEvents(float dt)
{
	if (m_timeUntilNextLevelEvent > 0.f && PROTO_ENABLE_LEVEL_EVENTS && m_gameMode != kGameMode_Lobby)
	{
		m_timeUntilNextLevelEvent = Calc::Max(0.f, m_timeUntilNextLevelEvent - dt);

		if (m_timeUntilNextLevelEvent == 0.f)
		{
			const LevelEvent e = getRandomLevelEvent();
			//const LevelEvent e = kLevelEvent_SpikeWalls;
			//const LevelEvent e = kLevelEvent_GravityWell;
			//const LevelEvent e = kLevelEvent_EarthQuake;

			triggerLevelEvent(e);
		}
	}

	if (m_timeUntilNextLevelEvent == 0.f)
	{
		m_timeUntilNextLevelEvent = PROTO_LEVEL_EVENT_INTERVAL;
	}

	m_levelEvents.quake.tick(*this, dt);

	if (m_levelEvents.gravityWell.endTimer.tickActive(dt))
	{
		// affect player speeds

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			Player & player = m_players[i];

			if (!player.m_isUsed && player.m_isAlive)
				continue;

			const float dx = m_levelEvents.gravityWell.m_x - player.m_pos[0];
			const float dy = m_levelEvents.gravityWell.m_y - player.m_pos[1];
			const float d = std::sqrtf(dx * dx + dy * dy);
			if (d != 0.f)
			{
				const float ax = dx / d;
				const float ay = dy / d;

				const float t = m_levelEvents.gravityWell.endTimer.getProgress();
				const float s = Calc::Lerp(EVENT_GRAVITYWELL_STRENGTH_BEGIN, EVENT_GRAVITYWELL_STRENGTH_END, t);
				const float strength = s * 10000.f / (d + 1.f);

				player.m_vel[0] += ax * strength * dt;
				player.m_vel[1] += ay * strength * dt;
			}
		}
	}

	if (m_levelEvents.destroyBlocks.m_remainingBlockCount > 0)
	{
		if (m_levelEvents.destroyBlocks.destructionTimer.tickComplete(dt))
		{
			// todo : destroy a random block

			m_levelEvents.destroyBlocks.destructionTimer = 0.f;
		}
	}

	if (m_levelEvents.timeDilation.endTimer.tickActive(dt))
	{
		//
	}

	m_levelEvents.spikeWalls.tick(*this, dt);

	if (m_levelEvents.wind.endTimer.tickActive(dt))
	{
		// affect player speeds

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			Player & player = m_players[i];

			if (!player.m_isUsed && player.m_isAlive)
				continue;

			player.m_vel[0] += 300.f * dt;
		}
	}

	if (m_levelEvents.barrelDrop.endTimer.tickActive(dt))
	{
		if (m_levelEvents.barrelDrop.spawnTimer.tickComplete(dt))
		{
			// todo : add barrel

			// todo : next tick

			m_levelEvents.barrelDrop.spawnTimer = 1.f;
		}
	}

	if (m_levelEvents.nightDayCycle.endTimer.tickActive(dt))
	{
		//
	}
}

void GameSim::tickRoundComplete(float dt)
{
	if (m_roundEnd.m_state == RoundEnd::kState_ShowWinner)
	{
		Assert(m_roundEnd.m_delay > 0.f);
		m_roundEnd.m_delay -= dt * m_roundEnd.m_delayTimeRcp;
		if (m_roundEnd.m_delay <= 0.f)
		{
			m_roundEnd.m_state = RoundEnd::kState_ShowResults;
			m_roundEnd.m_delayTimeRcp = 1.f / GAMESTATE_ROUNDCOMPLETE_SHOWRESULTS_TIME;
			m_roundEnd.m_delay = 1.f;
		}
	}
	else if (m_roundEnd.m_state == RoundEnd::kState_ShowResults)
	{
		Assert(m_roundEnd.m_delay > 0.f);
		m_roundEnd.m_delay -= dt * m_roundEnd.m_delayTimeRcp;
		if (m_roundEnd.m_delay <= 0.f)
		{
			m_roundEnd.m_state = RoundEnd::kState_LevelTransition;
			m_roundEnd.m_delayTimeRcp = 1.f / GAMESTATE_ROUNDCOMPLETE_TRANSITION_TIME;
			m_roundEnd.m_delay = 1.f;
		}
	}
	else if (m_roundEnd.m_state == RoundEnd::kState_LevelTransition)
	{
		Assert(m_roundEnd.m_delay > 0.f);
		m_roundEnd.m_delay -= dt * m_roundEnd.m_delayTimeRcp;
		if (m_roundEnd.m_delay <= 0.f)
		{
			if ((DEMOMODE && m_consecutiveRoundCount >= (uint32_t)MAX_CONSECUTIVE_ROUND_COUNT) || (getNumPlayers() < MIN_PLAYER_COUNT))
			{
				setGameState(kGameState_OnlineMenus);
			}
			else
			{
				newRound(0);
			}
		}
	}
}

template <typename T> void drawObjects(const T * objects, int numObjects)
{
	for (int i = 0; i < numObjects; ++i)
		if (objects[i].m_isActive)
			objects[i].draw();
}

template <typename T> void drawObjects(const GameSim & gameSim, const T * objects, int numObjects)
{
	for (int i = 0; i < numObjects; ++i)
		if (objects[i].m_isActive)
			objects[i].draw(gameSim);
}

void GameSim::drawPlay()
{
	CamParams camParams;
	camParams.shake = getScreenShake();
	camParams.zoom = m_effectiveZoom;
	camParams.zoomFocus = m_effectiveZoomFocus;

#if 0 // map scaling test
	const float asx = ARENA_SX_PIXELS;
	const float asy = ARENA_SY_PIXELS;
	const float asx2 = asx / 2.f;
	const float asy2 = asy / 2.f;
	const float dx = m_players[0].m_pos[0] - asx2;
	const float dy = m_players[0].m_pos[1] - asy2;
	const float d = sqrt(dx * dx + dy * dy);
	const float t = d / sqrt(asx2 * asx2 + asy2 * asy2);
	const float scale = 1.f * t + 1.2f * (1.f - t);
	
	gxTranslatef(+ARENA_SX_PIXELS/2.f, +ARENA_SY_PIXELS/2.f, 0.f);
	gxScalef(scale, scale, 1.f);
	gxTranslatef(-ARENA_SX_PIXELS/2.f, -ARENA_SY_PIXELS/2.f, 0.f);
#endif

	pushSurface(g_colorMap);
	{
		cpuTimingBlock(drawPlayColor);
		gpuTimingBlock(drawPlayColor);

		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);

		drawPlayColor(camParams);
	}
	popSurface();

	if (m_gameState == kGameState_RoundBegin && m_roundBegin.m_state == RoundBegin::kState_LevelTransition)
	{
		gpuTimingBlock(drawPlayRoundBeginLevelTransition);

		const float t = 1.f - m_roundBegin.m_delay;

	#if 1 // level transition shader test
		Shader shader("shaders/trans1");
		shader.setTexture("colormap", 0, g_colorMap->getTexture(), false);
		shader.setImmediate("tint", t, t, t);
		shader.setImmediate("time", t);
		g_colorMap->postprocess(shader);
	#endif
	}

	if (m_gameState == kGameState_RoundComplete && m_roundEnd.m_state == RoundEnd::kState_LevelTransition)
	{
		gpuTimingBlock(drawPlayRoundCompleteLevelTransition);

		const float t = m_roundEnd.m_delay;

	#if 1 // level transition shader test
		Shader shader("shaders/trans1");
		shader.setTexture("colormap", 0, g_colorMap->getTexture(), false);
		shader.setImmediate("tint", t, t, t);
		g_colorMap->postprocess(shader);
	#endif
	}

	pushSurface(g_lightMap);
	{
		cpuTimingBlock(drawPlayLight);
		gpuTimingBlock(drawPlayLight);

		const float v = getLightAmount();

		glClearColor(v, v, v, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		drawPlayLight(camParams);
	}
	popSurface();

	// compose

	applyLightMap(*g_colorMap, *g_lightMap, *g_finalMap);

	pushSurface(g_finalMap);
	{
		cpuTimingBlock(drawPlayHUD);
		gpuTimingBlock(drawPlayHUD);

		gxPushMatrix();
		applyCamParams(camParams, 1.f, 1.f);

		setBlend(BLEND_ALPHA);

		for (int i = 0; i < MAX_BLINDS_EFFECTS; ++i)
			m_blindsEffects[i].drawHud();

		gxPopMatrix();

		drawPlayHud(camParams);
	}
	popSurface();

#if 0 // fsfx test
	// fsfx

	Shader fsfx("fsfx-test");
	fsfx.setImmediate("time", m_roundTime);
	fsfx.setImmediate("color", 1.f, .5f, .25f, 1.f);
	g_finalMap->postprocess(fsfx);
#endif

	// blit

	{
		gpuTimingBlock(drawPlayBlitToBackbuffer);

		// todo : keep aspect ratio in mind here

		setBlend(BLEND_OPAQUE);
		setColor(255, 255, 255);
		glEnable(GL_TEXTURE_2D);
		{
			glBindTexture(GL_TEXTURE_2D, g_finalMap->getTexture());
			drawRect(0, 0, g_finalMap->getWidth(), g_finalMap->getHeight());
		}
		glDisable(GL_TEXTURE_2D);
		setBlend(BLEND_ALPHA);
	}
}

void GameSim::drawPlayColor(const CamParams & camParams)
{
	if (m_gameMode != kGameMode_Lobby)
	{
		gxPushMatrix();
		applyCamParams(camParams, BACKGROUND_ZOOM_MULTIPLIER, BACKGROUND_SCREENSHAKE_MULTIPLIER);
		{
			gpuTimingBlock(drawPlayColorBackground);
			cpuTimingBlock(drawPlayColorBackground);

			//setBlend(BLEND_OPAQUE);
			m_background.draw();
			//setBlend(BLEND_ALPHA);
		}
		gxPopMatrix();
	}

	gxPushMatrix();
	applyCamParams(camParams, 1.f, 1.f);

#if 0 // fsfx test
	Shader fsfx("fsfx-test3");
	fsfx.setImmediate("time", m_roundTime);
	g_colorMap->postprocess(fsfx);
#endif

	if (m_levelEvents.gravityWell.endTimer.isActive())
	{
		setColor(255, 255, 255, 127);
		Sprite("gravitywell/well.png").drawEx(
			m_levelEvents.gravityWell.m_x,
			m_levelEvents.gravityWell.m_y,
			m_roundTime * 90.f,
			1.f, true,
			FILTER_LINEAR);
		setColor(colorWhite);
	}

	// background blocks

	m_arena.drawBlocks(*this, 0);
	m_arena.drawBlocks(*this, 1);

	m_floorEffect.draw();

	drawObjects(m_lights, MAX_LIGHTS);
	drawObjects(m_portals, MAX_PORTALS);
	drawObjects(m_pickupSpawners, MAX_PICKUP_SPAWNERS);
	drawObjects(*this, m_tileSprites, MAX_TILE_SPRITES);
	drawObjects(*this, m_pickups, MAX_PICKUPS);
	drawObjects(&m_tokenHunt.m_token, 1);
	drawObjects(m_coinCollector.m_coins, MAX_COINS);
	drawObjects(m_movers, MAX_MOVERS);
	drawObjects(m_axes, MAX_AXES);
	drawObjects(m_pipebombs, MAX_PIPEBOMBS);
	drawObjects(m_footBalls, MAX_FOOTBALLS);
	drawObjects(m_footBallGoals, MAX_FOOTBALL_GOALS);

	// players

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		const Player & player = m_players[i];

		if (player.m_isUsed)
			player.draw();
	}

	// bullets

	m_bulletPool->draw();

	// animation effects

	for (int i = 0; i < MAX_ANIM_EFFECTS; ++i)
	{
		m_animationEffects[i].draw(kDrawLayer_Game);
	}

	// particles

	setBlend(BLEND_ADD);
	m_particlePool->draw();
	setBlend(BLEND_ALPHA);

	// particle effects

	for (int i = 0; i < MAX_PARTICLE_EFFECTS; ++i)
	{
		if (m_particleEffects[i].m_data.m_isActive)
		{
			m_particleEffects[i].draw();

			// todo : draw particle effect

			// todo : add layer support
		}
	}

	drawObjects(m_fireballs, MAX_FIREBALLS);

	// foreground blocks

	m_arena.drawBlocks(*this, 2);

	// spike walls

	m_levelEvents.spikeWalls.draw();

	if (g_devMode && m_gameMode == kGameMode_Lobby)
	{
		setColor(colorGreen);
		drawRectLine(
			GAMESTATE_LOBBY_STARTZONE_X,
			GAMESTATE_LOBBY_STARTZONE_Y,
			GAMESTATE_LOBBY_STARTZONE_X + GAMESTATE_LOBBY_STARTZONE_SX,
			GAMESTATE_LOBBY_STARTZONE_Y + GAMESTATE_LOBBY_STARTZONE_SY);
	}

	gxPopMatrix();
}

void GameSim::drawPlayDecal(const CamParams & camParams)
{
	gxPushMatrix();
	applyCamParams(camParams, 1.f, 1.f);

	for (int i = 0; i < MAX_DECALS; ++i)
	{
		if (m_decals[i].m_isActive)
			m_decals[i].draw();
	}

	gxPopMatrix();
}

template <typename T> void drawLightObjects(const T * objects, int numObjects)
{
	for (int i = 0; i < numObjects; ++i)
		if (objects[i].m_isActive)
			objects[i].drawLight();
}

template <typename T> void drawLightObjects(const GameSim & gameSim, const T * objects, int numObjects)
{
	for (int i = 0; i < numObjects; ++i)
		if (objects[i].m_isActive)
			objects[i].drawLight(gameSim);
}

void GameSim::drawPlayLight(const CamParams & camParams)
{
	gxPushMatrix();
	applyCamParams(camParams, 1.f, 1.f);

	setBlend(BLEND_ADD);

	drawLightObjects(m_lights, MAX_LIGHTS);
	drawLightObjects(m_tileSprites, MAX_TILE_SPRITES);
	drawLightObjects(m_pickups, MAX_PICKUPS);
	drawLightObjects(&m_tokenHunt.m_token, 1);
	drawLightObjects(m_coinCollector.m_coins, MAX_COINS);
	drawLightObjects(m_axes, MAX_AXES);
	drawLightObjects(m_pipebombs, MAX_PIPEBOMBS);
	drawLightObjects(m_footBalls, MAX_FOOTBALLS);
	drawLightObjects(m_footBalls, MAX_FOOTBALL_GOALS);

	// bullets

	m_bulletPool->drawLight();

	// particles

	m_particlePool->drawLight();

	// particle effects

	for (int i = 0; i < MAX_PARTICLE_EFFECTS; ++i)
	{
		if (m_particleEffects[i].m_data.m_isActive)
		{
			// todo : draw particle effect

			// todo : add layer support. check if it is in light layer
		}
	}

	drawLightObjects(m_fireballs, MAX_FIREBALLS);

	// blinds effects

	for (int i = 0; i < MAX_BLINDS_EFFECTS; ++i)
	{
		m_blindsEffects[i].drawLight();
	}

	// players

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		const Player & player = m_players[i];

		if (player.m_isUsed)
			player.drawLight();
	}

	setBlend(BLEND_ALPHA);

	gxPopMatrix();
}

void GameSim::drawPlayHud(const CamParams & camParams)
{
	gxPushMatrix();
	{
		applyCamParams(camParams, 1.f, 1.f);

		// animation effects

		for (int i = 0; i < MAX_ANIM_EFFECTS; ++i)
		{
			m_animationEffects[i].draw(kDrawLayer_PlayerHUD);
		}

		// todo : draw player level HUD elements
	}
	gxPopMatrix();

	// player screen HUD elements

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (m_players[i].m_isUsed && m_players[i].m_isActive)
			m_players[i].m_statusHud.draw(*this, m_players[i].m_index);

	// animation effects

	for (int i = 0; i < MAX_ANIM_EFFECTS; ++i)
	{
		m_animationEffects[i].draw(kDrawLayer_HUD);
	}

#if ITCHIO_BUILD
	if (m_gameState >= kGameState_RoundBegin)
	{
		setColor(colorWhite);
		Sprite("itch-game-buttons.png").drawEx(1495, 45);
	}
#endif

	if (m_gameState == kGameState_RoundBegin && m_roundBegin.m_state == RoundBegin::kState_ShowControls)
	{
		setColor(colorWhite);
		Sprite("ui/controls.png").draw();
	}
}

void GameSim::applyCamParams(const CamParams & camParams, float zoomFactor, float shakeFactor) const
{
	gxTranslatef(camParams.shake[0] * shakeFactor, camParams.shake[1] * shakeFactor, 0.f);
	const float zoom = Calc::Lerp(1.f, camParams.zoom, zoomFactor);
	const Vec2 zoomFocus(
		Calc::Lerp(GFX_SX/2.f, camParams.zoomFocus[0], zoomFactor),
		Calc::Lerp(GFX_SY/2.f, camParams.zoomFocus[1], zoomFactor));
#if 1
	gxTranslatef(GFX_SX/2.f, GFX_SY/2.f, 0.f);
	gxScalef(zoom, zoom, 1.f);
	gxTranslatef(-zoomFocus[0], -zoomFocus[1], 0.f);
#endif
}

void GameSim::getCurrentTimeDilation(float & timeDilation, bool & playerAttackTimeDilation) const
{
	timeDilation = 1.f;

	playerAttackTimeDilation = false;

	if (m_gameState == kGameState_RoundBegin || m_gameState == kGameState_Play || m_gameState == kGameState_RoundComplete)
	{
		float minMultiplier = 1.f;

		for (int i = 0; i < MAX_TIMEDILATION_EFFECTS; ++i)
		{
			if (m_timeDilationEffects[i].ticksRemaining != 0)
			{
				const float t = 1.f - (m_timeDilationEffects[i].ticksRemaining / float(m_timeDilationEffects[i].ticks));
				const float multiplier = Calc::Lerp(m_timeDilationEffects[i].multiplier1, m_timeDilationEffects[i].multiplier2, t);
				if (multiplier < minMultiplier)
					minMultiplier = multiplier;
			}
		}

		timeDilation *= minMultiplier;

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_players[i].m_isUsed && m_players[i].m_isAlive && m_players[i].m_timeDilationAttack.isActive())
				playerAttackTimeDilation = true;
		}

		if (playerAttackTimeDilation && PLAYER_EFFECT_TIMEDILATION_ON_OTHERS)
			timeDilation *= PLAYER_EFFECT_TIMEDILATION_MULTIPLIER;
	}

	if (m_gameState == kGameState_RoundComplete)
	{
		if (m_roundEnd.m_state == RoundEnd::kState_ShowWinner)
			timeDilation *= m_roundEnd.m_delay;
		else
			timeDilation *= 0.f;
	}

	timeDilation *= GAME_SPEED_MULTIPLIER;
}

void GameSim::playSound(const char * filename, int volume)
{
	if (!g_app->getSelectedClient() || g_app->getSelectedClient()->m_gameSim != this)
		return;

	g_app->playSound(filename, volume);
}

void GameSim::testCollision(const CollisionShape & shape, void * arg, CollisionCB cb)
{
	// todo : only do wrapping when needed

	if (true)
	{
		for (int dx = -1; dx <= +1; ++dx)
		{
			for (int dy = -1; dy <= +1; ++dy)
			{
				CollisionShape translatedShape = shape;

				translatedShape.translate(dx * ARENA_SX_PIXELS, dy * ARENA_SY_PIXELS);

				testCollisionInternal(translatedShape, -1, arg, cb);
			}
		}
	}
	else
	{
		testCollisionInternal(shape, -1, arg, cb);
	}
}

template <typename T>
void testCollision(T * objects, int numObjects, const CollisionShape & shape, void * arg, CollisionCB cb)
{
	for (int i = 0; i < numObjects; ++i)
	{
		if (objects[i].m_isActive)
		{
			objects[i].testCollision(shape, arg, cb);
		}
	}
}

void GameSim::testCollisionInternal(const CollisionShape & shape, uint32_t typeMask, void * arg, CollisionCB cb)
{
	if (typeMask & kCollisionType_Block)
	{
		// collide vs arena

		m_arena.testCollision(shape, arg, cb);
	}

	if (typeMask & kCollisionType_Player)
	{
		// collide vs players

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_players[i].m_isUsed && m_players[i].m_isAlive)
			{
				m_players[i].testCollision(shape, arg, cb);
			}
		}
	}

	if (typeMask & kCollisionType_PhysObj)
	{
		::testCollision(m_pickups, MAX_PICKUPS, shape, arg, cb);

		if (m_gameMode == kGameMode_TokenHunt)
		{
			::testCollision(&m_tokenHunt.m_token, 1, shape, arg, cb);
		}

		if (m_gameMode == kGameMode_CoinCollector)
		{
			::testCollision(m_coinCollector.m_coins, MAX_COINS, shape, arg, cb);
		}

		::testCollision(m_axes, MAX_AXES, shape, arg, cb);
		::testCollision(m_pipebombs, MAX_PIPEBOMBS, shape, arg, cb);
		::testCollision(m_footBalls, MAX_FOOTBALLS, shape, arg, cb);
		::testCollision(m_barrels, MAX_BARRELS, shape, arg, cb);

		// collide vs movers (?)

		for (int i = 0; i < MAX_MOVERS; ++i)
		{
			//m_movers[i].testCollision(box, cb, arg);
		}
	}
}

Pickup * GameSim::allocPickup()
{
	return allocObject(m_pickups, MAX_PICKUPS);
}

void GameSim::trySpawnPickup(PickupType type)
{
	Pickup * pickup = allocPickup();
	if (pickup)
	{
		const int kMaxLocations = ARENA_SX * ARENA_SY;
		int numLocations = kMaxLocations;
		int x[kMaxLocations];
		int y[kMaxLocations];

		if (m_arena.getRandomPickupLocations(x, y, numLocations, this,
			[](void * obj, int x, int y) 
			{
			#if 0 // todo : maintain list of previous spawn locations in GameSim
				GameSim * self = (GameSim*)obj;
				for (int i = 0; i < MAX_PICKUPS; ++i)
					if (self->m_pickups[i].blockX == x && self->m_pickups[i].blockY == y)
						return true;
			#endif
				return false;
			}))
		{
			if (DEBUG_RANDOM_CALLSITES)
				LOG_DBG("Random called from trySpawnPickup");
			const int index = Random() % numLocations;
			const int spawnX = x[index];
			const int spawnY = y[index];

			spawnPickup(*pickup, type, spawnX, spawnY);
		}
	}
}

void GameSim::spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY)
{
	pickup.setup(type, blockX * BLOCK_SX + BLOCK_SX/2.f, blockY * BLOCK_SY + BLOCK_SY/2.f);
}

bool GameSim::grabPickup(int x1, int y1, int x2, int y2, Pickup & grabbedPickup)
{
	CollisionShape collisionShape;
	collisionShape.set(
		Vec2(x1, y1),
		Vec2(x2, y1),
		Vec2(x2, y2),
		Vec2(x1, y2));

	for (int i = 0; i < MAX_PICKUPS; ++i)
	{
		Pickup & pickup = m_pickups[i];

		if (pickup.m_isActive)
		{
			CollisionShape pickupCollision(pickup.m_collisionShape, pickup.m_pos);

			if (collisionShape.intersects(pickupCollision))
			{
				grabbedPickup = pickup;

				pickup = Pickup();

				playSound("gun-pickup.ogg"); // sound that plays when a player picks up a pickup. todo : different sounds per type

				return true;
			}
		}
	}

	return false;
}

void GameSim::spawnFootball()
{
	FootBall * footBall = allocObject(m_footBalls, MAX_FOOTBALLS);
	
	if (footBall)
	{
		footBall->setup(
			m_footBrawl.ballSpawnPoint[0],
			m_footBrawl.ballSpawnPoint[1]);
	}
}

void GameSim::spawnToken()
{
	Token & token = m_tokenHunt.m_token;

	const int kMaxLocations = ARENA_SX * ARENA_SY;
	int numLocations = kMaxLocations;
	int x[kMaxLocations];
	int y[kMaxLocations];

	if (m_arena.getRandomPickupLocations(x, y, numLocations, this, 0))
	{
		const int index = Random() % numLocations;
		const int spawnX = x[index];
		const int spawnY = y[index];

		token.setup(spawnX, spawnY);
	}
}

bool GameSim::pickupToken(const CollisionInfo & collisionInfo)
{
	Token & token = m_tokenHunt.m_token;

	if (token.m_isDropped && token.m_dropTimer <= 0.f)
	{
		CollisionInfo tokenCollision;
		token.getCollisionInfo(tokenCollision);

		if (tokenCollision.intersects(collisionInfo))
		{
			token = Token();
			g_gameSim->playSound("token-pickup.ogg"); // sound that plays when the token is picked up by a player

			return true;
		}
	}

	return false;
}

//

Coin * GameSim::allocCoin()
{
	for (int i = 0; i < MAX_COINS; ++i)
	{
		if (!m_coinCollector.m_coins[i].m_isDropped)
		{
			return &m_coinCollector.m_coins[i];
		}
	}

	return 0;
}

void GameSim::spawnCoin()
{
	Coin * coin = allocCoin();

	if (coin)
	{
		const int kMaxLocations = ARENA_SX * ARENA_SY;
		int numLocations = kMaxLocations;
		int x[kMaxLocations];
		int y[kMaxLocations];

		if (m_arena.getRandomPickupLocations(x, y, numLocations, this, 0))
		{
			const int index = Random() % numLocations;
			const int spawnX = x[index];
			const int spawnY = y[index];

			coin->setup(spawnX, spawnY);
		}
	}
}

bool GameSim::pickupCoin(const CollisionInfo & collisionInfo)
{
	for (int i = 0; i < MAX_COINS; ++i)
	{
		Coin & coin = m_coinCollector.m_coins[i];

		if (coin.m_isDropped && coin.m_dropTimer <= 0.f)
		{
			CollisionInfo coinCollision;
			coin.getCollisionInfo(coinCollision);

			if (coinCollision.intersects(collisionInfo))
			{
				coin = Coin();
				g_gameSim->playSound("coin-pickup.ogg"); // sound that plays when a coin is picked up by a player

				return true;
			}
		}
	}

	return false;
}

//

uint16_t GameSim::spawnBullet(int16_t x, int16_t y, uint8_t _angle, BulletType type, BulletEffect effect, uint8_t ownerPlayerId)
{
	uint16_t id;
	m_bulletPool->alloc(&id, 1);

	if (id != INVALID_BULLET_ID)
	{
		Bullet & b = m_bulletPool->m_bullets[id];

		Assert(!b.isAlive);
		b = Bullet();
		b.isAlive = true;
		b.type = static_cast<BulletType>(type);
		b.effect = static_cast<BulletEffect>(effect);
		b.m_pos[0] = x;
		b.m_pos[1] = y;
		b.color = 0xffffffff;

		float angle = _angle / 128.f * float(M_PI);
		float velocity = 0.f;

		switch (type)
		{
		case kBulletType_Gun:
			velocity = BULLET_TYPE0_SPEED;
			b.maxWrapCount = BULLET_TYPE0_MAX_WRAP_COUNT;
			b.maxReflectCount = BULLET_TYPE0_MAX_REFLECT_COUNT;
			b.maxDistanceTravelled = BULLET_TYPE0_MAX_DISTANCE_TRAVELLED;
			b.maxDestroyedBlocks = 1;
			break;
		case kBulletType_Ice:
			velocity = BULLET_TYPE0_SPEED;
			b.maxWrapCount = BULLET_TYPE0_MAX_WRAP_COUNT;
			b.maxReflectCount = BULLET_TYPE0_MAX_REFLECT_COUNT;
			b.maxDistanceTravelled = BULLET_TYPE0_MAX_DISTANCE_TRAVELLED;
			b.maxDestroyedBlocks = 1;
			break;
		case kBulletType_Grenade:
			velocity = BULLET_GRENADE_NADE_SPEED;
			b.maxWrapCount = 100;
			b.m_noGravity = false;
			b.doBounce = true;
			b.bounceAmount = BULLET_GRENADE_NADE_BOUNCE_AMOUNT;
			b.noDamageMap = true;
			b.life = BULLET_GRENADE_NADE_LIFE;
			break;
		case kBulletType_GrenadeA:
			velocity = RandomFloat(BULLET_GRENADE_FRAG_SPEED_MIN, BULLET_GRENADE_FRAG_SPEED_MAX);
			b.maxWrapCount = 1;
			b.maxReflectCount = 0;
			b.maxDistanceTravelled = RandomFloat(BULLET_GRENADE_FRAG_RADIUS_MIN, BULLET_GRENADE_FRAG_RADIUS_MAX);
			b.maxDestroyedBlocks = 1;
			b.doDamageOwner = true;
			break;
		case kBulletType_Bubble:
			velocity = RandomFloat(BULLET_BUBBLE_SPEED_MIN, BULLET_BUBBLE_SPEED_MAX);
			b.noCollide = true;
			b.maxWrapCount = 1;
			b.maxDistanceTravelled = RandomFloat(BULLET_BUBBLE_RADIUS_MIN, BULLET_BUBBLE_RADIUS_MAX);
			b.playerDamageRadius = 15;
			break;
		case kBulletType_BloodParticle:
			velocity = RandomFloat(BULLET_BLOOD_SPEED_MIN, BULLET_BLOOD_SPEED_MAX);
			b.m_noGravity = false;
			b.noDamagePlayer = true;
			b.maxWrapCount = 1;
			b.life = BULLET_BLOOD_LIFE;
			break;

		default:
			Assert(false);
			break;
		}

		b.setVel(angle, velocity);

		b.ownerPlayerId = ownerPlayerId;
	}

	return id;
}

void GameSim::spawnAxe(Vec2 pos, Vec2 vel, int playerIndex)
{
	auto * axe = allocObject(m_axes, MAX_AXES);

	if (axe)
	{
		playSound("objects/axe/throw.ogg");
		axe->setup(pos, vel, playerIndex);
	}
}

bool GameSim::grabAxe(const CollisionInfo & collision)
{
	for (int i = 0; i < MAX_AXES; ++i)
	{
		if (m_axes[i].m_isActive && m_axes[i].m_hasLanded)
		{
			CollisionInfo axeCollision;
			m_axes[i].getCollisionInfo(axeCollision);

			if (collision.intersects(axeCollision))
			{
				playSound("objects/axe/pickup.ogg");
				m_axes[i] = Axe();
				return true;
			}
		}
	}
	return false;
}

void GameSim::spawnPipeBomb(Vec2 pos, Vec2 vel, int playerIndex)
{
	auto * pipeBomb = allocObject(m_pipebombs, MAX_PIPEBOMBS);

	if (pipeBomb)
	{
		playSound("objects/pipebomb/throw.ogg");
		pipeBomb->setup(pos, vel, playerIndex);
	}
}

void GameSim::spawnParticles(const ParticleSpawnInfo & spawnInfo)
{
	uint16_t ids[MAX_BULLETS];

	const uint16_t count = m_particlePool->alloc(ids, spawnInfo.count);

	for (int i = 0; i < count; ++i)
	{
		uint16_t id = ids[i];

		Bullet & b = m_particlePool->m_bullets[id];

		initBullet(*this, b, spawnInfo);

		b.doAgeAlpha = true;
	}
}

void GameSim::triggerLevelEvent(LevelEvent e)
{
	const char * name = "<unknown level event>";

	switch (e)
	{
	case kLevelEvent_EarthQuake:
		memset(&m_levelEvents.quake, 0, sizeof(m_levelEvents.quake));
		m_levelEvents.quake.start(*this);
		name = "Earthquake";
		break;

	case kLevelEvent_GravityWell:
		memset(&m_levelEvents.gravityWell, 0, sizeof(m_levelEvents.gravityWell));
		m_levelEvents.gravityWell.endTimer = EVENT_GRAVITYWELL_DURATION;
		m_levelEvents.gravityWell.m_x = GFX_SX / 2;
		m_levelEvents.gravityWell.m_y = GFX_SY / 2;
		name = "Gravity Well";
		break;

		/*
	case kLevelEvent_DestroyBlocks:
		memset(&m_levelEvents.destroyBlocks, 0, sizeof(m_levelEvents.destroyBlocks));
		m_levelEvents.destroyBlocks.m_remainingBlockCount = 0;
		name = "Block Destruction (not yet implemented)";
		break;
		*/

	case kLevelEvent_TimeDilation:
		memset(&m_levelEvents.timeDilation, 0, sizeof(m_levelEvents.timeDilation));
		m_levelEvents.timeDilation.endTimer = 3.f;
		addTimeDilationEffect(EVENT_TIMEDILATION_MULTIPLIER_BEGIN, EVENT_TIMEDILATION_MULTIPLIER_END, EVENT_TIMEDILATION_DURATION);
		name = "Time Dilation";
		break;

	case kLevelEvent_SpikeWalls:
		memset(&m_levelEvents.spikeWalls, 0, sizeof(m_levelEvents.spikeWalls));
		m_levelEvents.spikeWalls.start(true, true);
		name = "Spike Walls";
		break;

		/*
	case kLevelEvent_Wind:
		memset(&m_levelEvents.wind, 0, sizeof(m_levelEvents.wind));
		m_levelEvents.wind.endTimer = 3.f;
		name = "Wind (not yet implemented)";
		break;

	case kLevelEvent_BarrelDrop:
		memset(&m_levelEvents.barrelDrop, 0, sizeof(m_levelEvents.barrelDrop));
		m_levelEvents.barrelDrop.endTimer = 3.f;
		m_levelEvents.barrelDrop.spawnTimer = 1.f;
		name = "Barrel Drop (not yet implemented)";
		break;

	case kLevelEvent_NightDayCycle:
		memset(&m_levelEvents.nightDayCycle, 0, sizeof(m_levelEvents.nightDayCycle));
		m_levelEvents.nightDayCycle.endTimer = 3.f;
		name = "Day/Night Cycle (not yet implemented)";
		break;
		*/
	}

	if (!RECORDMODE)
		addAnnouncement(colorBlue, "Level Event: %s", name);
}

void GameSim::doQuake(float vel)
{
	const CollisionShape shape(Vec2(0.f, 0.f), Vec2(GFX_SX, GFX_SY));

	struct QuakeArgs
	{
		float vel;
	} args;

	args.vel = vel;

	testCollisionInternal(
		shape,
		kCollisionType_Player | kCollisionType_PhysObj,
		&args,
		[](const CollisionShape & shape, void * arg, PhysicsActor * actor, BlockAndDistance * block, Player * player)
		{
			QuakeArgs * quakeArgs = (QuakeArgs*)arg;

			if (actor && actor->m_isGrounded)
			{
				actor->m_vel[1] += quakeArgs->vel;
			}

			if (player && player->m_isAlive && player->m_isGrounded)
			{
				player->m_vel[1] = Calc::Sign(player->m_facing[1]) * quakeArgs->vel;
			}
		});
}

static Vec2 calculateBlastVelocity(Vec2Arg center, float radius, const Curve & speedCurve, Vec2Arg position)
{
	const Vec2 delta = position - center;
	const float distance = delta.CalcSize();
	if (distance != 0.f)
	{
		const Vec2 dir = delta / distance;
		const float t = distance / radius;
		const float speed = speedCurve.eval(t);
		return dir * speed;
	}
	else
		return Vec2(0.f, 0.f);
}

void GameSim::doBlastEffect(Vec2Arg center, float radius, const Curve & speedCurve)
{
	const Vec2 extents(radius, radius);
	const CollisionShape shape(center - extents, center + extents);

	struct BlastArgs
	{
		Vec2 center;
		float radius;
		const Curve * speedCurve;
	} args;

	args.center = center;
	args.radius = radius;
	args.speedCurve = &speedCurve;

	testCollisionInternal(
		shape,
		kCollisionType_PhysObj,
		&args,
		[](const CollisionShape & shape, void * arg, PhysicsActor * actor, BlockAndDistance * block, Player * player)
		{
			const BlastArgs * blastArgs = (BlastArgs*)arg;

			if (actor)
				actor->m_vel += calculateBlastVelocity(blastArgs->center, blastArgs->radius, *blastArgs->speedCurve, actor->m_pos);
			if (player)
				player->m_vel += calculateBlastVelocity(blastArgs->center, blastArgs->radius, *blastArgs->speedCurve, player->m_pos);
		});
}

void GameSim::addDecal(int x, int y, const Color & color, int sprite, float scale)
{
	for (int i = 0; i < MAX_DECALS; ++i)
	{
		Decal & decal = m_decals[i];
		if (!decal.m_isActive)
		{
			decal.m_isActive = true;
			decal.x = x;
			decal.y = y;
			decal.color[0] = color.r * 255.f;
			decal.color[1] = color.g * 255.f;
			decal.color[2] = color.b * 255.f;
			decal.sprite = sprite;
			decal.scale = scale;
			if (g_decalMap)
			{
				pushSurface(g_decalMap);
				{
					decal.draw();
				}
				popSurface();
			}
			return;
		}
	}

	AssertMsg(false, "unable to find free decal");

	if (DEBUG_RANDOM_CALLSITES)
		LOG_DBG("Random called from addDecal");
	m_decals[Random() % MAX_DECALS] = Decal();
	addDecal(x, y, color, sprite, scale);
}

void GameSim::addScreenShake(float dx, float dy, float stiffness, float life, bool fade)
{
	Assert(life != 0.f);

	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		ScreenShake & shake = m_screenShakes[i];
		if (!shake.m_isActive)
		{
			shake.m_isActive = true;
			shake.fade = fade;
			shake.life = life;
			shake.lifeRcp = 1.f / life;
			shake.stiffness = stiffness;

			shake.pos.Set(dx, dy);
			shake.vel.Set(0.f, 0.f);
			return;
		}
	}

	AssertMsg(false, "unable to find free screen shake");

	if (DEBUG_RANDOM_CALLSITES)
		LOG_DBG("Random called from addScreenShake");
	m_screenShakes[Random() % MAX_SCREEN_SHAKES] = ScreenShake();
	addScreenShake(dx, dy, stiffness, life, fade);
}

Vec2 GameSim::getScreenShake() const
{
	Vec2 result;

	for (int i = 0; i < MAX_SCREEN_SHAKES; ++i)
	{
		const ScreenShake & shake = m_screenShakes[i];
		if (shake.m_isActive)
			result += shake.pos * (shake.fade ? (shake.life * shake.lifeRcp) : 1.f);
	}

	return result;
}

void GameSim::addScreenShake_GunFire(Vec2Arg dir)
{
	const float strength = 5.f;
	addScreenShake(
		dir[0] * strength,
		dir[1] * strength,
		2500.f, .3f,
		true);
}

void GameSim::addZoomEffect(float zoom, float life, int player)
{
	Assert(life != 0.f);

	for (int i = 0; i < MAX_ZOOM_EFFECTS; ++i)
	{
		ZoomEffect & zoomEffect = m_zoomEffects[i];
		if (!zoomEffect.m_isActive)
		{
			zoomEffect.m_isActive = true;
			zoomEffect.zoom = zoom;
			zoomEffect.life = life;
			zoomEffect.lifeRcp = 1.f / life;
			zoomEffect.player = player;
			return;
		}
	}

	AssertMsg(false, "unable to find free zoom effect");

	if (DEBUG_RANDOM_CALLSITES)
		LOG_DBG("Random called from addZoomEffect");
	m_zoomEffects[Random() % MAX_ZOOM_EFFECTS] = ZoomEffect();
	addZoomEffect(zoom, life, player);
}

void GameSim::setDesiredZoom(float zoom)
{
	m_desiredZoom = zoom;
}

void GameSim::setDesiredZoomFocus(Vec2Arg focus)
{
	m_desiredZoomFocus = focus;
	m_desiredZoomFocusIsSet = true;
}

float GameSim::calculateEffectiveZoom() const
{
	if (ZOOM_FACTOR != 1.f)
		return ZOOM_FACTOR;

	Vec2 min, max;
	bool hasMinMax = false;
	float maxDistanceFromCenter = 0.f;

	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (m_players[i].m_isUsed && m_players[i].m_isActive)
		{
			if (!hasMinMax)
			{
				min = max = m_players[i].m_pos;
				hasMinMax = true;
			}
			else
			{
				min = min.Min(m_players[i].m_pos);
				max = max.Max(m_players[i].m_pos);
			}


			const Vec2 delta = m_players[i].m_pos - Vec2(ARENA_SX_PIXELS/2.f, ARENA_SY_PIXELS/2.f);
			maxDistanceFromCenter = Calc::Max(maxDistanceFromCenter, delta.CalcSize());
		}
	}

	float zoom = 1.f;

	if (hasMinMax)
	{
		const float dx = max[0] - min[0];
		const float dy = max[1] - min[1];

		const float scaleX = (GFX_SX - 200.f) / (dx + .0001f);
		const float scaleY = (GFX_SY - 200.f) / (dy + .0001f);

		const float strength = Calc::Clamp(1.f - maxDistanceFromCenter / ZOOM_PLAYER_MAX_DISTANCE, 0.f, 1.f);
		const float playerZoom = Calc::Clamp(Calc::Min(scaleX, scaleY), ZOOM_FACTOR_MIN, ZOOM_FACTOR_MAX);

		zoom = Calc::Max(zoom, Calc::Lerp(1.f, playerZoom, strength));
	}

	for (int i = 0; i < MAX_ZOOM_EFFECTS; ++i)
	{
		zoom = Calc::Max(zoom, Calc::Lerp(1.f, m_zoomEffects[i].zoom, m_zoomEffects[i].life));
	}

	return zoom;
}

Vec2 GameSim::calculateEffectiveZoomFocus() const
{
	if (ZOOM_PLAYER >= 0 && ZOOM_PLAYER < MAX_PLAYERS && m_players[ZOOM_PLAYER].m_isUsed && m_players[ZOOM_PLAYER].m_isActive)
		return m_players[ZOOM_PLAYER].m_pos;
	else if (m_desiredZoomFocusIsSet)
		return m_desiredZoomFocus;
	else
	{
		// todo : iterate over all player, calculate mid point

		int numPlayers = 0;
		Vec2 mid(0.f, 0.f);

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_players[i].m_isUsed && m_players[i].m_isActive)
			{
				numPlayers++;
				mid += m_players[i].m_pos;
			}
		}

		Vec2 result;

		if (numPlayers == 0)
			result = Vec2(ARENA_SX_PIXELS / 2.f, ARENA_SY_PIXELS / 2.f);
		else
			result = mid / numPlayers;

		//

		float playerWeight = 0.f;
		Vec2 playerPos(0.f, 0.f);

		for (int i = 0; i < MAX_ZOOM_EFFECTS; ++i)
		{
			const int p = m_zoomEffects[i].player;
			if (p != -1 && m_players[p].m_isUsed && m_players[p].m_isActive)
			{
				playerWeight += m_zoomEffects[i].life;
				playerPos += m_players[p].m_pos * m_zoomEffects[i].life;
			}
		}

		if (playerWeight != 0.f)
			result = playerPos / playerWeight;

		return result;
	}
}

void GameSim::tickZoom(float dt)
{
	// todo: slowly ease in to the new zoom values

	float effectiveZoom = calculateEffectiveZoom();
	Vec2 effectiveZoomFocus = calculateEffectiveZoomFocus();
	m_desiredZoomFocusIsSet = false;

	restrictZoomParams(effectiveZoom, effectiveZoomFocus);

	const float convergeSpeed = ZOOM_CONVERGE_SPEED;
	const float a = 1.f - convergeSpeed;
	const float b = convergeSpeed;
	m_effectiveZoom = m_effectiveZoom * a + effectiveZoom * b;
	m_effectiveZoomFocus = (m_effectiveZoomFocus * a) + (effectiveZoomFocus * b);
}

void GameSim::restrictZoomParams(float & zoom, Vec2 & zoomFocus) const
{
	Vec2 screenMin(0.f, 0.f);
	Vec2 screenMax(GFX_SX, GFX_SY);

	Vec2 min = zoomFocus - screenMax / zoom / 2.f;
	Vec2 max = zoomFocus + screenMax / zoom / 2.f;

	for (int i = 0; i < 2; ++i)
	{
		if (min[i] < screenMin[i])
		{
			const float delta = screenMin[i] - min[i];
			min[i] += delta;
			max[i] += delta;
		}	

		if (max[i] > screenMax[i])
		{
			const float delta = screenMax[i] - max[i];
			min[i] += delta;
			max[i] += delta;
		}
	}

	zoom = zoom;
	zoomFocus = (min + max) / 2.f;
}

void GameSim::addLightEffect(LightEffect::Type type, float time, float amount)
{
	for (int i = 0; i < MAX_LIGHT_EFFECTS; ++i)
	{
		LightEffect & e = m_lightEffects[i];
		if (e.life == 0.f)
		{
			if (type == LightEffect::kType_Darken)
				e.setDarken(time, amount);
			else if (type == LightEffect::kType_Lighten)
				e.setLighten(time, amount);
			else
				Assert(false);
			return;
		}
	}

	AssertMsg(false, "unable to find free light effect");
}

float GameSim::getLightAmount() const
{
	const int lightingDebugMode = LIGHTING_DEBUG_MODE % 5;

	float result = 1.f;

	if (lightingDebugMode == 1)
		result = .1f + (std::sin(g_TimerRT.Time_get() / 5.f) + 1.f) / 2.f * .9f;
	else if (lightingDebugMode == 2)
		result = 1.f;
	else if (lightingDebugMode == 3)
		result = .75f;
	else if (lightingDebugMode == 4)
		result = 0.f;

	// apply darken effects

	for (int i = 0; i < MAX_LIGHT_EFFECTS; ++i)
	{
		const LightEffect & e = m_lightEffects[i];

		if (e.type == LightEffect::kType_Darken && e.life != 0.f)
		{
			const float t = e.life * e.lifeRcp;
			result = Calc::Min(result, Calc::Lerp(1.f, e.amount, t));
		}
	}

	// apply lighten effects

	for (int i = 0; i < MAX_LIGHT_EFFECTS; ++i)
	{
		const LightEffect & e = m_lightEffects[i];

		if (e.type == LightEffect::kType_Lighten && e.life != 0.f)
		{
			const float t = e.life * e.lifeRcp;
			result = Calc::Max(result, Calc::Lerp(0.f, e.amount, t));
		}
	}

	return result;
}

void GameSim::addFloorEffect(int playerId, int x, int y, int size, int damageSize)
{
	m_floorEffect.trySpawnAt(*this, playerId, x, y, -BLOCK_SX, size, damageSize);
	m_floorEffect.trySpawnAt(*this, playerId, x, y, +BLOCK_SX, size, damageSize);
}

void GameSim::addBlindsEffect(int playerId, int x, int y, int size, bool vertical, float time, const char * text)
{
	for (int i = 0; i < MAX_BLINDS_EFFECTS; ++i)
	{
		if (m_blindsEffects[i].m_time == 0.f)
		{
			m_blindsEffects[i].setup(
				time,
				x,
				y,
				100,
				vertical,
				text);
			break;
		}
	}
}

Portal * GameSim::findPortal(float x1, float y1, float x2, float y2, bool mustEncapsulate, bool applySafeZone, int & id)
{
	for (int i = 0; i < MAX_PORTALS; ++i)
	{
		Portal & portal = m_portals[i];

		if (portal.m_isActive && portal.intersects(x1, y1, x2, y2, mustEncapsulate, applySafeZone))
		{
			id = i;
			return &m_portals[i];
		}
	}

	return 0;
}

TileSprite * GameSim::findTileSpriteAtPos(int x, int y)
{
	for (int i = 0; i < MAX_TILE_SPRITES; ++i)
	{
		TileSprite & tileSprite = m_tileSprites[i];

		if (tileSprite.m_isActive && tileSprite.intersects(x, y))
			return &tileSprite;
	}

	return 0;
}

TileSprite * GameSim::findTileSpriteAtBlockXY(int blockX, int blockY)
{
	const int x = blockX * BLOCK_SX + BLOCK_SX/2;
	const int y = blockY * BLOCK_SY + BLOCK_SY/2;
	return findTileSpriteAtPos(x, y);
}

void GameSim::addAnimationFx(DrawLayer layer, const char * fileName, int x, int y, bool flipX, bool flipY)
{
	auto * animFx = allocObject(m_animationEffects, MAX_ANIM_EFFECTS);

	if (animFx)
	{
		*animFx = AnimationFxState();
		animFx->m_isActive = true;
		animFx->m_layer = layer;
		animFx->m_fileName = fileName;
		animFx->m_state.x = x;
		animFx->m_state.y = y;
		animFx->m_state.flipX = flipX;
		animFx->m_state.flipY = flipY;
		animFx->m_state.startAnim(Spriter(fileName), 0);
	}
}

void GameSim::addAnnouncement(const Color & color, const char * message, ...)
{
	char text[1024];
	va_list args;
	va_start(args, message);
	vsprintf_s(text, sizeof(text), message, args);
	va_end(args);

	AnnounceInfo info;
	info.timeLeft = 3.f;
	info.message = text;
	info.color = color;
	m_annoucements.push_back(info);
}

void GameSim::addFireBall()
{
	auto * fireBall = allocObject(m_fireballs, MAX_FIREBALLS);

	if (fireBall)
	{
		fireBall->load("backgrounds/VolcanoTest/Fireball/fireball.scml", *this, (Random() % 1400) + 260, -80, RandomFloat(70.0f, 110.0f), RandomFloat(.2f, .4f));
	}
}

//

void updatePhysics(GameSim & gameSim, Vec2 & pos, Vec2 & vel, float dt, const CollisionShape & shape, void * arg, PhysicsUpdateCB cb)
{
	for (int i = 0; i < 2; ++i)
	{
		Vec2 delta;
		delta[i] = vel[i] * dt;

		if (delta[i] == 0.f)
			continue;

		const Vec2 newPos = pos + delta;

		PhysicsUpdateInfo updateInfo;

		updateInfo.arg = arg;
		updateInfo.cb = cb;
		updateInfo.shape = shape;
		updateInfo.shape.translate(newPos[0], newPos[1]);

		updateInfo.axis = i;
		updateInfo.pos = newPos;
		updateInfo.delta = delta;

		gameSim.testCollision(
			updateInfo.shape,
			&updateInfo,
			[](const CollisionShape & shape, void * arg, PhysicsActor * actor, BlockAndDistance * blockInfo, Player * player)
			{
				PhysicsUpdateInfo & updateInfo = *reinterpret_cast<PhysicsUpdateInfo*>(arg);
				updateInfo.actor = actor;
				updateInfo.blockInfo = blockInfo;
				updateInfo.player = player;

				int flags = 0;

				bool collision = false;

				Vec2 contactNormal;
				float contactDistance;

				if (blockInfo)
				{
					Block * block = blockInfo->block;

					if (!((1 << block->type) & kBlockMask_Solid))
						flags |= kPhysicsUpdateFlag_DontCollide;

					CollisionShape blockShape;
					Arena::getBlockCollision(
						block->shape,
						blockShape,
						blockInfo->x,
						blockInfo->y);

					if (shape.checkCollision(blockShape, updateInfo.delta, contactDistance, contactNormal))
					{
						collision = true;
					}
				}
				else
				{
					collision = true;

					// todo : contact info for player/actor
					contactDistance = 0.f;
					contactNormal.Set(0.f, 0.f);
				}

				if (collision)
				{
					updateInfo.contactRestitution = 0.f;

					if (updateInfo.cb)
					{
						updateInfo.contactNormal = contactNormal;
						updateInfo.contactDistance = contactDistance;

						flags |= updateInfo.cb(updateInfo);
					}

					if (!(flags & kPhysicsUpdateFlag_DontCollide))
					{
						ContactInfo contact;
						contact.n = contactNormal;
						contact.d = contactDistance;
						contact.r = updateInfo.contactRestitution + 1.f;
						contact.f = flags;
						updateInfo.contacts.push_back(contact);
					}
				}
			});

		pos = newPos;

		auto u = std::unique(updateInfo.contacts.begin(), updateInfo.contacts.end());
		updateInfo.contacts.resize(std::distance(updateInfo.contacts.begin(), u));

		for (auto contact = updateInfo.contacts.begin(); contact != updateInfo.contacts.end(); ++contact)
		{
			const Vec2 offset = contact->n * contact->d;

			pos += offset;

			if (!(contact->f & kPhysicsUpdateFlag_DontUpdateVelocity))
			{
				const float d = vel * contact->n * contact->r;

				if (d > 0.f)
				{
					vel -= contact->n * d;

					//logDebug("vel = %f, %f", m_vel[0], m_vel[1]);
				}
			}
		}
	}
}
