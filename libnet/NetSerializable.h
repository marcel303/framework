#pragma once

#include <vector>
#include "libnet_forward.h"

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
	void Serialize(std::string & s);

	template <typename T>
	inline void Serialize(T & v)
	{
		if (IsSend())
			m_bitStream->Write<T>(v);
		else
			m_bitStream->Read<T>(v);
	}
};

//

class NetSerializable : public NetSerializationContext
{
	bool m_isDirty;

public:
	NetSerializable();

	bool IsDirty() const;
	void SetDirty();
	void ResetDirty();

	void SerializeStruct(bool init, bool send, BitStream & bitStream);

	virtual void SerializeStruct() = 0;
};

//

class NetSerializableObject
{
	typedef std::vector<NetSerializable*> SerializableArray;
	typedef SerializableArray::iterator SerializableArrayItr;

	SerializableArray m_serializables;

public:
	NetSerializableObject();

	void Register(NetSerializable* serializable);
	bool Serialize(bool init, bool send, BitStream & bitStream);
};
