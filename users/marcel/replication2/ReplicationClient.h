#ifndef REPLICATIONCLIENT_H
#define REPLICATIONCLIENT_H
#pragma once

#include <list>
#include <map>
#include "Client.h"
#include "ReplicationObject.h"

class ReplicationClient
{
public:
	struct CreateOrDestroy
	{
		enum Type
		{
			kType_Create,
			kType_Destroy
		};

		CreateOrDestroy(ReplicationObject * object, Type type)
			: m_object(type == kType_Create ? object : 0)
			, m_objectID(object->GetObjectID())
			, m_type(type)
		{
		}

		ReplicationObject * m_object;
		uint16_t m_objectID; // must be cached so wen can serialize the destroy message when an object is removed
		Type m_type;
	};

	typedef std::list<CreateOrDestroy> CreateOrDestroyList;

public:
	ReplicationClient();
	~ReplicationClient();

	void Initialize(Channel * channel, void * up);

	void SV_AddObject(ReplicationObject * object);
	void CL_AddObject(ReplicationObject * object);
	void CL_RemoveObject(ReplicationObject * object);
	ReplicationObject * CL_FindObject(uint16_t objectID);

	CreateOrDestroyList::iterator SV_Find(CreateOrDestroyList & collection, int objectID);
	void SV_Move(uint16_t objectID, CreateOrDestroyList & src, CreateOrDestroyList & dst);

public:
	Channel * m_channel;
	void * m_up;

	CreateOrDestroyList m_createdOrDestroyed;

	typedef std::map<int, ReplicationObject*> ReplicationObjectColl;
	ReplicationObjectColl m_clientObjects;
};

#endif
