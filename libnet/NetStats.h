#pragma once

#include <stdint.h>
#include "Debugging.h"
#include "libnet_config.h"

#if LIBNET_ENABLE_NET_STATS

template <class T>
class NetStatsValue
{
	struct HistoryRecord
	{
		HistoryRecord()
		{
			m_time = 0;
			m_value = 0;
		}

		uint64_t m_time;
		T m_value;
	};

	int Wrap(int position) const
	{
		while (position < 0)
			position += m_historySize;

		while (position >= m_historySize)
			position -= m_historySize;

		return position;
	}

	T CalculateRunningSum() const
	{
		if (m_size < 2)
			return 0;

		const HistoryRecord & prevRecord = m_history[Wrap(m_currentWritePosition - m_size)];
		const HistoryRecord & currRecord = m_history[Wrap(m_currentWritePosition - 1)];

		return currRecord.m_value - prevRecord.m_value;
	}

	uint64_t CalculateRunningTime() const
	{
		if (m_size < 2)
			return 0;

		const HistoryRecord & prevRecord = m_history[Wrap(m_currentWritePosition - m_size)];
		const HistoryRecord & currRecord = m_history[Wrap(m_currentWritePosition - 1)];

		return currRecord.m_time - prevRecord.m_time;
	}

	const static int DEFAULT_HISTORY_SIZE = 100;

	HistoryRecord * m_history;
	int m_historySize;
	int m_currentWritePosition;
	int m_size;
	HistoryRecord m_currentRecord;

public:
	NetStatsValue()
	{
		m_history = 0;
		m_historySize = 0;
		m_currentWritePosition = 0;
		m_size = 0;

		SetHistorySize(DEFAULT_HISTORY_SIZE);
	}

	~NetStatsValue()
	{
		SetHistorySize(0);
	}

	void SetHistorySize(int size)
	{
		if (m_history)
		{
			delete[] m_history;
			m_history = 0;
			m_historySize = 0;
			m_currentWritePosition = 0;
			m_size = 0;
		}

		if (size != 0)
		{
			m_history = new HistoryRecord[size];
			m_historySize = size;
		}
	}

	void Commit(uint64_t time)
	{
		m_currentRecord.m_time = time;

		m_history[m_currentWritePosition] = m_currentRecord;

		m_currentWritePosition++;

		if (m_currentWritePosition > m_size)
			m_size = m_currentWritePosition;

		if (m_currentWritePosition == m_historySize)
			m_currentWritePosition = 0;
	}

	void Increment(const T & value)
	{
		m_currentRecord.m_value += value;
	}

	float CalculateAveragePerFrame() const
	{
		if (m_size < 2)
			return 0;

		return float(CalculateRunningSum()) / (m_size - 1);
	}

	float CalculateAveragePerSecond() const
	{
		const uint64_t runningTime = CalculateRunningTime();

		if (runningTime != 0)
			return CalculateRunningSum() / float(runningTime / 1000000.0f);
		else
			return 0;
	}

	T CalculateMin() const
	{
		const int size = GetSize();

		if (size == 0)
			return 0;

		T min = GetValue(0);

		for (int i = 1; i < size; ++i)
		{
			const T value = GetValue(i);
			if (value < min)
				min = value;
		}

		return min;
	}

	T CalculateMax() const
	{
		const int size = GetSize();

		if (size == 0)
			return 0;

		T max = GetValue(0);

		for (int i = 1; i < size; ++i)
		{
			const T value = GetValue(i);
			if (value > max)
				max = value;
		}

		return max;
	}

	//

	int GetSize() const
	{
		return m_size < 2 ? 0 : (m_size - 1);
	}

	T GetValue(int offset) const
	{
		if (m_size < 2)
			return 0;

		const int prevPosition = Wrap(m_currentWritePosition - m_size + offset + 0);
		const int currPosition = Wrap(m_currentWritePosition - m_size + offset + 1);

		Assert(currPosition != m_currentWritePosition);

		const HistoryRecord & prevRecord = m_history[prevPosition];
		const HistoryRecord & currRecord = m_history[currPosition];

		return currRecord.m_value - prevRecord.m_value;
	}

	T GetTotalValue() const
	{
		if (m_size == 0)
			return 0;

		return m_history[m_currentWritePosition - 1].m_value;
	}

	T GetCurrentValue() const
	{
		return m_currentRecord.m_value;
	}
};

#define NET_STAT_EXTERN(name) extern NetStatsValue<int> name
#define NET_STAT_DEFINE(name) NetStatsValue<int> name
#define NET_STAT_INC(name) name.Increment(1)
#define NET_STAT_ADD(name, value) name.Increment(value)
#define NET_STAT_COMMIT(name, timeUS) name.Commit(timeUS)

#else

#define NET_STAT_EXTERN(name)
#define NET_STAT_DEFINE(name)
#define NET_STAT_INC(name) do { } while (false)
#define NET_STAT_ADD(name, value) do { } while (false)
#define NET_STAT_COMMIT(name, time) do { } while (false)

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
NET_STAT_EXTERN(NetStat_ReliableTransportAcksSent);
NET_STAT_EXTERN(NetStat_ReliableTransportAcksReceived);
NET_STAT_EXTERN(NetStat_ReliableTransportAcksIgnored);
NET_STAT_EXTERN(NetStat_ReliableTransportNacksSent);
NET_STAT_EXTERN(NetStat_ReliableTransportNacksReceived);
NET_STAT_EXTERN(NetStat_ReliableTransportNacksIgnored);
NET_STAT_EXTERN(NetStat_ReliableTransportReceives);
NET_STAT_EXTERN(NetStat_ReliableTransportReceivesIgnored);

void PrintNetStats();
void CommitNetStats();
