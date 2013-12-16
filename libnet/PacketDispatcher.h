#pragma once

#include <stdint.h>
#include "libnet_forward.h"

class PacketDispatcher
{
public:
	static void Dispatch(Packet & packet, Channel * channel);
	static bool RegisterProtocol(uint32_t protocolId, PacketListener * listener);
};

