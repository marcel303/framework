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

class BulletPool;
class NetSpriteManager;
class PlayerNetObject;

struct Player
{
	PlayerNetObject * m_netObject;

	Player()
	{
		memset(this, 0, sizeof(Player));
	}

	Player(uint8_t index, uint16_t owningChannelId)
	{
		memset(this, 0, sizeof(Player));

		m_index = index;
		m_owningChannelId = owningChannelId;

		m_characterIndex = -1;
		m_facing.Set(+1.f, +1.f);

		m_animAllowGravity = true;
		m_animAllowSteering = true;

		m_weaponType = kPlayerWeapon_Fire;

		m_lastSpawnIndex = -1;
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

	bool hasValidCharacterIndex() const;

	void setAnim(int anim, bool play, bool restart);
	void setAttackDirection(int dx, int dy);
	void applyAnim();

	uint32_t getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const;
	uint32_t getIntersectingBlocksMask(int x, int y) const;

	void handleNewGame();
	void handleNewRound();

	void respawn();
	bool handleDamage(float amount, Vec2Arg velocity, Player * attacker);
	void awardScore(int score);

	char * makeCharacterFilename(const char * filename);

	bool m_isUsed;

	uint8_t m_index;
	uint16_t m_owningChannelId;

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
			, hasCollision(false)
			, collision()
			, attackVel()
			, cooldown(0.f)
		{
			memset(this, 0, sizeof(AttackInfo));
		}

		bool attacking;
		bool hitDestructible;
		bool hasCollision;
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
			memset(this, 0, sizeof(TeleportInfo));
		}

		bool cooldown; // when set, we're waiting for the player to exit (x, y), which is the destination of the previous teleport
		int16_t x;
		int16_t y;
	} m_teleport;

	struct JumpInfo
	{
		JumpInfo()
		{
			memset(this, 0, sizeof(JumpInfo));
		}

		bool cancelStarted;
		bool cancelled;
		int16_t cancelX;
		int8_t cancelFacing;
	} m_jump;

	float m_respawnTimer; // when this timer counts to zero, the player is automatically respawn
	bool m_canRespawn; // set when the player is allowed to respawn, which is after the death animation is done
	bool m_canTaunt; // set when the player is allowed to taunt, which is after the death animation is done. it's reset after a taunt
	bool m_isRespawn; // set after the first respawn. the first spawn is special, as the player doesn't need to press X and isn't allowed to use taunt
	int m_lastSpawnIndex;

	bool m_isGrounded; // set when the player is walking on ground
	bool m_isAttachedToSticky;
	bool m_isAnimDriven; // an animation is active that drives the player using animation actions/triggers

	bool m_isAirDashCharged; // reset when air dash is used. set when the player hits the ground
	bool m_isWallSliding;

	bool m_animVelIsAbsolute; // should the animation velocity be added to or replace the regular player velocity?
	bool m_animAllowGravity;
	bool m_animAllowSteering; // allow the player to control the character?
	Vec2 m_animVel;

	bool m_enterPassthrough; // if set, the player will move through passthrough blocks, without having to press DOWN. this mode is set when using the sword-down attack, and reset when the player hits the ground
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

struct GameStateData
{
	GameStateData()
	{
		memset(this, 0, sizeof(GameStateData));

		m_gameState = kGameState_Play;
	}

	uint32_t Random();
	float RandomFloat(float min, float max) { float t = (Random() & 4095) / 4095.f; return t * min + (1.f - t) * max; }
	uint32_t GetTick();

	uint32_t m_tick;
	uint32_t m_randomSeed;

	GameState m_gameState;

	Player m_players[MAX_PLAYERS];

	Pickup m_pickups[MAX_PICKUPS];
	Pickup m_grabbedPickup;
	uint64_t m_nextPickupSpawnTick;
};

class GameSim : public GameStateData
{
public:
	// serialized

	Arena m_arena;

	BulletPool * m_bulletPool;

	BulletPool * m_particlePool;

	ScreenShake m_screenShakes[MAX_SCREEN_SHAKES];

	// non-serialized (RPC)

	PlayerNetObject * m_playerNetObjects[MAX_PLAYERS];

	GameSim();
	~GameSim();

	uint32_t calcCRC() const;
	void serialize(NetSerializationContext & context);
	void clearPlayerPtrs() const;
	void setPlayerPtrs() const;

	void setGameState(::GameState gameState);

	void tick();
	void tickLobby();
	void tickPlay();
	void tickRoundComplete();
	void anim(float dt);

	void trySpawnPickup(PickupType type);
	void spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY);
	Pickup * grabPickup(int x1, int y1, int x2, int y2);

	void spawnParticles(const ParticleSpawnInfo & spawnInfo);

	void addScreenShake(float dx, float dy, float stiffness, float life);
	Vec2 getScreenShake() const;
};

extern GameSim * g_gameSim;
