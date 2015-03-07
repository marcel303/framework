#pragma once

#include <stdint.h>

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
	kGameState_RoundComplete
};

enum GameMode
{
	kGameMode_DeathMatch,
	kGameMode_TokenHunt,
	kGameMode_CoinCollector,
	kGameMode_COUNT
};

extern const char * g_gameModeNames[kGameMode_COUNT];

enum PlayerAnim
{
	kPlayerAnim_NULL,
	kPlayerAnim_Idle,
	kPlayerAnim_InAir,
	kPlayerAnim_Jump,
	kPlayerAnim_WallSlide,
	kPlayerAnim_Walk,
	kPlayerAnim_Attack,
	kPlayerAnim_AttackUp,
	kPlayerAnim_AttackDown,
	kPlayerAnim_Fire,
	kPlayerAnim_RocketPunch_Charge,
	kPlayerAnim_RocketPunch_Attack,
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
	kPlayerSpecial_COUNT
};

enum PlayerTrait
{
	kPlayerTrait_StickyWalk = 0x1,
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

struct CollisionInfo
{
	bool intersects(const CollisionInfo & other) const
	{
		return
			x2 >= other.x1 &&
			y2 >= other.y1 &&
			x1 <= other.x2 &&
			y1 <= other.y2;
	}

	bool intersects(int x, int y) const
	{
		return
			x2 >= x &&
			y2 >= y &&
			x1 <= x &&
			y1 <= y;
	}

	int16_t x1;
	int16_t y1;
	int16_t x2;
	int16_t y2;
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
