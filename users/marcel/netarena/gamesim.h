#pragma once

#include "arena.h"
#include "gametypes.h"
#include "Random.h"
#include "Vec2.h"

#define MAX_PLAYERS 4
//#define MAX_BULLETS 1000
//#define MAX_PARTICLES 1000
#define MAX_PICKUPS 10
#define MAX_SCREEN_SHAKES 4

#include <string.h> // todo : cpp

class PlayerNetObject;

struct Player
{
	PlayerNetObject * m_netObject;

	Player()
	{
		memset(this, 0, sizeof(*this));

		m_characterIndex = -1;
		m_facing.Set(+1.f, +1.f);

		m_animAllowGravity = true;
		m_animAllowSteering = true;

		m_weaponType = kPlayerWeapon_Fire;
	}

	void tick(float dt); // todo : remove dt
	void draw();
	void drawAt(int x, int y);
	void debugDraw();

	void playSecondaryEffects(PlayerEvent e);

	void getPlayerCollision(CollisionInfo & collision);
	void getDamageHitbox(CollisionInfo & collision);
	void getAttackCollision(CollisionInfo & collision);
	float getAttackDamage(Player * other);

	bool isAnimOverrideAllowed(int anim) const;
	float mirrorX(float x) const;
	float mirrorY(float y) const;

	uint32_t getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const;
	uint32_t getIntersectingBlocksMask(int x, int y) const;

	void handleNewGame();
	void handleNewRound();

	void respawn();
	bool handleDamage(float amount, Vec2Arg velocity, Player * attacker);
	void awardScore(int score);

	char * makeCharacterFilename(const char * filename);

	bool m_isAlive;
	uint8_t m_characterIndex;
	float m_controlDisableTime;

	//

	int8_t m_score;
	int16_t m_totalScore;

	//

	Vec2 m_pos;
	Vec2 m_vel;

	Vec2 m_facing;

	//

	uint8_t m_anim;
	bool m_animPlay;

	//

	int8_t m_attackDirection[2];

	//

	uint8_t m_weaponAmmo;
	uint8_t m_weaponType;

	//

	CollisionInfo m_collision;

	uint32_t m_blockMask;

	//

	struct AttackInfo
	{
		AttackInfo()
			: attacking(false)
			, hitDestructible(false)
			, attackVel()
			, hasCollision(false)
			, cooldown(0.f)
		{
		}

		bool attacking : 1;
		bool hitDestructible : 1;
		bool hasCollision : 1;
		CollisionInfo collision;
		Vec2 attackVel;
		float cooldown; // this timer needs to hit zero before the player can attack again. it's decremented AFTER the attack animation has finished
	} m_attack;

	struct TeleportInfo
	{
		TeleportInfo()
			: cooldown(false)
			, x(0)
			, y(0)
		{
		}

		bool cooldown : 1; // when set, we're waiting for the player to exit (x, y), which is the destination of the previous teleport
		int16_t x;
		int16_t y;
	} m_teleport;

	struct JumpInfo
	{
		JumpInfo()
		{
			memset(this, 0, sizeof(*this));
		}

		bool cancelStarted : 1;
		bool cancelled : 1;
		int16_t cancelX;
		int8_t cancelFacing;
	} m_jump;

	float m_respawnTimer; // when this timer counts to zero, the player is automatically respawn
	bool m_canRespawn; // set when the player is allowed to respawn, which is after the death animation is done
	bool m_canTaunt; // set when the player is allowed to taunt, which is after the death animation is done. it's reset after a taunt
	bool m_isRespawn; // set after the first respawn. the first spawn is special, as the player doesn't need to press X and isn't allowed to use taunt

	bool m_isGrounded : 1; // set when the player is walking on ground
	bool m_isAttachedToSticky : 1;
	bool m_isAnimDriven : 1; // an animation is active that drives the player using animation actions/triggers

	bool m_isAirDashCharged : 1; // reset when air dash is used. set when the player hits the ground
	bool m_isWallSliding : 1;

	bool m_animVelIsAbsolute : 1; // should the animation velocity be added to or replace the regular player velocity?
	bool m_animAllowGravity : 1;
	bool m_animAllowSteering : 1; // allow the player to control the character?
	Vec2 m_animVel;

	bool m_enterPassthrough : 1; // if set, the player will move through passthrough blocks, without having to press DOWN. this mode is set when using the sword-down attack, and reset when the player hits the ground
};

struct ScreenShake
{
	bool isActive;
	Vec2 pos;
	Vec2 vel;
	float stiffness;
	float life;

	void tick(float dt)
	{
		Vec2 force = pos * (-stiffness);
		vel += force * dt;
		pos += vel * dt;

		life -= dt;

		if (life <= 0.f)
			isActive = false;
	}
};

class GameSim
{
public:
	ArenaNetObject m_arenaNetObject;

	struct GameState
	{
		GameState()
		{
			memset(this, 0, sizeof(*this));
		}

		uint32_t Random();
		uint32_t GetTick();

		uint32_t m_tick;
		uint32_t m_randomSeed;

		Player m_players[MAX_PLAYERS];

		Pickup m_pickups[MAX_PICKUPS];
		Pickup m_grabbedPickup;
		uint64_t m_nextPickupSpawnTick;
	} m_state;

	Arena m_arena;

	PlayerNetObject * m_players[MAX_PLAYERS];

	ScreenShake m_screenShakes[MAX_SCREEN_SHAKES];

	GameSim()
		: m_arenaNetObject()
		, m_state()
	{
		m_arena.init(&m_arenaNetObject);

		for (int i = 0; i < MAX_PLAYERS; ++i)
			m_players[i] = 0;
	}

	uint32_t calcCRC() const;
	void serialize(NetSerializationContext & context);

	void tick();
	void anim(float dt);

	void trySpawnPickup(PickupType type);
	void spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY);
	Pickup * grabPickup(int x1, int y1, int x2, int y2);

	void addScreenShake(Vec2 delta, float stiffness, float life);
	Vec2 getScreenShake() const;
};

extern GameSim * g_gameSim;
