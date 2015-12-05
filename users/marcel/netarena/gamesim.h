#pragma once

#include <list> // annoucements
#include <string.h> // memset
#include "arena.h"
#include "background.h"
#include "fireball.h"
#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"
#include "hud.h"
#include "levelevents.h"
#include "particle.h"
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

struct PlayerInputState
{
	PlayerInputState()
		: m_actions(0)
		, m_inactivityTime(0.f)
	{
	}

	PlayerInput m_prevState;
	PlayerInput m_currState;
	uint32_t m_actions;
	float m_inactivityTime;

	bool wasDown(int input) const { return (m_prevState.buttons & input) != 0; }
	bool isDown(int input) const { return (m_currState.buttons & input) != 0; }
	bool wentDown(int input) const { return !wasDown(input) && isDown(input); }
	bool wentUp(int input) const { return wasDown(input) && !isDown(input); }
	void next(bool doInactivityCheck, float dt)
	{
		if ((m_prevState != m_currState) || (m_actions != 0))
		{
			m_inactivityTime = 0.f;
			m_prevState = m_currState;
			m_actions = 0;
		}
		else
		{
			if (doInactivityCheck)
				m_inactivityTime += dt;
			else
				m_inactivityTime = 0.f;
		}
	}
	Vec2 getAnalogDirection() const { return m_currState.getAnalogDirection(); }
};

struct PlayerAiming
{
	PlayerAiming()
	{
		memset(this, 0, sizeof(PlayerAiming));
	}

	void begin(float treshold)
	{
		Assert(!isActive);
		*this = PlayerAiming();
		isActive = true;
		this->treshold = treshold;
	}

	void end()
	{
		Assert(isActive);
		*this = PlayerAiming();
	}

	void tick(GameSim & gameSim, const PlayerInputState & input, float dt)
	{
		Assert(isActive);

		if (isActive)
		{
			const Vec2 analog = input.getAnalogDirection();

			if (analog.CalcSize() >= treshold)
			{
				aim = analog.CalcNormalized();
				aimIsValid = true;
			}
		}
	}

	void drawBelow(Vec2Arg pos) const
	{
		// todo : draw circle thingy or whatever, to indicate that aiming is active
	}

	void drawAbove(Vec2Arg pos) const
	{
		if (aimIsValid)
		{
			const Vec2 dir = aim;
			const Vec2 p1 = pos + dir * 50.f;
			const Vec2 p2 = pos + dir * 100.f;
			setColor(colorGreen);
			drawLine(
				p1[0],
				p1[1],
				p2[0],
				p2[1]);
			setColor(colorWhite);
		}
	}

