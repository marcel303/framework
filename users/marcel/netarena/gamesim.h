#pragma once

#include <string.h> // memset
#include "arena.h"
#include "gametypes.h"
#include "physobj.h"
#include "Random.h"
#include "Vec2.h"

class BulletPool;
class Color;
class NetSpriteManager;
struct ParticleSpawnInfo;
class PlayerInstanceData;

#pragma pack(push)
#pragma pack(1)

struct Player
{
	PlayerInstanceData * m_instanceData;

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
		m_enableInAirAnim = true;

		m_lastSpawnIndex = -1;
	}

	void tick(float dt); // todo : remove dt
	void draw() const;
	void drawAt(int x, int y) const;
	void drawLight() const;
	void debugDraw() const;

	void playSecondaryEffects(PlayerEvent e);

	bool getPlayerCollision(CollisionInfo & collision) const;
	void getDamageHitbox(CollisionInfo & collision) const;
	void getAttackCollision(CollisionInfo & collision) const;
	float getAttackDamage(Player * other) const;

	bool isAnimOverrideAllowed(int anim) const;
	float mirrorX(float x) const;
	float mirrorY(float y) const;

	bool hasValidCharacterIndex() const;
	void setDisplayName(const std::string & name);

	void setAnim(int anim, bool play, bool restart);
	void clearAnimOverrides();
	void setAttackDirection(int dx, int dy);
	void applyAnim();

	uint32_t getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const;
	uint32_t getIntersectingBlocksMask(int x, int y) const;

	void handleNewGame();
	void handleNewRound();

	void respawn();
	void cancelAttack();
	void handleImpact(Vec2Arg velocity);
	bool shieldAbsorb(float amount);
	bool handleDamage(float amount, Vec2Arg velocity, Player * attacker);
	bool handleIce(Vec2Arg velocity, Player * attacker);
	bool handleBubble(Vec2Arg velocity, Player * attacker);
	void awardScore(int score);
	void dropCoins(int numCoins);

	void pushWeapon(PlayerWeapon weapon, int ammo);
	PlayerWeapon popWeapon();

	char * makeCharacterFilename(const char * filename);

	// allocation
	bool m_isUsed;

	uint8_t m_index;
	uint16_t m_owningChannelId;

	// character select
	char m_displayName[MAX_PLAYER_DISPLAY_NAME + 1];
	bool m_isReadyUpped;

	// alive state
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

	PlayerWeapon m_weaponStack[MAX_WEAPON_STACK_SIZE];
	uint8_t m_weaponStackSize;

	//

	struct
	{
		PlayerSpecial type;
		float meleeAnimTimer;
		int meleeCounter;
		float attackDownHeight;
		bool attackDownActive;
	} m_special;

	int m_traits; // PlayerTrait

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

	struct ShieldInfo
	{
		ShieldInfo()
		{
			memset(this, 0, sizeof(ShieldInfo));
		}

		int shield;
	} m_shield;

	struct IceInfo
	{
		float timer;
	} m_ice;

	struct BubbleInfo
	{
		float timer;
	} m_bubble;

	float m_respawnTimer; // when this timer counts to zero, the player is automatically respawn
	bool m_canRespawn; // set when the player is allowed to respawn, which is after the death animation is done
	bool m_canTaunt; // set when the player is allowed to taunt, which is after the death animation is done. it's reset after a taunt
	bool m_isRespawn; // set after the first respawn. the first spawn is special, as the player doesn't need to press X and isn't allowed to use taunt
	int m_lastSpawnIndex;
	int m_spawnInvincibilityTicks;

	bool m_isGrounded; // set when the player is walking on ground
	bool m_isAttachedToSticky;
	bool m_isAnimDriven; // an animation is active that drives the player using animation actions/triggers
	bool m_enableInAirAnim;

	bool m_isAirDashCharged; // reset when air dash is used. set when the player hits the ground
	bool m_isWallSliding;

	bool m_isUsingJetpack;

	bool m_animVelIsAbsolute; // should the animation velocity be added to or replace the regular player velocity?
	bool m_animAllowGravity;
	bool m_animAllowSteering; // allow the player to control the character?
	Vec2 m_animVel;

	bool m_enterPassthrough; // if set, the player will move through passthrough blocks, without having to press DOWN. this mode is set when using the sword-down attack, and reset when the player hits the ground

	struct TokenHunt
	{
		TokenHunt()
		{
			memset(this, 0, sizeof(TokenHunt));
		}

		bool m_hasToken;
	} m_tokenHunt;
};

struct Pickup : PhysicsActor
{
	bool isAlive;

	PickupType type;
	uint8_t blockX;
	uint8_t blockY;

	Pickup()
	{
		memset(this, 0, sizeof(Pickup));
	}

