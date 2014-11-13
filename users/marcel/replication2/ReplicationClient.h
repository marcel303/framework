#ifndef REPLICATIONCLIENT_H
#define REPLICATIONCLIENT_H
#pragma once

#include <list>
#include <map>
#include "Client.h"
#include "ReplicationObject.h"
#include "ReplicationObjectState.h"

class ReplicationClient
{
public:
	ReplicationClient();
	~ReplicationClient();

	void Initialize(Channel * channel, void * up);

	void SV_AddObject(ReplicationObject * object);
	void CL_AddObject(ReplicationObject * object);
	void CL_RemoveObject(ReplicationObject * object);
	ReplicationObject * CL_FindObject(int objectID);

	ReplicationObjectStateColl::iterator SV_Find(ReplicationObjectStateColl & collection, int objectID);
	void SV_Move(int objectID, ReplicationObjectStateColl & src, ReplicationObjectStateColl & dst);

	Channel * m_channel;
	void* m_up;

public: // fixme
	ReplicationObjectStateColl m_createdOrDestroyed;
	ReplicationObjectStateColl m_active;

//private: // fixme
	typedef std::map<int, ReplicationObject*> ReplicationObjectColl;

	ReplicationObjectColl m_clientObjects;
};

#endif
