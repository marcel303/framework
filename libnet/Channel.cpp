#include <algorithm>
#include "Channel.h"
#include "ChannelManager.h"
#include "NetDiag.h"
#include "NetProtocols.h"
#include "NetStats.h"
#include "NetTimer.h"
#include "PacketDispatcher.h"

#define LOG_CHANNEL_DBG(fmt, ...) LOG_DBG("channel [%09u]: " # fmt, static_cast<uint32_t>(m_id), __VA_ARGS__)

Channel::Channel(ChannelType channelType, ChannelSide channelSide, uint32_t protocolMask)
	: m_channelMgr(0)
	, m_socket()
	, m_address()
	, m_id(0)
	, m_destinationId(0)
	, m_sendQueue()
	, m_pingTimer()
#if LIBNET_CHANNEL_ENABLE_TIMEOUTS == 1
	, m_timeoutTimer()
#endif
	, m_rtt(0)
	, m_delayTimer()
	, m_delayedReceivePackets()
	, m_channelType(channelType)
	, m_channelSide(channelSide)
	, m_protocolMask(protocolMask)
	, m_queueForDestroy(false)
	, m_rtQueue()
	, m_rtSndId(0)
	, m_rtRcvId(0)
	, m_rtAckId(0)
	, m_txBegun(false)
	, m_txSize(0)
{
	InitSendQueue();

	m_pingTimer.Initialize(&g_netTimer);
	m_pingTimer.SetIntervalMS(LIBNET_CHANNEL_PING_INTERVAL);
	m_pingTimer.Start();

#if LIBNET_CHANNEL_ENABLE_TIMEOUTS == 1
	m_timeoutTimer.Initialize(&g_netTimer);
	m_timeoutTimer.SetIntervalMS(LIBNET_CHANNEL_TIMEOUT_INTERVAL);
#endif
}

Channel::~Channel()
{
}

void Channel::Initialize(ChannelManager * channelMgr, SharedNetSocket socket)
{
	m_channelMgr = channelMgr;
	m_socket = socket;
}

bool Channel::Connect(const NetAddress & address)
{
#if LIBNET_CHANNEL_ENABLE_TIMEOUTS == 1
	m_timeoutTimer.Start();
#endif

	m_address = address;

	PacketBuilder<4> packetBuilder;

	const uint8_t protocolId = PROTOCOL_CHANNEL;
	const uint8_t messageId = CHANNELMSG_CONNECT;
	const uint16_t channelId = m_id;

	packetBuilder.Write8(&protocolId);
	packetBuilder.Write8(&messageId);
	packetBuilder.Write16(&channelId);

	const Packet packet = packetBuilder.ToPacket();

	NetStats::Inc(NetStat_PacketsSent);
	NetStats::Inc(NetStat_BytesSent, packet.GetSize());

	return m_socket->Send(packet.GetData(), packet.GetSize(), &m_address);
}

void Channel::Disconnect()
{
	Flush();

	PacketBuilder<4> packetBuilder;

	const uint8_t protocolId = PROTOCOL_CHANNEL;
	const uint8_t messageId = CHANNELMSG_DISCONNECT;
	const uint16_t channelId = m_id;

	packetBuilder.Write8(&protocolId);
	packetBuilder.Write8(&messageId);
	packetBuilder.Write16(&channelId);

	Send(packetBuilder.ToPacket(), true);

	//m_address = NetAddress(0, 0, 0, 0, 0);

#if LIBNET_CHANNEL_ENABLE_TIMEOUTS == 1
	m_timeoutTimer.Stop();
#endif
}

