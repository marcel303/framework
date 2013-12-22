#include <alloc.h>
#include <assert.h>
#include <stdint.h>
#include <vector>

//

extern void CheckIntegrity();
extern void TestAllocationSystem();

//

static const int GUARD_L = 0x35675672;
static const int GUARD_R = 0x58924365;

struct AllocInfo
{
	uintptr_t p;
	size_t size;
};

static const int kMaxAllocations = 192*1024;
static AllocInfo s_allocInfos[kMaxAllocations] = { };
static int s_freeIndex[kMaxAllocations] = { };
static int s_freeCount = 0;
static bool s_initialized = false;

static void CheckIntegrity(int index)
{
	if (index >= 0 && index < kMaxAllocations)
	{
		AllocInfo & info = s_allocInfos[index];
		
		assert(info.p != 0);

		const int guardL = *reinterpret_cast<int*>(info.p             + 4);
		const int guardR = *reinterpret_cast<int*>(info.p + info.size - 4);
		
		assert(guardL == GUARD_L);
		assert(guardR == GUARD_R);
	}
	else
	{
		assert(index >= 0 && index < kMaxAllocations); // corrupted index
	}
}

void CheckIntegrity()
{
	for (int i = 0; i < kMaxAllocations; ++i)
	{
		if (s_allocInfos[i].p != 0)
		{
			CheckIntegrity(i);
		}
	}
}

static void Activate()
{
	if (s_initialized)
		return;
	while (s_freeCount < kMaxAllocations)
	{
		s_freeIndex[s_freeCount] = s_freeCount;
		s_freeCount++;
	}
	s_initialized = true;
}

static void * Alloc(size_t size)
{
	Activate();
	
	size += sizeof(int) * 3;
	
	uintptr_t p = reinterpret_cast<uintptr_t>(malloc(size));
	
	assert(s_freeCount >= 1);
	--s_freeCount;
	const int index = s_freeIndex[s_freeCount];
	AllocInfo & info = s_allocInfos[index];
	assert(info.p == 0);
	info.p = p;
	info.size = size;
	
	// [index] [GUARD] [0101010101010101] [GUARD]
	
	*reinterpret_cast<int*>(p           ) = index;
	*reinterpret_cast<int*>(p        + 4) = GUARD_L;
	*reinterpret_cast<int*>(p + size - 4) = GUARD_R;
	
	return reinterpret_cast<void*>(p + 8);
}

static void Free(void * _p)
{
	if (_p == 0)
		return;
	
	uintptr_t p = reinterpret_cast<uintptr_t>(_p) - 8;

	const int index = *reinterpret_cast<int*>(p);
	
	CheckIntegrity(index);

	AllocInfo & info = s_allocInfos[index];
	assert(info.p == p);
	info.p = 0;
	info.size = 0;
	
	s_freeIndex[s_freeCount] = index;
	s_freeCount++;
	
	free(reinterpret_cast<void*>(p));
}

//

void * operator new(size_t size) throw(std::bad_alloc)
{
	return Alloc(size);
}

void * operator new[](size_t size) throw(std::bad_alloc)
{
	return Alloc(size);
}

void operator delete(void * p) throw()
{
	Free(p);
}

void operator delete[](void * p) throw()
{
	Free(p);
}

//

void TestAllocationSystem()
{
	std::vector<int*> allocations;
	
	for (int n = 0; n < 4; ++n)
	{
		for (int i = 0; i < kMaxAllocations / 2; ++i)
		{
			int * p = new int[32];
			
			p[31] = 3;
			
			allocations.push_back(p);
		}
		
		CheckIntegrity();
		
		for (size_t i = 0; i < allocations.size(); ++i)
		{
			int * p = allocations[i];
			
			delete [] p;
		}
		
		allocations.clear();
	}
}
