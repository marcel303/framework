#include <algorithm>
#include "BitStream.h"
#include "Debug.h"
#include "MyProtocols.h"
#include "PacketDispatcher.h"
#include "ReplicationManager.h"
#include "Stats.h"
#include "Types.h"

namespace Replication
{
	Manager::Manager()
	{
		m_handler = 0;
		m_tick = 0;
		m_serverObjectCreationId = 0;
	}

	Manager::~Manager()
	{
		while (m_serverObjects.size() > 0)
			SV_RemoveObject(m_serverObjects.begin()->first);

		while (m_serverClients.size() > 0)
			SV_DestroyClient(m_serverClients.begin()->first);

		while (m_clientClients.size() > 0)
			CL_DestroyClient(m_clientClients.begin()->first);
	}

	int Manager::SV_CreateClient(::Client * client, void * up)
	{
		return CreateClientEx(client, true, up);
	}

	int Manager::CL_CreateClient(::Client * client, void * up)
	{
		return CreateClientEx(client, false, up);
	}

	void Manager::SV_DestroyClient(int clientID)
	{
		ClientCollItr i = m_serverClients.find(clientID);

		AssertMsg(i != m_serverClients.end(), "client does not exist. clientId=%d", clientID);
		if (i != m_serverClients.end())
		{
			m_serverClientsCache.erase(i->second->GetClient()->m_channel);
			delete i->second;
			m_clientIDs.Free(clientID);
			m_serverClients.erase(i);
		}
	}

	void Manager::CL_DestroyClient(int clientID)
	{
		ClientCollItr i = m_clientClients.find(clientID);

		AssertMsg(i != m_clientClients.end(), "client does not exist. clientId=%d", clientID);
		if (i != m_clientClients.end())
		{
			m_clientClientsCache.erase(i->second->GetClient()->m_channel);
			delete i->second;
			m_clientClients.erase(i);
		}
	}

