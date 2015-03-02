#include "BitStream.h"
#include "ChannelManager.h"
#include "Hash.h"
#include "NetDiag.h"
#include "NetProtocols.h"
#include "Packet.h"
#include "RpcManager.h"

RpcManager::RpcManager(ChannelManager * channelMgr)
	: m_channelMgr(channelMgr)
	, m_handler(0)
{
}

RpcManager::~RpcManager()
{
	NetAssert(m_registrations.empty());
	NetAssert(m_handler == 0);
}

void RpcManager::SetDefaultHandler(RpcHandler handler)
{
	m_handler = handler;
}

bool RpcManager::RegisterWithID(uint32_t method, RpcHandler handler)
{
	NetAssert(method != -1);

	RegistrationMap::const_iterator i = m_registrations.find(method);
	NetAssert(i == m_registrations.end());

	if (i == m_registrations.end())
	{
		m_registrations[method] = handler;

		return true;
	}

	return false;
}

uint32_t RpcManager::Register(const char * name, RpcHandler handler)
{
	const uint32_t method = HashFunc::Hash_FNV1a(name, strlen(name));
	NetAssert(method != -1 && method >= 128);

	if (RegisterWithID(method, handler))
		return method;
	else
		return -1;
}

void RpcManager::Unregister(uint32_t method, RpcHandler handler)
{
	RegistrationMap::const_iterator i = m_registrations.find(method);
	NetAssert(i != m_registrations.end() && i->second == handler);

	if (i != m_registrations.end())
	{
		m_registrations.erase(i);
	}
}

void RpcManager::Call(uint32_t method, const BitStream & bs, ChannelPool channelPool, uint16_t * channelId, bool broadcast, bool invokeLocal)
{
	const uint8_t protocolId = PROTOCOL_RPC;
	const uint16_t payloadSize = bs.GetDataSize();
	
	BitStream bs2;
	bs2.WriteBits(PROTOCOL_RPC, 8);

	const bool shortMethod = (method < 128);
	bs2.WriteBit(shortMethod);
	if (shortMethod)
		bs2.WriteBits(method, 7);
	else
		bs2.Write(method);

	bs2.WriteAlignedBytes(bs.GetData(), Net::BitsToBytes(bs.GetDataSize()));

	Packet p(bs2.GetData(), Net::BitsToBytes(bs2.GetDataSize()));

	if (broadcast)
	{
		Assert(!channelId);

		for (ChannelManager::ChannelMap::iterator i = m_channelMgr->m_channels.begin(); i != m_channelMgr->m_channels.end(); ++i)
		{
			Channel * channel = i->second;

			if (channel->m_isConnected && channel->m_channelPool == channelPool && channel->m_channelType == ChannelType_Connection)
			{
				channel->Send(p, 0);
			}
		}
	}
	else
	{
		Assert(channelId || invokeLocal);

		if (channelId)
		{
			Channel * channel = m_channelMgr->FindChannel(*channelId);

			Assert(channel);

			if (channel)
			{
				channel->Send(p, 0);
			}
		}
	}

	if (invokeLocal)
	{
		BitStream bs3(bs.GetData(), bs.GetDataSize());

		CallInternal(0, method, bs3);
	}
}

void RpcManager::OnReceive(Packet & packet, Channel * channel)
{
	Packet bsPacket;
	packet.ExtractTillEnd(bsPacket);
	BitStream bs(bsPacket.GetData(), Net::BytesToBits(bsPacket.GetSize()));

	uint32_t method;
	const bool shortMethod = bs.ReadBit();
	if (shortMethod)
		method = bs.ReadBits<uint32_t>(7);
	else
		bs.Read(method);

	bs.ReadAlign();

	CallInternal(channel, method, bs);
}

void RpcManager::CallInternal(Channel * channel, uint32_t method, BitStream & bs)
{
	RegistrationMap::const_iterator i = m_registrations.find(method);
	NetAssert(i != m_registrations.end() || (method < 128 && m_handler));

	if (i != m_registrations.end())
	{
		RpcHandler handler = i->second;

		handler(channel, method, bs);
	}
	else if (method < 128 && m_handler)
	{
		m_handler(channel, method, bs);
	}
}
