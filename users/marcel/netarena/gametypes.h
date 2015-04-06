#pragma once

#include <stdint.h>
#include "Vec2.h"

#pragma pack(push)
#pragma pack(1)

enum NetAction
{
	kPlayerInputAction_CycleGameMode,
	kNetAction_PlayerInputAction,
	kNetAction_TextChat
};

enum NetPlayerAction
{
	kPlayerInputAction_PrevChar,
	kPlayerInputAction_NextChar,
	kPlayerInputAction_ReadyUp
};

enum GameState
{
	kGameState_MainMenus,
	kGameState_Connecting,
	kGameState_OnlineMenus,
	kGameState_NewGame,
	kGameState_Play,
	kGameState_RoundComplete,
	kGameState_COUNT
};

extern const char * g_gameStateNames[kGameState_COUNT];

enum GameMode
{
	kGameMode_DeathMatch,
	kGameMode_TokenHunt,
	kGameMode_CoinCollector,
	kGameMode_COUNT
};

enum ObjectType
{
	kObjectType_Undefined,
	kObjectType_Player,
	kObjectType_Pickup,
	kObjectType_PipeBomb,
	kObjectType_COUNT
};

extern const char * g_gameModeNames[kGameMode_COUNT];

enum PlayerAnim
{
	kPlayerAnim_NULL,
	kPlayerAnim_Idle,
	kPlayerAnim_InAir,
	kPlayerAnim_Jump,
	kPlayerAnim_DoubleJump,
	kPlayerAnim_WallSlide,
	kPlayerAnim_Walk,
	kPlayerAnim_Attack,
	kPlayerAnim_AttackUp,
	kPlayerAnim_AttackDown,
	kPlayerAnim_Fire,
	kPlayerAnim_RocketPunch_Charge,
	kPlayerAnim_RocketPunch_Attack,
	kPlayerAnim_Zweihander_Charge,
	kPlayerAnim_Zweihander_Attack,
	kPlayerAnim_Zweihander_AttackDown,
	kPlayerAnim_Zweihander_Stunned,
	kPlayerAnim_AirDash,
	kPlayerAnim_Spawn,
	kPlayerAnim_Die,
	kPlayerAnim_COUNT
};

enum PlayerWeapon
{
	kPlayerWeapon_None,
	kPlayerWeapon_Sword,
	kPlayerWeapon_Fire,
	kPlayerWeapon_Ice,
	kPlayerWeapon_Bubble,
	kPlayerWeapon_Grenade,
	kPlayerWeapon_TimeDilation,
	kPlayerWeapon_COUNT
};

// special ability that is bound to the Y button
enum PlayerSpecial
{
	kPlayerSpecial_None,
	kPlayerSpecial_RocketPunch,
	kPlayerSpecial_DoubleSidedMelee,
	kPlayerSpecial_DownAttack,
	kPlayerSpecial_Shield,
	kPlayerSpecial_Invisibility,
	kPlayerSpecial_Jetpack,
	kPlayerSpecial_Zweihander,
	kPlayerSpecial_AxeThrow,
	kPlayerSpecial_Pipebomb,
	kPlayerSpecial_COUNT
};

extern const char * g_playerSpacialNames[kPlayerSpecial_COUNT];

enum PlayerTrait
{
	kPlayerTrait_StickyWalk = 1 << 0,
	kPlayerTrait_DoubleJump = 1 << 1,
	kPlayerTrait_AirDash = 1 << 2
};

enum PlayerEvent
{
	kPlayerEvent_Spawn,
	kPlayerEvent_Respawn,
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

struct PlayerInput
{
	PlayerInput()
		: buttons(0)
		, analogX(0)
		, analogY(0)
	{
	}

	bool operator!=(const PlayerInput & other)
	{
		return
			buttons != other.buttons ||
			analogX != other.analogX ||
			analogY != other.analogY;
	}

