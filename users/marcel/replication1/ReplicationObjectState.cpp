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

			m_versioning.clear();

			for (size_t i = 0; i < m_object->m_serverParameters->m_parameters.size(); ++i)
				m_versioning.push_back(m_object->m_serverParameters->m_parameters[i]->m_version);
		}

		m_lastUpdate = 0;
	}

	int ObjectState::SV_RequireVersionedUpdate() const
	{
		int parameterCount = 0;

		for (size_t i = 0; i < m_object->m_serverParameters->m_parameters.size(); ++i)
			if (m_object->m_serverParameters->m_parameters[i]->m_version != m_versioning[i])
				parameterCount++;

		return parameterCount;
	}
}