	int Manager::SV_AddObject(const std::string & className, Object * object)
	{
		Assert(object);

		// fixme : may cause errors when object with objectID already exists on client and has not been
		// deleted (yet). free objectID's only when all clients are synced?

		uint16_t objectID = m_objectIDs.Allocate();
		uint32_t creationID = m_serverObjectCreationId++;

		object->SetObjectID(objectID);
		object->SetCreationID(creationID);

		m_serverObjects[objectID] = object;

		for (ClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
			SyncClientObject(i->second, object);

		return objectID;
	}

	void Manager::SV_RemoveObject(int objectID)
	{
		// todo : we should probably mark the object destroyed, and *really* destroy it once the
		//        last update serialization has taken place, to ensure any critical updates
		//        are received by the client too

		for (ClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
		{
			Client * client = i->second;

			ObjectStateColl createdOrDestroyed;
			ObjectStateColl active;

			client->SV_Move(objectID, client->m_createdOrDestroyed, createdOrDestroyed);
			client->SV_Move(objectID, client->m_active, active);

			Assert((createdOrDestroyed.size() + active.size()) <= 1);
			Assert(createdOrDestroyed.empty() || !createdOrDestroyed.front().m_isDestroyed);

			for (ObjectStateCollItr j = createdOrDestroyed.begin(); j != createdOrDestroyed.end(); ++j)
			{
				// object shouldn't be destroyed twice
				Assert(!j->m_isDestroyed);
				if (j->m_isDestroyed)
				{
					// but if it is, make sure that the client at least gets the message
					client->m_createdOrDestroyed.push_back(*j);
				}
				else
				{
					// object creation message wasn't sent yet. keep it off the list
				}
			}

			for (ObjectStateCollItr j = active.begin(); j != active.end(); ++j)
			{
				// add object to the destroyed object list

				Assert(!j->m_isDestroyed);
				j->m_object = 0;
				j->m_isDestroyed = true;
				client->m_createdOrDestroyed.push_back(*j);
			}
			
			AssertMsg(!(createdOrDestroyed.empty() && active.empty()), "could not find object in server client. objectId=%d", objectID);
		}

		ObjectCollItr j = m_serverObjects.find(objectID);

		AssertMsg(j != m_serverObjects.end(), "object does not exist. objectId=%d", objectID);
		if (j != m_serverObjects.end())
		{
			m_serverObjects.erase(j);
			m_objectIDs.Free(objectID);
		}
	}

	bool Manager::CL_DestroyObject(Client * client, int objectID)
	{
		Assert(client);

		Object * object = client->CL_FindObject(objectID);

		if (object)
		{
			client->CL_RemoveObject(object);
			OnObjectDestroy(client, object);
			return true;
		}

		return false;
	}

	void Manager::CL_Shutdown()
	{
		for (ClientCollItr i = m_clientClients.begin(); i != m_clientClients.end(); ++i)
		{
			Client * client = i->second;

			while (client->m_clientObjects.size() > 0)
			{
				Object * object = client->m_clientObjects.begin()->second;
				CL_DestroyObject(client, object->GetObjectID());
			}

			//CL_DestroyClient(m_clientClients.begin()->first);
		}
	}

	void Manager::SV_Update()
	{
		for (ClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
		{
			Client * client = i->second;

			// handle object creation and destruction

			for (ObjectStateCollItr j = client->m_createdOrDestroyed.begin(); j != client->m_createdOrDestroyed.end(); ++j)
			{
				if (j->m_isDestroyed)
				{
					// object has been destroyed. send destruction message to client

					BitStream bitStream;

					const uint16_t objectID = j->m_objectID;

					bitStream.Write(objectID);

					RepMgrPacketBuilder packetBuilder;
					Packet packet = MakePacket(REPMSG_DESTROY, packetBuilder, bitStream);
					client->GetClient()->m_channel->Send(packet, 0);
				}
				else
				{
					// object has been created. send creation message to client

					BitStream bitStream;

					const uint16_t objectID = j->m_object->GetObjectID();
					const std::string & className = j->m_object->ClassName();

					bitStream.Write(objectID);
					bitStream.WriteString(className);

					j->m_object->Serialize(bitStream, true, true);

					RepMgrPacketBuilder packetBuilder;
					Packet packet = MakePacket(REPMSG_CREATE, packetBuilder, bitStream);
					client->GetClient()->m_channel->Send(packet, 0);

					client->m_active.push_back(*j);
				}
			}

			client->m_createdOrDestroyed.clear();
		}

		// all clients should share the same view now with regard to the set of active objects

		if (!m_serverClients.empty())
		{
			Client* clientForActiveObjects = m_serverClients.begin()->second; // fixme : make this nicer..

		for (ObjectStateCollItr j = clientForActiveObjects->m_active.begin(); j != clientForActiveObjects->m_active.end(); ++j)
		{
			//if (j->m_object->m_serverNeedUpdate)
			{
				// Go through server objects & replicate.

				BitStream bitStream;

				const uint16_t objectID = j->m_object->GetObjectID();

				bitStream.Write(objectID);

				if (j->m_object->Serialize(bitStream, false, true))
				{
					RepMgrPacketBuilder packetBuilder;
					Packet packet = MakePacket(REPMSG_UPDATE, packetBuilder, bitStream);

					for (ClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
					{
						Client* client = i->second;

						client->GetClient()->m_channel->Send(packet, 0);
					}
				}
			}
		}

		}

		m_tick++;
	}

	void Manager::CL_Update()
	{
	}

	void Manager::CL_RegisterHandler(Handler * handler)
	{
		Assert(handler);

		m_handler = handler;
	}

	void Manager::OnObjectDestroy(Client * client, Object * object)
	{
		Assert(client);
		Assert(object);

		Assert(m_handler);

		m_handler->OnReplicationObjectDestroy(client, object);
	}

	void Manager::OnReceive(Packet & packet, Channel * channel)
	{
		NET_STAT_ADD(Replication_BytesReceived, packet.GetSize());

		uint8_t messageID;

		if (!packet.Read8(&messageID))
		{
			Assert(0);
			return;
		}

		uint16_t bitStreamSize;

		if (!packet.Read16(&bitStreamSize))
		{
			Assert(0);
			return;
		}

		Packet bitStreamPacket;

		if (!packet.Extract(bitStreamPacket, Net::BitsToBytes(bitStreamSize), true))
		{
			Assert(0);
			return;
		}

		BitStream bitStream(bitStreamPacket.GetData(), bitStreamSize);

		switch (messageID)
		{
		case REPMSG_CREATE:
			HandleCreate(bitStream, channel);
			break;
		case REPMSG_DESTROY:
			HandleDestroy(bitStream, channel);
			break;
		case REPMSG_UPDATE:
			HandleUpdate(bitStream, channel);
			break;
		default:
			AssertMsg(false, "unknown message. messageId=%d", messageID);
			break;
		}
	}

	void Manager::HandleCreate(BitStream & bitStream, Channel * channel)
	{
		Client * client = CL_FindClient(channel);

		if (!client)
		{
			AssertMsg(false, "received create from unknown channel. channelId=%d", channel->m_id);
			return;
		}

		uint16_t objectID;
		bitStream.Read(objectID);

		// Check if object already created. If so, ack.
		if (client->CL_FindObject(objectID) != 0)
		{
			AssertMsg(false, "object already created. objectId=%d", objectID);
			return;
		}

		std::string className;
		className = bitStream.ReadString();

		Object * object;

		// Retrieve parameters through callback.
		if (!m_handler->OnReplicationObjectCreate1(client, className, &object))
		{
			AssertMsg(false, "unable to create object. className=%s", className.c_str());
			return;
		}

		NET_STAT_INC(Replication_ObjectsCreated);

		object->SetObjectID(objectID);
		object->Serialize(bitStream, true, false);

		m_handler->OnReplicationObjectCreate2(client, object);

		client->CL_AddObject(object);
	}

	void Manager::HandleDestroy(BitStream & bitStream, Channel * channel)
	{
		Client * client = CL_FindClient(channel);

		if (!client)
		{
			DB_ERR("Received destroy request from unknown channel (%d)", channel->m_id);
			return;
		}

		uint16_t objectID;

		bitStream.Read(objectID);

		if (CL_DestroyObject(client, objectID))
		{
			NET_STAT_INC(Replication_ObjectsDestroyed);
		}
		else
		{
			DB_ERR("Received destroy request for non-existing object (%d)", objectID);
			return;
		}
	}

	void Manager::HandleUpdate(BitStream & bitStream, Channel * channel)
	{
		NET_STAT_INC(Replication_ObjectsUpdated);

		Client * client = CL_FindClient(channel);

		if (!client)
		{
			DB_ERR("Received update from unknown channel (%d)", channel->m_id);
			return;
		}

		uint16_t objectID;

		bitStream.Read(objectID);

		Object * object = client->CL_FindObject(objectID);

		if (object)
		{
			object->Serialize(bitStream, false, false);
		}
		else
		{
			DB_ERR("\tReceived update for non-existing object (%d)", objectID);
			return;
		}
	}

	int Manager::CreateClientEx(::Client * client, bool serverSide, void * up)
	{
		Assert(client);

		int clientID = m_clientIDs.Allocate();

		Client * repClient = new Client();
		repClient->Initialize(client, up);

		if (serverSide)
		{
			m_serverClients[clientID] = repClient;
			m_serverClientsCache[client->m_channel] = repClient;

			SyncClient(repClient);
		}
		else
		{
			m_clientClients[clientID] = repClient;
			m_clientClientsCache[client->m_channel] = repClient;
		}

		return clientID;
	}

	void Manager::SyncClient(Client * client)
	{
		std::vector<Object*> objects;

		for (ObjectCollItr i = m_serverObjects.begin(); i != m_serverObjects.end(); ++i)
			objects.push_back(i->second);

		std::sort(objects.begin(), objects.end(), [] (Object* o1, Object* o2) { return o1->GetCreationID() < o2->GetCreationID(); });

		for (std::vector<Object*>::iterator i = objects.begin(); i != objects.end(); ++i)
			SyncClientObject(client, *i);
	}

	void Manager::SyncClientObject(Client * client, Object * object)
	{
		client->SV_AddObject(object);
	}

	Client * Manager::SV_FindClient(Channel * channel)
	{
		ClientCacheItr i = m_serverClientsCache.find(channel);

		if (i == m_serverClientsCache.end())
			return 0;
		else
			return i->second;
	}

	Client * Manager::CL_FindClient(Channel * channel)
	{
		ClientCacheItr i = m_clientClientsCache.find(channel);

		if (i == m_clientClientsCache.end())
			return 0;
		else
			return i->second;
	}

	Object * Manager::SV_FindObject(int objectID)
	{
		ObjectCollItr i = m_serverObjects.find(objectID);

		if (i == m_serverObjects.end())
			return 0;
		else
			return i->second;
	}

	Packet Manager::MakePacket(uint8_t messageID, RepMgrPacketBuilder & packetBuilder, BitStream & bitStream) const
	{
		uint8_t protocolID = PROTOCOL_REPLICATION;
		packetBuilder.Write8(&protocolID);
		packetBuilder.Write8(&messageID);
		uint16_t bitStreamSize = bitStream.GetDataSize();
		packetBuilder.Write16(&bitStreamSize);
		packetBuilder.Write(bitStream.GetData(), Net::BitsToBytes(bitStream.GetDataSize()));
		return packetBuilder.ToPacket();
	}
}
