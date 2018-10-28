#include <string.h>
#include "BinaryDiff.h"
#include "Log.h"

BinaryDiffResult BinaryDiff(const void * bytes1, const void * bytes2, uint32_t byteCount, uint32_t skipThreshold)
{
	uint32_t diffCount = 0;
	uint32_t diffBytes = 0;

	const uint8_t * a1 = reinterpret_cast<const uint8_t *>(bytes1);
	const uint8_t * a2 = reinterpret_cast<const uint8_t *>(bytes2);

	uint32_t batchBegin = 0;
	uint32_t batchEnd = 0;
	uint32_t skipSize = 0;
	bool inBatch = false;

	BinaryDiffEntry * head = 0;
	BinaryDiffEntry * curr = 0;

	uint32_t idx = 0;

	for (;;)
	{
		const bool atEnd = idx == byteCount;
		const bool equal = idx != byteCount && a1[idx] == a2[idx];

		if (inBatch)
		{
			if (atEnd || (equal && skipSize == skipThreshold))
			{
				// end current batch

				uint32_t batchSize = batchEnd - batchBegin + 1;
				
				curr = new BinaryDiffEntry(curr, batchBegin, batchSize);

				if (head == 0)
					head = curr;

				diffBytes += batchSize;
				diffCount += 1;

				inBatch = false;
			}
			else
			{
				if (equal)
				{
					skipSize++;
				}
				else
				{
					skipSize = 0;

					batchEnd = idx;
				}
			}
		}
		
		if (atEnd)
		{
			break;
		}

		if (inBatch == false && equal == false)
		{
			// begin new batch

			batchBegin = idx;
			batchEnd = idx;
			skipSize = 0;

			inBatch = true;
		}

		idx++;
	}

	return BinaryDiffResult(head, diffCount, diffBytes);
}

bool BinaryDiffValidate(const void * bytes1, const void * bytes2, uint32_t byteCount, const BinaryDiffEntry * entries)
{
	bool result = true;
	
	int8_t * temp = new int8_t[byteCount];

	const uint8_t * a1 = reinterpret_cast<const uint8_t *>(bytes1);
	const uint8_t * a2 = reinterpret_cast<const uint8_t *>(bytes2);

	for (uint32_t i = 0; i < byteCount; ++i)
	{
		temp[i] = a1[i] == a2[i] ? -1 : 1;
	}

	for (const BinaryDiffEntry * entry = entries; entry; entry = entry->m_next)
	{
		if (entry->m_size == 0)
		{
			LOG_ERR("diff entry size is 0", 0);
			result = false;
		}
		const uint32_t idx1 = entry->m_offset;
		const uint32_t idx2 = entry->m_offset + entry->m_size;
		for (uint32_t i = idx1; i < idx2; ++i)
		{
			if (temp[i] != -1)
			{
				if (temp[i] != 1)
				{
					LOG_ERR("byte at index %u covered by 2 or more diff entries", i);
					result = false;
				}
				temp[i] = 0;
			}
		}
	}

	for (uint32_t i = 0; i < byteCount; ++i)
	{
		if (temp[i] == 1)
		{
			LOG_ERR("byte at index %u not covered by a diff entry", i);
			result = false;
		}
	}

	delete[] temp;
	temp = 0;
	
	return result;
}

bool ApplyBinaryDiff(const void * sourceBytes, void * destBytes, uint32_t byteCount, const BinaryDiffEntry * entries)
{
	const uint8_t * src = reinterpret_cast<const uint8_t *>(sourceBytes);
	      uint8_t * dst = reinterpret_cast<      uint8_t *>(destBytes);

	for (const BinaryDiffEntry * entry = entries; entry; entry = entry->m_next)
	{
		// ensure binary diff entries do not write outside of destBytes
		
		if (entry->m_offset + entry->m_size > byteCount)
			return false;

		memcpy(dst + entry->m_offset, src + entry->m_offset, entry->m_size);
	}
	
	return true;
}

BinaryDiffPackage MakeBinaryDiffPackage(const void * bytes, uint32_t byteCount, const BinaryDiffResult & diff)
{
	size_t size = 0;

	for (const BinaryDiffEntry * entry = diff.m_diffs.get(); entry; entry = entry->m_next)
	{
		size += entry->m_size;
	}

	BinaryDiffPackage result;

	result.m_diff = diff;
	result.m_data.resize(size);

	if (!result.m_data.empty())
	{
		const uint8_t * src = reinterpret_cast<const uint8_t *>(bytes);
			  uint8_t * dst = reinterpret_cast<      uint8_t *>(&result.m_data[0]);

		for (const BinaryDiffEntry * entry = diff.m_diffs.get(); entry; entry = entry->m_next)
		{
			Assert(entry->m_offset + entry->m_size <= byteCount);

			if (entry->m_offset + entry->m_size <= byteCount)
				memcpy(dst, src + entry->m_offset, entry->m_size);

			dst += entry->m_size;
		}
	}

	return result;
}

bool ApplyBinaryDiffPackage(void * destBytes, uint32_t byteCount, const BinaryDiffPackage & package)
{
	if (!package.m_data.empty())
	{
		const uint8_t * src = reinterpret_cast<const uint8_t *>(&package.m_data[0]);
			  uint8_t * dst = reinterpret_cast<      uint8_t *>(destBytes);

		for (const BinaryDiffEntry * entry = package.m_diff.m_diffs.get(); entry; entry = entry->m_next)
		{
			Assert(entry->m_offset + entry->m_size <= byteCount);
			
			// ensure binary diff entries do not write outside of destBytes
			
			if (entry->m_offset + entry->m_size > byteCount)
				return false;
			
			memcpy(dst + entry->m_offset, src, entry->m_size);

			src += entry->m_size;
		}
	}
	
	return true;
}
