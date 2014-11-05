#include "Debug.h"
#include "ReplicationObjectState.h"

namespace Replication
{
	ObjectState::ObjectState(Object * object)
		: m_object(object)
		, m_existsOnClient(false)
		, m_isDestroyed(false)
	{
		Assert(object);

		if (m_object)
		{
			m_objectID = m_object->m_objectID;
		}
	}
}
