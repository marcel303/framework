#ifndef REPLICATIONCLIENT_H
#define REPLICATIONCLIENT_H
#pragma once

#include <list>
#include <map>
//#include "Channel.h"
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

		void Initialize(::Client* client, void* up);

		void SV_AddObject(Object* object);
		void CL_AddObject(Object* object);
		void CL_RemoveObject(Object* object);
		Object* CL_FindObject(int objectID);

		void SV_Prioritize();

		ObjectStateCollItr SV_Find(ObjectStateColl& collection, int objectID);
		void SV_Move(int objectID, ObjectStateColl& src, ObjectStateColl& dst);

		inline ::Client* GetClient()
		{
			return m_client;
		}

	private:
		static void SV_CalculatePriority(ObjectState& state);
		static bool SV_ComparePriorities(const ObjectState& state1, const ObjectState& state2);

		::Client* m_client;
		void* m_up;

	public: // fixme
		ObjectStateColl m_created;   // Waiting for ACK.
		ObjectStateColl m_destroyed; // Waiting for ACK.
		ObjectStateColl m_active;    // Active objects.

	//private: // fixme
		typedef std::map<int, Object*> ObjectColl;
		typedef ObjectColl::iterator ObjectCollItr;

		ObjectColl m_clientObjects;
	};
}

#endif
