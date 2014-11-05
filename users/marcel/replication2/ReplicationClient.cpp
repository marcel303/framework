#include <algorithm>
#include "Debug.h"
#include "ReplicationClient.h"

namespace Replication
{
	Client::Client()
	{
		m_client = 0;
		m_up = 0;
	}

	Client::~Client()
	{
		while (m_clientObjects.size() > 0)
			CL_RemoveObject(m_clientObjects.begin()->second);
	}

	void Client::Initialize(::Client* client, void* up)
	{
		FASSERT(client);

		m_client = client;
		m_up = up;
	}

	void Client::SV_AddObject(Object* object)
	{
		FASSERT(object);

		ObjectState state(object);

		m_created.push_back(state);
	}

	void Client::CL_AddObject(Object* object)
	{
		FASSERT(object);

		m_clientObjects[object->m_objectID] = object;
	}

	void Client::CL_RemoveObject(Object* object)
	{
		FASSERT(object);

		m_clientObjects.erase(object->m_objectID);
	}

	Object* Client::CL_FindObject(int objectID)
	{
		ObjectCollItr i = m_clientObjects.find(objectID);

		if (i == m_clientObjects.end())
			return 0;
		else
			return i->second;
	}

	ObjectStateCollItr Client::SV_Find(ObjectStateColl& collection, int objectID)
	{
		// FIXME: Speedup.
		for (ObjectStateCollItr j = collection.begin(); j != collection.end(); ++j)
			if (j->m_objectID == objectID)
				return j;

		return collection.end();
	}

	void Client::SV_Move(int objectID, ObjectStateColl& src, ObjectStateColl& dst)
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
