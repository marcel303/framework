#pragma once

#include <stdint.h>
#include <vector>
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

class BinaryDiffPackage
{
public:
	BinaryDiffResult m_diff;
	std::vector<uint8_t> m_data;
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

/* Applies a binary diff.
 * Note that this method isn't very useful in practice. It assumes you can provide the source array as well as the output array.
 */
void ApplyBinaryDiff(const void * sourceBytes, void * destBytes, uint32_t byteCount, const BinaryDiffEntry * entries);

/* Generates a binary diff package, that contains both the binary diff result, and the data needed to apply the diff.
 */
BinaryDiffPackage MakeBinaryDiffPackage(const void * bytes, uint32_t byteCount, const BinaryDiffResult & diff);

/* Applies a binary diff package.
 */
void ApplyBinaryDiffPackage(void * destBytes, uint32_t byteCount, const BinaryDiffPackage & package);
