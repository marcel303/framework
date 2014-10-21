#include "BitStream.h"
#include "ChannelManager.h"
#include "Hash.h"
#include "NetDiag.h"
#include "NetProtocols.h"
#include "Packet.h"
#include "RpcManager.h"

RpcManager::RpcManager(ChannelManager * channelMgr)
	: m_channelMgr(channelMgr)
{
}

RpcManager::~RpcManager()
{
	NetAssert(m_registrations.empty());
}

uint32_t RpcManager::Register(const char * name, RpcHandler handler)
{
	const uint32_t method = HashFunc::Hash_FNV1a(name, strlen(name));
	NetAssert(method != 0);

	RegistrationMap::const_iterator i = m_registrations.find(method);
	NetAssert(i == m_registrations.end());

	if (i == m_registrations.end())
	{
		m_registrations[method] = handler;

		return method;
	}

	return 0;
}

void RpcManager::Unregister(uint32_t method, RpcHandler handler)
{
	RegistrationMap::const_iterator i = m_registrations.find(method);
	NetAssert(i != m_registrations.end());

	if (i != m_registrations.end())
	{
		m_registrations.erase(i);
	}
}

void RpcManager::Call(uint32_t method, const BitStream & bs, ChannelSide channelSide, uint32_t * channelId, bool broadcast, bool invokeLocal)
{
	NetAssert(channelId == nullptr); // not yet implemented!

	const uint8_t protocolId = PROTOCOL_RPC;
	const uint16_t payloadSize = bs.GetDataSize();
	
	PacketBuilder<1024> b;
	b.Write8(&protocolId);
	b.Write32(&method);
	b.Write16(&payloadSize);
	b.Write(bs.GetData(), (bs.GetDataSize() + 7) >> 3);

	Packet p = b.ToPacket();

	if (broadcast)
	{
		for (ChannelManager::ChannelMap::iterator i = m_channelMgr->m_channels.begin(); i != m_channelMgr->m_channels.end(); ++i)
		{
			Channel * channel = i->second;

			if (channel->m_channelSide == channelSide && channel->m_channelType == ChannelType_Client)
			{
				channel->SendReliable(p);
			}
		}
	}

	if (invokeLocal)
	{
		BitStream bs2(bs.GetData(), bs.GetDataSize());

		CallInternal(method, bs2);
	}
}

void RpcManager::OnReceive(Packet & packet, Channel * channel)
{
	uint32_t method;
	uint16_t payloadSize;
	
	if (packet.Read32(&method) && packet.Read16(&payloadSize))
	{
		Packet payload;

		if (packet.Extract(payload, (payloadSize + 7) >> 3))
		{
			BitStream bs(packet.GetData(), payloadSize);

			CallInternal(method, bs);
		}
	}
}

void RpcManager::CallInternal(uint32_t method, BitStream & bs)
{
	RegistrationMap::const_iterator i = m_registrations.find(method);
	NetAssert(i != m_registrations.end());

	if (i != m_registrations.end())
	{
		RpcHandler handler = i->second;

		handler(bs);
	}
}
