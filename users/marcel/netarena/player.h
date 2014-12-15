#pragma once

#include <string>
#include "netobject.h"
#include "Vec2.h"

class Dictionary;
class Sprite;

enum PlayerWeapon
{
	kPlayerWeapon_Sword,
	kPlayerWeapon_Fire,
	kPlayerWeapon_COUNT
};

enum PlayerEvent
{
	kPlayerEvent_Spawn,
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

class PlayerPos_NS : public NetSerializable
{
	virtual void SerializeStruct()
	{
		if (IsSend())
		{
			int16_t xCompressed = static_cast<int16_t>(x);
			int16_t yCompressed = static_cast<int16_t>(y);

			Serialize(xCompressed);
			Serialize(yCompressed);
		}
		else
		{
			int16_t xCompressed;
			int16_t yCompressed;

			Serialize(xCompressed);
			Serialize(yCompressed);

			x = static_cast<float>(xCompressed);
			y = static_cast<float>(yCompressed);
		}

		//

		bool facing;
		
		facing = xFacing < 0 ? true : false;
		Serialize(facing);
		xFacing = facing ? -1 : +1;

		facing = yFacing < 0 ? true : false;
		Serialize(facing);
		yFacing = facing ? -1 : +1;
	}

public:
	PlayerPos_NS(NetSerializableObject * owner)
		: NetSerializable(owner, (1 << REPLICATION_CHANNEL_UNRELIABLE))
		, x(0.f)
		, y(0.f)
		, xFacing(+1)
		, yFacing(+1)
	{
		SetChannel(REPLICATION_CHANNEL_UNRELIABLE);
	}

	float & operator[](int index)
	{
		return (index == 0) ? x : y;
	}

	float x;
	float y;

	int xFacing;
	int yFacing;
};

class PlayerState_NS : public NetSerializable
{
	virtual void SerializeStruct();

public:
	PlayerState_NS(NetSerializableObject * owner);

	bool isAlive;
	uint16_t score;
	uint16_t totalScore;
	int8_t playerId;
	uint8_t characterIndex;
};

class PlayerAnim_NS : public NetSerializable
{
	virtual void SerializeStruct();

public:
	PlayerAnim_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
		, m_anim(0)
		, m_play(false)
	{
	}

	void SetAnim(int anim, bool play, bool restart)
	{
		if (anim != m_anim || play != m_play || restart)
		{
			m_anim = anim;
			m_play = play;
			SetDirty();
		}
	}

	void SetPlay(bool play)
	{
		if (play != m_play)
		{
			m_play = play;
			SetDirty();
		}
	}

	int m_anim;
	bool m_play;
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

	int x1;
	int y1;
	int x2;
	int y2;
};

class Player : public NetObject
{
	friend class PlayerAnim_NS;

	PlayerPos_NS m_pos;
	Vec2 m_vel;
	PlayerState_NS m_state;
	PlayerAnim_NS m_anim;
	PlayerWeapon m_selectedWeapon;

	bool m_isAuthorative;

	CollisionInfo m_collision;
	uint32_t m_blockMask;

	struct AttackInfo
	{
		AttackInfo()
			: attacking(false)
			, hitDestructible(false)
			, attackVel()
		{
		}

		bool attacking;
		bool hitDestructible;
		Vec2 attackVel;
		CollisionInfo collision;
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

	bool m_isGrounded;
	bool m_isAttachedToSticky;
	bool m_isAnimDriven;

	bool m_isAirDashCharged;
	bool m_isWallSliding;

	bool m_animVelIsAbsolute;
	Vec2 m_animVel;

	Sprite * m_sprite;
	float m_spriteScale;

	static void handleAnimationAction(const std::string & action, const Dictionary & args);

public:
	void getPlayerCollision(CollisionInfo & collision);
	void getAttackCollision(CollisionInfo & collision);
	float getAttackDamage(Player * other);

	bool isAnimOverrideAllowed(int anim) const;
	float mirrorX(float x) const;
	float mirrorY(float y) const;

	// ReplicationObject
	virtual bool RequiresUpdating() const { return true; }

	// NetObject
	virtual NetObjectType getType() const { return kNetObjectType_Player; }

public:
	Player(uint32_t netId = 0, uint16_t owningChannelId = 0);
	~Player();

	void playSecondaryEffects(PlayerEvent e);

	void tick(float dt);
	void draw();
	void debugDraw();

	uint32_t getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const;
	uint32_t getIntersectingBlocksMask(int x, int y) const;

	void handleNewGame();
	void handleNewRound();

	void respawn();
	void handleDamage(float amount, Vec2Arg velocity, Player * attacker);
	void awardScore(int score);

	int getScore() const { return m_state.score; }
	int getTotalScore() const { return m_state.totalScore; }

	void setPlayerId(int id);

	int getCharacterIndex() const { return m_state.characterIndex; }
	void handleCharacterIndexChange();
	char * makeCharacterFilename(const char * filename);

	struct InputState
	{
		InputState()
			: m_controllerIndex(-1)
			, m_prevButtons(0)
			, m_currButtons(0)
		{
		}

		int m_controllerIndex;

		uint16_t m_prevButtons;
		uint16_t m_currButtons;

		bool wasDown(int input) { return (m_prevButtons & input) != 0; }
		bool isDown(int input) { return (m_currButtons & input) != 0; }
		bool wentDown(int input) { return !wasDown(input) && isDown(input); }
		bool wentUp(int input) { return wasDown(input) && !isDown(input); }
		void next() { m_prevButtons = m_currButtons; }
	} m_input;
};
