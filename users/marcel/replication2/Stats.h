#ifndef STATS_H
#define STATS_H
#pragma once

#include "NetStats.h"
#include "Timer.h"

NET_STAT_EXTERN(Scene_ObjectCount);

NET_STAT_EXTERN(Gfx_Fps);

class Stats
{
public:
	static Stats & I()
	{
		static Stats stats;
		return stats;
	}

	void CommitReplication()
	{
		const uint64_t time = g_TimerRT.TimeUS_get();

		NET_STAT_COMMIT(NetStat_ReplicationBytesReceived, time);
		NET_STAT_COMMIT(NetStat_ReplicationObjectsCreated, time);
		NET_STAT_COMMIT(NetStat_ReplicationObjectsDestroyed, time);
		NET_STAT_COMMIT(NetStat_ReplicationObjectsUpdated, time);
	}

	void CommitScene()
	{
		const uint64_t time = g_TimerRT.TimeUS_get();

		NET_STAT_COMMIT(Scene_ObjectCount, time);
	}

private:
	inline Stats()
	{
	}
};

#endif
