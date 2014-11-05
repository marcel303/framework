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
	}

	Manager::~Manager()
	{
		while (m_serverObjects.size() > 0)
			SV_DestroyObject(m_serverObjects.begin()->first);

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

		if (i != m_serverClients.end())
		{
			m_serverClientsCache.erase(i->second->GetClient()->m_channel);
			delete i->second;
			m_clientIDs.Free(clientID);
			m_serverClients.erase(i);
		}
		else
		{
			DB_ERR("Client not found");
			Assert(0);
		}
	}

	void Manager::CL_DestroyClient(int clientID)
	{
		ClientCollItr i = m_clientClients.find(clientID);

		if (i != m_clientClients.end())
		{
			m_clientClientsCache.erase(i->second->GetClient()->m_channel);
			delete i->second;
			m_clientClients.erase(i);
		}
		else
		{
			DB_ERR("Client not found");
		}
	}

	int Manager::SV_CreateObject(const std::string & className, NetSerializableObject * serializableObject)
	{
		Assert(serializableObject);

		// FIXME: May cause errors when object with objectID already exists on client and has not been deleted (yet).
		// Free objectID's only when all clients synced?
		int objectID = m_objectIDs.Allocate();

		Object * object = new Object();
		object->SV_Initialize(objectID, className, serializableObject);

		m_serverObjects[objectID] = object;

		for (ClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
			SyncClientObject(i->second, object);

		return objectID;
	}

	void Manager::SV_DestroyObject(int objectID)
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
			Assert(createdOrDestroyed.empty() || !createdOrDestroyed.front().IsDestroyed());

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

				Assert(!j->IsDestroyed());
				j->m_isDestroyed = true;
				client->m_createdOrDestroyed.push_back(*j);
			}
			
			if (createdOrDestroyed.empty() && active.empty())
			{
				DB_ERR("Could not find object in server client");
				return;
			}
		}

		ObjectCollItr j = m_serverObjects.find(objectID);

		if (j != m_serverObjects.end())
		{
			delete j->second;
			m_serverObjects.erase(j);
			m_objectIDs.Free(objectID);
		}
		else
		{
			DB_ERR("Could not find object");
			return;
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
			delete object;
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
				CL_DestroyObject(client, object->m_objectID);
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
					client->GetClient()->m_channel->Send(packet);
				}
				else
				{
					// object has been created. send creation message to client

					BitStream bitStream;

					const uint16_t objectID = j->m_objectID;
					const std::string & className = j->m_object->m_className;

					bitStream.Write(objectID);
					bitStream.WriteString(className);

					j->m_object->Serialize(bitStream, true, true);

					RepMgrPacketBuilder packetBuilder;
					Packet packet = MakePacket(REPMSG_CREATE, packetBuilder, bitStream);
					client->GetClient()->m_channel->Send(packet);

					client->m_active.push_back(*j);
				}
			}

			client->m_createdOrDestroyed.clear();

			for (ObjectStateCollItr j = client->m_active.begin(); j != client->m_active.end(); ++j)
			{
				//if (j->m_object->m_serverNeedUpdate)
				{
					// Go through server objects & replicate.

					BitStream bitStream;

					const uint16_t objectID = j->m_objectID;

					bitStream.Write(objectID);

					if (j->m_object->Serialize(bitStream, false, true))
					{
						RepMgrPacketBuilder packetBuilder;
						Packet packet = MakePacket(REPMSG_UPDATE, packetBuilder, bitStream);
						client->GetClient()->m_channel->Send(packet);
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

	bool Manager::OnObjectCreate(Client * client, Object * object)
	{
		Assert(client);
		Assert(object);

		Assert(m_handler);

		NetSerializableObject * serializableObject;

		// Retrieve parameters through callback.
		if (m_handler->OnReplicationObjectCreate1(client, object->m_className, &serializableObject, &object->m_up))
		{
			object->CL_Initialize2(serializableObject);
			return true;
		}
		else
		{
			DB_ERR("\tCannot instantiate object");
			return false;
		}
	}

	void Manager::OnObjectDestroy(Client * client, Object * object)
	{
		Assert(client);
		Assert(object);

		Assert(m_handler);

		m_handler->OnReplicationObjectDestroy(client, object->m_up);
	}

	void Manager::OnReceive(Packet & packet, Channel * channel)
	{
		Stats::I().m_rep.m_bytesReceived.Inc(packet.GetSize());

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
		case REPMSG_CREATE_ACK:
			HandleCreateAck(bitStream, channel);
			break;
		case REPMSG_DESTROY:
			HandleDestroy(bitStream, channel);
			break;
		case REPMSG_DESTROY_ACK:
			HandleDestroyAck(bitStream, channel);
			break;
		case REPMSG_UPDATE:
			HandleUpdate(bitStream, channel);
			break;
		default:
			DB_ERR("Unknown message ID");
			Assert(0);
			break;
		}

		// TODO: As server, respond to ACK's.
		// As client, respond to create, destroy.
	}

	void Manager::HandleCreate(BitStream & bitStream, Channel * channel)
	{
		Client * client = CL_FindClient(channel);

		if (!client)
		{
			DB_ERR("Received create from unknown channel (%d)", channel->m_id);
			return;
		}

		uint16_t objectID;
		std::string className;

		bitStream.Read(objectID);

		// Check if object already created. If so, ack.
		if (client->CL_FindObject(objectID) != 0)
		{
			SendCreateAck(objectID, channel);
			DB_ERR("\tObject already created (%d)", objectID);
			return;
		}

		Stats::I().m_rep.m_objectsCreated.Inc(1);

		className = bitStream.ReadString();

		Object * object = new Object();
		object->CL_Initialize1(objectID, className);

		if (!OnObjectCreate(client, object))
		{
			DB_ERR("\tUnable to create object");
			Assert(0);
			delete object;
			return;
		}

		object->Serialize(bitStream, true, false);

		m_handler->OnReplicationObjectCreate2(client, object->m_up);

		client->CL_AddObject(object);

		SendCreateAck(objectID, channel);
	}

	void Manager::HandleCreateAck(BitStream & bitStream, Channel * channel)
	{
		// TODO: Check if server.

		uint16_t objectID;

		bitStream.Read(objectID);

		Client * client = SV_FindClient(channel);

		if (client)
		{
			// TODO: speedup~
			// FIXME: make method of repclient.
			ObjectStateCollItr state = client->m_active.end();

			for (ObjectStateCollItr i = client->m_active.begin(); i != client->m_active.end(); ++i)
				if (i->m_objectID == objectID)
					state = i;

			if (state != client->m_active.end())
			{
				state->m_existsOnClient = true;
			}
			else
			{
				DB_ERR("Received ACK for already created/destroyed/unknown object");
				return;
			}
		}
		else
		{
			Assert(0);
			return;
		}
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
			Stats::I().m_rep.m_objectsDestroyed.Inc(1);
			SendDestroyAck(objectID, channel);
		}
		else
		{
			SendDestroyAck(objectID, channel);
			DB_ERR("Received destroy request for non-existing object (%d)", objectID);
			return;
		}
	}

	void Manager::HandleDestroyAck(BitStream & bitStream, Channel * channel)
	{
		// TODO: Check if server.

		Client * client = SV_FindClient(channel);

		if (!client)
		{
			DB_ERR("Received destroy ACK from unknown channel (%d)", channel->m_id);
			return;
		}

		uint16_t objectID;

		bitStream.Read(objectID);

	/*
		bool found = false;

		for (ObjectStateCollItr i = client->m_destroyed.begin(); i != client->m_destroyed.end();)
		{
			if (i->m_objectID == objectID)
			{
				i = client->m_destroyed.erase(i);
				found = true;
			}
			else
				++i;
		}

		if (!found)
		{
			SendDestroyAck(objectID, channel);
			DB_ERR("Received destroy ACK for non-existing object (%d)", objectID);
			return;
		}
	*/
	}

	void Manager::HandleUpdate(BitStream & bitStream, Channel * channel)
	{
		Stats::I().m_rep.m_objectsUpdated.Inc(1);

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
		for (ObjectCollItr i = m_serverObjects.begin(); i != m_serverObjects.end(); ++i)
			SyncClientObject(client, i->second);
	}

	void Manager::SyncClientObject(Client * client, Object * object)
	{
		client->SV_AddObject(object);
	}

	bool Manager::SendCreateAck(int _objectID, Channel * channel)
	{
		BitStream bitStream;

		const uint16_t objectID = _objectID;

		bitStream.Write(objectID);

		RepMgrPacketBuilder packetBuilder;
		Packet packet = MakePacket(REPMSG_CREATE_ACK, packetBuilder, bitStream);
		return channel->Send(packet);
	}

	bool Manager::SendDestroyAck(int _objectID, Channel * channel)
	{
		BitStream bitStream;

		const uint16_t objectID = _objectID;

		bitStream.Write(objectID);

		RepMgrPacketBuilder packetBuilder;
		Packet packet = MakePacket(REPMSG_DESTROY_ACK, packetBuilder, bitStream);
		return channel->Send(packet);
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
