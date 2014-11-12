#ifndef REPLICATIONMANAGER_H
#define REPLICATIONMANAGER_H
#pragma once

#include <map>
#include <string>
#include "Client.h"
#include "MyProtocols.h"
#include "NetHandlePool.h"
#include "NetSerializable.h"
#include "PacketListener.h"
#include "ReplicationClient.h"
#include "ReplicationHandler.h"
#include "ReplicationObject.h"
#include "ReplicationObjectState.h"

namespace Replication
{
	class Manager : public PacketListener
	{
	public:
		Manager();
		~Manager();

		int SV_CreateClient(::Client * client, void * up);
		int CL_CreateClient(::Client * client, void * up);
		void SV_DestroyClient(int clientID);
		void CL_DestroyClient(int clientID);

		int SV_AddObject(const std::string & className, IObject * object);
		void SV_RemoveObject(int objectID);

		bool CL_DestroyObject(Client * client, int objectID);
		void CL_Shutdown();

		void SV_Update();
		void CL_Update();

		void CL_RegisterHandler(Handler* handler);

		void OnObjectDestroy(Client * client, Object * object);

		virtual void OnReceive(Packet & packet, Channel * channel);

		void HandleCreate(BitStream & bitStream, Channel * channel);
		void HandleDestroy(BitStream & bitStream, Channel * channel);
		void HandleUpdate(BitStream & bitStream, Channel * channel);

	private:
		typedef std::map<int, Client*> ClientColl;
		typedef ClientColl::iterator ClientCollItr;
		typedef std::map<Channel*, Client*> ClientCache;
		typedef ClientCache::iterator ClientCacheItr;

		typedef std::map<int, Object*> ObjectColl;
		typedef ObjectColl::iterator ObjectCollItr;

	private:
		typedef PacketBuilder<2048> RepMgrPacketBuilder;

		int CreateClientEx(::Client * client, bool serverSide, void * up);

		void SyncClient(Client * client);
		void SyncClientObject(Client * client, Object * object);

		Client * SV_FindClient(Channel * channel);
		Client * CL_FindClient(Channel * channel);
		Object * SV_FindObject(int objectID);

		Packet MakePacket(uint8_t messageID, RepMgrPacketBuilder & packetBuilder, BitStream & bitStream) const;

		ClientColl m_serverClients;
		ObjectColl m_serverObjects;
		ClientColl m_clientClients;

		ClientCache m_serverClientsCache; // channel -> client.
		ClientCache m_clientClientsCache; // channel -> client.

		HandlePool<uint32_t> m_clientIDs;
		HandlePool<uint16_t> m_objectIDs;

		Handler * m_handler;
		int m_tick;
		int m_serverObjectCreationId;
	};
}

#endif
