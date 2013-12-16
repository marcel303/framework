#include <string.h>
#include "Packet.h"

bool Packet::Read(void * dst, uint32_t size)
{
	if (m_cursor + size > m_size)
	{
		//NetAssert(false);
		return false;
	}

	Copy(&m_data[m_cursor], dst, size);

	m_cursor += size;

	return true;
}

void Packet::Copy(const void * __restrict src, void * __restrict dst, uint32_t size)
{
	const uint8_t * __restrict _src = reinterpret_cast<const uint8_t * __restrict>(src);
	uint8_t       * __restrict _dst = reinterpret_cast<uint8_t *       __restrict>(dst);

	for (uint32_t i = size; i != 0; --i)
		*_dst++ = *_src++;

	//memcpy(dst, src, size);
}
