#pragma once

#include <stdint.h>
#include "libnet_forward.h"

class PacketDispatcher
{
	PacketListener * m_protocolListeners[LIBNET_DISPATCHER_MAX_PROTOCOLS];

public:
	PacketDispatcher();

	void Dispatch(Packet & packet, Channel * channel);
	bool RegisterProtocol(uint32_t protocolId, PacketListener * listener);
	bool UnregisterProtocol(uint32_t protocolId, PacketListener * listener);
};