	void setup(PickupType type, int blockX, int blockY);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct Token : PhysicsActor
{
	bool m_isDropped;
	float m_dropTimer;

	Token()
	{
		memset(this, 0, sizeof(Token));
	}

	void setup(int blockX, int blockY);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};
\
struct Coin : PhysicsActor
{
	bool m_isDropped;
	float m_dropTimer;

	Coin()
	{
		memset(this, 0, sizeof(Coin));
	}

	void setup(int blockX, int blockY);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct Barrel
{
	bool m_isActive;
	Vec2 m_pos;
	int m_tick; // barrel velocity/position depends on tick number

	Barrel()
	{
		memset(this, 0, sizeof(Barrel));
	}

	void setup(int x, int y);

	void tick(GameSim & gameSim, float dt) { } // todo : check if we are hit by a player using melee, if > GFX_SY, disable
	void draw() const { }
	void drawLight() const { }
};

struct Torch
{
	bool m_isAlive;
	Vec2 m_pos;
	float m_color[4];

	Torch()
	{
		memset(this, 0, sizeof(Torch));
	}

	void setup(float x, float y, const Color & color);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct ScreenShake
{
	bool isActive;
	Vec2 pos;
	Vec2 vel;
	float stiffness;
	float life;

	ScreenShake()
	{
		memset(this, 0, sizeof(ScreenShake));
	}

	void tick(float dt);
};

struct FloorEffect
{
	struct ActiveTile
	{
		int16_t x;
		int16_t y;
		int8_t size;
		int8_t damageSize;
		int8_t dx;
		int8_t time;
		int8_t playerId;
	} m_tiles[MAX_FLOOR_EFFECT_TILES];

	FloorEffect()
	{
		memset(this, 0, sizeof(FloorEffect));
	}

	void tick(GameSim & gameSim, float dt);
	void trySpawnAt(GameSim & gameSim, int playerId, int x, int y, int dx, int size, int damageSize);
};

// level events

// todo : some events can be combined. make a list of events that can be combined!

struct LevelEventTimer
{
	float m_duration;
	float m_time;

	// tick timer and return whether its active
	bool tickActive(float dt)
	{
		Assert(m_time >= 0.f && m_time <= m_duration);
		if (m_time < m_duration)
		{
			m_time += dt;
			if (m_time >= m_duration)
				m_time = m_duration;
			return true;
		}
		return false;
	}

	// tick timer and return whether it completed
	bool tickComplete(float dt)
	{
		Assert(m_time >= 0.f && m_time <= m_duration);
		if (m_time < m_duration)
		{
			m_time += dt;
			if (m_time >= m_duration)
			{
				m_time = m_duration;
				return true;
			}
		}
		return false;
	}

	float getProgress() const
	{
		Assert(m_time >= 0.f && m_time <= m_duration);
		if (m_duration == 0.f)
			return 0.f;
		else
			return m_time / m_duration;
	}

	bool isActive() const
	{
		Assert(m_time >= 0.f && m_time <= m_duration);
		return m_time < m_duration;
	}

	void operator=(float time)
	{
		Assert(time >= 0.f);
		m_duration = time;
		m_time = 0.f;
	}
};

// the earth suddenly starts shaking. players will loose their footing and be kicked into the air
struct LevelEvent_EarthQuake
{
	LevelEventTimer endTimer;
	LevelEventTimer quakeTimer;
};

// a gravity well appears. players get sucked into it and get concentrated into a smaller area
// note : should be possible to escape the well by moving, to avoid getting stuck against walls
//         but it should be hard near the center to escape!
struct LevelEvent_GravityWell
{
	LevelEventTimer endTimer;
	int m_x;
	int m_y;
};

// destructible blocks start exploding! the level will slowly disintegrate!
// note : should only be activated when there's enough blocks in a level. stop at 50% or so of the
//        starting number of blocks
struct LevelEvent_DestroyDestructibleBlocks
{
	int m_remainingBlockCount;
	LevelEventTimer destructionTimer;
};

// time suddenly slows down for a while
// note : need something visual to indicate the effect. maybe a giant hourglas appears on the background
//        layer, floating about
struct LevelEvent_TimeDilation
{
	LevelEventTimer endTimer;
};

// spike walls start closing in from the left, right, or both sides
// note : should be non lethal when they appear. deploy spikes at some point
// note : disable respawning while effect is active to avoid respawning in wall?
struct LevelEvent_SpikeWalls
{
	LevelEventTimer endTimer;

