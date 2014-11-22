#pragma once

#include "netobject.h"
#include "Vec2.h"

class PlayerPos_NS : public NetSerializable
{
	virtual void SerializeStruct()
	{
		Serialize(x);
		Serialize(y);
	}

public:
	PlayerPos_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
	{
		x = 0.f;
		y = 0.f;
	}

	float & operator[](int index)
	{
		return (index == 0) ? x : y;
	}

	float x;
	float y;
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
	virtual void SerializeStruct()
	{
		Serialize(anim);

		if (IsRecv())
		{
			// todo : trigger animation
		}
	}

public:
	PlayerAnim_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
	{
	}

	std::string anim;
};

class Player : public NetObject
{
	PlayerPos_NS m_pos;
	Vec2 m_vel;
	PlayerState_NS m_state;
	PlayerAnim_NS m_anim;

	struct
	{
		float x1;
		float y1;
		float x2;
		float y2;
	} m_collision;

	// ReplicationObject
	virtual bool RequiresUpdating() const { return true; }

	// NetObject
	virtual NetObjectType getType() const { return kNetObjectType_Player; }

public:
	Player(uint16_t owningChannelId = 0)
		: m_pos(this)
		, m_state(this)
		, m_anim(this)
	{
		setOwningChannelId(owningChannelId);

		m_collision.x1 = -PLAYER_SX / 2.f;
		m_collision.x2 = +PLAYER_SX / 2.f;
		m_collision.y1 = -PLAYER_SY;
		m_collision.y2 = 0.f;
	}

	~Player()
	{
	}

	void tick(float dt);
	void draw();

	uint32_t getIntersectingBlocksMask(float x, float y) const;

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
