#pragma once

#include <Windows.h>

#define VPROTECTALLOCATOR_DEBUG 1
#define VPROTECTALLOCATOR_UNITTEST 1

#if VPROTECTALLOCATOR_DEBUG
	#include <assert.h>
	#include <stdio.h>
	#define vp_assert assert
#else
	#define vp_assert
#endif

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
	static const size_t kMinAlign = 1;

	size_t m_numPages;       // total number of allocated pages (non committed initially)
	size_t * m_allocSizes;   // array which maintains the size of each allocation. used for book keeping
	char * m_allocPages;     // page array. points to the start of the allocated address space
	size_t m_allocPageIndex; // index where to start looking for free space in the array of pages

	inline void verify(BOOL result) const
	{
	#if VPROTECTALLOCATOR_DEBUG
		if (!result)
		{
			DWORD error = GetLastError();
			printf("VProtectAllocator: error: %x\n", error);
		}
	#endif

		vp_assert(result);
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

	void * Alloc(size_t size, size_t align)
	{
		vp_assert(size != 0);

		// align the requested size

		if (align < kMinAlign)
			align = kMinAlign;

		size = (size + align - 1) & ~(align - 1);

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
				vp_assert(i < m_numPages);

				if (m_allocSizes[i] != -1)
				{
					free = false;

					begin = i + m_allocSizes[i]; // next search will start at i + m_allocSizes[i]
				}
			}

			if (free)
			{
				// mark pages as used

				m_allocSizes[begin] = numPages;

				for (size_t i = begin + 1; i < begin + numPages; ++i)
					vp_assert(m_allocSizes[i] == -1);

				// memory for this allocation starts here

				char * result = m_allocPages + begin * kPageSize;

				// commit the pages and make sure we can read from, and write to, this memory

				result = (char*)VirtualAlloc(result, (numPages - 1) * kPageSize, MEM_COMMIT, PAGE_READWRITE);

				vp_assert(result != nullptr);

				// shift the result to the end of the page boundary

				size_t shift = (kPageSize - (size & (kPageSize - 1))) & (kPageSize - 1);

				result += shift;

				// next alloc starts here

				m_allocPageIndex = begin + numPages;

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

		vp_assert(begin != end);

		return nullptr;
	}

	void Free(const void * p)
	{
		// calculate which page the allocation belongs to, and get the allocation size

		size_t begin = ((char*)p - m_allocPages) / kPageSize;
		size_t numPages = m_allocSizes[begin];
		vp_assert(numPages != -1);
		vp_assert(numPages >= 2);

		// mark the pages as free

		m_allocSizes[begin] = -1;

		for (size_t i = begin + 1; i < begin + numPages; ++i)
			vp_assert(m_allocSizes[i] == -1);

		// protect and decommit the pages

		DWORD oldProtect;
		verify(VirtualProtect(m_allocPages + begin * kPageSize, (numPages - 1) * kPageSize, PAGE_NOACCESS, &oldProtect));
		verify(VirtualFree(m_allocPages + begin * kPageSize, (numPages - 1) * kPageSize, MEM_DECOMMIT));
	}

	bool IsAlloc(const void * p)
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

	void ShrinkAllocSize(const void * p, size_t newSize)
	{
		// calculate which page the allocation belongs to, and get the allocation size

		size_t begin = ((char*)p - m_allocPages) / kPageSize;
		size_t oldNumPages = m_allocSizes[begin];
		vp_assert(oldNumPages != -1);

		// calculate new page count

		size_t newNumPages = (newSize + kPageSize - (newSize ? 1 : 0)) / kPageSize + 1;

		vp_assert(newNumPages >= 2);
		vp_assert(newNumPages <= oldNumPages);

		if (newNumPages == oldNumPages)
			return;

		// update book keeping

		m_allocSizes[begin] = newNumPages;

		for (size_t i = 1; i < oldNumPages; ++i)
			vp_assert(m_allocSizes[begin + i] == -1);

		// add new guard page

		DWORD oldProtect;
		size_t newEnd = begin + newNumPages;

		verify(VirtualProtect(m_allocPages + (newEnd - 1) * kPageSize, kPageSize, PAGE_NOACCESS, &oldProtect));
		verify(VirtualFree(m_allocPages + (newEnd - 1) * kPageSize, kPageSize, MEM_DECOMMIT));

		// protect and decommit the freed pages

		if (oldNumPages - newNumPages - 1 > 0)
		{
			verify(VirtualProtect(m_allocPages + newEnd * kPageSize, (oldNumPages - newNumPages - 1) * kPageSize, PAGE_NOACCESS, &oldProtect));
			verify(VirtualFree(m_allocPages + newEnd * kPageSize, (oldNumPages - newNumPages - 1) * kPageSize, MEM_DECOMMIT));
		}
	}

	size_t GetAllocSize(const void * p)
	{
		// calculate which page the allocation belongs to, and get the allocation size

		size_t begin = ((char*)p - m_allocPages) / kPageSize;
		size_t numPages = m_allocSizes[begin];
		vp_assert(numPages != -1);

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

		vp_assert(CalcTotalAllocSize() == 0);
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

#if VPROTECTALLOCATOR_UNITTEST

#include <algorithm>
#include <assert.h>
#include <deque>
#include <list>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <vector>

static VProtectAllocator * s_alloc = nullptr;

static void VInit()
{
	if (s_alloc == nullptr)
	{
		s_alloc = (VProtectAllocator*)VirtualAlloc(NULL, sizeof(VProtectAllocator), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		s_alloc->Init(1024*1024*32);
	}
}

static void * VAlloc(size_t size)
{
	VInit();

	return s_alloc->Alloc(size, 16);
}

static void VFree(void * p)
{
	VInit();

	s_alloc->Free(p);
}

void * operator new(size_t size)
{
	return VAlloc(size);
}

void * operator new[](size_t size)
{
	return VAlloc(size);
}

void operator delete(void * p)
{
	VFree(p);
}

void operator delete[](void * p)
{
	VFree(p);
}

class VOverride
{
	VProtectAllocator * m_oldAlloc;

public:
	VOverride(VProtectAllocator * alloc)
	{
		m_oldAlloc = s_alloc;
		s_alloc = alloc;
	}

	~VOverride()
	{
		s_alloc = m_oldAlloc;
	}
};

static void test()
{
	VProtectAllocator a;

	a.Init(1024*1024*256);

	for (size_t j = 0; j < 4; ++j)
	{
		uint64_t bytesAllocated = 0;

		for (size_t i = 0; i < 1024*64; ++i)
		{
			size_t align = 16;
			size_t s = 1 + (rand() % 1024) * 32;

			bytesAllocated += s;

			void * p = a.Alloc(s, align);

			vp_assert(a.IsAlloc(p));
			vp_assert(a.GetAllocSize(p) >= s && a.GetAllocSize(p) < s + align);

			memset(p, 0, s);

			//memset(p, 0, s + 1); // will cause an access violation

			if (rand() % 32)
			{
				a.Free(p);

				//memset(p, 0, s); // will cause an access violation

				vp_assert(!a.IsAlloc(p));
			}
		}

		size_t s = a.CalcTotalAllocSize();

		printf("%u Mb still allocated at end of run. total alloc size %u Mb\n", s / (1024*1024), bytesAllocated / (1024*1024));

		a.FreeAll();
	}
}

int main(int argc, char * argv[])
{
	printf("testing interface and stress testing the allocator..\n");

	//for (int i = 0; i < 8; ++i)
	{
		test();
	}

	printf("done\n");

	printf("testing STL containers.. ");

	for (int i = 0; i < 2; ++i)
	{
		std::map<int, int> m;
		std::vector<int> v;
		std::deque<int> d;
		std::list<int> l;

		for (int j = 0; j < 1024; ++j)
		{
			int r = rand();

			m[r] = j;

			v.push_back(m[r]);
			v.push_back(r);

			if (j & 1)
				d.push_front(r);
			else
				d.push_back(j);

			l.push_front(j);
		}

		std::map<int, int> t = m;

		while (!l.empty())
		{
			l.pop_back();
		}

		while (!d.empty())
		{
			if (d.size() & 1)
				d.pop_back();
			else
				d.pop_front();
		}

		std::sort(v.begin(), v.end());

		std::reverse(v.begin(), v.end());

		while (!v.empty())
		{
			v.pop_back();

			v.shrink_to_fit();
		}
	}

	printf("done\n");

	printf("testing reallocation.. ");

	{
		VProtectAllocator a;
		VOverride o(&a);

		a.Init(1024*1024*128);

		size_t pageSize = 4096; // we assume here the page size is 4096. this is fine for this unit test, but it's subject to change!

		size_t initialSize = pageSize * 128;

		void * p = a.Alloc(initialSize, pageSize);

		{
		std::map<int, int> m;
		std::vector<int> v;

		for (size_t i = a.GetAllocSize(p); i > 0; --i)
		{
			a.ShrinkAllocSize(p, i);

			if ((i % pageSize) == 0)
			{
				vp_assert(a.GetAllocSize(p) == i);
				//vp_assert(a.CalcTotalAllocSize() == i + pageSize);
			}
			else
			{
				vp_assert(a.GetAllocSize(p) >= i && a.GetAllocSize(p) < i + pageSize);
				//vp_assert(a.CalcTotalAllocSize() == pageSize * (i + 1));
			}

			memset(p, 0, i);

			if (i <= 8)
			{
				//memset(p, 0, i + 1); // will cause an access violation
			}

			if ((i % 32) == 0)
			{
				size_t size = a.GetAllocSize(p);

				a.Free(p);

				p = a.Alloc(size, pageSize);
			}

			// throw a few other allocation in the mix

			if ((i % 1024) == 0)
			{
				m.clear();
				v.clear();
			}

			int r = rand();

			m[r] = i;

			v.push_back(m[r]);
			v.push_back(r);
		}
		}

		a.Free(p);

		vp_assert(a.CalcTotalAllocSize() == 0);
	}

	printf("done\n");

	printf("(press any key to continue)\n");

	getchar();

	return 0;
}

#endif
