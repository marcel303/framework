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
	auto i = m_clientObjects.find(objectID);

	if (i == m_clientObjects.end())
		return 0;
	else
		return i->second;
}

ReplicationObjectStateColl::iterator ReplicationClient::SV_Find(ReplicationObjectStateColl & collection, int objectID)
{
	for (auto i = collection.begin(); i != collection.end(); ++i)
		if (i->m_objectID == objectID)
			return i;

	return collection.end();
}

void ReplicationClient::SV_Move(int objectID, ReplicationObjectStateColl & src, ReplicationObjectStateColl & dst)
{
	auto i = SV_Find(src, objectID);

	if (i != src.end())
	{
		ReplicationObjectState state = *i;
		src.erase(i);
		dst.push_back(state);
	}
}
