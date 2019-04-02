#include "ChannelManager.h"
#include "ChannelTypes.h"
#include "NetDiag.h"
#include "NetProtocols.h"
#include "NetStats.h"
#include "PacketDispatcher.h"
#include <algorithm>
#include <vector>

ChannelManager::ChannelManager()
	: m_socket()
	, m_listenChannel(0)
	, m_serverVersion(0)
	, m_channelTimeout(LIBNET_CHANNEL_TIMEOUT_INTERVAL)
	, m_channels()
	, m_channelIds()
	, m_packetDispatcher(0)
	, m_handler(0)
	, m_destroyedChannels()
{
}

ChannelManager::~ChannelManager()
{
	NetAssert(m_socket.get() == 0);
	NetAssert(m_channels.empty());
	NetAssert(m_packetDispatcher == 0);
	NetAssert(m_handler == 0);
	NetAssert(m_listenChannel == 0);
}

bool ChannelManager::Initialize(PacketDispatcher * packetDispatcher, ChannelHandler * handler, SharedNetSocket socket, bool enableServer, uint32_t serverVersion)
{
	LOG_DBG("ChannelManager::Initialize: enableServer=%d", (int)enableServer);
	Assert(m_handler == 0);
	Assert(handler != 0);

	m_packetDispatcher = packetDispatcher;
	m_handler = handler;

	m_socket = socket;

	if (enableServer)
	{
		m_listenChannel = CreateListenChannel(ChannelPool_Server);

		LOG_DBG("ChannelManager::Initialize: listenChannel=%p", m_listenChannel);
	}

	m_serverVersion = serverVersion;

	LOG_DBG("ChannelManager::Initialize: done", 0);

	return true;
}

void ChannelManager::Shutdown(bool sendDisconnectNotification)
{
	LOG_DBG("ChannelManager::Shutdown: sendDisconnectNotification=%d, numClients=%d", (int)sendDisconnectNotification, (int)m_channels.size());

	while (m_channels.size())
	{
		Channel * channel = m_channels.begin()->second;

		if (channel->m_channelType == ChannelType_Listen && channel->m_state != ChannelState_Disconnected)
		{
			channel->m_state = ChannelState_Disconnected;
		}

		if (channel->m_channelType == ChannelType_Connection && channel->m_state != ChannelState_Disconnected)
		{
			channel->Disconnect(sendDisconnectNotification, false);
		}

		DestroyChannel(channel);
	}

	m_destroyedChannels.clear();

	m_listenChannel = 0;

	m_packetDispatcher = 0;
	m_handler = 0;

	SharedNetSocket nullSocket;
	m_socket = nullSocket;

	LOG_DBG("ChannelManager::Shutdown [done]", 0);
}

bool ChannelManager::IsInitialized()
{
	return m_handler != 0;
}

void ChannelManager::SetChannelTimeoutMS(uint32_t timeout)
{
	m_channelTimeout = timeout;
}

Channel * ChannelManager::CreateListenChannel(ChannelPool channelPool)
{
	return CreateChannelEx(ChannelType_Listen, channelPool);
}

Channel * ChannelManager::CreateChannel(ChannelPool channelPool)
{
	return CreateChannelEx(ChannelType_Connection, channelPool);
}

Channel * ChannelManager::CreateChannelEx(ChannelType channelType, ChannelPool channelPool)
{
	Channel * channel = new Channel(channelType, channelPool, (1 << PROTOCOL_CHANNEL));

	channel->Initialize(this, m_socket);
	channel->m_id = m_channelIds.Allocate();

	channel->SetConnected(true);

	m_channels[channel->m_id] = channel;

	return channel;
}

void ChannelManager::DestroyChannel(Channel * channel)
{
	NetAssert(channel->m_state == ChannelState_Disconnected);
	LOG_DBG("DestroyChannel: channel=%p", channel);

	if (m_handler && channel->m_channelType == ChannelType_Connection)
	{
		/**/ if (channel->m_channelPool == ChannelPool_Client)
			m_handler->CL_OnChannelDisconnect(channel);
		else if (channel->m_channelPool == ChannelPool_Server)
			m_handler->SV_OnChannelDisconnect(channel);
		else
			NetAssert(false);
	}

	m_channels.erase(channel->m_id);

	m_channelIds.Free(channel->m_id);

	delete channel;
}

void ChannelManager::DestroyChannelQueued(Channel * channel)
{
	if (!channel->m_queueForDestroy)
	{
		NetAssert(std::find(m_destroyedChannels.begin(), m_destroyedChannels.end(), channel) == m_destroyedChannels.end());
		LOG_DBG("DestroyChannelQueued: channel=%p", channel);
		m_destroyedChannels.push_back(channel);
		channel->m_queueForDestroy = true;
	}
}

