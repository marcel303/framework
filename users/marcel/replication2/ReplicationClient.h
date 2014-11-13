#ifndef REPLICATIONCLIENT_H
#define REPLICATIONCLIENT_H
#pragma once

#include <list>
#include <map>
#include "Client.h"
#include "ReplicationObject.h"
#include "ReplicationObjectState.h"

namespace Replication
{
	class Client
	{
	public:
		Client();
		~Client();

		void Initialize(Channel * channel, void * up);

		void SV_AddObject(Object * object);
		void CL_AddObject(Object * object);
		void CL_RemoveObject(Object * object);
		Object * CL_FindObject(int objectID);

		ObjectStateCollItr SV_Find(ObjectStateColl & collection, int objectID);
		void SV_Move(int objectID, ObjectStateColl & src, ObjectStateColl & dst);

		Channel * m_channel;
		void* m_up;

	public: // fixme
		ObjectStateColl m_createdOrDestroyed;
		ObjectStateColl m_active;

	//private: // fixme
		typedef std::map<int, Object*> ObjectColl;
		typedef ObjectColl::iterator ObjectCollItr;

		ObjectColl m_clientObjects;
	};
}

#endif
