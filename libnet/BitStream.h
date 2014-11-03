#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include "Debugging.h"

namespace Net
{
	inline size_t BitsToBytes(size_t bits)
	{
		return (bits + 7) >> 3;
	}

	inline size_t BytesToBits(size_t bytes)
	{
		return bytes << 3;
	}
}

class BitStream
{
	uint8_t * m_data;
	uint32_t m_dataSize;
	bool m_ownData;
	uint32_t m_cursor;

	inline void CheckReadCursor(const uint32_t size) const
	{
		Assert(m_cursor + size <= m_dataSize);
	}

	void CheckWriteCursor(const uint32_t size)
	{
		if (m_cursor + size > m_dataSize)
		{
			if (m_data)
			{
				const uint32_t newDataSize = (((m_dataSize + size) * 2 + 7) >> 3) << 3;
				m_data = static_cast<uint8_t*>(realloc(m_data, newDataSize >> 3));
				m_dataSize = newDataSize;
			}
			else
			{
				const uint32_t newDataSize = ((size + 7) >> 3) << 3;
				m_data = static_cast<uint8_t*>(malloc(newDataSize >> 3));
				m_dataSize = newDataSize;
			}
		}
	}

public:
	BitStream()
		: m_data(0)
		, m_dataSize(0)
		, m_ownData(true)
		, m_cursor(0)
	{
	}

	BitStream(const void * data, uint32_t dataSize)
		: m_data(static_cast<uint8_t*>(const_cast<void*>(data)))
		, m_dataSize(dataSize)
		, m_ownData(false)
		, m_cursor(0)
	{
	}

	~BitStream()
	{
		if (m_ownData)
			free(m_data);
		m_data = 0;
		m_dataSize = 0;
		m_cursor = 0;
	}

	void ReadAlign()
	{
		if (m_cursor & 7)
		{
			const uint32_t offset = 8 - (m_cursor & 7);
			CheckReadCursor(offset);
			m_cursor += offset;
		}
	}

	void WriteAlign()
	{
		if (m_cursor & 7)
		{
			const uint32_t offset = 8 - (m_cursor & 7);
			CheckWriteCursor(offset);
			m_cursor += offset;
		}
	}

	bool ReadBit()
	{
		CheckReadCursor(1);

		const uint32_t byteIdx = m_cursor >> 3;
		const uint32_t bitIdx = m_cursor & 7;
		m_cursor++;

		const uint8_t byte = m_data[byteIdx];
		return (byte & (1 << bitIdx)) != 0;
	}

	void WriteBit(bool v)
	{
		CheckWriteCursor(1);

		const uint32_t byteIdx = m_cursor >> 3;
		const uint32_t bitIdx = m_cursor & 7;
		const uint8_t bitMask = v ? (1 << bitIdx) : 0;
		m_cursor++;

		uint8_t & byte = m_data[byteIdx];

		if (bitIdx == 0)
			byte = bitMask;
		else
			byte |= bitMask;
	}
	
	template <typename T>
	T ReadBits(const uint32_t numBits)
	{
		T v = (T)0;
		for (uint32_t i = 0; i < numBits; ++i)
			v |= ReadBit() << i;
		return v;
	}

	template <typename T>
	void WriteBits(const T v, const uint32_t numBits)
	{
		for (uint32_t i = 0; i < numBits; ++i)
			WriteBit((v & (1 << i)) != 0);
	}

	template <typename T>
	void Read(T & v)
	{
		if ((m_cursor & 7) == 0)
		{
			CheckReadCursor(sizeof(T) << 3);

			v = *reinterpret_cast<T*>(&m_data[m_cursor >> 3]);
			m_cursor += sizeof(v) << 3;
		}
		else
		{
			v = ReadBits<T>(sizeof(T) << 3);
		}
	}

	template <typename T>
	void Write(const T v)
	{
		if ((m_cursor & 7) == 0)
		{
			CheckWriteCursor(sizeof(T) << 3);

			*reinterpret_cast<T*>(&m_data[m_cursor >> 3]) = v;
			m_cursor += sizeof(v) << 3;
		}
		else
		{
			WriteBits(v, sizeof(v) << 3);
		}
	}

	void Read(float & v)
	{
		uint32_t * p = reinterpret_cast<uint32_t*>(&v);

		Read<uint32_t>(*p);
	}

	void Write(float & v)
	{
		uint32_t * p = reinterpret_cast<uint32_t*>(&v);

		Write<uint32_t>(*p);
	}

	void ReadAlignedBytes(void * v, const size_t s)
	{
		ReadAlign();

		CheckReadCursor(s << 3);
		memcpy(v, &m_data[m_cursor >> 3], s);
		m_cursor += s << 3;
	}

	void WriteAlignedBytes(const void * v, const size_t s)
	{
		WriteAlign();

		CheckWriteCursor(s << 3);
		memcpy(&m_data[m_cursor >> 3], v, s);
		m_cursor += s << 3;
	}

	std::string ReadString()
	{
		ReadAlign();

		uint16_t size;
		CheckReadCursor(sizeof(size) << 3);
		Read(size);

		std::string s;
		s.resize(size);
		ReadAlignedBytes(&s[0], size);

		return s;
	}

	void WriteString(const std::string & s)
	{
		WriteAlign();

		Assert(s.length() <= 65535);
		const uint16_t size = s.length();

		CheckWriteCursor(sizeof(size) << 3);
		Write(size);

		WriteAlignedBytes(&s[0], size);
	}

	void ReadSkip(const size_t s)
	{
		CheckReadCursor(s);
		m_cursor += s;
	}

	const void * GetData() const
	{
		return m_data;
	}

	uint32_t GetDataSize() const
	{
		return m_cursor;
	}
};

//

void WriteDiff(BitStream & bs, class BinaryDiffResult & diff, const void * v);
void ReadDiff(BitStream & bs, void * v);
