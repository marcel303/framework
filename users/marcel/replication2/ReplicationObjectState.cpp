#include "Debug.h"
#include "ReplicationObjectState.h"

namespace Replication
{
	ObjectState::ObjectState(Object * object)
		: m_object(object)
		, m_isDestroyed(false)
	{
		Assert(object);

		if (m_object)
		{
			m_objectID = m_object->GetObjectID();
		}
	}
}
