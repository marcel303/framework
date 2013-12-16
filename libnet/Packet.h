#pragma once

#include "NetAddress.h"
#include "ChannelTypes.h"

class Packet
{
public:
	inline Packet();
	inline Packet(const void * data, uint32_t size);
	inline Packet(const void * data, uint32_t size, const NetAddress & rcvAddress);

	inline const void * GetData() const;
	inline uint32_t GetSize() const;

	inline bool Read8(void * dst);
	inline bool Read16(void * dst);
	inline bool Read32(void * dst);

	inline bool Seek(uint32_t cursor);
	inline bool Skip(uint32_t bytes);
	inline bool Extract(Packet & out_packet, uint32_t size);
	inline bool ExtractTillEnd(Packet & out_packet);
	inline bool CopyTo(void * dst, uint32_t dstSize) const;

	NetAddress m_rcvAddress; // FIXME, private

private:
	bool Read(void * dst, uint32_t size);
	static void Copy(const void * __restrict src, void * __restrict dst, uint32_t size);

	const uint8_t * m_data;
	uint32_t m_size;
	uint32_t m_cursor;
};

template <uint32_t MAX_SIZE>
class PacketBuilder
{
public:
	inline PacketBuilder();

	inline uint32_t GetSize() const;

	inline bool Seek(uint32_t cursor);
	inline bool Skip(uint32_t bytes);
	inline void Clear();

	inline bool Write(const void * src, uint32_t size);
	inline bool Write8(const void * src);
	inline bool Write16(const void * src);
	inline bool Write32(const void * src);
	inline bool WritePacket(const Packet & packet);

	inline Packet ToPacket();
	inline bool CopyTo(void * dst, uint32_t dstSize) const;

private:
	static void Copy(const void * __restrict src, void * __restrict dst, uint32_t size);

	uint32_t m_size;
	uint32_t m_cursor;
	uint8_t m_data[MAX_SIZE];
};

#include "Packet.inl"
