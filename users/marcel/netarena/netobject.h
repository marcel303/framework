#pragma once

#include "NetSerializable.h"
#include "ReplicationObject.h"

enum NetObjectType
{
	kNetObjectType_Arena,
	kNetObjectType_Player
};

class NetObject_NS : public NetSerializable
{
	virtual void SerializeStruct()
	{
		bool hasNetId = (netId != 0);
		Serialize(hasNetId);
		if (hasNetId)
			Serialize(netId);
	}

public:
	NetObject_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
		, netId(0)
	{
	}

	uint32_t netId;
};

class NetObject : public NetSerializableObject, public ReplicationObject
{
	NetObject_NS m_netObject_NS;

	virtual bool Serialize(BitStream & bitStream, bool init, bool send, int channel)
	{
		return NetSerializableObject::Serialize(init, send, channel, bitStream);
	}

public:
	NetObject()
		: m_netObject_NS(this)
	{
	}

	virtual NetObjectType getType() const = 0;

	uint32_t getNetId() const { return m_netObject_NS.netId; }
	void setNetId(uint32_t netId) { m_netObject_NS.netId = netId; }
};
