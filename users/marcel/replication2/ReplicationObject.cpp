#include "BitStream.h"
#include "Debug.h"
#include "ReplicationObject.h"
#include "ReplicationObjectState.h"
#include "Types.h"

namespace Replication
{
	Object::Object()
	{
		m_objectID = 0;
		m_up = 0;
		m_serverSerializableObject = 0;
		m_clientSerializableObject = 0;
		m_serverNeedUpdate = false;
	}

	void Object::SV_Initialize(int objectID, const std::string& className, NetSerializableObject* serializableObject)
	{
		FASSERT(serializableObject);

		m_objectID = objectID;
		m_className = className;
		m_serverSerializableObject = serializableObject;
		m_serverNeedUpdate = RequireUpdating();
	}

	void Object::CL_Initialize1(int objectID, const std::string& className)
	{
		m_objectID = objectID;
		m_className = className;
	}

	void Object::CL_Initialize2(NetSerializableObject* serializableObject)
	{
		m_clientSerializableObject = serializableObject;
	}

	bool Object::Serialize(BitStream& bitStream, SERIALIZATION_MODE mode, bool send)
	{
		// todo : don't send packet if not updated

		NetSerializableObject * serializableObject = send ? m_serverSerializableObject : m_clientSerializableObject;

		serializableObject->Serialize((mode == SM_CREATE), send, bitStream);

		return true;
	}

	bool Object::RequireUpdating() const
	{
		// todo : check if dirty ?

		return true;
	}
}
