#include "Debug.h"
#include "ReplicationObjectState.h"

namespace Replication
{
	ObjectState::ObjectState(Object* object)
	{
		FASSERT(object);

		m_object = object;

		if (m_object)
		{
			m_objectID = m_object->m_objectID;
		}
	}
}
