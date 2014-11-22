#pragma once

#include "netobject.h"

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
		x = 0;
		y = 0;
	}

	int16_t x;
	int16_t y;
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
	PlayerState_NS m_state;
	PlayerAnim_NS m_anim;

	// ReplicationObject
	virtual bool RequiresUpdating() const { return true; }

	// NetObject
	virtual NetObjectType getType() const { return kNetObjectType_Player; }

public:
	Player(uint16_t owningChannelId = 0)
		: m_pos(this)
		, m_state(this)
		, m_anim(this)
		, m_buttons(0)
	{
		setOwningChannelId(owningChannelId);
	}

	~Player()
	{
	}

	void tick(float dt);
	void draw();

	uint16_t m_buttons;
};
