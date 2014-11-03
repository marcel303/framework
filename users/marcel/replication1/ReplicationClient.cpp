// TODO: Make compare methodes inline and non member.
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

	void Client::SV_Prioritize()
	{
		// Sort object priorities. Objects at the top of the list should be transmitted first.
		std::for_each(m_created.begin(), m_created.end(), SV_CalculatePriority);
		std::for_each(m_destroyed.begin(), m_destroyed.end(), SV_CalculatePriority);
		std::for_each(m_active.begin(), m_active.end(), SV_CalculatePriority);
		
		m_created.sort(SV_ComparePriorities);
		m_destroyed.sort(SV_ComparePriorities);
		m_active.sort(SV_ComparePriorities);
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

	void Client::SV_CalculatePriority(ObjectState& state)
	{
		Priority* priority = 0;

		Object* object = state.m_object;

		if (object)
			priority = object->m_serverPriority;

		if (priority)
		{
			if (priority->m_culled)
				state.m_priority = 0.0f;
			else
				state.m_priority = priority->Calc(state.m_skipCount);
		}
		else
			state.m_priority = 1.0f + state.m_skipCount;

		state.m_tiebreaker = rand();
	}

	bool Client::SV_ComparePriorities(const ObjectState& state1, const ObjectState& state2)
	{
		if (state1.m_priority > state2.m_priority)
			return true;
		if (state1.m_priority < state2.m_priority)
			return false;

		return state1.m_tiebreaker < state2.m_tiebreaker;
	}
}
