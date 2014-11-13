#include "Debug.h"
#include "ReplicationObjectState.h"

ReplicationObjectState::ReplicationObjectState(ReplicationObject * object)
	: m_object(object)
	, m_isDestroyed(false)
{
	Assert(object);

	if (m_object)
	{
		m_objectID = m_object->GetObjectID();
	}
}
