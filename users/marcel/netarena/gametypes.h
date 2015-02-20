#pragma once

#include <stdint.h>

enum NetAction
{
	kNetAction_StartGame,
	kNetAction_PlayerInputAction
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
	kGameMode_CoinCollector
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
