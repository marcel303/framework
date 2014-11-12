#ifndef REPLICATIONOBJECTSTATE_H
#define REPLICATIONOBJECTSTATE_H
#pragma once

#include <list>
#include "ReplicationObject.h"

namespace Replication
{
	// todo : remove ObjectState class / refactor so it's only used for the create/destroy list
	class ObjectState
	{
	public:
		ObjectState(Object * object);

		Object * m_object;
		int m_objectID; // Must be cached instead of derived from m_object, because it must exist when object deleted for destroy messages to clients to be repeated.
		bool m_isDestroyed;
	};

	typedef std::list<ObjectState> ObjectStateColl;
	typedef ObjectStateColl::iterator ObjectStateCollItr;
}

#endif
