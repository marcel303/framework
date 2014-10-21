#pragma once

#include <stdint.h>
#include "SharedPtr.h"

class BinaryDiffEntry;
class BinaryDiffResult;

class BinaryDiffEntry
{
public:
	inline BinaryDiffEntry(BinaryDiffEntry * prev, uint32_t offset, uint32_t size)
		: m_next(0)
		, m_offset(offset)
		, m_size(size)
	{
		if (prev)
		{
			prev->m_next = this;
		}
	}

	BinaryDiffEntry * m_next;
	uint32_t m_offset;
	uint32_t m_size;
};

class BinaryDiffResult
{
public:
	inline BinaryDiffResult()
		: m_diffCount(0)
		, m_diffBytes(0)
	{
	}

	inline BinaryDiffResult(BinaryDiffEntry * diffs, uint32_t diffCount, uint32_t diffBytes)
		: m_diffs(diffs)
		, m_diffCount(diffCount)
		, m_diffBytes(diffBytes)
	{
	}

	SharedPtr<BinaryDiffEntry> m_diffs;
	uint32_t m_diffCount;
	uint32_t m_diffBytes;
};

/* Performs a diff operation on two byte arrays.
 * For each delta segment, a diff entry is created and added to a list.
 * Use the treshold to skip N bytes instead of emitting a new segment to reduce overhead.
 */
BinaryDiffResult BinaryDiff(const void * bytes1, const void * bytes2, uint32_t byteCount, uint32_t skipTreshold);

/* Validate the result of a binary diff.
 * Returns true if the diff is valid, false otherwise.
 */
bool BinaryDiffValidate(const void * bytes1, const void * bytes2, uint32_t byteCount, const BinaryDiffEntry * entries);