void Channel::Update(uint32_t time)
{
	// Read messages.
	{
		static ReceiveData receiveData;

		while (Receive(receiveData))
		{
#if LIBNET_CHANNEL_SIMULATED_PACKETLOSS != 0
			uint32_t loss = rand() % 1000;
			if (loss < LIBNET_CHANNEL_SIMULATED_PACKETLOSS)
				continue;
#endif

			const void * data          = receiveData.m_data;
			const uint32_t size        = receiveData.m_size;
			const NetAddress & address = receiveData.m_address;

			Packet packet(data, size, address);

			PacketDispatcher::Dispatch(packet, this);
		}
	}

	if (m_channelType == ChannelType_Client)
	{
		// Handle reliable communications.
		for (size_t i = 0; i < m_rtQueue.size(); ++i)
		{
			RTPacket & packet = m_rtQueue[i];

			if (time >= packet.m_nextSend)
			{
				packet.m_lastSend = time;
				packet.m_nextSend = time + 500;

				PacketBuilder<6> headerBuilder;

				const uint8_t protocolId = PROTOCOL_CHANNEL;
				const uint8_t messageId = CHANNELMSG_RT_UPDATE;
				const uint32_t packetId = packet.m_id;

				headerBuilder.Write8(&protocolId);
				headerBuilder.Write8(&messageId);
				headerBuilder.Write32(&packetId);

				const Packet header = headerBuilder.ToPacket();

				const uint32_t headerSize = header.GetSize();
				const uint32_t packetSize = packet.m_dataSize;

				SendBegin(headerSize + packetSize);
				Send(header);
				Send(Packet(packet.m_data, packet.m_dataSize));
				SendEnd();

#if LIBNET_CHANNEL_LOG_RT == 1
				LOG_CHANNEL_DBG("RT UPD sent: %u",
					static_cast<uint32_t>(packetId));
#endif
			}
		}

		// Send ping at regular interval.
		while (m_pingTimer.ReadTick())
		{
			PacketBuilder<6> packetBuilder;

			const uint8_t protocolId = PROTOCOL_CHANNEL;
			const uint8_t messageId = CHANNELMSG_PING;
			const uint32_t time = static_cast<uint32_t>(m_pingTimer.TimeUS_get());

			packetBuilder.Write8(&protocolId);
			packetBuilder.Write8(&messageId);
			packetBuilder.Write32(&time);

			const Packet packet = packetBuilder.ToPacket();

			Send(packet, false);

#if LIBNET_CHANNEL_LOG_PINGPONG == 1
			LOG_CHANNEL_DBG("sent ping message", 0);
#endif
		}

#if LIBNET_CHANNEL_ENABLE_TIMEOUTS == 1
		// Check for connection time out.
		if (m_timeoutTimer.ReadTick())
		{
			Disconnect();

			m_channelMgr->DestroyChannelQueued(this);

			LOG_CHANNEL_DBG("timeout", 0);
		}
#endif
	}

	Flush();
}

void Channel::Flush()
{
	if (m_sendQueue.GetSize() > 4)
	{
		// write header
		const uint8_t protocolId = PROTOCOL_CHANNEL;
		const uint8_t messageId = CHANNELMSG_UNPACK;
		const uint16_t size = static_cast<uint16_t>(m_sendQueue.GetSize()) - 4;

		m_sendQueue.Seek(0);
		m_sendQueue.Write8(&protocolId);
		m_sendQueue.Write8(&messageId);
		m_sendQueue.Write16(&size);

		Send(m_sendQueue.ToPacket(), true);

		InitSendQueue();
	}
}

bool Channel::Send(const Packet & packet, bool priority)
{
	NetAssert(m_address.IsValid());

#if LIBNET_CHANNEL_ENABLE_PACKING == 0
	priority = true; // Disable packing
#endif

	if (priority)
	{
		PacketBuilder<4> headerBuilder;

		const uint8_t protocolId = PROTOCOL_CHANNEL;
		const uint8_t messageId = CHANNELMSG_TRUNK;
		const uint16_t destinationId = m_destinationId;

		headerBuilder.Write8(&protocolId);
		headerBuilder.Write8(&messageId);
		headerBuilder.Write16(&destinationId);

		const uint32_t headerSize = headerBuilder.GetSize();
		const uint32_t packetSize = packet.GetSize();

		const uint32_t bufferSize = headerSize + packetSize;
		uint8_t * buffer          = reinterpret_cast<uint8_t *>(alloca(bufferSize));

		headerBuilder.CopyTo(buffer, headerSize);
		packet.CopyTo(buffer + headerSize, packetSize);

		NetStats::Inc(NetStat_PacketsSent);
		NetStats::Inc(NetStat_BytesSent, bufferSize);

		return m_socket->Send(buffer, bufferSize, &m_address);
	}
	else
	{
		const uint32_t packetSize = packet.GetSize();

		bool autoTx = (m_txBegun == false);

		if (autoTx)
			SendBegin(packetSize);

		// write packet
		NetAssert(packetSize <= m_txSize);
		m_txSize -= packetSize;
		m_sendQueue.WritePacket(packet);

		if (autoTx)
			SendEnd();

		return true;
	}
}