	uint16_t buttons;
	int8_t analogX;
	int8_t analogY;
};

enum PickupType
{
	kPickupType_Ammo,
	kPickupType_Nade,
	kPickupType_Shield,
	kPickupType_Ice,
	kPickupType_Bubble,
	kPickupType_TimeDilation,
	kPickupType_COUNT
};

enum BulletType
{
	kBulletType_A,
	kBulletType_B,
	kBulletType_Grenade,
	kBulletType_GrenadeA,
	kBulletType_ParticleA,
	kBulletType_COUNT
};

enum BulletEffect
{
	kBulletEffect_Damage,
	kBulletEffect_Ice,
	kBulletEffect_Bubble
};

struct CollisionBox
{
	Vec2 min;
	Vec2 max;

	bool intersects(const CollisionBox & other) const
	{
		return
			max[0] >= other.min[0] &&
			max[1] >= other.min[1] &&
			min[0] <= other.max[0] &&
			min[1] <= other.max[1];
	}

	bool intersects(float x, float y) const
	{
		return
			max[0] >= x &&
			max[1] >= y &&
			min[0] <= x &&
			min[1] <= y;
	}
};

struct CollisionInfo : CollisionBox
{
	CollisionInfo getTranslated(Vec2 offset) const
	{
		CollisionInfo result;
		result.min = min + offset;
		result.max = max + offset;
		return result;
	}
};

struct CollisionShape
{
	const static int kMaxPoints = 4;

	Vec2 points[kMaxPoints];
	int numPoints;

	//

	CollisionShape()
	{
		//memset(this, 0, sizeof(*this));
	}

	CollisionShape(const CollisionBox & box)
	{
		//memset(this, 0, sizeof(*this));

		*this = box;
	}

	CollisionShape(Vec2Arg min, Vec2Arg max)
	{
		set(
			Vec2(min[0], min[1]),
			Vec2(max[0], min[1]),
			Vec2(max[0], max[1]),
			Vec2(min[0], max[1]));
	}

	void setEmpty()
	{
		numPoints = 0;
	}

	void set(Vec2 p1, Vec2 p2, Vec2 p3)
	{
		points[0] = p1;
		points[1] = p2;
		points[2] = p3;
		numPoints = 3;

		fixup();
	}

	void set(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4)
	{
		points[0] = p1;
		points[1] = p2;
		points[2] = p3;
		points[3] = p4;
		numPoints = 4;

		fixup();
	}

	const CollisionShape & operator=(const CollisionBox & box);

	void fixup();
	void translate(float x, float y);

	void getMinMax(Vec2 & min, Vec2 & max) const;
	float projectedMax(Vec2Arg n) const;
	Vec2 getSegmentNormal(int idx) const;
	bool intersects(const CollisionShape & other) const;
	bool checkCollision(const CollisionShape & other, Vec2Arg delta, float & contactDistance, Vec2 & contactNormal) const;

	void debugDraw(bool drawNormals = true) const;
};

template <int SIZE>
struct FixedString
{
	FixedString()
	{
		memset(m_data, 0, SIZE + 1);
	}

	size_t length() const
	{
		return strlen(m_data);
	}

	const char * c_str() const
	{
		return m_data;
	}

	void operator=(const char * str)
	{
		const size_t len = strlen(str);
		const size_t copySize = (len > SIZE) ? SIZE : len;
		for (size_t i = 0; i < copySize; ++i)
			m_data[i] = str[i];
		for (size_t i = copySize; i < SIZE + 1; ++i)
			m_data[i] = 0;
	}

	bool operator==(const char * str) const
	{
		return strcmp(m_data, str) == 0;
	}

	bool operator!=(const char * str) const
	{
		return strcmp(m_data, str) != 0;
	}

	bool operator<(const FixedString & other) const
	{
		return strcmp(m_data, other.m_data) < 0;
	}

	char m_data[SIZE + 1];
};

#pragma pack(pop)
