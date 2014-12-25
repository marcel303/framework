#pragma once

#include "arena.h"
#include "gametypes.h"
#include "Vec2.h"

#define MAX_PLAYERS 4
#define MAX_BULLETS 1000
#define MAX_PARTICLES 1000
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
	int m_characterIndex;

	//

	int m_score;
	int m_totalScore;

	//

	Vec2 m_pos;
	Vec2 m_vel;

	Vec2 m_facing;

	//

	int m_anim;
	bool m_animPlay;

	//

	int m_attackDirection[2];

	//

	int m_weaponAmmo;
	int m_weaponType;

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

		bool attacking;
		bool hitDestructible;
		Vec2 attackVel;
		CollisionInfo collision;
		bool hasCollision;
	} m_attack;

	struct TeleportInfo
	{
		TeleportInfo()
			: cooldown(false)
			, x(0)
			, y(0)
		{
		}

		bool cooldown;
		int x;
		int y;
	} m_teleport;

	struct JumpInfo
	{
		JumpInfo()
		{
			memset(this, 0, sizeof(*this));
		}

		bool cancelStarted;
		bool cancelled;
		int cancelX;
		float cancelFacing;
	} m_jump;

	bool m_isGrounded;
	bool m_isAttachedToSticky;
	bool m_isAnimDriven;

	bool m_isAirDashCharged;
	bool m_isWallSliding;

	bool m_animVelIsAbsolute;
	Vec2 m_animVel;
	bool m_animAllowGravity;
	bool m_animAllowSteering;

	//Dictionary m_props;

	//Sprite * m_sprite;
	//float m_spriteScale;

	bool m_isRespawn;
};

class GameSim
{
public:
	ArenaNetObject m_arenaNetObject;

	struct GameState
	{
		GameState(ArenaNetObject * arenaNetObject)
		{
			memset(this, 0, sizeof(*this));

			m_arena.init(arenaNetObject);
		}

		uint32_t Random();
		uint32_t GetTick();

		uint32_t m_tick;
		uint32_t m_randomSeed;

		Arena m_arena;

		Player m_players[MAX_PLAYERS];

		Pickup m_pickups[MAX_PICKUPS];
		Pickup m_grabbedPickup;
		uint64_t m_nextPickupSpawnTick;

	} m_state;

	PlayerNetObject * m_players[MAX_PLAYERS];

	GameSim()
		: m_arenaNetObject()
		, m_state(&m_arenaNetObject)
	{
	}

	uint32_t calcCRC() const;

	void setPlayerInputs(uint8_t playerId, uint32_t buttons);
	void tick();

	void trySpawnPickup(PickupType type);
	void spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY);
	Pickup * grabPickup(int x1, int y1, int x2, int y2);
};
