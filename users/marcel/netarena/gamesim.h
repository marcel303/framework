#pragma once

#include "arena.h"
#include "gametypes.h"
#include "Random.h"
#include "Vec2.h"

#define MAX_PLAYERS 4
//#define MAX_BULLETS 1000
//#define MAX_PARTICLES 1000
#define MAX_PICKUPS 10

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
	void handleDamage(float amount, Vec2Arg velocity, Player * attacker);
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
		{
		}

		bool attacking : 1;
		bool hitDestructible : 1;
		bool hasCollision : 1;
		CollisionInfo collision;
		Vec2 attackVel;
	} m_attack;

	struct TeleportInfo
	{
		TeleportInfo()
			: cooldown(false)
			, x(0)
			, y(0)
		{
		}

		bool cooldown : 1;
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


	float m_respawnTimer;
	bool m_canRespawn;
	bool m_isRespawn;

	bool m_isGrounded : 1;
	bool m_isAttachedToSticky : 1;
	bool m_isAnimDriven : 1;

	bool m_isAirDashCharged : 1;
	bool m_isWallSliding : 1;

	bool m_animVelIsAbsolute : 1;
	bool m_animAllowGravity : 1;
	bool m_animAllowSteering : 1;
	Vec2 m_animVel;
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

	void trySpawnPickup(PickupType type);
	void spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY);
	Pickup * grabPickup(int x1, int y1, int x2, int y2);
};
