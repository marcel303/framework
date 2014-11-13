#pragma once

#include <stdint.h>
#include <vector>
#include "libnet_forward.h"
#include "BitStream.h"

class NetSerializationContext
{
	bool m_init;
	bool m_send;
	BitStream * m_bitStream;

public:
	NetSerializationContext();

	void Set(bool init, bool send, BitStream & bitStream);
	void Reset();

	const BitStream & GetBitStream() const { return *m_bitStream; }

	inline bool IsInit() const { return m_init; }
	inline bool IsSend() const { return m_send; }
	inline bool IsRecv() const { return !m_send; }

	void Align();
	void Serialize(bool & v);
	void Serialize(float & v);
	void Serialize(std::string & s);

	template <typename T>
	inline void Serialize(T & v)
	{
		if (IsSend())
			m_bitStream->Write<T>(v);
		else
			m_bitStream->Read<T>(v);
	}

	template <typename T>
	inline void SerializeBits(T & v, uint32_t numBits)
	{
		if (IsSend())
			m_bitStream->WriteBits<T>(v, numBits);
		else
			v = m_bitStream->ReadBits<T>(numBits);
	}
};

//

class NetSerializable : public NetSerializationContext
{
	NetSerializableObject * m_owner;
	bool m_isDirty;

public:
	NetSerializable(NetSerializableObject * owner);

	void SetOwner(NetSerializableObject * owner);
	NetSerializableObject * GetOwner() const { return m_owner; }

	bool IsDirty() const;
	void SetDirty();
	void ResetDirty();

	void SerializeStruct(bool init, bool send, BitStream & bitStream);

	virtual void SerializeStruct() = 0;
};

//

class NetSerializableObject
{
	std::vector<NetSerializable*> m_serializables;

public:
	NetSerializableObject();

	void Register(NetSerializable * serializable);
	bool Serialize(bool init, bool send, BitStream & bitStream);
	bool IsDirty() const;
};
