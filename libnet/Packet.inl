#pragma once

#include "NetDiag.h"
#include "Packet.h"

inline Packet::Packet()
	: m_rcvAddress()
	, m_data(0)
	, m_size(0)
	, m_cursor(0)
{
}

inline Packet::Packet(const void * data, uint32_t size)
	: m_rcvAddress()
	, m_data(reinterpret_cast<const uint8_t *>(data))
	, m_size(size)
	, m_cursor(0)
{
	NetAssert(m_data != 0);
}

inline Packet::Packet(const void * data, uint32_t size, const NetAddress & rcvAddress)
	: m_rcvAddress(rcvAddress)
	, m_data(reinterpret_cast<const uint8_t *>(data))
	, m_size(size)
	, m_cursor(0)
{
	NetAssert(m_data != 0);
}

inline const void * Packet::GetData() const
{
	return m_data;
}

inline uint32_t Packet::GetSize() const
{
	return m_size;
}

inline bool Packet::Read8(void * dst)
{
	return Read(dst, 1);
}

inline bool Packet::Read16(void * dst)
{
	return Read(dst, 2);
}

inline bool Packet::Read32(void * dst)
{
	return Read(dst, 4);
}

inline bool Packet::Seek(uint32_t cursor)
{
	if (cursor > m_size)
	{
		NetAssert(cursor <= m_size);
		return false;
	}

	m_cursor = cursor;

	return true;
}

inline bool Packet::Skip(uint32_t bytes)
{
	return Seek(m_cursor + bytes);
}

inline bool Packet::Extract(Packet & out_packet, uint32_t size, bool incrementCursor)
{
	if (m_cursor + size > m_size)
	{
		NetAssert(m_cursor + size <= m_size);
		return false;
	}

	out_packet = Packet(m_data + m_cursor, size, m_rcvAddress);

	if (incrementCursor)
		m_cursor += size;

	return true;
}

inline bool Packet::ExtractTillEnd(Packet & out_packet)
{
	uint32_t size = m_size - m_cursor;

	return Extract(out_packet, size, false);
}

inline bool Packet::CopyTo(void * dst, uint32_t dstSize) const
{
	if (m_size > dstSize)
	{
		NetAssert(m_size <= dstSize);
		return false;
	}
	Copy(m_data, dst, m_size);
	return true;
}

//

template <uint32_t MAX_SIZE>
inline PacketBuilder<MAX_SIZE>::PacketBuilder()
	: m_size(0)
	, m_cursor(0)
{
}

template <uint32_t MAX_SIZE>
inline uint32_t PacketBuilder<MAX_SIZE>::GetSize() const
{
	return m_size;
}

template <uint32_t MAX_SIZE>
inline bool PacketBuilder<MAX_SIZE>::Seek(uint32_t cursor)
{
	if (cursor > m_size)
	{
		NetAssert(cursor <= m_size);
		return false;
	}

	m_cursor = cursor;

	return true;
}

template <uint32_t MAX_SIZE>
inline bool PacketBuilder<MAX_SIZE>::Skip(uint32_t bytes)
{
	return Seek(m_cursor + bytes);
}

template <uint32_t MAX_SIZE>
inline void PacketBuilder<MAX_SIZE>::Clear()
{
	m_size = 0;
	m_cursor = 0;
}

template <uint32_t MAX_SIZE>
inline bool PacketBuilder<MAX_SIZE>::Write(const void * src, uint32_t size)
{
	if (m_cursor + size > MAX_SIZE)
	{
		NetAssert(m_cursor + size <= MAX_SIZE);
		return false;
	}

	Copy(src, &m_data[m_cursor], size);

	m_cursor += size;

	if (m_cursor > m_size)
		m_size = m_cursor;

	return true;
}

template <uint32_t MAX_SIZE>
inline bool PacketBuilder<MAX_SIZE>::Write8(const void * src)
{
	return Write(src, 1);
}

template <uint32_t MAX_SIZE>
inline bool PacketBuilder<MAX_SIZE>::Write16(const void * src)
{
	return Write(src, 2);
}

template <uint32_t MAX_SIZE>
inline bool PacketBuilder<MAX_SIZE>::Write32(const void * src)
{
	return Write(src, 4);
}

template <uint32_t MAX_SIZE>
inline bool PacketBuilder<MAX_SIZE>::WritePacket(const Packet & packet)
{
	const void * data = packet.GetData();
	const uint32_t size = packet.GetSize();

	return Write(data, size);
}

template <uint32_t MAX_SIZE>
inline Packet PacketBuilder<MAX_SIZE>::ToPacket()
{
	return Packet(m_data, m_size);
}

template <uint32_t MAX_SIZE>
inline bool PacketBuilder<MAX_SIZE>::CopyTo(void * dst, uint32_t dstSize) const
{
	if (m_size > dstSize)
	{
		NetAssert(m_size <= dstSize);
		return false;
	}
	Copy(m_data, dst, m_size);
	return true;
}

template <uint32_t MAX_SIZE>
void PacketBuilder<MAX_SIZE>::Copy(const void * __restrict src, void * __restrict dst, uint32_t size)
{
	const uint8_t * __restrict _src = reinterpret_cast<const uint8_t * __restrict>(src);
	uint8_t       * __restrict _dst = reinterpret_cast<uint8_t *       __restrict>(dst);

	for (uint32_t i = size; i != 0; --i)
		*_dst++ = *_src++;

	//memcpy(dst, src, size);
}