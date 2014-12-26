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

	BitStream & GetBitStream() { return *m_bitStream; }

	inline bool IsInit() const { return m_init; }
	inline bool IsSend() const { return m_send; }
	inline bool IsRecv() const { return !m_send; }

	void Align();
	void Serialize(bool & v);
	void Serialize(float & v);
	void SerializeFloatRange(float & v, float min, float max, uint32_t numBits);
	void Serialize(std::string & s);
	void SerializeBytes(void * bytes, uint32_t numBytes);

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
	uint8_t m_channel;
	uint8_t m_channelMask;
	bool m_isDirty;

protected:
	void SetChannel(uint8_t channel) { Assert(m_channelMask & (1 << channel)); m_channel = channel; }

public:
	NetSerializable(NetSerializableObject * owner, uint8_t channelMask = 0x1, uint8_t channel = 0);
	virtual ~NetSerializable();

	void SetOwner(NetSerializableObject * owner);
	NetSerializableObject * GetOwner() const { return m_owner; }

	uint8_t GetChannel() const { return m_channel; }
	uint8_t GetChannelMask() const { return m_channelMask; }

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
	bool Serialize(bool init, bool send, int channel, BitStream & bitStream);
	bool IsDirty() const;
};
