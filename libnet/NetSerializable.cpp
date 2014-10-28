#include <algorithm>
#include "BitStream.h"
#include "NetDiag.h"
#include "NetSerializable.h"

NetSerializationContext::NetSerializationContext()
	: m_init(false)
	, m_send(false)
	, m_bitStream(0)
{
}

void NetSerializationContext::Set(bool init, bool send, BitStream & bitStream)
{
	m_init = init;
	m_send = send;
	m_bitStream = &bitStream;
}

void NetSerializationContext::Reset()
{
	m_init = false;
	m_send = false;
	m_bitStream = 0;
}

void NetSerializationContext::Align()
{
	if (IsSend())
		m_bitStream->WriteAlign();
	else
		m_bitStream->ReadAlign();
}

void NetSerializationContext::Serialize(bool & v)
{
	if (IsSend())
		m_bitStream->WriteBit(v);
	else
		v = m_bitStream->ReadBit();
}

void NetSerializationContext::Serialize(float & v)
{
	if (IsSend())
		m_bitStream->Write(v);
	else
		m_bitStream->Read(v);
}


void NetSerializationContext::Serialize(std::string & s)
{
	if (IsSend())
		m_bitStream->WriteString(s);
	else
		s = m_bitStream->ReadString();
}

//

NetSerializable::NetSerializable(NetSerializableObject * owner)
	: m_owner(0)
	, m_isDirty(false)
{
	if (owner)
	{
		owner->Register(this);
	}
}

void NetSerializable::SetOwner(NetSerializableObject * owner)
{
	NetAssert(m_owner == 0);

	m_owner = owner;
}

bool NetSerializable::IsDirty() const
{
	return m_isDirty;
}

void NetSerializable::SetDirty()
{
	m_isDirty = true;
}

void NetSerializable::ResetDirty()
{
	m_isDirty = false;
}

void NetSerializable::SerializeStruct(bool init, bool send, BitStream & bitStream)
{
	NetSerializationContext::Set(init, send, bitStream);
	{
		SerializeStruct();
	}
	NetSerializationContext::Reset();
}

//

NetSerializableObject::NetSerializableObject()
{
}

void NetSerializableObject::Register(NetSerializable* serializable)
{
	NetAssert(std::find(m_serializables.begin(), m_serializables.end(), serializable) == m_serializables.end());

	m_serializables.push_back(serializable);

	serializable->SetOwner(this);
}

bool NetSerializableObject::Serialize(bool init, bool send, BitStream & bitStream)
{
	bool result = false;

	for (SerializableArrayItr i = m_serializables.begin(); i != m_serializables.end(); ++i)
	{
		NetSerializable * serialiable = *i;

		bool isDirty;

		if (send)
		{
			isDirty = serialiable->IsDirty();

			bitStream.WriteBit(isDirty);
		}
		else
		{
			isDirty = bitStream.ReadBit();
		}

		if (isDirty)
		{
			result = true;

			serialiable->SerializeStruct(init, send, bitStream);

			serialiable->ResetDirty();
		}
	}

	return result;
}
