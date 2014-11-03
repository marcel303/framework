#ifndef REPLICATIONOBJECTSTATE_H
#define REPLICATIONOBJECTSTATE_H
#pragma once

#include <list>
#include "ReplicationObject.h"

namespace Replication
{
	class ObjectState
	{
	public:
		ObjectState(Object* object);

		int SV_RequireVersionedUpdate() const;

		Object* m_object;
		int m_objectID; // Must be cached instead of derived from m_object, because it must exist when object deleted for destroy messages to clients to be repeated.

		int m_lastUpdate;
		int m_skipCount;

		float m_priority;
		int m_tiebreaker;

		std::vector<int> m_versioning;
	};

	typedef std::list<ObjectState> ObjectStateColl;
	typedef ObjectStateColl::iterator ObjectStateCollItr;
}

#endif
