#pragma once

#include "libnet_forward.h"

class PacketListener
{
public:
	virtual ~PacketListener();

	virtual void OnReceive(Packet & packet, Channel * channel) = 0;
};