void ChannelManager::Update(uint64_t time)
{
	for (ChannelMapItr i = m_channels.begin(); i != m_channels.end(); ++i)
		if (!i->second->m_queueForDestroy)
			i->second->Update(time);

	// Destroy disconnected client channels.
	for (size_t i = 0; i < m_destroyedChannels.size(); ++i)
		DestroyChannel(m_destroyedChannels[i]);

	m_destroyedChannels.clear();

#if LIBNET_ENABLE_NET_STATS
	uint32_t maxRTT = 0;
	for (ChannelMapItr i = m_channels.begin(); i != m_channels.end(); ++i)
		if (i->second->m_rtt > maxRTT)
			maxRTT = i->second->m_rtt;
	TIMER_ADD_MICROSECONDS(NetStat_ChannelRTTMax, maxRTT);
#endif
}

void ChannelManager::OnReceive(Packet & packet, Channel * channel)
{
	uint8_t messageId;

	if (packet.Read8(&messageId))
	{
		switch (messageId)
		{
		case CHANNELMSG_TRUNK:
			HandleTrunk(packet, channel);
			break;
		case CHANNELMSG_CONNECT:
			HandleConnect(packet, channel);
			break;
		case CHANNELMSG_CONNECT_OK:
			HandleConnectOK(packet, channel);
			break;
		case CHANNELMSG_CONNECT_ERROR:
			HandleConnectError(packet, channel);
			break;
		case CHANNELMSG_CONNECT_ACK:
			HandleConnectAck(packet, channel);
			break;
		case CHANNELMSG_DISCONNECT:
			HandleDisconnect(packet, channel);
			break;
		case CHANNELMSG_DISCONNECT_ACK:
			HandleDisconnectAck(packet, channel);
			break;
		case CHANNELMSG_PING:
			channel->HandlePing(packet);
			break;
		case CHANNELMSG_PONG:
			channel->HandlePong(packet);
			break;
		case CHANNELMSG_RT_UPDATE:
			channel->HandleRTUpdate(packet);
			break;
		case CHANNELMSG_RT_ACK:
			channel->HandleRTAck(packet);
			break;
		case CHANNELMSG_RT_NACK:
			channel->HandleRTNack(packet);
			break;
		case CHANNELMSG_UNPACK:
			HandleUnpack(packet, channel);
			break;
		default:
			LOG_ERR("chanmgr: message: unknown message type: %u", static_cast<uint32_t>(messageId));
			NetAssert(false);
			break;
		}
	}
	else
	{
		LOG_ERR("chanmgr: message: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleTrunk(Packet & packet, Channel * channel)
{
	uint16_t channelId;

	if (packet.Read16(&channelId))
	{
		if (LIBNET_CHANNELMGR_LOG_TRUNK)
			LOG_DBG("chanmgr: trunk: redirecting to channel %u", static_cast<uint32_t>(channelId));

		Channel * channel2 = FindChannel(channelId);

		if (channel2 != 0)
		{
			Packet packet2;

			if (packet.ExtractTillEnd(packet2))
			{
				m_packetDispatcher->Dispatch(packet2, channel2);
			}
			else
			{
				LOG_ERR("failed to extract packet", 0);
				NetAssert(false);
			}
		}
		else
		{
			LOG_WRN("chanmgr: trunk: trunk to unknown channel [trunk %09u => %09u]",
				static_cast<uint32_t>(channel->m_id),
				static_cast<uint32_t>(channelId));
			NetAssert(false);
		}
	}
	else
	{
		LOG_ERR("chanmgr: trunk: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleConnect(Packet & packet, Channel * channel)
{
	uint32_t serverVersion;
	uint16_t channelId;

	if (packet.Read16(&channelId))
	{
		LOG_INF("chanmgr: connect: received connection attempt from client channel [%09u]",
			static_cast<uint32_t>(channelId));
#if defined(WINDOWS)
		LOG_INF("chanmgr: connect: peer address: %u.%u.%u.%u:%u",
			packet.m_rcvAddress.GetSockAddr()->sin_addr.S_un.S_un_b.s_b1,
			packet.m_rcvAddress.GetSockAddr()->sin_addr.S_un.S_un_b.s_b2,
			packet.m_rcvAddress.GetSockAddr()->sin_addr.S_un.S_un_b.s_b3,
			packet.m_rcvAddress.GetSockAddr()->sin_addr.S_un.S_un_b.s_b4,
			packet.m_rcvAddress.GetSockAddr()->sin_port);
#endif

		if (!packet.Read32(&serverVersion))
		{
			LOG_ERR("chanmgr: connect: failed to read from packet", 0);
			NetAssert(false);
		}
		else if (serverVersion != m_serverVersion)
		{
			LOG_INF("chanmgr: connect: incompatible server version [%09u]. got %u9u, expected %09u",
				static_cast<uint32_t>(channelId),
				static_cast<uint32_t>(serverVersion),
				static_cast<uint32_t>(m_serverVersion));

			PacketBuilder<6> replyBuilder;

			const uint8_t protocolId = PROTOCOL_CHANNEL;
			const uint8_t messageId = CHANNELMSG_CONNECT_ERROR;
			const uint16_t destinationId = channelId;

			replyBuilder.Write8(&protocolId);
			replyBuilder.Write8(&messageId);
			replyBuilder.Write16(&destinationId);

			Packet reply = replyBuilder.ToPacket();

			m_socket->Send(reply.GetData(), reply.GetSize(), &packet.m_rcvAddress);

			LOG_INF("chanmgr: connect: sent ConnectError to client channel %09u",
				static_cast<uint32_t>(channelId));
		}
		else
		{
			Channel * newChannel = CreateChannelEx(ChannelType_Connection, ChannelPool_Server);
			newChannel->m_destinationId = channelId;
			newChannel->m_address = packet.m_rcvAddress;
			newChannel->m_state = ChannelState_Connecting;

			LOG_INF("chanmgr: connect: created new server channel [%09u]",
				static_cast<uint32_t>(newChannel->m_id));

			PacketBuilder<6> replyBuilder;

			const uint8_t protocolId = PROTOCOL_CHANNEL;
			const uint8_t messageId = CHANNELMSG_CONNECT_OK;
			const uint16_t destinationId = channelId;
			const uint16_t newDestinationId = newChannel->m_id;

			replyBuilder.Write8(&protocolId);
			replyBuilder.Write8(&messageId);
			replyBuilder.Write16(&destinationId);
			replyBuilder.Write16(&newDestinationId);

			Packet reply = replyBuilder.ToPacket();

			newChannel->SendUnreliable(reply, false);

			LOG_INF("chanmgr: connect: sent ConnectOK to client channel %09u with request to change destination ID to %09u",
				static_cast<uint32_t>(newChannel->m_destinationId),
				static_cast<uint32_t>(newDestinationId));
		}
	}
	else
	{
		LOG_ERR("chanmgr: connect: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleConnectOK(Packet & packet, Channel * channel)
{
	uint16_t channelId;
	uint16_t newDestinationId;

	if (packet.Read16(&channelId) &&
		packet.Read16(&newDestinationId))
	{
		LOG_INF("chanmgr: connect-ok: received OK from server channel %09u for client channel %09u",
			static_cast<uint32_t>(newDestinationId),
			static_cast<uint32_t>(channelId));

		Channel * channel2 = FindChannel(channelId);

		if (channel2/* && channel2 == channel*/)
		{
			channel2->m_destinationId = newDestinationId;
			channel2->m_state = ChannelState_Connected;

			LOG_INF("chanmgr: connect-ok: updated destination server channel ID to %09u",
				static_cast<uint32_t>(channel2->m_destinationId));

			PacketBuilder<4> replyBuilder;

			const uint8_t protocolId = PROTOCOL_CHANNEL;
			const uint8_t messageId = CHANNELMSG_CONNECT_ACK;
			const uint16_t destinationId = channel2->m_destinationId;

			replyBuilder.Write8(&protocolId);
			replyBuilder.Write8(&messageId);
			replyBuilder.Write16(&destinationId);

			Packet reply = replyBuilder.ToPacket();

			channel2->SendUnreliable(reply, false);

			LOG_INF("chanmgr: connect-ok: sent ACK to server channel %u", channel2->m_destinationId);

			channel2->m_protocolMask = 0xffffffff;

			if (m_handler)
				m_handler->CL_OnChannelConnect(channel2);
		}
		else
		{
			if (channel2 == 0)
			{
				LOG_ERR("chanmgr: connect-ok: unknown channel %u", channelId);
				NetAssert(false);
			}
			else
			{
				LOG_ERR("chanmgr: connect-ok: channel mismatch %u", channelId);
				NetAssert(false);
			}
		}
	}
	else
	{
		LOG_ERR("chanmgr: connect-ok: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleConnectError(Packet & packet, Channel * channel)
{
	uint16_t channelId;

	if (packet.Read16(&channelId))
	{
		LOG_ERR("chanmgr: connect-error: received error from channel %09u",
			static_cast<uint32_t>(channel->m_id));

		Channel * channel2 = FindChannel(channelId);

		if (channel2/* && channel2 == channel*/)
		{
			channel2->Disconnect(false, false);
		}
		else
		{
			if (channel2 == 0)
			{
				LOG_ERR("chanmgr: connect-error: unknown channel %u", channelId);
				NetAssert(false);
			}
			else
			{
				LOG_ERR("chanmgr: connect-error: channel mismatch %u", channelId);
				NetAssert(false);
			}
		}
	}
	else
	{
		LOG_ERR("chanmgr: connect-error: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleConnectAck(Packet & packet, Channel * channel)
{
	// TODO: When ACK, send client channelID for validation purposes.
	uint16_t channelId;

	if (packet.Read16(&channelId))
	{
		if (channel->m_id == channelId)
		{
			LOG_INF("chanmgr: connect-ack: received ACK for server channel %09u from client channel %09u",
				static_cast<uint32_t>(channelId),
				static_cast<uint32_t>(channel->m_destinationId));

			channel->m_state = ChannelState_Connected;

			channel->m_protocolMask = 0xffffffff;

			if (m_handler)
				m_handler->SV_OnChannelConnect(channel);
		}
		else
		{
			LOG_ERR("chanmgr: connect-ack: channel mismatch %09u, expected %09u",
				static_cast<uint32_t>(channelId),
				static_cast<uint32_t>(channel->m_id));
			NetAssert(false);
		}
	}
	else
	{
		LOG_ERR("chanmgr: connect-ack: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleDisconnect(Packet & packet, Channel * channel)
{
	uint16_t channelId;
	uint8_t expectAck;

	if (packet.Read16(&channelId) && packet.Read8(&expectAck))
	{
		if (channel->m_destinationId == channelId)
		{
			LOG_INF("chanmgr: disconnect: received disconnect for server channel %u from client channel %u, expecting ack? %s",
				channel->m_id,
				channel->m_destinationId,
				expectAck ? "yes" : "no");

			if (expectAck)
			{
				PacketBuilder<4> replyBuilder;

				const uint8_t protocolId = PROTOCOL_CHANNEL;
				const uint8_t messageId = CHANNELMSG_DISCONNECT_ACK;
				const uint16_t destinationId = channel->m_destinationId;

				replyBuilder.Write8(&protocolId);
				replyBuilder.Write8(&messageId);
				replyBuilder.Write16(&destinationId);

				Packet reply = replyBuilder.ToPacket();

				channel->SendUnreliable(reply, false);

				LOG_INF("chanmgr: disconnect-ok: sent ACK to client channel %u", channel->m_destinationId);
			}

			channel->Disconnect(false, false);
		}
		else
		{
			LOG_ERR("chanmgr: disconnect: channel mismatch %09u, expected %09u",
				static_cast<uint32_t>(channelId),
				static_cast<uint32_t>(channel->m_destinationId));
			NetAssert(false);
		}
	}
	else
	{
		LOG_ERR("chanmgr: disconnect: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleDisconnectAck(Packet & packet, Channel * channel)
{
	uint16_t channelId;

	if (packet.Read16(&channelId))
	{
		if (channel->m_id == channelId)
		{
			LOG_INF("chanmgr: disconnect-ack: received ACK for server channel %09u from client channel %09u",
				static_cast<uint32_t>(channelId),
				static_cast<uint32_t>(channel->m_destinationId));

			DestroyChannelQueued(channel);
		}
		else
		{
			LOG_ERR("chanmgr: disconnect-ack: channel mismatch %09u, expected %09u",
				static_cast<uint32_t>(channelId),
				static_cast<uint32_t>(channel->m_id));
			NetAssert(false);
		}
	}
	else
	{
		LOG_ERR("chanmgr: disconnect-ack: failed to read from packet", 0);
		NetAssert(false);
	}
}

void ChannelManager::HandleUnpack(Packet & packet, Channel * channel)
{
	uint16_t size;

	if (packet.Read16(&size))
	{
		while (size > 0)
		{
			uint16_t size2;

			if (packet.Read16(&size2))
			{
				if (size >= 2)
					size -= 2;
				else
				{
					LOG_ERR("chanmgr: unpack: invalid packet size", 0);
					NetAssert(false);
					break;
				}

				Packet extracted;

				if (packet.Extract(extracted, size2, true))
				{
					if (size >= size2)
						size -= size2;
					else
					{
						LOG_ERR("chanmgr: unpack: invalid packet size", 0);
						NetAssert(false);
						break;
					}

					m_packetDispatcher->Dispatch(extracted, channel);
				}
				else
				{
					LOG_ERR("chanmgr: unpack: failed to read from packet", 0);
					NetAssert(false);
					break;
				}
			}
			else
			{
				LOG_ERR("chanmgr: unpack: failed to read from packet", 0);
				NetAssert(false);
				break;
			}
		}
	}
	else
	{
		LOG_ERR("chanmgr: unpack: failed to read from packet", 0);
		NetAssert(false);
	}
}

Channel * ChannelManager::FindChannel(uint32_t id)
{
	ChannelMapItr i = m_channels.find(id);

	if (i == m_channels.end())
	{
		return 0;
	}
	else
	{
		return i->second;
	}
}
