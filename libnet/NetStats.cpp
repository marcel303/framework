#include <string.h>
#include "Log.h"
#include "NetStats.h"

const static uint32_t kMaxNetStats = 32;

struct NetStatsData
{
	NetStatsData()
	{
		memset(m_values, 0, sizeof(m_values));
	}

	uint32_t m_values[kMaxNetStats];
} s_netStats;

void NetStats::Inc(NetStat stat)
{
	s_netStats.m_values[stat] += 1;
}

void NetStats::Inc(NetStat stat, uint32_t count)
{
	s_netStats.m_values[stat] += count;
}

void NetStats::Show()
{
#define SHOW(stat) LOG_INF("%s: %u", #stat, s_netStats.m_values[stat])

	SHOW(NetStat_BytesSent);
	SHOW(NetStat_BytesReceived);
	SHOW(NetStat_PacketsSent);
	SHOW(NetStat_PacketsReceived);
	SHOW(NetStat_ProtocolInvalid);
	SHOW(NetStat_ProtocolMasked);

#undef SHOW
}