	bool isActive;
	float treshold;
	bool aimIsValid;
	Vec2 aim;
};

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

		m_spriterState = SpriterState();

		m_animAllowGravity = true;
		m_animAllowSteering = true;
		m_enableInAirAnim = true;

		m_lastSpawnIndex = -1;

		m_statusHud = PlayerStatusHud();
	}

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawAt(bool flipX, bool flipY, int x, int y, const SpriterState & spriterState, const spriter::Drawable * drawables, int numDrawables) const;
	void drawLight() const;
	void drawLightAt(int x, int y) const;
	void debugDraw() const;

	void testCollision(const CollisionShape & shape, void * arg, CollisionCB cb);

	bool getPlayerControl() const;

	Vec2 getPlayerCenter() const;
	bool getPlayerCollision(CollisionInfo & collision) const;
	void getDamageHitbox(CollisionShape & shape) const;
	bool getAttackCollision(CollisionShape & shape, Vec2Arg shift = Vec2()) const;
	float getAttackDamage(Player * other) const;

	bool isAnimOverrideAllowed(PlayerAnim anim) const;
	float mirrorX(float x) const;
	float mirrorY(float y) const;

	bool hasValidCharacterIndex() const;
	void setDisplayName(const std::string & name);

	void setAnim(PlayerAnim anim, bool play, bool restart);
	void clearAnimOverrides();
	void setAttackDirection(int dx, int dy);
	void applyAnim();

	uint32_t getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const;
	uint32_t getIntersectingBlocksMask(int x, int y) const;

	void handleNewGame();
	void handleNewRound();
	void handleLeave();

	bool respawn(Vec2 * pos);
	void despawn(bool willRespawn);
	bool isSpawned() const;
	void cancelAttack();
	void handleImpact(Vec2Arg velocity);
	bool shieldAbsorb(float amount);
	bool handleDamage(float amount, Vec2Arg velocity, Player * attacker, bool isNeutralDamage = false);
	bool handleIce(Vec2Arg velocity, Player * attacker);
	bool handleBubble(Vec2Arg velocity, Player * attacker);
	void awardScore(int score);
	void handleKill(bool hasScored, bool isFirstKill);
	void dropCoins(int numCoins);
	void dropWeapons(Vec2Arg velocity);

	void pushWeapon(PlayerWeapon weapon, int ammo);
	PlayerWeapon popWeapon();
	void addKnockBack(float strength);

	void handleJumpCollision();

	// special : rocket punch
	void beginRocketPunch();
	void endRocketPunch(bool stunned);

	// special : axe
	void beginAxeThrow();
	void endAxeThrow();
	void tickAxeThrow(float dt);
	Vec2 getAxeThrowPos() const;

	// special : shield
	void beginShieldSpecial();
	void endShieldSpecial();
	void tickShieldSpecial(float dt);
	bool shieldSpecialReflect(Vec2Arg pos, Vec2 & dir) const;

	// special : grapple
	void beginGrapple();
	void endGrapple();
	void tickGrapple(float dt);
	bool findGrappleAnchorPos(Vec2 & anchorPos, float & length) const;
	Vec2 getGrapplePos() const;
	float getGrappleLength() const;

	// special : ninja dash
	bool findNinjaDashTarget(Vec2 & destination);

	// special : pipebomb
	void beginPipebomb();
	void endPipebomb();
	void tickPipebomb(float dt);

	// allocation
	bool m_isUsed;

	uint8_t m_index;
	uint16_t m_owningChannelId;

	// character select
	FixedString<MAX_PLAYER_DISPLAY_NAME> m_displayName;
	bool m_isReadyUpped;
	bool m_isActive;

	// alive state
	bool m_isAlive;
	uint8_t m_characterIndex;
	float m_controlDisableTime;

	// input
	PlayerInputState m_input;

	//

	int8_t m_score;
	int16_t m_totalScore;

	//

	Vec2 m_pos;
	Vec2 m_vel;
	Vec2 m_lastTotalVel;

	Vec2 m_facing;
	float m_facingAnim;

	//

	PlayerAnim m_anim;
	bool m_animPlay;

	SpriterState m_spriterState;

	//

	int8_t m_attackDirection[2];

	//

	SpriterState m_emblemSpriterState;
	float m_multiKillTimer;
	int m_multiKillCount;
	int m_killingSpree;

	//

	PlayerWeapon m_weaponStack[MAX_WEAPON_STACK_SIZE];
	uint8_t m_weaponStackSize;

	//

	struct SpecialInfo
	{
		SpecialInfo()
		{
			memset(this, 0, sizeof(SpecialInfo));
		}

		float meleeAnimTimer;
		int meleeCounter;
		float attackDownHeight;
		bool attackDownActive;
	} m_special;

	//

	CollisionInfo m_collision;

	uint32_t m_blockMask;

	int8_t m_dirBlockMaskDir[2];
	uint32_t m_dirBlockMask[2];
	uint32_t m_oldBlockMask;

	//

	struct AttackInfo
	{
		AttackInfo()
		{
			memset(this, 0, sizeof(AttackInfo));
		}

		bool attacking;
		bool hitDestructible;
		bool hasCollision;
		CollisionInfo collision;
		Vec2 attackVel;
		float cooldown; // this timer needs to hit zero before the player can attack again. it's decremented AFTER the attack animation has finished
		bool allowCancelTimeDilationAttack;

		struct RocketPunch
		{
			RocketPunch()
			{
				memset(this, 0, sizeof(RocketPunch));
			}

			enum State
			{
				kState_Charge,
				kState_Attack,
				kState_Stunned
			};

			bool isActive;
			State state;
			float chargeTime;
			float maxDistance;
			float distance;
			Vec2 speed;
			float stunTime;
		} m_rocketPunch;

		struct AxeThrow
		{
			AxeThrow()
			{
				memset(this, 0, sizeof(AxeThrow));
			}

			bool isActive;
			PlayerAiming aiming;
		} m_axeThrow;

		struct Zweihander
		{
			enum State
			{
				kState_Idle,
				kState_Charge,
				kState_Attack,
				kState_AttackDown,
				kState_Stunned
			};

			Zweihander();

			bool isActive() const;
			void begin(Player & player);
			void end(Player & player);

			void handleAttackAnimComplete(Player & player);
			void tick(Player & player, float dt);

			State state;
			float timer;
		} m_zweihander;
	} m_attack;

	struct TimeDilationAttack
	{
		TimeDilationAttack()
		{
			memset(this, 0, sizeof(TimeDilationAttack));
		}

		bool isActive() const
		{
			return timeRemaining != 0.f;
		}

		void tick(float dt)
		{
			timeRemaining -= dt;
			if (timeRemaining < 0.f)
				timeRemaining = 0.f;
		}

		float timeRemaining;
	} m_timeDilationAttack;

	struct TeleportInfo
	{
		TeleportInfo()
		{
			memset(this, 0, sizeof(TeleportInfo));
		}

		bool cooldown; // when set, we're waiting for the player to exit the portal (id), which is the destination of the previous teleport
		int lastPortalId;
	} m_teleport;

	struct JumpInfo
	{
		JumpInfo()
		{
			memset(this, 0, sizeof(JumpInfo));
		}

		float jumpVelocityLeft;

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

			spriterState = SpriterState();
		}

		bool hasShield;
		SpriterState spriterState;
	} m_shield;

	struct IceInfo
	{
		IceInfo()
		{
			memset(this, 0, sizeof(IceInfo));
		}

		float timer;
		int bounceFrames[2];
	} m_ice;

	struct BubbleInfo
	{
		BubbleInfo()
		{
			memset(this, 0, sizeof(BubbleInfo));

			spriterState = SpriterState();
		}

		float timer;
		int bounceFrames[2];
		SpriterState spriterState;
	} m_bubble;

	struct GrappleInfo
	{
		GrappleInfo()
		{
			memset(this, 0, sizeof(GrappleInfo));
		}

		enum State
		{
			State_Inactive,
			State_Aiming,
			State_Attaching,
			State_Attached,
			State_Detaching
		};

		State state;
		PlayerAiming aiming;
		Vec2 anchorPos;
		float distance;
	} m_grapple;

	struct PipebombInfo
	{
		PipebombInfo()
		{
			memset(this, 0, sizeof(PipebombInfo));
		}

		enum State
		{
			State_Inactive,
			State_Deploy
		};

		State state;
		float time;
	} m_pipebomb;

	struct AxeInfo
	{
		AxeInfo()
		{
			memset(this, 0, sizeof(AxeInfo));
		}

		bool hasAxe;
		float recoveryTime;
	} m_axe;

	struct ShieldSpecial
	{
		ShieldSpecial()
		{
			memset(this, 0, sizeof(ShieldSpecial));

			spriterState = SpriterState();
		}

		bool isActive() const
		{
			return (state == State_Active);
		}

		enum State
		{
			State_Inactive,
			State_Active
		};

		State state;
		float charge;
		float cooldown;
		SpriterState spriterState;
	} m_shieldSpecial;

	float m_respawnTimer; // when this timer counts to zero, the player is automatically respawn
	float m_respawnTimerRcp;
	bool m_canRespawn; // set when the player is allowed to respawn, which is after the death animation is done
	bool m_canTaunt; // set when the player is allowed to taunt, which is after the death animation is done. it's reset after a taunt
	bool m_isRespawn; // set after the first respawn. the first spawn is special, as the player doesn't need to press X and isn't allowed to use taunt
	int m_lastSpawnIndex;
	float m_spawnInvincibilityTime;
	float m_spawnMarkerTime;

	bool m_isGrounded; // set when the player is walking on ground
	bool m_isAttachedToSticky;
	bool m_isAnimDriven; // an animation is active that drives the player using animation actions/triggers
	bool m_enableInAirAnim;
	float m_groundDashDistance; // distance travelled at high velocity while grounded. used for ground dust particles

	bool m_isAirDashCharged; // reset when air dash is used. set when the player hits the ground
	bool m_isInPassthrough;
	int8_t m_isHuggingWall;
	bool m_isWallSliding;
	float m_wallSlideDistance;

	int m_emoteId;
	float m_emoteTime;

	struct JetpackInfo
	{
		JetpackInfo()
		{
			memset(this, 0, sizeof(JetpackInfo));
		}

		bool isActive;
		float fxTime;
		float dashRemaining;
		Vec2 oldAnalog;
		Vec2 steeringSpeed;
	} m_jetpack;

	bool m_animVelIsAbsolute; // should the animation velocity be added to or replace the regular player velocity?
	bool m_animAllowGravity;
	bool m_animAllowSteering; // allow the player to control the character?
	Vec2 m_animVel;

	bool m_enterPassthrough; // if set, the player will move through passthrough blocks, without having to press DOWN. this mode is set when using the sword-down attack, and reset when the player hits the ground

	float m_pipebombCooldown;

	struct TokenHunt
	{
		TokenHunt()
		{
			memset(this, 0, sizeof(TokenHunt));
		}

		bool m_hasToken;
	} m_tokenHunt;

	// HUD

	PlayerStatusHud m_statusHud;
};

