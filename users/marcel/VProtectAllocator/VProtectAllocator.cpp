#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

/*
[msmit@nixxes.com] the VProtectAllocator makes it easier to hunt for memory corruption due to memory
writes past the allocated size. an extra page is allocated, that's adjacent to the returned memory.
this adjacent page serves as a guard area, and memory writes to this page will cause an access violation.

[ ..........  ALLOCATED PAGES .......... ] [ GUARD PAGE ]
                                               ^
                                               | memory write here will cause an access violation.
*/
class VProtectAllocator
{
	static const size_t kPageSize = 4096;

	size_t m_numPages;       // total number of allocated pages (non committed initially)
	size_t * m_allocSizes;   // array which maintains the size of each allocation. used for book keeping
	char * m_allocPages;     // page array. points to the start of the allocated address space
	size_t m_allocPageIndex; // index where to start looking for free space in the array of pages

	void verify(BOOL result)
	{
		assert(result);
	}

public:
	VProtectAllocator()
		: m_numPages(0)
		, m_allocSizes(0)
		, m_allocPages(0)
		, m_allocPageIndex(0)
	{
	}

	~VProtectAllocator()
	{
		Shut();
	}

	void Init(size_t memSize)
	{
		m_numPages = (memSize + kPageSize - 1) / kPageSize;

		// allocate allocation size array we use for book keeping
		m_allocSizes = (size_t*)VirtualAlloc(NULL, sizeof(size_t) * m_numPages, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		memset(m_allocSizes, -1, sizeof(size_t) * m_numPages);

		// allocate address space
		m_allocPages = (char*)VirtualAlloc(NULL, kPageSize * m_numPages, MEM_RESERVE, PAGE_NOACCESS);
		m_allocPageIndex = 0;
	}

	void Shut()
	{
		m_numPages = 0;

		if (m_allocSizes)
		{
			verify(VirtualFree(m_allocSizes, 0, MEM_RELEASE));
			m_allocSizes = 0;
		}

		if (m_allocPages)
		{
			verify(VirtualFree(m_allocPages, 0, MEM_RELEASE));
			m_allocPages = 0;
		}

		m_allocPageIndex = 0;
	}

	void * Alloc(size_t size)
	{
		// determine the number of pages to allocate

		size_t numPages = (size + kPageSize - (size ? 1 : 0)) / kPageSize + 1;

		// will it fit starting at m_allocPageIndex? if not, start searching at the first page

		if (m_allocPageIndex + numPages > m_numPages)
			m_allocPageIndex = 0;

		// the search ends when 'begin' reaches 'end'

		size_t end = m_allocPageIndex;
		size_t begin = m_allocPageIndex;

		do
		{
			// find free adjacent pages to fulfill this allocation

			bool free = true;

			for (size_t i = begin; i < begin + numPages && free; ++i)
			{
				assert(i < m_numPages);

				if (m_allocSizes[i] != -1)
				{
					free = false;

					begin = i + 1; // next search will start at i + 1
				}
			}

			if (free)
			{
				// mark pages as used

				for (size_t i = begin; i < begin + numPages; ++i)
					m_allocSizes[i] = numPages;

				// memory for this allocation starts here

				char * result = m_allocPages + begin * kPageSize;

				// commit the pages and make sure we can read from, and write to, this memory

				result = (char*)VirtualAlloc(result, (numPages - 1) * kPageSize, MEM_COMMIT, PAGE_READWRITE);

				assert(result != nullptr);

				// shift the result to the end of the page boundary

				size_t shift = (kPageSize - (size & (kPageSize - 1))) & (kPageSize - 1);

				result += shift;

				// next alloc starts here

				m_allocPageIndex += numPages;

				return result;
			}
			else
			{
				// will it fit starting at 'begin'? if not, start searching at the first page

				if (begin + numPages > m_numPages)
					begin = 0;
			}
		} while (begin != end);

		// uh-oh.. the allocation failed!

		assert(begin != end);

		return nullptr;
	}

	void Free(void * p)
	{
		// calculate which page the allocation belongs to, and get the allocation size

		size_t begin = ((char*)p - m_allocPages) / kPageSize;
		size_t numPages = m_allocSizes[begin];
		assert(numPages != -1);

		// mark the pages as free

		for (size_t i = begin; i < begin + numPages; ++i)
		{
			assert(m_allocSizes[i] == numPages);
			m_allocSizes[i] = -1;
		}

		// protect and decommit the pages

		DWORD oldProtect;
		verify(VirtualProtect(m_allocPages + begin * kPageSize, (numPages - 1) * kPageSize, PAGE_NOACCESS, &oldProtect));
		verify(VirtualFree(m_allocPages + begin * kPageSize, (numPages - 1) * kPageSize, MEM_DECOMMIT));
	}

	bool IsAlloc(void * p)
	{
		// calculate which page the allocation belongs to, if it were an actual allocation

		size_t begin = ((char*)p - m_allocPages) / kPageSize;

		// is it within our memory range?

		if (begin < 0 || begin >= m_numPages)
			return false;

		// is it actually allocated?

		if (m_allocSizes[begin] == -1)
			return false;

		// assume that it actually is one of our allocations

		return true;
	}

	size_t GetAllocSize(void * p)
	{
		// calculate which page the allocation belongs to, and get the allocation size

		size_t begin = ((char*)p - m_allocPages) / kPageSize;
		size_t numPages = m_allocSizes[begin];
		assert(numPages != -1);

		size_t shift = ((char*)p - m_allocPages) & (kPageSize - 1);
		size_t result = (numPages - 1) * kPageSize - shift;

		return result;
	}

	void FreeAll()
	{
		size_t i = 0;

		while (i < m_numPages)
		{
			if (m_allocSizes[i] != -1)
			{
				size_t numPages = m_allocSizes[i];

				Free(m_allocPages + i * kPageSize);

				i += numPages;
			}
			else
			{
				i += 1;
			}
		}

		assert(CalcTotalAllocSize() == 0);
	}

	size_t CalcTotalAllocSize() const
	{
		size_t numPages = 0;
		size_t i = 0;

		while (i < m_numPages)
		{
			if (m_allocSizes[i] != -1)
			{
				numPages += m_allocSizes[i];

				i += m_allocSizes[i];
			}
			else
			{
				i += 1;
			}
		}

		return numPages * kPageSize;
	}
};

static void test()
{
	VProtectAllocator a;

	a.Init(1024*1024*256);

	for (size_t j = 0; j < 8; ++j)
	{
		uint64_t bytesAllocated = 0;

		for (size_t i = 0; i < 1024*64; ++i)
		{
			size_t s = 1 + (rand() % 1024) * 32;

			bytesAllocated += s;

			void * p = a.Alloc(s);

			assert(a.IsAlloc(p));
			assert(a.GetAllocSize(p) == s);

			memset(p, 0, s);

			//memset(p, 0, s + 1); // will cause an access violation

			if (rand() % 32)
			{
				a.Free(p);

				//memset(p, 0, s); // will cause an access violation

				assert(!a.IsAlloc(p));
			}
		}

		size_t s = a.CalcTotalAllocSize();

		printf("%u Mb still allocated at end of run. total alloc size %u Mb\n", s / (1024*1024), bytesAllocated / (1024*1024));

		a.FreeAll();
	}
}

int main(int argc, char * argv[])
{
	for (int i = 0; i < 8; ++i)
	{
		test();
	}

	return 0;
}