bool Channel::SendBegin(uint32_t size)
{
	const uint32_t OVERHEAD = 2;
	const uint32_t newSize1 = OVERHEAD + size;
	const uint32_t newSize2 = m_sendQueue.GetSize() + newSize1;

	if (newSize1 > kMaxDatagramSize)
	{
		NetAssert(false);
		return false;
	}

	if (newSize2 > kMaxDatagramSize)
		Flush();

	m_txBegun = true;
	m_txSize = newSize1;

	// write header
	const uint32_t headerSize = 2;
	NetAssert(headerSize <= m_txSize);
	m_txSize -= headerSize;
	const uint16_t packetSize = static_cast<uint16_t>(size);
	m_sendQueue.Write16(&packetSize);

	return true;
}

void Channel::SendEnd()
{
	NetAssert(m_txSize == 0);

	m_txBegun = false;
}

void Channel::SendReliable(const Packet & packet)
{
	RTPacket temp;

	temp.m_acknowledged = false;
	temp.m_id = m_rtSndId;
	temp.m_lastSend = 0;
	temp.m_nextSend = 0;
	temp.m_dataSize = packet.GetSize();
	packet.CopyTo(temp.m_data, kMaxDatagramSize);

	m_rtSndId++;

	m_rtQueue.push_back(temp);

#if LIBNET_CHANNEL_LOG_RT == 1
	LOG_CHANNEL_DBG("RT ENQ: %u",
		static_cast<uint32_t>(temp.m_id));
#endif
}

void Channel::SendSelf(const Packet & packet, uint32_t delay, NetAddress * address)
{
	if (address == 0)
		address = &m_address;

	DelayedPacket temp;

	temp.m_time = static_cast<uint32_t>(m_delayTimer.TimeMS_get());
	temp.m_delay = delay;
	temp.Set(packet.GetData(), packet.GetSize(), *address);

	m_delayedReceivePackets.push_front(temp);
	
	std::sort(m_delayedReceivePackets.begin(), m_delayedReceivePackets.end());
}

bool Channel::Receive(ReceiveData & rcvData)
{
	if (m_delayedReceivePackets.size() > 0)
	{
		// NOTE: Delay reduced packets by a few MS to ensure packets sent to self receive before updates from server.
		//const uint32_t time1 = static_cast<uint32_t>(m_delayTimer.TimeMS_get());
		const uint32_t time1 = static_cast<uint32_t>(m_delayTimer.TimeMS_get()) + 5;
		const uint32_t time2 = m_delayedReceivePackets[0].m_time + m_delayedReceivePackets[0].m_delay;

		if (time1 >= time2)
		{
			const DelayedPacket & delayedPacket = m_delayedReceivePackets[0];
			memcpy(rcvData.m_data, delayedPacket.m_data, delayedPacket.m_dataSize);
			rcvData.Set(delayedPacket.m_dataSize, delayedPacket.m_address);

			NetStats::Inc(NetStat_PacketsReceived);
			NetStats::Inc(NetStat_BytesReceived, rcvData.m_size);

			m_delayedReceivePackets.pop_front();

			return true;
		}
	}

	if (m_socket->Receive(rcvData.m_data, kMaxDatagramSize, &rcvData.m_size, &rcvData.m_address))
	{
		if (LIBNET_CHANNEL_SIMULATED_PING != 0)
		{
			Packet packet(rcvData.m_data, rcvData.m_size, rcvData.m_address);
			SendSelf(packet, LIBNET_CHANNEL_SIMULATED_PING, &rcvData.m_address);
		}
		else
		{
			NetStats::Inc(NetStat_PacketsReceived);
			NetStats::Inc(NetStat_BytesReceived, rcvData.m_size);
			return true;
		}
	}

	return false;
}

void Channel::HandlePing(Packet & packet)
{
#if LIBNET_CHANNEL_LOG_PINGPONG == 1
	LOG_CHANNEL_DBG("received ping message", 0);
#endif

	uint32_t time;

	if (!packet.Read32(&time))
	{
		NetAssert(false);
		return;
	}

	PacketBuilder<6> reply;

	const uint8_t protocolId = PROTOCOL_CHANNEL;
	const uint8_t messageId = CHANNELMSG_PONG;

	reply.Write8(&protocolId);
	reply.Write8(&messageId);
	reply.Write32(&time);

	Send(reply.ToPacket());

#if LIBNET_CHANNEL_LOG_PINGPONG == 1
	LOG_CHANNEL_DBG("sent pong message", 0);
#endif
}

