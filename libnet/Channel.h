#pragma once

#include <deque>
#include <list>
#include "ChannelTypes.h"
#include "libnet_config.h"
#include "NetSocket.h"
#include "Packet.h"
#include "PolledTimer.h"

class ChannelManager;

//const static uint32_t kMaxDatagramSize = 32 * 1024;
const static uint32_t kMaxDatagramSize = 1400;

class ReceiveData
{
public:
	ReceiveData()
		: m_size(0)
	{
	}

	void Set(uint32_t size, const NetAddress & address)
	{
		m_size = size;
		m_address = address;
	}

	uint32_t m_size;
	NetAddress m_address;
	uint8_t m_data[kMaxDatagramSize];
};

class Channel
{
public:
	Channel(ChannelType channelType, ChannelPool channelPool, uint32_t protocolMask);
	~Channel();

	void Initialize(ChannelManager * channelMgr, SharedNetSocket socket);
	void SetConnected(bool connected);

	bool Connect(const NetAddress & address);
	void Disconnect(bool sendDisconnectNotification = true, bool waitForAck = false);
	void Update(uint64_t time);
	void Flush();

	bool Send(const Packet & packet, int channelSendFlags);
	bool SendBegin(uint32_t size);
	void SendEnd();
	bool Receive(ReceiveData & rcvData);

	bool SendUnreliable(const Packet & packet, bool sendImmediately);
	bool SendReliable(const Packet & packet);
	bool SendSelf(const Packet & packet, uint32_t delay, NetAddress * address = 0);

	void HandlePing(Packet & packet);
	void HandlePong(Packet & packet);
	void HandleRTUpdate(Packet & packet);
	void HandleRTAck(Packet & packet);
	void HandleRTNack(Packet & packet);

	void InitSendQueue();
	void OnReceive();

//private: //FIXME
	class DelayedPacket
	{
	public:
		uint32_t m_time;
		uint32_t m_delay;
		NetAddress m_address;
		uint32_t m_dataSize;
		uint8_t m_data[kMaxDatagramSize];

		inline bool Set(const void * data, uint32_t size, const NetAddress & address)
		{
			NetAssert(size <= kMaxDatagramSize);
			if (size > kMaxDatagramSize)
				return false;
			m_address = address;
			m_dataSize = size;
			memcpy(m_data, data, size);
			return true;
		}

		inline bool operator<(const DelayedPacket & packet) const
		{
			const uint32_t time1 = m_time + m_delay;
			const uint32_t time2 = packet.m_time + packet.m_delay;

			return time1 < time2;
		}
	};

	typedef std::list<DelayedPacket> DelayedPacketList;
	typedef DelayedPacketList::iterator DelayedPacketListItr;

	ChannelManager * m_channelMgr;
	SharedNetSocket m_socket;
	NetAddress m_address;
	uint16_t m_id;
	uint16_t m_destinationId;
	PacketBuilder<kMaxDatagramSize> m_sendQueue;
	PolledTimer m_pingTimer;
#if LIBNET_CHANNEL_ENABLE_TIMEOUTS == 1
	PolledTimer m_timeoutTimer;
#endif
	uint32_t m_rtt; // round trip time, in microseconds
	PolledTimer m_delayTimer;
	DelayedPacketList m_delayedReceivePackets;
	ChannelType m_channelType;
	ChannelPool m_channelPool;
	uint32_t m_protocolMask;
	bool m_queueForDestroy;

	// Reliable transport stuff.
	class RTPacket
	{
	public:
		uint32_t m_id;
		uint64_t m_lastSend;
		uint64_t m_nextSend; // Derived.
		bool m_acknowledged;
		uint32_t m_dataSize;
		uint8_t m_data[kMaxDatagramSize]; // fixme.. use allocator
	};
	std::deque<RTPacket> m_rtQueue;
	uint32_t m_rtSndId; // ID of next send packet.
	uint32_t m_rtRcvId; // ID of next receive packet.
	uint32_t m_rtAckId; // ID of last acknowledged packet.

	bool m_txBegun;
	uint32_t m_txSize;
};
