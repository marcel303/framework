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

	CreateOrDestroy create(object, CreateOrDestroy::kType_Create);

	m_createdOrDestroyed.push_back(create);
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

ReplicationObject * ReplicationClient::CL_FindObject(uint16_t objectID)
{
	auto i = m_clientObjects.find(objectID);

	if (i == m_clientObjects.end())
		return 0;
	else
		return i->second;
}

ReplicationClient::CreateOrDestroyList::iterator ReplicationClient::SV_Find(CreateOrDestroyList & collection, int objectID)
{
	for (auto i = collection.begin(); i != collection.end(); ++i)
		if (i->m_objectID == objectID)
			return i;

	return collection.end();
}

void ReplicationClient::SV_Move(uint16_t objectID, CreateOrDestroyList & src, CreateOrDestroyList & dst)
{
	auto i = SV_Find(src, objectID);

	if (i != src.end())
	{
		CreateOrDestroy createOrDestroy = *i;
		src.erase(i);
		dst.push_back(createOrDestroy);
	}
}
