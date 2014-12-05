#pragma once

#include "libnet_config.h"

#if LIBNET_ENABLE_NET_STATS

#include "StatTimers.h"

#define NET_STAT_EXTERN(name) TIMER_EXTERN(name)
#define NET_STAT_DEFINE(name, path) TIMER_DEFINE(name, PerSecond, path)
#define NET_STAT_INC(name) TIMER_INC(name)
#define NET_STAT_ADD(name, count) TIMER_ADD(name, count)

#else

#define NET_STAT_EXTERN(name)
#define NET_STAT_DEFINE(name, path)
#define NET_STAT_INC(name) do { } while (false)
#define NET_STAT_ADD(name, count) do { } while (false)

#endif

NET_STAT_EXTERN(NetStat_BytesSent);
NET_STAT_EXTERN(NetStat_BytesReceived);
NET_STAT_EXTERN(NetStat_PacketsSent);
NET_STAT_EXTERN(NetStat_PacketsReceived);
NET_STAT_EXTERN(NetStat_ProtocolInvalid); // message received with an invalid protocol ID
NET_STAT_EXTERN(NetStat_ProtocolMasked); // protocol is disabled by channel mask

NET_STAT_EXTERN(NetStat_ReliableTransportUpdatesSent);
NET_STAT_EXTERN(NetStat_ReliableTransportUpdateResends);
NET_STAT_EXTERN(NetStat_ReliableTransportUpdateLimitReached);
NET_STAT_EXTERN(NetStat_ReliableTransportUpdatesReceived);
NET_STAT_EXTERN(NetStat_ReliableTransportUpdatesIgnored);
NET_STAT_EXTERN(NetStat_ReliableTransportAcksSent);
NET_STAT_EXTERN(NetStat_ReliableTransportAcksReceived);
NET_STAT_EXTERN(NetStat_ReliableTransportAcksIgnored);
NET_STAT_EXTERN(NetStat_ReliableTransportNacksSent);
NET_STAT_EXTERN(NetStat_ReliableTransportNacksReceived);
NET_STAT_EXTERN(NetStat_ReliableTransportNacksIgnored);

NET_STAT_EXTERN(NetStat_ReplicationBytesReceived);
NET_STAT_EXTERN(NetStat_ReplicationObjectsCreated);
NET_STAT_EXTERN(NetStat_ReplicationObjectsDestroyed);
NET_STAT_EXTERN(NetStat_ReplicationObjectsUpdated);
