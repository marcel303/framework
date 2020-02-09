#include "BinaryDiff.h"
#include "BitStream.h"

void WriteDiff(BitStream & bs, class BinaryDiffResult & diff, const void * v)
{
	const uint8_t * bytes = static_cast<const uint8_t*>(v);

	BinaryDiffEntry * entry = diff.m_diffs.get();

	bs.WriteBit(entry != nullptr);

	while (entry)
	{
		uint32_t offset = entry->m_offset;
		uint32_t size = entry->m_size;

		bs.Write(offset);
		bs.Write(size);
		bs.WriteAlignedBytes(&bytes[offset], size);

		entry = entry->m_next;

		bs.WriteBit(entry != nullptr);
	}
}

void ReadDiff(BitStream & bs, void * v)
{
	uint8_t * bytes = static_cast<uint8_t*>(v);

	bool next = bs.ReadBit();

	while (next)
	{
		uint32_t offset;
		uint32_t size;

		bs.Read(offset);
		bs.Read(size);
		bs.ReadAlignedBytes(&bytes[offset], size);

		next = bs.ReadBit();
	}
}
