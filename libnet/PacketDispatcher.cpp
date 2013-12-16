#include "Channel.h"
#include "libnet_config.h"
#include "NetDiag.h"
#include "NetStats.h"
#include "PacketDispatcher.h"
#include "PacketListener.h"

static PacketListener * s_protocolListeners[LIBNET_DISPATCHER_MAX_PROTOCOLS] = { 0 };

void PacketDispatcher::Dispatch(Packet & packet, Channel * channel)
{
	uint8_t protocolId;
	
	if (packet.Read8(&protocolId))
	{
		if (protocolId >= LIBNET_DISPATCHER_MAX_PROTOCOLS || s_protocolListeners[protocolId] == 0)
		{
			LOG_ERR("dispatcher: dispatch: invalid protocol", 0);
			NetStats::Inc(NetStat_ProtocolInvalid);
			NetAssert(false);
		}
		else
		{
			if (channel->m_protocolMask & (1 << protocolId))
			{
				s_protocolListeners[protocolId]->OnReceive(packet, channel);
			}
			else
			{
				LOG_ERR("dispatcher: dispatch: protocol is masked by channel", 0);
				NetStats::Inc(NetStat_ProtocolMasked);
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
	if (protocolId >= LIBNET_DISPATCHER_MAX_PROTOCOLS)
	{
		NetAssert(false);
		return false;
	}

	s_protocolListeners[protocolId] = listener;

	return true;
}
