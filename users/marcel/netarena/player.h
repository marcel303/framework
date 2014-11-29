#pragma once

#include <string>
#include "netobject.h"
#include "Vec2.h"

class Dictionary;
class Sprite;

class PlayerPos_NS : public NetSerializable
{
	virtual void SerializeStruct()
	{
		Serialize(x);
		Serialize(y);

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
		: NetSerializable(owner)
		, x(0.f)
		, y(0.f)
		, xFacing(+1)
		, yFacing(+1)
	{
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
	virtual void SerializeStruct()
	{
		Serialize(isAlive);
	}

public:
	PlayerState_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
		, isAlive(false)
	{
	}

	bool isAlive;
};

class PlayerAnim_NS : public NetSerializable
{
	virtual void SerializeStruct();

	int m_lastAnim;
	bool m_lastPlay;

public:
	PlayerAnim_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
		, m_lastAnim(0)
		, m_lastPlay(false)
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

class Player : public NetObject
{
	friend class PlayerAnim_NS;

	PlayerPos_NS m_pos;
	Vec2 m_vel;
	PlayerState_NS m_state;
	PlayerAnim_NS m_anim;

	bool m_isAuthorative;

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

		float x1;
		float y1;
		float x2;
		float y2;
	} m_collision;

	struct AttackInfo
	{
		AttackInfo()
			: attacking(false)
			, timeLeft(0.f)
			, attackVel()
		{
		}

		bool attacking;
		float timeLeft;
		Vec2 attackVel;
		CollisionInfo collision;
	} m_attack;

	bool m_isGrounded;
	bool m_isAttachedToSticky;

	Sprite * m_sprite;

	static void handleAnimationAction(const std::string & action, const Dictionary & args);

	void getPlayerCollision(CollisionInfo & collision);
	void getAttackCollision(CollisionInfo & collision);
	float getAttackDamage(Player * other);

	// ReplicationObject
	virtual bool RequiresUpdating() const { return true; }

	// NetObject
	virtual NetObjectType getType() const { return kNetObjectType_Player; }

public:
	Player(uint32_t netId = 0);
	~Player();

	void tick(float dt);
	void draw();
	void debugDraw();

	uint32_t getIntersectingBlocksMask(float x, float y) const;

	void handleDamage(float amount, Vec2Arg velocity);

	struct InputState
	{
		InputState()
			: m_prevButtons(0)
			, m_currButtons(0)
		{
		}

		uint16_t m_prevButtons;
		uint16_t m_currButtons;

		bool wasDown(int input) { return (m_prevButtons & input) != 0; }
		bool isDown(int input) { return (m_currButtons & input) != 0; }
		bool wentDown(int input) { return !wasDown(input) && isDown(input); }
		bool wentUp(int input) { return wasDown(input) && !isDown(input); }
		void next() { m_prevButtons = m_currButtons; }
	} m_input;
};
