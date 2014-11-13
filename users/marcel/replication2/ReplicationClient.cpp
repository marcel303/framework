#include <algorithm>
#include "Debug.h"
#include "ReplicationClient.h"

ReplicationClient::ReplicationClient()
{
	m_up = 0;
}

ReplicationClient::~ReplicationClient()
{
	while (m_clientObjects.size() > 0)
		CL_RemoveObject(m_clientObjects.begin()->second);
}

void ReplicationClient::Initialize(Channel * channel, void * up)
{
	Assert(channel);

	m_channel = channel;
	m_up = up;
}

void ReplicationClient::SV_AddObject(ReplicationObject * object)
{
	Assert(object);

	// todo : assert object isn't created yet

	ReplicationObjectState state(object);

	m_createdOrDestroyed.push_back(state);
}

void ReplicationClient::CL_AddObject(ReplicationObject * object)
{
	Assert(object);

	m_clientObjects[object->GetObjectID()] = object;
}

void ReplicationClient::CL_RemoveObject(ReplicationObject * object)
{
	Assert(object);

	m_clientObjects.erase(object->GetObjectID());
}

ReplicationObject * ReplicationClient::CL_FindObject(int objectID)
{
	ReplicationObjectCollItr i = m_clientObjects.find(objectID);

	if (i == m_clientObjects.end())
		return 0;
	else
		return i->second;
}

ReplicationObjectStateCollItr ReplicationClient::SV_Find(ReplicationObjectStateColl & collection, int objectID)
{
	for (ReplicationObjectStateCollItr j = collection.begin(); j != collection.end(); ++j)
		if (j->m_objectID == objectID)
			return j;

	return collection.end();
}

void ReplicationClient::SV_Move(int objectID, ReplicationObjectStateColl & src, ReplicationObjectStateColl & dst)
{
	ReplicationObjectStateCollItr i = SV_Find(src, objectID);

	if (i != src.end())
	{
		ReplicationObjectState state = *i;
		src.erase(i);
		dst.push_back(state);
	}
}
