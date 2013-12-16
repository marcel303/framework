#pragma once

#include <stdint.h>

enum NetStat
{
	NetStat_BytesSent,
	NetStat_BytesReceived,
	NetStat_PacketsSent,
	NetStat_PacketsReceived,
	NetStat_ProtocolInvalid, // message received with an invalid protocol ID
	NetStat_ProtocolMasked // protocol is disabled by channel mask
};

class NetStats
{
public:
	static void Inc(NetStat stat);
	static void Inc(NetStat stat, uint32_t count);
	static void Show();
};
