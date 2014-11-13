#include <algorithm>
#include "Debug.h"
#include "ReplicationClient.h"

namespace Replication
{
	Client::Client()
	{
		m_up = 0;
	}

	Client::~Client()
	{
		while (m_clientObjects.size() > 0)
			CL_RemoveObject(m_clientObjects.begin()->second);
	}

	void Client::Initialize(Channel * channel, void * up)
	{
		Assert(channel);

		m_channel = channel;
		m_up = up;
	}

	void Client::SV_AddObject(Object * object)
	{
		Assert(object);

		// todo : assert object isn't created yet

		ObjectState state(object);

		m_createdOrDestroyed.push_back(state);
	}

	void Client::CL_AddObject(Object * object)
	{
		Assert(object);

		m_clientObjects[object->GetObjectID()] = object;
	}

	void Client::CL_RemoveObject(Object * object)
	{
		Assert(object);

		m_clientObjects.erase(object->GetObjectID());
	}

	Object * Client::CL_FindObject(int objectID)
	{
		ObjectCollItr i = m_clientObjects.find(objectID);

		if (i == m_clientObjects.end())
			return 0;
		else
			return i->second;
	}

	ObjectStateCollItr Client::SV_Find(ObjectStateColl & collection, int objectID)
	{
		for (ObjectStateCollItr j = collection.begin(); j != collection.end(); ++j)
			if (j->m_objectID == objectID)
				return j;

		return collection.end();
	}

	void Client::SV_Move(int objectID, ObjectStateColl & src, ObjectStateColl & dst)
	{
		ObjectStateCollItr i = SV_Find(src, objectID);

		if (i != src.end())
		{
			ObjectState state = *i;
			src.erase(i);
			dst.push_back(state);
		}
	}
}