	bool m_right;
	bool m_left;
};

// the wind suddenly starts blowing. players get accelerated in the left/right direction
// note: need visual. maybe a wind layer with leafs on the foreground layer, maybe rain?
struct LevelEvent_Wind
{
	LevelEventTimer endTimer;
};

// barrels start dropping from the sky! barrels can be hit for powerful attack
// note : should do auto aim to some extent so it's actually possible to hit other players
// note : collision should be disabled on barrels. fake gravity/no gravity/floatiness
struct LevelEvent_BarrelDrop
{
	LevelEventTimer endTimer;
	LevelEventTimer spawnTimer;
};

// the level turns dark for a while. players have limited vision
// note : accompanied by lightning and rain effects?
struct LevelEvent_NightDayCycle
{
	LevelEventTimer endTimer;
};

enum LevelEvent
{
	kLevelEvent_EarthQuake,
	kLevelEvent_GravityWell,
	kLevelEvent_DestroyBlocks,
	kLevelEvent_TimeDilation,
	kLevelEvent_SpikeWalls,
	kLevelEvent_Wind,
	kLevelEvent_BarrelDrop,
	kLevelEvent_NightDayCycle,
	kLevelEvent_COUNT
};

//

struct GameStateData
{
	GameStateData()
	{
		memset(this, 0, sizeof(GameStateData));
	}

	uint32_t Random();
	float RandomFloat(float min, float max) { float t = (Random() & 4095) / 4095.f; return t * min + (1.f - t) * max; }
	uint32_t GetTick() const;
	float getRoundTime() const;
	void addTimeDilationEffect(float multiplier1, float multiplier2, float duration);
	LevelEvent getRandomLevelEvent();

	uint32_t m_tick;
	uint32_t m_randomSeed;

	uint32_t m_gameStartTicks;

	struct TimeDilationEffect
	{
		float multiplier1;
		float multiplier2;
		int ticks;
		int ticksRemaining;
	} m_timeDilationEffects[MAX_TIMEDILATION_EFFECTS];

	GameState m_gameState;
	GameMode m_gameMode;

	float m_roundTime;
	uint32_t m_nextRoundNumber;
	uint32_t m_roundCompleteTicks;
	uint32_t m_roundCompleteTimeDilationTicks;

	Player m_players[MAX_PLAYERS];

	// pickups

	Pickup m_pickups[MAX_PICKUPS];
	Pickup m_grabbedPickup;
	uint64_t m_nextPickupSpawnTick;

	// barrels

	Barrel m_barrels[MAX_BARRELS];

	// effects

	FloorEffect m_floorEffect;

	Torch m_torches[MAX_TORCHES];

	// level events

	struct LevelEvents
	{
		LevelEvents()
		{
			memset(this, 0, sizeof(LevelEvents));
		}

		LevelEvent_EarthQuake quake;
		LevelEvent_GravityWell gravityWell;
		LevelEvent_DestroyDestructibleBlocks destroyBlocks;
		LevelEvent_TimeDilation timeDilation;
		LevelEvent_SpikeWalls spikeWalls;
		LevelEvent_Wind wind;
		LevelEvent_BarrelDrop barrelDrop;
		LevelEvent_NightDayCycle nightDayCycle;
	} m_levelEvents;

	float m_timeUntilNextLevelEvent;

	// support for game modes

	struct TokenHunt
	{
		Token m_token;
	} m_tokenHunt;

	struct CoinCollector
	{
		CoinCollector()
			: m_nextSpawnTick(0)
		{
		}

		Coin m_coins[MAX_COINS];
		uint64_t m_nextSpawnTick;
	} m_coinCollector;
};

#pragma pack(pop)

class GameSim : public GameStateData
{
public:
	// serialized

	Arena m_arena;

	BulletPool * m_bulletPool;

	BulletPool * m_particlePool;

	ScreenShake m_screenShakes[MAX_SCREEN_SHAKES];

	// non-serialized (RPC)

	PlayerInstanceData * m_playerInstanceDatas[MAX_PLAYERS];

	GameSim();
	~GameSim();

#if ENABLE_GAMESTATE_DESYNC_DETECTION
	uint32_t calcCRC() const;
#endif

	void serialize(NetSerializationContext & context);
	void clearPlayerPtrs() const;
	void setPlayerPtrs() const;

	void setGameState(::GameState gameState);
	void setGameMode(GameMode gameMode);

	void newGame();
	void newRound(const char * mapOverride);
	void endRound();

	void load(const char * filename);
	void resetGameWorld();
	void resetPlayers();
	void resetGameSim();

	void tick();
	void tickMenus();
	void tickPlay();
	void tickRoundComplete();
	void anim(float dt);

	void playSound(const char * filename, int volume = 100);

	void trySpawnPickup(PickupType type);
	void spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY);
	Pickup * grabPickup(int x1, int y1, int x2, int y2);

	void spawnToken();
	bool pickupToken(const CollisionInfo & collisionInfo);

	Coin * allocCoin();
	void spawnCoin();
	bool pickupCoin(const CollisionInfo & collisionInfo);

	uint16_t spawnBullet(int16_t x, int16_t y, uint8_t angle, BulletType type, BulletEffect effect, uint8_t ownerPlayerId);
	void spawnParticles(const ParticleSpawnInfo & spawnInfo);

	void addScreenShake(float dx, float dy, float stiffness, float life);
	Vec2 getScreenShake() const;

	void addFloorEffect(int playerId, int x, int y, int size, int damageSize);
};

extern GameSim * g_gameSim;
