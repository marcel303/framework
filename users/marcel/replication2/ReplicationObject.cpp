#include "BitStream.h"
#include "Debug.h"
#include "ReplicationObject.h"
#include "ReplicationObjectState.h"
#include "Types.h"

namespace Replication
{
	Object::Object()
		: IObject()
	{
		m_up = 0;
		m_serverObject = 0;
		m_clientObject = 0;
		m_serverNeedUpdate = false;
		m_serverObjectCreationId = 0;
	}

	void Object::SV_Initialize(int objectID, int creationID, const std::string & className, IObject * object)
	{
		Assert(object);

		SetObjectID(objectID);
		m_className = className;
		m_serverObject = object;
		m_serverNeedUpdate = RequireUpdating();
		m_serverObjectCreationId = creationID;
	}

	void Object::CL_Initialize1(int objectID, const std::string & className)
	{
		SetObjectID(objectID);
		m_className = className;
	}

	void Object::CL_Initialize2(IObject * object)
	{
		m_clientObject = object;
	}

	bool Object::Serialize(BitStream & bitStream, bool init, bool send)
	{
		IObject * object =
			send
			? m_serverObject
			: m_clientObject;

		return object->Serialize(bitStream, init, send);
	}

	bool Object::RequireUpdating() const
	{
		// todo : check if dirty ?

		return true;
	}
}
