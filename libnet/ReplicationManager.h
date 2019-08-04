#pragma once

#include "NetHandlePool.h"
#include "NetSerializable.h"
#include "PacketListener.h"
#include "ReplicationClient.h"
#include "ReplicationHandler.h"
#include "ReplicationObject.h"
#include <map>

class ReplicationManager : public PacketListener
{
public:
	ReplicationManager();
	~ReplicationManager();

	int SV_CreateClient(Channel * channel, void * up);
	int CL_CreateClient(Channel * channel, void * up);

	void SV_DestroyClient(int clientID);
	void CL_DestroyClient(int clientID);

	int SV_AddObject(ReplicationObject * object);
	void SV_RemoveObject(int objectID);

	void SV_Shutdown();
	void CL_Shutdown();

	void SV_Update();
	void CL_Update();

	void CL_RegisterHandler(ReplicationHandler * handler);

private:
	virtual void OnReceive(Packet & packet, Channel * channel);

	void HandleCreate(BitStream & bitStream, Channel * channel);
	void HandleDestroy(BitStream & bitStream, Channel * channel);
	void HandleUpdate(BitStream & bitStream, Channel * channel);

private:
	typedef std::map<int, ReplicationClient*> ReplicationClientColl;
	typedef std::map<Channel*, ReplicationClient*> ReplicationClientCache;
	typedef std::map<int, ReplicationObject*> ReplicationObjectColl;

private:
	typedef PacketBuilder<2048> RepMgrPacketBuilder;

	int CreateClientEx(Channel * channel, bool serverSide, void * up);

	void SyncClient(ReplicationClient * client);
	void SyncClientObject(ReplicationClient * client, ReplicationObject * object);

	ReplicationClient * SV_FindClient(Channel * channel);
	ReplicationClient * CL_FindClient(Channel * channel);
	ReplicationObject * SV_FindObject(int objectID);

	bool CL_DestroyObject(ReplicationClient * client, int objectID);

	Packet MakePacket(uint8_t messageID, RepMgrPacketBuilder & packetBuilder, BitStream & bitStream) const;

	ReplicationClientColl m_serverClients;
	ReplicationObjectColl m_serverObjects;
	ReplicationClientColl m_clientClients;

	ReplicationClientCache m_serverClientsCache; // channel -> client.
	ReplicationClientCache m_clientClientsCache; // channel -> client.

	HandlePool<uint32_t> m_clientIDs;
	HandlePool<uint16_t> m_objectIDs;

	ReplicationHandler * m_handler;
	int m_tick;
	int m_serverObjectCreationId;
};
