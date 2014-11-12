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
		m_object = 0;
		m_serverObjectCreationId = 0;
	}

	void Object::Initialize(int objectID, int creationID, IObject * object)
	{
		Assert(object);

		SetObjectID(objectID);
		m_object = object;
		m_serverObjectCreationId = creationID;
	}
}