void Channel::HandlePong(Packet & packet)
{
	uint32_t time;

	if (!packet.Read32(&time))
	{
		NetAssert(false);
		return;
	}

	const uint32_t time2 = static_cast<uint32_t>(m_pingTimer.TimeUS_get());

	m_rtt = time2 - time;

#if LIBNET_CHANNEL_LOG_PINGPONG == 1
	LOG_CHANNEL_DBG("received pong message. rtt = %gms", m_rtt / 1000.0f);
#endif

#if LIBNET_CHANNEL_ENABLE_TIMEOUTS == 1
	m_timeoutTimer.Restart();
#endif
}

void Channel::HandleRTUpdate(Packet & packet)
{
	uint32_t packetId;

	if (!packet.Read32(&packetId))
	{
		NetAssert(false);
		return;
	}

#if LIBNET_CHANNEL_LOG_RT == 1
	LOG_CHANNEL_DBG("RT UPD rcvd: %u (time=%llu)",
		static_cast<uint32_t>(packetId),
		static_cast<uint64_t>(g_netTimer.TimeMS_get()));
#endif

	if (packetId == m_rtRcvId)
	{
		PacketBuilder<6> reply;

		const uint8_t protocolId = PROTOCOL_CHANNEL;
		const uint8_t messageId = CHANNELMSG_RT_ACK;
		
		reply.Write8(&protocolId);
		reply.Write8(&messageId);
		reply.Write32(&packetId);

		Send(reply.ToPacket());

#if LIBNET_CHANNEL_LOG_RT == 1
		LOG_CHANNEL_DBG("RT ACK sent: %u",
			static_cast<uint32_t>(packetId));
#endif

		m_rtRcvId++;

		Packet packet2;
		
		if (packet.ExtractTillEnd(packet2))
		{
			PacketDispatcher::Dispatch(packet2, this);

#if LIBNET_CHANNEL_LOG_RT == 1
			LOG_CHANNEL_DBG("RT UPD rcvd: %u handled",
				static_cast<uint32_t>(packetId));
#endif
		}
		else
		{
			LOG_ERR("failed to extract packet", 0);
			NetAssert(false);
		}
	}
	else
	{
#if LIBNET_CHANNEL_LOG_RT == 1
		if (packetId > m_rtRcvId)
			LOG_CHANNEL_DBG("RT UPD rcvd: ID > receive ID", 0);
		else
			LOG_CHANNEL_DBG("RT UPD rcvd: ID < receive ID", 0);
#endif
	}
}

void Channel::HandleRTAck(Packet & packet)
{
	uint32_t packetId;

	if (!packet.Read32(&packetId))
	{
		NetAssert(false);
		return;
	}

#if LIBNET_CHANNEL_LOG_RT == 1
	LOG_CHANNEL_DBG("RT ACK rcvd: %u", 
		static_cast<uint32_t>(packetId));
#endif

	if (packetId >= m_rtAckId)
	{
#if LIBNET_CHANNEL_LOG_RT == 1
		if (packetId > m_rtSndId)
		{
			LOG_CHANNEL_DBG("received ack for unsent message", 0);
		}
#endif

		// Remove queued packets.
		while (m_rtQueue.size() > 0 && m_rtQueue[0].m_id <= packetId)
		{
#if LIBNET_CHANNEL_LOG_RT == 1
			LOG_CHANNEL_DBG("RT ACK rcvd: %u handled",
				static_cast<uint32_t>(m_rtQueue[0].m_id));
#endif

			m_rtQueue.pop_front();
		}

		m_rtAckId = packetId + 1;
	}
	else
	{
#if LIBNET_CHANNEL_LOG_RT == 1
		LOG_CHANNEL_DBG("RT ACK rcvd: ID < ack ID", 0);
#endif
	}
}

void Channel::InitSendQueue()
{
	m_sendQueue.Clear();

	// write dummy header. will be filled in when the queue is eventually flushed
	uint32_t sendQueueHdr = 0;
	m_sendQueue.Write32(&sendQueueHdr);
}
