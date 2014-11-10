#ifndef STATS_H
#define STATS_H
#pragma once

#include "NetStats.h"
#include "Timer.h"

NET_STAT_EXTERN(Replication_BytesReceived);
NET_STAT_EXTERN(Replication_ObjectsCreated);
NET_STAT_EXTERN(Replication_ObjectsDestroyed);
NET_STAT_EXTERN(Replication_ObjectsUpdated);

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

		NET_STAT_COMMIT(Replication_BytesReceived, time);
		NET_STAT_COMMIT(Replication_ObjectsCreated, time);
		NET_STAT_COMMIT(Replication_ObjectsDestroyed, time);
		NET_STAT_COMMIT(Replication_ObjectsUpdated, time);
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
