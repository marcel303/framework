#include "Log.h"
#include "NetStats.h"
#include "Timer.h"

NET_STAT_DEFINE(NetStat_BytesSent);
NET_STAT_DEFINE(NetStat_BytesReceived);
NET_STAT_DEFINE(NetStat_PacketsSent);
NET_STAT_DEFINE(NetStat_PacketsReceived);
NET_STAT_DEFINE(NetStat_ProtocolInvalid);
NET_STAT_DEFINE(NetStat_ProtocolMasked);

NET_STAT_DEFINE(NetStat_ReliableTransportUpdatesSent);
NET_STAT_DEFINE(NetStat_ReliableTransportUpdateResends);
NET_STAT_DEFINE(NetStat_ReliableTransportUpdateLimitReached);
NET_STAT_DEFINE(NetStat_ReliableTransportAcksSent);
NET_STAT_DEFINE(NetStat_ReliableTransportAcksReceived);
NET_STAT_DEFINE(NetStat_ReliableTransportAcksIgnored);
NET_STAT_DEFINE(NetStat_ReliableTransportNacksSent);
NET_STAT_DEFINE(NetStat_ReliableTransportNacksReceived);
NET_STAT_DEFINE(NetStat_ReliableTransportNacksIgnored);
NET_STAT_DEFINE(NetStat_ReliableTransportReceives);
NET_STAT_DEFINE(NetStat_ReliableTransportReceivesIgnored);

NET_STAT_DEFINE(NetStat_ReplicationBytesReceived);
NET_STAT_DEFINE(NetStat_ReplicationObjectsCreated);
NET_STAT_DEFINE(NetStat_ReplicationObjectsDestroyed);
NET_STAT_DEFINE(NetStat_ReplicationObjectsUpdated);

#if LIBNET_ENABLE_NET_STATS

static void PrintNetStat(const char * name, const NetStatsValue<int> & stat)
{
	LOG_INF("%s: min=%08u, max=%08u, total=%08u, average=%08.2f",
		name,
		stat.CalculateMin(),
		stat.CalculateMax(),
		stat.GetTotalValue(),
		stat.CalculateAveragePerSecond());
}

void PrintNetStats()
{
	PrintNetStat("bytes sent      ", NetStat_BytesSent);
	PrintNetStat("bytes received  ", NetStat_BytesReceived);
	PrintNetStat("packets sent    ", NetStat_PacketsSent);
	PrintNetStat("packets received", NetStat_PacketsReceived);
	PrintNetStat("invalid protocol", NetStat_ProtocolInvalid);
	PrintNetStat("masked protocol ", NetStat_ProtocolMasked);

	PrintNetStat("RT updates sent    ", NetStat_ReliableTransportUpdatesSent);
	PrintNetStat("RT updates resent  ", NetStat_ReliableTransportUpdateResends);
	PrintNetStat("RT update limited  ", NetStat_ReliableTransportUpdateLimitReached);
	PrintNetStat("RT acks sent       ", NetStat_ReliableTransportAcksSent);
	PrintNetStat("RT acks received   ", NetStat_ReliableTransportAcksReceived);
	PrintNetStat("RT acks ignored    ", NetStat_ReliableTransportAcksIgnored);
	PrintNetStat("RT nacks sent      ", NetStat_ReliableTransportNacksSent);
	PrintNetStat("RT nacks received  ", NetStat_ReliableTransportNacksReceived);
	PrintNetStat("RT nacks ignored   ", NetStat_ReliableTransportNacksIgnored);
	PrintNetStat("RT updates received", NetStat_ReliableTransportReceives);
	PrintNetStat("RT updates ignored ", NetStat_ReliableTransportReceivesIgnored);
}

void CommitNetStats()
{
	const uint64_t time = g_TimerRT.TimeUS_get();

	NET_STAT_COMMIT(NetStat_BytesSent, time);
	NET_STAT_COMMIT(NetStat_BytesReceived, time);
	NET_STAT_COMMIT(NetStat_PacketsSent, time);
	NET_STAT_COMMIT(NetStat_PacketsReceived, time);
	NET_STAT_COMMIT(NetStat_ProtocolInvalid, time);
	NET_STAT_COMMIT(NetStat_ProtocolMasked, time);

	NET_STAT_COMMIT(NetStat_ReliableTransportUpdatesSent, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportUpdateResends, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportUpdateLimitReached, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportAcksSent, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportAcksReceived, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportAcksIgnored, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportNacksSent, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportNacksReceived, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportNacksIgnored, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportReceives, time);
	NET_STAT_COMMIT(NetStat_ReliableTransportReceivesIgnored, time);
}

#else

void PrintNetStats() { }
void CommitNetStats() { }

#endif