struct Pickup : PhysicsActor
{
	PickupType m_pickupType;

	Pickup()
	{
		memset(this, 0, sizeof(Pickup));
	}

	void setup(PickupType type, float x, float y);

	void tick(GameSim & gameSim, float dt);
	void draw(const GameSim & gameSim) const;
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

struct FootBall : PhysicsActor
{
	bool m_hasBeenTouched; // ball needs to be touched once for it to be affected by gravity
	bool m_isDropped;
	SpriterState m_spriterState;

	FootBall()
	{
		memset(this, 0, sizeof(FootBall));
	}

	void setup(int x, int y);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct FootBallGoal
{
	bool m_isActive;

	FootBallGoal()
	{
		memset(this, 0, sizeof(FootBallGoal));
	}

	void setup(int x, int y);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

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

struct Mover
{
	bool m_isActive;

	FixedString<64> m_sprite;
	int m_sx;
	int m_sy;

	int m_x1;
	int m_y1;
	int m_x2;
	int m_y2;
	int m_speed;
	float m_moveMultiplier;
	float m_moveAmount;

	Mover()
	{
		memset(this, 0, sizeof(Mover));
	}

	void setSprite(const char * filename);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;

	Vec2 getPosition() const;
	Vec2 getSpeed() const;

	void getCollisionInfo(CollisionInfo & collisionInfo) const;

	bool intersects(CollisionInfo & collisionInfo) const;
};

struct Axe : PhysicsActor
{
	bool m_hasLanded;
	bool m_throwDone;
	int m_playerIndex;
	float m_travelTime;
	float m_fadeTime;
	SpriterState m_spriterState;

	Axe()
	{
		memset(this, 0, sizeof(Axe));
	}

	void setup(Vec2Arg pos, Vec2Arg vel, int playerIndex);

	void tick(GameSim & gameSim, float dt);
	void endThrow();
	void draw() const;
	void drawLight() const;
};

struct PipeBomb : PhysicsActor
{
	bool m_exploded;
	bool m_hasLanded;
	int m_playerIndex;
	float m_activationTime;
	SpriterState m_spriterState;

	PipeBomb()
	{
		memset(this, 0, sizeof(PipeBomb));
	}

	void setup(Vec2Arg pos, Vec2Arg vel, int playerIndex);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;

	bool explode();
};

struct Barrel : PhysicsActor
{
	int m_tick; // barrel velocity/position depends on tick number

	Barrel()
	{
		memset(this, 0, sizeof(Barrel));
	}

	void setup(Vec2Arg pos);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

// todo : implement Debris object
struct Debris : PhysicsActor
{
	FixedString<16> m_name;

	Debris()
	{
		memset(this, 0, sizeof(Debris));
	}

	void setup(Vec2Arg pos, float size);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
};

struct Light
{
	bool m_isActive;
	Vec2 m_pos;
	float m_color[4];

	Light()
	{
		memset(this, 0, sizeof(Light));
	}

	void setup(float x, float y, const Color & color);

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawLight() const;
};

struct ParticleEffect
{
	static const int kMaxParticleSystems = 6;

	struct Data
	{
		Data()
			: m_isActive(false)
			, m_x(0)
			, m_y(0)
		{
		}

		bool m_isActive;
		FixedString<16> m_filename;
		int m_x;
		int m_y;
	} m_data;

	ParticleSystem m_system[kMaxParticleSystems];

	void setup(const char * filename, int x, int y);
	void draw();
};

struct Portal
{
	Portal()
	{
		memset(this, 0, sizeof(Portal));
	}

	void setup(float x1, float y1, float x2, float y2, int key);
	bool intersects(float x1, float y1, float x2, float y2, bool mustEncapsulate, bool applySafeZone) const;
	bool doTeleport(GameSim & gameSim, Portal *& destination, int & destinationId);
	Vec2 getDestinationPos(Vec2Arg offset) const;

	void tick(GameSim & gameSim, float dt);
	void draw() const;

	bool m_isActive;
	int m_x1, m_y1, m_x2, m_y2;
	int m_key;
};

struct PickupSpawner
{
	PickupSpawner()
	{
		memset(this, 0, sizeof(PickupSpawner));
	}

	void setup(float x1, float y1, float x2, float y2, PickupType type, float interval);
	void tick(GameSim & gameSim, float dt);
	void draw() const;

	bool m_isActive;
	float m_x, m_y;
	PickupType m_type;
	float m_interval;
	float m_timeLeft;
};

struct TileSprite
{
	TileSprite()
	{
		memset(this, 0, sizeof(TileSprite));
	}

	void setup(const char * name, int x, int y, int blockX, int blockY, int blockSx, int blockSy);
	void tick(GameSim & gameSim, float dt);
	void draw(const GameSim & gameSim) const;
	void drawLight() const;

	void startAnim(const char * name);

	bool intersects(int x, int y) const;

	bool m_isActive;
	int m_pivotX;
	int m_pivotY;
	FixedString<32> m_spriter;
	SpriterState m_spriterState;
	int m_x1, m_y1, m_x2, m_y2;
	TransitionInfo m_transition;
};

struct Decal
{
	bool m_isActive;
	int16_t x;
	int16_t y;
	uint8_t color[3];
	uint8_t sprite;
	float scale;

	Decal()
	{
		memset(this, 0, sizeof(Decal));
	}

	void tick(GameSim & gameSim, float dt);
	void draw() const;
	void drawAt(int x, int y) const;
};

Color getDecalColor(int playerIndex, Vec2Arg direction);

struct ScreenShake
{
	bool m_isActive;
	bool fade;
	Vec2 pos;
	Vec2 vel;
	float stiffness;
	float life;
	float lifeRcp;

	ScreenShake()
	{
		memset(this, 0, sizeof(ScreenShake));
	}

	void tick(GameSim & gameSim, float dt);
};

struct ZoomEffect
{
	bool m_isActive;
	float zoom;
	float life;
	float lifeRcp;
	int player;

	ZoomEffect()
	{
		memset(this, 0, sizeof(ZoomEffect));
	}

	void tick(GameSim & gameSim, float dt);
};

struct LightEffect
{
	enum Type
	{
		kType_Darken,
		kType_Lighten
	};

	bool m_isActive;
	Type type;
	float life;
	float lifeRcp;
	float amount;

	LightEffect()
	{
		memset(this, 0, sizeof(LightEffect));
	}

	void setDarken(float time, float amount);
	void setLighten(float time, float amount);

	void tick(GameSim & gameSim, float dt);
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
		float time;
		float damageTime;
		int8_t playerId;

		void getCollisionInfo(CollisionInfo & collisionInfo) const;
	} m_tiles[MAX_FLOOR_EFFECT_TILES];

	FloorEffect()
	{
		memset(this, 0, sizeof(FloorEffect));
	}

	void tick(GameSim & gameSim, float dt);
	void draw();

	void trySpawnAt(GameSim & gameSim, int playerId, int x, int y, int dx, int size, int damageSize);
};

struct BlindsEffect
{
	BlindsEffect()
	{
		memset(this, 0, sizeof(BlindsEffect));
	}

	bool m_isActive;
	float m_duration;
	float m_time;
	int m_x;
	int m_y;
	int m_size;
	bool m_vertical;
	FixedString<MAX_PLAYER_DISPLAY_NAME> m_text;

	void setup(float time, int x, int y, int size, bool vertical, const char * text);

	void tick(GameSim & gameSim, float dt);
	void drawLight();
	void drawHud();
};

struct AnimationFxState
{
	AnimationFxState()
	{
		memset(this, 0, sizeof(AnimationFxState));

		m_state = SpriterState();
	}

	bool m_isActive;
	DrawLayer m_layer;
	FixedString<32> m_fileName;
	SpriterState m_state;

	void tick(GameSim & gameSim, float dt);
	void draw(DrawLayer layer);
};

//

struct GameStateData
{
	GameStateData()
	{
		memset(this, 0, sizeof(GameStateData));

		//m_desiredGameMode = kGameMode_DeathMatch;
		m_desiredGameMode = kGameMode_FootBrawl;
	}

	uint32_t Random();
	float RandomFloat(float min, float max) { float t = (Random() & 4095) / 4095.f; return t * min + (1.f - t) * max; }
	int RandomInt(int min, int max) { return min + (Random() % (max - min + 1)); }
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
	GameMode m_desiredGameMode;

	float m_physicalTimeStep;
	float m_roundTime;
	float m_physicalRoundTime;
	uint32_t m_nextRoundNumber;
	uint32_t m_consecutiveRoundCount;
	bool m_isFirstKill;

	struct RoundBegin
	{
		enum State
		{
			kState_ShowControls,
			kState_LevelTransition,
			kState_SpawnPlayers,
			kState_FightMessage
		};

		RoundBegin()
		{
			memset(this, 0, sizeof(*this));
		}

		State m_state;
		float m_delay;
		float m_delayTimeRcp;
	} m_roundBegin;

	struct RoundEnd
	{
		enum State
		{
			kState_ShowWinner,
			kState_ShowResults,
			kState_LevelTransition
		};

		RoundEnd()
		{
			memset(this, 0, sizeof(*this));
		}

		State m_state;
		float m_delay;
		float m_delayTimeRcp;
	} m_roundEnd;

	Player m_players[MAX_PLAYERS];
	Pickup m_pickups[MAX_PICKUPS];
	float m_nextPickupSpawnTimeRemaining;
	Mover m_movers[MAX_MOVERS];
	Axe m_axes[MAX_AXES];
	PipeBomb m_pipebombs[MAX_PIPEBOMBS];
	FootBall m_footBalls[MAX_FOOTBALLS];
	FootBallGoal m_footBallGoals[MAX_FOOTBALL_GOALS];
	Barrel m_barrels[MAX_BARRELS];
	FloorEffect m_floorEffect;
	BlindsEffect m_blindsEffects[MAX_BLINDS_EFFECTS];
	Light m_lights[MAX_LIGHTS];
	Portal m_portals[MAX_PORTALS];
	PickupSpawner m_pickupSpawners[MAX_PICKUP_SPAWNERS];
	TileSprite m_tileSprites[MAX_TILE_SPRITES];
	AnimationFxState m_animationEffects[MAX_ANIM_EFFECTS];
	Decal m_decals[MAX_DECALS];
	ScreenShake m_screenShakes[MAX_SCREEN_SHAKES];

	// zoom effects

	ZoomEffect m_zoomEffects[MAX_ZOOM_EFFECTS];

	float m_desiredZoom;
	Vec2 m_desiredZoomFocus;
	bool m_desiredZoomFocusIsSet;
	float m_effectiveZoom;
	Vec2 m_effectiveZoomFocus;

	LightEffect m_lightEffects[MAX_LIGHT_EFFECTS];

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

	// background

	Background m_background;

	// fireballs

	FireBall m_fireballs[MAX_FIREBALLS];

	// support for game modes

	struct FootBrawl
	{
		FootBrawl()
		{
			ballSpawnPoint[0] = ARENA_SX_PIXELS / 2;
			ballSpawnPoint[1] = ARENA_SY_PIXELS / 3;
		}

		float ballSpawnPoint[2];
	} m_footBrawl;

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

	ParticleEffect m_particleEffects[MAX_PARTICLE_EFFECTS];

	BulletPool * m_particlePool;

	// non-serialized (RPC)

	PlayerInstanceData * m_playerInstanceDatas[MAX_PLAYERS];

	struct AnnounceInfo
	{
		float timeLeft;
		std::string message;
		Color color;
	};

	std::list<AnnounceInfo> m_annoucements;

	GameSim();
	~GameSim();

#if ENABLE_GAMESTATE_DESYNC_DETECTION
	uint32_t calcCRC() const;
#endif

	void serialize(NetSerializationContext & context);
	void clearPlayerPtrs() const;
	void setPlayerPtrs() const;
	PlayerInstanceData * allocPlayer(uint16_t owningChannelId);
	void freePlayer(PlayerInstanceData * instanceData);
	int getNumPlayers() const;

	void setGameState(::GameState gameState);
	void setGameMode(GameMode gameMode);

	void newGame();
	void newRound(const char * mapOverride);
	void endRound();

	void load(const char * name);
	void resetGameWorld();
	void resetPlayers();
	void resetGameSim();

	void tick();
	void tickMenus();
	void tickRoundBegin(float dt);
	void tickPlay();
	void tickPlayPickupSpawn(float dt);
	void tickPlayLevelEvents(float dt);
	void tickRoundComplete(float dt);

	struct CamParams
	{
		Vec2 shake;
		Vec2 zoomFocus;
		float zoom;
	};

	void drawPlay();
	void drawPlayColor(const CamParams & camParams);
	void drawPlayDecal(const CamParams & camParams);
	void drawPlayLight(const CamParams & camParams);
	void drawPlayHud(const CamParams & camParams);
	void applyCamParams(const CamParams & camParams, float zoomFactor, float shakeFactor) const;

	void getCurrentTimeDilation(float & timeDilation, bool & playerAttackTimeDilation) const;

	void playSound(const char * filename, int volume = 100);

	void testCollision(const CollisionShape & shape, void * arg, CollisionCB cb);
	void testCollisionInternal(const CollisionShape & shape, uint32_t typeMask, void * arg, CollisionCB cb);

	Pickup * allocPickup();
	void trySpawnPickup(PickupType type);
	void spawnPickup(Pickup & pickup, PickupType type, int blockX, int blockY);
	bool grabPickup(int x1, int y1, int x2, int y2, Pickup & pickup);

	void spawnFootball();

	void spawnToken();
	bool pickupToken(const CollisionInfo & collisionInfo);

	Coin * allocCoin();
	void spawnCoin();
	bool pickupCoin(const CollisionInfo & collisionInfo);

	uint16_t spawnBullet(int16_t x, int16_t y, uint8_t angle, BulletType type, BulletEffect effect, uint8_t ownerPlayerId);
	void spawnParticles(const ParticleSpawnInfo & spawnInfo);

	void spawnAxe(Vec2 pos, Vec2 vel, int playerIndex);
	bool grabAxe(const CollisionInfo & collision);

	void spawnPipeBomb(Vec2 pos, Vec2 vel, int playerIndex);

	void triggerLevelEvent(LevelEvent e);

	void doQuake(float vel);
	void doBlastEffect(Vec2Arg center, float radius, const Curve & speedCurve);

	void addDecal(int x, int y, const Color & color, int sprite, float scale);

	void addScreenShake(float dx, float dy, float stiffness, float life, bool fade);
	Vec2 getScreenShake() const;
	void addScreenShake_GunFire(Vec2Arg dir);

	void addZoomEffect(float zoom, float life, int player);
	void setDesiredZoom(float zoom);
	void setDesiredZoomFocus(Vec2Arg focusPoint);
	float calculateEffectiveZoom() const;
	Vec2 calculateEffectiveZoomFocus() const;
	void restrictZoomParams(float & zoom, Vec2 & zoomFocus) const;
	void tickZoom(float dt);

	void addLightEffect(LightEffect::Type type, float time, float amount);
	float getLightAmount() const;

	void addFloorEffect(int playerId, int x, int y, int size, int damageSize);

	void addBlindsEffect(int playerId, int x, int y, int size, bool vertical, float time, const char * text);

	Portal * findPortal(float x1, float y1, float x2, float y2, bool mustEncapsulate, bool applySafeZone, int & id);

	TileSprite * findTileSpriteAtPos(int x, int y);
	TileSprite * findTileSpriteAtBlockXY(int blockX, int blockY);

	void addAnimationFx(DrawLayer layer, const char * fileName, int x, int y, bool flipX = false, bool flipY = false);

	void addAnnouncement(const Color & color, const char * message, ...);
	void addEmote(int playerId, int emoteId);

	void addFireBall();
};

extern GameSim * g_gameSim;

//

struct PhysicsUpdateInfo;

struct ContactInfo
{
	Vec2 n;
	float d;
	float r;
	int f;

	bool operator==(const ContactInfo & other) const
	{
		return
			n == other.n &&
			d == other.d &&
			r == other.r &&
			f == other.f;
	}
};

enum PhysicsUpdateFlags
{
	kPhysicsUpdateFlag_DontCollide = 1 << 0,
	kPhysicsUpdateFlag_DontUpdateVelocity = 1 << 1
};

// return one or more PhysicsUpdateFlags
typedef int (*PhysicsUpdateCB)(PhysicsUpdateInfo & updateInfo);

struct PhysicsUpdateInfo
{
	CollisionShape shape;
	void * arg;
	PhysicsUpdateCB cb;

	int axis;
	Vec2 pos;
	Vec2 delta;

	PhysicsActor * actor;
	BlockAndDistance * blockInfo;
	Player * player;
	Vec2 contactNormal;
	float contactDistance;
	float contactRestitution;

	std::vector<ContactInfo> contacts;
};

void updatePhysics(GameSim & gameSim, Vec2 & pos, Vec2 & vel, float dt, const CollisionShape & shape, void * arg, PhysicsUpdateCB cb);
