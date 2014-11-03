// TODO: Let client cache class structures, to prevent sending structure for each new object.
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

	int Manager::SV_CreateClient(::Client* client, void* up)
	{
		return CreateClientEx(client, true, up);
	}

	int Manager::CL_CreateClient(::Client* client, void* up)
	{
		return CreateClientEx(client, false, up);
	}

	void Manager::SV_DestroyClient(int clientID)
	{
		DB_TRACE("");

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
			FASSERT(0);
		}
	}

	void Manager::CL_DestroyClient(int clientID)
	{
		DB_TRACE("");

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
			FASSERT(0);
		}
	}

	int Manager::SV_CreateObject(const std::string& className, ParameterList* parameters, Priority* priority)
	{
		DB_TRACE("");

		FASSERT(parameters);
		FASSERT(priority);

		// FIXME: May cause errors when object with objectID already exists on client and has not been deleted (yet).
		// Free objectID's only when all clients synced?
		int objectID = m_objectIDs.Allocate();

		Object* object = new Object();
		object->SV_Initialize(objectID, className, parameters, priority);

		m_serverObjects[objectID] = object;

		for (ClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
			SyncClientObject(i->second, object);

		DB_TRACE("OK");

		return objectID;
	}

	void Manager::SV_DestroyObject(int objectID)
	{
		DB_TRACE("");

		for (ClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
		{
			Client* client = i->second;

			ObjectStateColl found;

			client->SV_Move(objectID, client->m_created, found);
			client->SV_Move(objectID, client->m_active, found);

			if (found.size() == 1)
			{
				for (ObjectStateCollItr j = found.begin(); j != found.end(); ++j)
				{
					j->m_object = 0;
					j->m_lastUpdate = 0;
					client->m_destroyed.push_back(*j);
				}
			}
			else
			{
				DB_ERR("Could not find object in server client");
				FASSERT(0);
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
			FASSERT(0);
			return;
		}

		DB_TRACE("OK");
	}

	bool Manager::CL_DestroyObject(Client* client, int objectID)
	{
		DB_TRACE("");

		FASSERT(client);

		Object* object = client->CL_FindObject(objectID);

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
		DB_TRACE("");

		for (ClientCollItr i = m_clientClients.begin(); i != m_clientClients.end(); ++i)
		{
			Client* client = i->second;

			while (client->m_clientObjects.size() > 0)
			{
				Object* object = client->m_clientObjects.begin()->second;
				CL_DestroyObject(client, object->m_objectID);
			}

			//CL_DestroyClient(m_clientClients.begin()->first);
		}

		DB_TRACE("OK");
	}

	void Manager::SV_Update()
	{
		for (ClientCollItr i = m_serverClients.begin(); i != m_serverClients.end(); ++i)
		{
			Client* client = i->second;
			const int MAX_BANDWIDTH = 2000; // ~50kb/s.
			//const int MAX_BANDWIDTH = 1000; // ~25kb/s.
			const int UPDATE_INTERVAL = 1;
			const int CREATE_INTERVAL = 20;
			const int DESTROY_INTERVAL = 20;

			for (ObjectStateCollItr j = client->m_created.begin(); j != client->m_created.end(); ++j)
				j->m_skipCount = m_tick - j->m_lastUpdate;
			for (ObjectStateCollItr j = client->m_destroyed.begin(); j != client->m_destroyed.end(); ++j)
				j->m_skipCount = m_tick - j->m_lastUpdate;
			for (ObjectStateCollItr j = client->m_active.begin(); j != client->m_active.end(); ++j)
				j->m_skipCount = m_tick - j->m_lastUpdate;

			client->SV_Prioritize();

			int bandwidth = 0;

			for (ObjectStateCollItr j = client->m_created.begin(); j != client->m_created.end() && bandwidth < MAX_BANDWIDTH; ++j)
			{
				if (j->m_skipCount >= CREATE_INTERVAL)
				{
					// Go through server objects & replicate.

					BitStream bitStream;

					const uint16_t objectID = j->m_objectID;
					const std::string& className = j->m_object->m_className;

					bitStream.Write(objectID);
					bitStream.WriteString(className);

					if (true/*j->m_object->SerializeStructure(packet)*/)
					{
						if (j->m_object->SV_Serialize(bitStream, Object::SM_CREATE, *j))
						{
							RepMgrPacketBuilder packetBuilder;
							Packet packet = MakePacket(REPMSG_CREATE, packetBuilder, bitStream);
							client->GetClient()->m_channel->Send(packet);

							j->m_lastUpdate = m_tick;

							bandwidth += packet.GetSize();

							if (bandwidth >= MAX_BANDWIDTH) // FIXME: Make warning
								DB_ERR("Max bandwidth reached (%d / %d)", bandwidth, MAX_BANDWIDTH);

							//printf("[CREATE] Bandwidth: %d.\n", bandwidth);
						}
						else
						{
							DB_ERR("Failed to create replication:create packet");
							FASSERT(0);
						}
					}
					else
					{
						DB_ERR("Failed to create replication:create packet");
						FASSERT(0);
					}
				}
			}

			for (ObjectStateCollItr j = client->m_destroyed.begin(); j != client->m_destroyed.end() && bandwidth < MAX_BANDWIDTH; ++j)
			{
				if (j->m_skipCount >= DESTROY_INTERVAL)
				{
					// Go through server objects & replicate.

					BitStream bitStream;

					const uint16_t objectID = j->m_objectID;

					bitStream.Write(objectID);

					RepMgrPacketBuilder packetBuilder;
					Packet packet = MakePacket(REPMSG_DESTROY, packetBuilder, bitStream);
					client->GetClient()->m_channel->Send(packet);

					j->m_lastUpdate = m_tick;

					bandwidth += packet.GetSize();

					if (bandwidth >= MAX_BANDWIDTH)
						DB_TRACE("Max bandwidth reached (%d / %d).\n", bandwidth, MAX_BANDWIDTH);

					//printf("[DESTROY] Bandwidth: %d.\n", bandwidth);
				}
			}

			for (ObjectStateCollItr j = client->m_active.begin(); j != client->m_active.end() && bandwidth < MAX_BANDWIDTH; ++j)
			{
				if (j->m_object->m_serverNeedUpdate)
				{
					if (j->m_skipCount >= UPDATE_INTERVAL)
					{
						// Go through server objects & replicate.

						BitStream bitStream;

						const uint16_t objectID = j->m_objectID;

						bitStream.Write(objectID);

						if (j->m_object->SV_Serialize(bitStream, Object::SM_UPDATE, *j))
						{
							RepMgrPacketBuilder packetBuilder;
							Packet packet = MakePacket(REPMSG_UPDATE, packetBuilder, bitStream);
							client->GetClient()->m_channel->Send(packet);

							j->m_lastUpdate = m_tick;

							bandwidth += packet.GetSize();

							if (bandwidth >= MAX_BANDWIDTH) // fixme, make warn
								DB_ERR("Max bandwidth reached (%d / %d).\n", bandwidth, MAX_BANDWIDTH);

							//printf("[UPDATE] Bandwidth: %d.\n", bandwidth);
						}
						else
						{
							DB_ERR("Failed to replication:update packet");
							FASSERT(0);
						}
					}
				}

				if (j->m_object->m_serverNeedVersionUpdate)
				{
					if (j->SV_RequireVersionedUpdate())
					{
						BitStream bitStream;

						const uint16_t objectID = j->m_objectID;

						bitStream.Write(objectID);

						if (j->m_object->SV_Serialize(bitStream, Object::SM_VERSION, *j))
						{
							RepMgrPacketBuilder packetBuilder;
							Packet packet = MakePacket(REPMSG_VERSION, packetBuilder, bitStream);
							client->GetClient()->m_channel->SendReliable(packet);

							bandwidth += packet.GetSize();

							if (bandwidth >= MAX_BANDWIDTH) // fixme, make warn
								DB_ERR("Max bandwidth reached (%d / %d)", bandwidth, MAX_BANDWIDTH);

							//printf("[VERSION] Bandwidth: %d.\n", bandwidth);
						}
						else
						{
							DB_ERR("Failed to replication:version packet");
							FASSERT(0);
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

	void Manager::CL_RegisterHandler(Handler* handler)
	{
		FASSERT(handler);

		m_handler = handler;
	}

	bool Manager::OnObjectCreate(Client* client, Object* object)
	{
		DB_TRACE("");

		FASSERT(client);
		FASSERT(object);

		FASSERT(m_handler);

		ParameterList* parameters;

		// Retrieve parameters through callback.
		if (m_handler->OnReplicationObjectCreate1(client, object->m_className, &parameters, &object->m_up))
		{
			//printf("Created new object.\n");
			object->CL_Initialize2(parameters);
			return true;
		}
		else
		{
			DB_ERR("\tCannot instantiate object");
			FASSERT(0);
			return false;
		}
	}

	void Manager::OnObjectDestroy(Client* client, Object* object)
	{
		//DB_TRACE("");

		FASSERT(client);
		FASSERT(object);

		FASSERT(m_handler);

		m_handler->OnReplicationObjectDestroy(client, object->m_up);

		//printf("Destroyed object.\n");
	}

	void Manager::OnReceive(Packet& packet, Channel* channel)
	{
		Stats::I().m_rep.m_bytesReceived.Inc(packet.GetSize());

		uint8_t messageID;

		if (!packet.Read8(&messageID))
		{
			DB_ERR("WTFHAX?\n");
			FASSERT(0);
			return;
		}

		uint16_t bitStreamSize;

		if (!packet.Read16(&bitStreamSize))
		{
			DB_ERR("WTFHAX?\n");
			FASSERT(0);
			return;
		}

		Packet bitStreamPacket;

		if (!packet.Extract(bitStreamPacket, Net::BitsToBytes(bitStreamSize), true))
		{
			DB_ERR("WTFHAX?\n");
			FASSERT(0);
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
		case REPMSG_VERSION:
			HandleVersion(bitStream, channel);
			break;
		default:
			DB_ERR("Unknown message ID");
			FASSERT(0);
			break;
		}

		// TODO: As server, respond to ACK's.
		// As client, respond to create, destroy.
	}

	void Manager::HandleCreate(BitStream& bitStream, Channel* channel)
	{
		//DB_TRACE("");

		Client* client = CL_FindClient(channel);

		if (!client)
		{
			DB_ERR("Received create from unknown channel (%d)", channel->m_id);
			//FASSERT(0);
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

		//printf("Creating object.\n");
		//printf("\tObjectID: %d.\n", objectID);
		//printf("\tClassName: %s.\n", className.c_str());

		Object* object = new Object();
		object->CL_Initialize1(objectID, className);

		if (!OnObjectCreate(client, object))
		{
			DB_ERR("\tUnable to create object");
			FASSERT(0);
			delete object;
			return;
		}

	#if 0
		if (!object->CL_DeSerializeStructure(packet))
		{
			DB_ERR("\tFailed to deserialize object structure");
			FASSERT(0);
			delete object;
			return;
		}
	#else
		// FIXME: Move #if to ReplicationObject::DeSerializeStructure.
		if (object->m_clientParameters)
		{
			for (size_t i = 0; i < object->m_clientParameters->m_parameters.size(); ++i)
			{
				// (CREATE)
				object->m_clientIndicesCreate.push_back((int)i);

				// (UPDATE)
				if (object->m_clientParameters->m_parameters[i]->m_rep == REP_CONTINUOUS)
					object->m_clientIndicesUpdate.push_back((int)i);
				else
					object->m_clientIndicesUpdate.push_back(-1);

				// (VERSIONED)
				if (object->m_clientParameters->m_parameters[i]->m_rep == REP_ONCHANGE)
					object->m_clientIndicesVersioned.push_back((int)i);
				else
					object->m_clientIndicesVersioned.push_back(-1);
			}
		}
	#endif

		if (!object->CL_DeSerialize(bitStream, Object::SM_CREATE))
		{
			// TODO: Log.
			DB_ERR("\tFailed to deserialize object data");
			FASSERT(0);
			delete object;
			return;
		}

		m_handler->OnReplicationObjectCreate2(client, object->m_up);

		client->CL_AddObject(object);

		SendCreateAck(objectID, channel);
	}

	void Manager::HandleCreateAck(BitStream& bitStream, Channel* channel)
	{
		//DB_TRACE("");

		// TODO: Check if server.

		uint16_t objectID;

		bitStream.Read(objectID);

		//printf("\tObjectID: %d.\n", objectID);

		Client* client = SV_FindClient(channel);

		if (client)
		{
			// TODO: speedup~
			// FIXME: make method of repclient.
			ObjectStateCollItr state = client->m_created.end();

			for (ObjectStateCollItr i = client->m_created.begin(); i != client->m_created.end(); ++i)
				if (i->m_objectID == objectID)
					state = i;

			if (state != client->m_created.end())
			{
				ObjectState temp = *state;
				temp.m_lastUpdate = 0;
				client->m_created.erase(state);
				client->m_active.push_back(temp);
			}
			else
			{
				DB_ERR("Received ACK for already created/destroyed/unknown object");
				//FASSERT(0);
				return;
			}
		}
		else
		{
			DB_ERR("WTFHAX?");
			FASSERT(0);
			return;
		}
	}

	void Manager::HandleDestroy(BitStream& bitStream, Channel* channel)
	{
		//DB_TRACE("");

		Client* client = CL_FindClient(channel);

		if (!client)
		{
			DB_ERR("Received destroy request from unknown channel (%d)", channel->m_id);
			//FASSERT(0);
			return;
		}

		uint16_t objectID;

		bitStream.Read(objectID);

		//printf("\tObjectID: %d.\n", objectID);

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

	void Manager::HandleDestroyAck(BitStream& bitStream, Channel* channel)
	{
		//DB_TRACE("");
		// TODO: Check if server.

		Client* client = SV_FindClient(channel);

		if (!client)
		{
			DB_ERR("Received destroy ACK from unknown channel (%d)", channel->m_id);
			//FASSERT(0);
			return;
		}

		uint16_t objectID;

		bitStream.Read(objectID);

		//printf("\tObjectID: %d.\n", objectID);

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
	}

	void Manager::HandleUpdate(BitStream& bitStream, Channel* channel)
	{
		//printf("[RCV UPDATE]\n");
		Stats::I().m_rep.m_objectsUpdated.Inc(1);

		Client* client = CL_FindClient(channel);

		if (!client)
		{
			DB_ERR("Received update from unknown channel (%d)", channel->m_id);
			//FASSERT(0);
			return;
		}

		uint16_t objectID;

		bitStream.Read(objectID);

		//printf("\tObjectID: %d.\n", objectID);

		Object* object = client->CL_FindObject(objectID);

		if (object)
		{
			if (!object->CL_DeSerialize(bitStream, Object::SM_UPDATE))
			{
				DB_ERR("Unable to deserialize object");
				FASSERT(0);
				return;
			}
		}
		else
		{
			DB_ERR("\tReceived update for non-existing object (%d)", objectID);
			//FASSERT(0);
			return;
		}
	}

	void Manager::HandleVersion(BitStream& bitStream, Channel* channel)
	{
		//printf("[RCV VERSION]\n");
		Stats::I().m_rep.m_objectsVersioned.Inc(1);

		Client* client = CL_FindClient(channel);

		if (!client)
		{
			DB_ERR("Received version from unknown channel (%d)", channel->m_id);
			//FASSERT(0);
			return;
		}

		uint16_t objectID;

		bitStream.Read(objectID);

		//printf("\tObjectID: %d.\n", objectID);

		Object* object = client->CL_FindObject(objectID);

		if (object)
		{
			if (!object->CL_DeSerialize(bitStream, Object::SM_VERSION))
			{
				DB_ERR("Unable to deserialize object");
				FASSERT(0);
				return;
			}
		}
		else
		{
			DB_ERR("\tReceived version for non-existing object (%d)", objectID);
			//FASSERT(0);
			return;
		}
	}

	int Manager::CreateClientEx(::Client* client, bool serverSide, void* up)
	{
		FASSERT(client);

		int clientID = m_clientIDs.Allocate();

		Client* repClient = new Client();
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

	void Manager::SyncClient(Client* client)
	{
		for (ObjectCollItr i = m_serverObjects.begin(); i != m_serverObjects.end(); ++i)
			SyncClientObject(client, i->second);
	}

	void Manager::SyncClientObject(Client* client, Object* object)
	{
		client->SV_AddObject(object);
	}

	bool Manager::SendCreateAck(int _objectID, Channel* channel)
	{
		BitStream bitStream;

		const uint16_t objectID = _objectID;

		bitStream.Write(objectID);

		RepMgrPacketBuilder packetBuilder;
		Packet packet = MakePacket(REPMSG_CREATE_ACK, packetBuilder, bitStream);
		return channel->Send(packet);
	}

	bool Manager::SendDestroyAck(int _objectID, Channel* channel)
	{
		BitStream bitStream;

		const uint16_t objectID = _objectID;

		bitStream.Write(objectID);

		RepMgrPacketBuilder packetBuilder;
		Packet packet = MakePacket(REPMSG_DESTROY_ACK, packetBuilder, bitStream);
		return channel->Send(packet);
	}

	Client* Manager::SV_FindClient(Channel* channel)
	{
		ClientCacheItr i = m_serverClientsCache.find(channel);

		if (i == m_serverClientsCache.end())
			return 0;
		else
			return i->second;
	}

	Client* Manager::CL_FindClient(Channel* channel)
	{
		ClientCacheItr i = m_clientClientsCache.find(channel);

		if (i == m_clientClientsCache.end())
			return 0;
		else
			return i->second;
	}

	Object* Manager::SV_FindObject(int objectID)
	{
		ObjectCollItr i = m_serverObjects.find(objectID);

		if (i == m_serverObjects.end())
			return 0;
		else
			return i->second;
	}

	Packet Manager::MakePacket(uint8_t messageID, RepMgrPacketBuilder& packetBuilder, BitStream& bitStream) const
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
