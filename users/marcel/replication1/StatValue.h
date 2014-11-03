#ifndef STATSVALUE_H
#define STATSVALUE_H
#pragma once

#include <malloc.h>
#include "Types.h"

template <class T>
class StatState
{
public:
	inline void Initialize(uint64_t time, T value)
	{
		m_time = time;
		m_value = value;
	}

	uint64_t m_time;
	T m_value;
};

template <class T>
class StatValue
{
public:
	inline StatValue()
	{
		m_values = 0;
		m_historySize = 0;
		m_position = 0;
		m_size = 0;

		SetHistorySize(DEFAULT_HISTORY_SIZE);
	}

	inline ~StatValue()
	{
		SetHistorySize(0);
	}

	inline void SetHistorySize(int size)
	{
		if (m_values)
		{
			delete[] m_values;
			m_values = 0;
			m_historySize = 0;
			m_position = 0;
			m_size = 0;
		}

		if (size == 0)
			return;

		m_values = new StatState<T>[size];
		m_historySize = size;
	}

	inline void Add(uint64_t time, T value)
	{
		StatState<T> temp;

		temp.Initialize(time, value);

		m_values[m_position] = temp;

		m_position++;

		if (m_position > m_size)
			m_size = m_position;

		if (m_position >= m_historySize)
			m_position = 0;
	}

	inline void Inc(T value)
	{
		m_values[Loop(m_position - 1)].m_value += value;
	}

	inline T CalcSum() const
	{
		T sum = 0;

		for (int i = 0; i < m_size; ++i)
			sum += m_values[i].m_value;

		return sum;
	}

	inline T CalcAverage() const
	{
		if (m_size == 0)
			return 0;

		return CalcSum() / m_size;
	}

	inline float CalcAverageF() const
	{
		if (m_size == 0)
			return 0;

		return float(CalcSum()) / m_size;
	}

	inline float CalcAverageFS() const
	{
		if (m_size == 0)
			return 0;

		return float(CalcSum()) / (CalcTime() / 1000000.0f);
	}

	inline uint64_t CalcTime() const
	{
		if (m_size == 0)
			return 0;

		StatState<T> value1 = m_values[Loop(m_position - m_size)];
		StatState<T> value2 = m_values[Loop(m_position - 1)];

		return value2.m_time - value1.m_time;
	}

	inline T CalcMin() const
	{
		if (m_size == 0)
			return 0;

		T min = m_values[0].m_value;

		for (int i = 1; i < m_size; ++i)
			if (m_values[i].m_value < min)
				min = m_values[i].m_value;

		return min;
	}

	inline T CalcMax() const
	{
		if (m_size == 0)
			return 0;

		T max = m_values[0].m_value;

		for (int i = 1; i < m_size; ++i)
			if (m_values[i].m_value > max)
				max = m_values[i].m_value;

		return max;
	}

//FIXME private:
	inline int Loop(int position) const
	{
		while (position < 0)
			position += m_historySize;

		while (position >= m_historySize)
			position -= m_historySize;

		return position;
	}

	const static int DEFAULT_HISTORY_SIZE = 100;

	StatState<T>* m_values;
	int m_historySize;
	int m_position;
	int m_size;
};

typedef StatValue<int> StatInt;
typedef StatValue<float> StatFloat;

#endif
