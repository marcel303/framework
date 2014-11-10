#include "Log.h"
#include "NetStats.h"
#include "Timer.h"

NET_STAT_DEFINE(NetStat_BytesSent);
NET_STAT_DEFINE(NetStat_BytesReceived);
NET_STAT_DEFINE(NetStat_PacketsSent);
NET_STAT_DEFINE(NetStat_PacketsReceived);
NET_STAT_DEFINE(NetStat_ProtocolInvalid);
NET_STAT_DEFINE(NetStat_ProtocolMasked);

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
}

#else

void PrintNetStats() { }
void CommitNetStats() { }

#endif
