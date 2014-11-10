#include "Channel.h"
#include "libnet_config.h"
#include "NetDiag.h"
#include "NetStats.h"
#include "PacketDispatcher.h"
#include "PacketListener.h"

PacketDispatcher::PacketDispatcher()
{
	memset(m_protocolListeners, 0, sizeof(m_protocolListeners));
}

void PacketDispatcher::Dispatch(Packet & packet, Channel * channel)
{
	uint8_t protocolId;
	
	if (packet.Read8(&protocolId))
	{
		if (protocolId >= LIBNET_DISPATCHER_MAX_PROTOCOLS || m_protocolListeners[protocolId] == 0)
		{
			LOG_ERR("dispatcher: dispatch: invalid protocol", 0);
			NET_STAT_INC(NetStat_ProtocolInvalid);
			NetAssert(false);
		}
		else
		{
			if (channel->m_protocolMask & (1 << protocolId))
			{
				m_protocolListeners[protocolId]->OnReceive(packet, channel);
			}
			else
			{
				LOG_ERR("dispatcher: dispatch: protocol is masked by channel", 0);
				NET_STAT_INC(NetStat_ProtocolMasked);
				return;
			}
		}
	}
	else
	{
		LOG_ERR("dispatcher: dispatch: failed to read from packet", 0);
		NetAssert(false);
		return;
	}
}

bool PacketDispatcher::RegisterProtocol(uint32_t protocolId, PacketListener * listener)
{
	NetAssert(protocolId < LIBNET_DISPATCHER_MAX_PROTOCOLS);
	if (protocolId >= LIBNET_DISPATCHER_MAX_PROTOCOLS)
	{
		return false;
	}

	m_protocolListeners[protocolId] = listener;

	return true;
}

bool PacketDispatcher::UnregisterProtocol(uint32_t protocolId, PacketListener * listener)
{
	NetAssert(protocolId < LIBNET_DISPATCHER_MAX_PROTOCOLS);
	if (protocolId >= LIBNET_DISPATCHER_MAX_PROTOCOLS)
	{
		return false;
	}

	NetAssert(m_protocolListeners[protocolId] == listener);
	if (m_protocolListeners[protocolId] != listener)
	{
		return false;
	}

	m_protocolListeners[protocolId] = nullptr;

	return true;
}
