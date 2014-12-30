#pragma once

#include <stdint.h>

enum GameState
{
	kGameState_Undefined,
	kGameState_Lobby,
	kGameState_NewGame,
	kGameState_Play,
	kGameState_RoundComplete
};

enum PlayerWeapon
{
	kPlayerWeapon_Sword,
	kPlayerWeapon_Fire,
	kPlayerWeapon_Grenade,
	kPlayerWeapon_COUNT
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
	kPickupType_COUNT
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

struct Pickup
{
	bool isAlive;

	PickupType type;
	uint8_t blockX;
	uint8_t blockY;
	int16_t x1;
	int16_t y1;
	int16_t x2;
	int16_t y2;
	uint16_t spriteId;
};
