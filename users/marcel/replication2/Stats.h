#ifndef STATS_H
#define STATS_H
#pragma once

#include "StatValue.h"
#include "Timer.h"

class StatsNet
{
public:
	inline void Next()
	{
		uint64_t time = g_TimerRT.TimeUS_get();

		m_packetsSent.Add(time, 0);
		m_packetsReceived.Add(time, 0);
		m_bytesSent.Add(time, 0);
		m_bytesSentOverhead.Add(time, 0);
		m_bytesReceived.Add(time, 0);
		m_invalidPacketCount.Add(time, 0);
		m_invalidProtocolCount.Add(time, 0);
	}

	StatInt m_packetsSent;
	StatInt m_packetsReceived;
	StatInt m_bytesSent;
	StatInt m_bytesSentOverhead;
	StatInt m_bytesReceived;
	StatInt m_rtt;
	StatInt m_invalidPacketCount;
	StatInt m_invalidProtocolCount;
};

class StatsRep
{
public:
	inline void Next()
	{
		uint64_t time = g_TimerRT.TimeUS_get();

		m_bytesReceived.Add(time, 0);
		m_objectsCreated.Add(time, 0);
		m_objectsDestroyed.Add(time, 0);
		m_objectsUpdated.Add(time, 0);
		m_objectsVersioned.Add(time, 0);
	}

	StatInt m_bytesReceived;
	StatInt m_objectsCreated;
	StatInt m_objectsDestroyed;
	StatInt m_objectsUpdated;
	StatInt m_objectsVersioned;
};

class StatsScene
{
public:
	inline void Next()
	{
		uint64_t time = g_TimerRT.TimeUS_get();

		m_objectCount.Add(time, 0);
	}

	StatInt m_objectCount;
};

class StatsGfx
{
public:
	inline StatsGfx()
	{
		m_fps.SetHistorySize(20);
	}

	inline void Next()
	{
	}

	inline void NextFps(int fps)
	{
		uint64_t time = g_TimerRT.TimeUS_get();

		m_fps.Add(time, fps);
	}

	StatInt m_fps;
};

class Stats
{
public:
	static Stats& I()
	{
		static Stats stats;
		return stats;
	}

	inline void NextNet()
	{
		m_net.Next();
	}

	inline void NextRep()
	{
		m_rep.Next();
	}

	inline void NextScene()
	{
		m_scene.Next();
	}

	inline void NextGfx()
	{
		m_gfx.Next();
	}

	StatsNet m_net;
	StatsRep m_rep;
	StatsScene m_scene;
	StatsGfx m_gfx;

private:
	inline Stats()
	{
	}
};

#endif
